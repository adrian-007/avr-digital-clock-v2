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

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#include "OneWireDev.hpp"
#include "ScopedInterruptLock.hpp"

#define DS_Rx(dir, pin)             (dir)     &= ~(1 << (pin))
#define DS_Tx(dir, pin)             (dir)     |=  (1 << (pin))

#define DS_PullUP(port, pin)        (port)    |=  (1 << (pin))
#define DS_PullDOWN(port, pin)      (port)    &= ~(1 << (pin))

OneWireDev::OneWireDev(volatile uint8_t* _port, volatile uint8_t* _dir, volatile uint8_t* _pin, uint8_t _pinnum) : port(_port), dir(_dir), pin(_pin), pinnum(_pinnum), resolution(12)
{
    DS_Rx(*dir, pinnum);
}

void OneWireDev::init(volatile uint8_t* _port, volatile uint8_t* _dir, volatile uint8_t* _pin, uint8_t _pinnum)
{
    port = _port;
    dir = _dir;
    pin = _pin;
    pinnum = _pinnum;

    DS_Rx(*dir, pinnum);
}

bool OneWireDev::reset()
{
    bool presence = false;

    {
        ScopedInterruptLock sil;

        DS_Tx(*dir, pinnum);
        DS_PullDOWN(*port, pinnum);
    }

    _delay_us(500);

    {
        ScopedInterruptLock sil;

        DS_Rx(*dir, pinnum);
        _delay_us(60);

        presence = bit_is_clear(*pin, pinnum);
    }

    _delay_us(480);

    return presence;
}

void OneWireDev::send(uint8_t byte)
{
    ScopedInterruptLock sil;
    uint8_t mask = 0x01;

    DS_Tx(*dir, pinnum);

    for(uint8_t i = 0; i < 8; ++i)
    {
        if(byte & mask)
        {
            DS_PullDOWN(*port, pinnum);
            _delay_us(4);

            DS_PullUP(*port, pinnum);
            _delay_us(76);
        }
        else
        {
            DS_PullDOWN(*port, pinnum);
            _delay_us(76);

            DS_PullUP(*port, pinnum);
            _delay_us(4);
        }

        mask <<= 1;
    }
}

uint8_t OneWireDev::read()
{
    ScopedInterruptLock sil;
    uint8_t b = 0;

    for(uint8_t i = 0; i < 8; ++i)
    {
        DS_Tx(*dir, pinnum);
        DS_PullDOWN(*port, pinnum);

        _delay_us(2);

        DS_Rx(*dir, pinnum);

        _delay_us(10);

        if(bit_is_set(*pin, pinnum))
        {
            b |= (1 << i);
        }

        _delay_us(60);
    }

    return b;
}

void OneWireDev::set_config(uint8_t resolution, uint8_t tH /*= 0xFF*/, uint8_t tL /*= 0xFF*/, bool toEEPROM /*= false*/)
{
    bool p;

    if((p = reset()))
    {
        send(DS_CMD_SKIP_ROM);
        send(DS_CMD_WRITE_SCRATCHPAD);

        send(tH);
        send(tL);

        switch(resolution)
        {
            case 9:
            {
                send(0x00);
                break;
            }
            case 10:
            {
                send(1 << 5);
                break;
            }
            case 11:
            {
                send(1 << 6);
                break;
            }
            default:
            {
                send(1 << 5 | 1 << 6);
                resolution = 12;
                break;
            }
        }

        DS_Tx(*dir, pinnum);
        DS_PullUP(*port, pinnum);

        _delay_ms(10);

        this->resolution = resolution;
    }

    if(toEEPROM && p && (p = reset()))
    {
        send(DS_CMD_SKIP_ROM);
        send(DS_CMD_COPY_SCRATCHPAD);
    }
}

int16_t OneWireDev::request_convert(bool wait)
{
    bool p = reset();

    if(p)
    {
        send(DS_CMD_SKIP_ROM);
        send(DS_CMD_CONVERT_T);

        // strong pull-up for conversion time
        DS_Tx(*dir, pinnum);
        DS_PullUP(*port, pinnum);

        switch(resolution)
        {
            case 9:
            {
                if(wait)
                {
                    _delay_ms(150);
                }
                return 150;
            }
            case 10:
            {
                if(wait)
                {
                    _delay_ms(300);
                }
                return 300;
            }
            case 11:
            {
                if(wait)
                {
                    _delay_ms(500);
                }
                return 500;
            }
            default:
            {
                if(wait)
                {
                    _delay_ms(1000);
                }
                return 1000;
            }
        }
    }

    return -1;
}

bool OneWireDev::convert_result(volatile bool& sign, volatile int8_t& temp, volatile int8_t& fraction)
{
    int16_t t = 0xFFFF;
    bool p = reset();

    temp = fraction = 0xFF;
    sign = false;

    if(p)
    {
        send(DS_CMD_SKIP_ROM);
        send(DS_CMD_READ_SCRATCHPAD);

        t = (int16_t)read();
        t |= (int16_t)read() << 8;

        if((t & 0xF800) != 0)
        {
            t = (~t + 1) & ~0xF800;
            sign = true;
        }

        temp = t >> 4;
        fraction = t & 0x000F;

        t = 12 - resolution;

        for(uint8_t i = 0; i < t; ++i)
        {
            fraction &= ~(1 << i);
        }

        t = ((int16_t)fraction * 625) / 100;

        fraction = t / 10;

        if((t % 10) >= 5)
        {
            fraction += 1;
        }

        for(int8_t i = 0; i < 7; ++i)
        {
            read();
        }
    }

    DS_Tx(*dir, pinnum);
    DS_PullUP(*port, pinnum);

    return p;
}
