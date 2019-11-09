/*
 * Copyright (C) 2013 adrian_007, adrian-007 on o2 point pl
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef F_CPU
#if F_CPU == 16000000
#define SET_OSCILLATOR_VALUE() // no point doing that, 16MHz is external crystal
#elif F_CPU == 8000000
#define SET_OSCILLATOR_VALUE() OSCCAL = 0xB7
#elif F_CPU == 4000000
#define SET_OSCILLATOR_VALUE() OSCCAL = 0xB5
#elif F_CPU == 2000000
#define SET_OSCILLATOR_VALUE() OSCCAL = 0xBA
#elif F_CPU == 1000000
#define SET_OSCILLATOR_VALUE() OSCCAL = 0xBA
#endif
#endif

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>

#include "OneWireDev.hpp"
#include "ScopedInterruptLock.hpp"
#include "BufferedDisplays.hpp"
#include "Button.hpp"
#include "TimerTask.hpp"
#include "BaseClock.hpp"
#include "ClockCorrection.hpp"

#define PARITY_EEPROM_ADDR     (reinterpret_cast<uint32_t*>(0x0A))
#define CALIB_EEPROM_ADDR      (reinterpret_cast<uint32_t*>(0x1A))
#define ALARM_EEPROM_ADDR      (reinterpret_cast<uint32_t*>(0x2A))

#define ENABLE_BUZZER()        (PORTB &= ~(1 << 0))
#define DISABLE_BUZZER()       (PORTB |=  (1 << 0))
#define TOGGLE_BUZZER()        (PORTB ^=  (1 << 0))

#define ENABLE_INDICATOR()     (PORTB |=  (1 << 3))
#define DISABLE_INDICATOR()    (PORTB &= ~(1 << 3))
#define TOGGLE_INDICATOR()     (PORTB ^=  (1 << 3))

#define ENABLE_DOTS()          (PORTC &= ~(1 << 1))
#define DISABLE_DOTS()         (PORTC |=  (1 << 1))
#define TOGGLE_DOTS()          (PORTC ^=  (1 << 1))

volatile int8_t currentSensor = 0;
volatile int16_t timer2Counter = 0;

OneWireDev dev[2];
BufferedDisplays display;
BaseClock timeClock;
BaseClock alarmClock;
ClockCorrection correctionClock;

BaseClock* currentClock = 0;

enum
{
    TASK_CONFIG_CLOCK = 0,
    TASK_SENSOR_READ,
    TASK_LAST
};

TimerTask TIMER_tasks[TASK_LAST];

enum
{
    BUTTON_CONFIG = 0,
    BUTTON_INCREMENT,
    BUTTON_DECREMENT,
    BUTTON_ALARM,
    BUTTON_LAST
};

Button buttons[BUTTON_LAST];

enum
{
    STATE_WORKING = 0,
    STATE_CONFIG_MINUTES,
    STATE_CONFIG_HOURS,
    STATE_LAST
};

volatile uint8_t state = STATE_WORKING;

void setupPorts()
{
    DDRA =  (1 << 0 | 1 << 1 | 1 << 2 | 1 << 3 | 0 << 4 | 0 << 5 | 0 << 6 | 0 << 7);
    DDRB =  (1 << 0 | 0 << 1 | 0 << 2 | 1 << 3 | 0 << 4 | 0 << 5 | 0 << 6 | 0 << 7);
    DDRC =  (1 << 0 | 1 << 1 | 1 << 2 | 1 << 3 | 1 << 4 | 1 << 5 | 1 << 6 | 1 << 7);
    DDRD =  (1 << 0 | 1 << 1 | 1 << 2 | 1 << 3 | 1 << 4 | 1 << 5 | 1 << 6 | 1 << 7);

    PORTA = (1 << 0 | 1 << 1 | 1 << 2 | 1 << 3 | 1 << 4 | 1 << 5 | 1 << 6 | 1 << 7);
    PORTB = (1 << 0 | 1 << 1 | 1 << 2 | 0 << 3 | 1 << 4 | 1 << 5 | 1 << 6 | 1 << 7);
    PORTC = (0 << 0 | 1 << 1 | 1 << 2 | 1 << 3 | 1 << 4 | 1 << 5 | 1 << 6 | 1 << 7);
    PORTD = (0 << 0 | 0 << 1 | 0 << 2 | 0 << 3 | 0 << 4 | 0 << 5 | 0 << 6 | 0 << 7);
}

void setupButtons()
{
    buttons[BUTTON_CONFIG].init(&PINA, PINA7);
    buttons[BUTTON_INCREMENT].init(&PINA, PINA6);
    buttons[BUTTON_DECREMENT].init(&PINA, PINA5);
    buttons[BUTTON_ALARM].init(&PINA, PINA4);
}

void setupInterrupts()
{
    // Timer2 is based on 32 kHz XTAL
    ASSR  |= 1 << AS2;                       // async mode
    TCCR2 |= 1 << WGM21 | 1 << CS21;         // CTC mode, prescaler 8
    TCNT2  = 0;
    OCR2   = 7;

    while(ASSR & (1 << TCR2UB | 1 << TCN2UB | 1 << OCR2UB));

    TIMSK |= 1 << OCIE2;
}

int main()
{
    SET_OSCILLATOR_VALUE();

    // Wait for power supply stabilization
    _delay_ms(400);

    // Ensure that PD0:PD1 will work
    UCSRB = 0x00;
    SPCR  = 0x00;

    setupPorts();
    setupButtons();

    _delay_ms(100);

    dev[0].init(&PORTB, &DDRB, &PINB, PINB2);
    dev[1].init(&PORTB, &DDRB, &PINB, PINB1);

    display.setDisplay(-1);
    display.setChar(-1);

    for(int8_t i = 0; i < 8; ++i)
    {
        display.setBuffer(i, '-');
        display.setEnabled(i, true);
    }

    for(int8_t i = 0; i < 2; ++i)
    {
        if(dev[i].reset())
        {
            dev[i].set_config(12);
        }
        _delay_ms(1);
    }

    // Read calibration value
    {
        eeprom_busy_wait();
        int32_t calib = static_cast<int32_t>(eeprom_read_dword(CALIB_EEPROM_ADDR));
        correctionClock.set(calib);
    }

    // Read last alarm value
    {
        eeprom_busy_wait();
        int32_t alarmTicks = static_cast<int32_t>(eeprom_read_dword(ALARM_EEPROM_ADDR));
        alarmTicks -= alarmTicks % 60; // get rid of seconds, in case of corrupted EEPROM memory chunk
        alarmClock.set(alarmTicks);
    }

    setupInterrupts();
    sei();

    do
    {
        _delay_ms(10);
    }
    while(buttons[BUTTON_CONFIG].pressed() == false);
    buttons[BUTTON_CONFIG].waitForRelease();

    currentClock = &timeClock;

    do
    {
        timeClock.setPaused(state != STATE_WORKING);

        if(buttons[BUTTON_ALARM].pressed() && buttons[BUTTON_ALARM].waitForRelease())
        {
            if(state == STATE_WORKING)
            {
                TIMER_tasks[TASK_CONFIG_CLOCK].setId(4);
                TIMER_tasks[TASK_CONFIG_CLOCK].setElapsed(128);
            }
            else
            {
                if(TIMER_tasks[TASK_CONFIG_CLOCK].id() > 0)
                {
                    TIMER_tasks[TASK_CONFIG_CLOCK].setId(3);
                    TIMER_tasks[TASK_CONFIG_CLOCK].setElapsed(128);
                }
            }
        }
        else if(buttons[BUTTON_INCREMENT].pressed() && buttons[BUTTON_DECREMENT].pressed())
        {
            do
            {
                _delay_ms(10);
            }
            while(buttons[BUTTON_INCREMENT].pressed() || buttons[BUTTON_DECREMENT].pressed());

            TIMER_tasks[TASK_CONFIG_CLOCK].setId(5);
            TIMER_tasks[TASK_CONFIG_CLOCK].setElapsed(256);
        }

        if(TIMER_tasks[TASK_CONFIG_CLOCK].finished())
        {
            switch(TIMER_tasks[TASK_CONFIG_CLOCK].id())
            {
                case 1:
                {
                    currentClock == &alarmClock ? ENABLE_INDICATOR() : DISABLE_INDICATOR();

                    display.setBuffer(4, -1);
                    display.setBuffer(5, -1);
                    display.setBuffer(6, '-');
                    display.setBuffer(7, '-');

                    display.setBuffer(4, -1, true);
                    display.setBuffer(5, -1, true);
                    display.setBuffer(6, -1, true);
                    display.setBuffer(7, -1, true);

                    state = STATE_CONFIG_MINUTES;
                    uint32_t delay = 4;

                    if(buttons[BUTTON_CONFIG].pressed() && buttons[BUTTON_CONFIG].waitForRelease())
                    {
                        TIMER_tasks[TASK_CONFIG_CLOCK].setId(2);
                        delay = 128;
                    }
                    else if(buttons[BUTTON_INCREMENT].pressed())
                    {
                        currentClock->tick(60);
                        delay = 64;
                    }
                    else if(buttons[BUTTON_DECREMENT].pressed())
                    {
                        currentClock->tick(-60);
                        delay = 64;
                    }

                    TIMER_tasks[TASK_CONFIG_CLOCK].setElapsed(delay);
                    break;
                }
                case 2:
                {
                    currentClock == &alarmClock ? ENABLE_INDICATOR() : DISABLE_INDICATOR();

                    display.setBuffer(4, '-');
                    display.setBuffer(5, '-');
                    display.setBuffer(6, -1);
                    display.setBuffer(7, -1);

                    display.setBuffer(4, -1, true);
                    display.setBuffer(5, -1, true);
                    display.setBuffer(6, -1, true);
                    display.setBuffer(7, -1, true);

                    state = STATE_CONFIG_HOURS;
                    uint32_t delay = 4;

                    if(buttons[BUTTON_CONFIG].pressed() && buttons[BUTTON_CONFIG].waitForRelease())
                    {
                        TIMER_tasks[TASK_CONFIG_CLOCK].setId(6); // goto eeprom save case
                        delay = 16;
                    }
                    else if(buttons[BUTTON_INCREMENT].pressed())
                    {
                        currentClock->tick(3600);
                        delay = 64;
                    }
                    else if(buttons[BUTTON_DECREMENT].pressed())
                    {
                        currentClock->tick(-3600);
                        delay = 64;
                    }

                    TIMER_tasks[TASK_CONFIG_CLOCK].setElapsed(delay);
                    break;
                }
                case 3:
                {
                    if(currentClock == &timeClock)
                    {
                        currentClock = &alarmClock;
                    }
                    else
                    {
                        currentClock = &timeClock;
                    }
                    TIMER_tasks[TASK_CONFIG_CLOCK].setId(1);
                    TIMER_tasks[TASK_CONFIG_CLOCK].setElapsed(16);
                    break;
                }
                case 4:
                {
                    // just toggle alarm
                    alarmClock.setPaused(!alarmClock.paused());
                    TIMER_tasks[TASK_CONFIG_CLOCK].setId(0);
                    TIMER_tasks[TASK_CONFIG_CLOCK].setElapsed(128);
                    break;
                }
                case 5:
                {
                    currentClock = &correctionClock;
                    TIMER_tasks[TASK_CONFIG_CLOCK].setId(2);
                    TIMER_tasks[TASK_CONFIG_CLOCK].setElapsed(256);
                    break;
                }
                case 6: // eeprom save
                {
                    {
                        ScopedInterruptLock lock;

                        int32_t ticks;
                        eeprom_busy_wait();
                        ticks = correctionClock.get();
                        eeprom_write_dword(CALIB_EEPROM_ADDR, ticks);

                        eeprom_busy_wait();
                        ticks = alarmClock.get();
                        ticks -= ticks % 60;
                        eeprom_write_dword(ALARM_EEPROM_ADDR, ticks);

                        // align to full minutes, drop remaining seconds
                        if(currentClock == &timeClock)
                        {
                            currentClock->normalize();
                        }
                    }

                    TIMER_tasks[TASK_CONFIG_CLOCK].setId(0);
                    TIMER_tasks[TASK_CONFIG_CLOCK].setElapsed(16);
                    break;
                }
                default:
                {
                    if(currentClock != 0 && buttons[BUTTON_CONFIG].pressed())
                    {
                        do
                        {
                            _delay_ms(10);
                        }
                        while(buttons[BUTTON_CONFIG].pressed());

                        TIMER_tasks[TASK_CONFIG_CLOCK].setId(1);
                        TIMER_tasks[TASK_CONFIG_CLOCK].setElapsed(256);

                        TIMER_tasks[TASK_SENSOR_READ].setId(0);
                        TIMER_tasks[TASK_SENSOR_READ].setElapsed(0);
                    }
                    else
                    {
                        state = STATE_WORKING;
                        currentClock = &timeClock;
                    }
                    break;
                }
            }
        }

        if(TIMER_tasks[TASK_SENSOR_READ].finished())
        {
            switch(TIMER_tasks[TASK_SENSOR_READ].id())
            {
                case 1:
                {
                    int16_t elapsed = dev[currentSensor].request_convert(false);

                    if(elapsed > 0)
                    {
                        TIMER_tasks[TASK_SENSOR_READ].setElapsed(512);
                        TIMER_tasks[TASK_SENSOR_READ].setId(2);
                    }
                    else
                    {
                        ++currentSensor;
                        currentSensor %= 2;
                    }
                    break;
                }
                case 2:
                {
                    int8_t temp[2];
                    bool sign;

                    dev[currentSensor].convert_result(sign, temp[0], temp[1]);

                    display.setBuffer(4, -1);
                    display.setBuffer(5, -1);
                    display.setBuffer(6, -1);
                    display.setBuffer(7, -1);

                    int index = 7;

                    display.setBuffer(index--, temp[1]);
                    display.setBuffer(index--, temp[0] % 10);
                    display.setBuffer(index, (temp[0] / 10) % 10);

                    if(display.getBuffer(index) != 0)
                    {
                        index--;
                    }

                    display.setBuffer(index--, sign ? '-' : -1);
                    display.setBuffer(4, currentSensor ? 1 : 0, true);
                    display.setBuffer(6, 1, true); // dot for fraction

                    TIMER_tasks[TASK_SENSOR_READ].setId(0);
                    TIMER_tasks[TASK_SENSOR_READ].setElapsed(1024);
                    break;
                }
                default:
                {
                    if(state == STATE_WORKING)
                    {
                        TIMER_tasks[TASK_SENSOR_READ].setId(1);
                        currentSensor = (currentSensor + 1) % 2;
                    }
                    else
                    {
                        for(int8_t i = 4; i < 8; ++i)
                        {
                            display.setBuffer(i, -1, true);
                        }

                        switch(state)
                        {
                            case STATE_CONFIG_MINUTES:
                            {
                                display.setBuffer(4, -1);
                                display.setBuffer(5, -1);
                                display.setBuffer(6, '-');
                                display.setBuffer(7, '-');
                                break;
                            }
                            case STATE_CONFIG_HOURS:
                            {
                                display.setBuffer(4, '-');
                                display.setBuffer(5, '-');
                                display.setBuffer(6, -1);
                                display.setBuffer(7, -1);
                                break;
                            }
                        }

                        TIMER_tasks[TASK_SENSOR_READ].setElapsed(8);
                    }
                    break;
                }
            }
        }

        if(currentClock != 0)
        {
            for(int8_t i = 0; i < 4; ++i)
            {
                display.setBuffer(i, currentClock->digit(i));
            }
        }
    }
    while(true);

    return 0;
}

ISR(TIMER2_COMP_vect)
{
    timer2Counter = (timer2Counter + 1) % 512;

    // 1s
    if(timer2Counter == 0)
    {
        if(timeClock.paused() == false)
        {
            int32_t tick = 1;

            // Every 10 minutes
            if((timeClock.get() % 600) == 0)
            {
                tick += correctionClock.checkCorrection();
            }

            timeClock.tick(tick);
        }
    }

    // 0.5s
    if((timer2Counter % 256) == 0)
    {
        if(timeClock.paused() == false)
        {
            TOGGLE_DOTS();

            if(alarmClock.paused())
            {
                DISABLE_BUZZER();
                DISABLE_INDICATOR();
            }
            else
            {
                const int32_t ticks = timeClock.get();
                const int32_t alarm = alarmClock.get();

                if(ticks >= alarm && ticks < (alarm + 3600))
                {
                    TOGGLE_BUZZER();
                    TOGGLE_INDICATOR();
                }
                else
                {
                    DISABLE_BUZZER();
                    ENABLE_INDICATOR();
                }
            }
        }
        else
        {
            ENABLE_DOTS();
            DISABLE_BUZZER();
            DISABLE_INDICATOR();
        }
    }

    if((timer2Counter % 16) == 0)
    {
        for(int8_t i = 0; i < BUTTON_LAST; ++i)
        {
            buttons[i].probe();
        }
    }

    for(int8_t i = 0; i < TASK_LAST; ++i)
    {
        TIMER_tasks[i].tick();
    }

    display.nextDisplay();
}
