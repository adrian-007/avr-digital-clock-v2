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

#ifndef ONE_WIRE_DEV_HPP_
#define ONE_WIRE_DEV_HPP_

#define DS_CMD_READ_ROM             0x33
#define DS_CMD_MATCH_ROM            0x55
#define DS_CMD_SEARCH_ROM           0xF0
#define DS_CMD_ALARM_SEARCH         0xEC
#define DS_CMD_SKIP_ROM             0xCC
#define DS_CMD_CONVERT_T            0x44
#define DS_CMD_READ_SCRATCHPAD      0xBE
#define DS_CMD_WRITE_SCRATCHPAD     0x4E
#define DS_CMD_COPY_SCRATCHPAD      0x48

class OneWireDev
{
public:
    OneWireDev() : port(0), dir(0), pin(0), pinnum(0), resolution(0)
    {
    }

    OneWireDev(volatile uint8_t* _port, volatile uint8_t* _dir, volatile uint8_t* _pin, uint8_t _pinnum);
    void init(volatile uint8_t* _port, volatile uint8_t* _dir, volatile uint8_t* _pin, uint8_t _pinnum);

    bool reset();
    void set_config(uint8_t resolution, uint8_t tH = 0xFF, uint8_t tL = 0xFF, bool toEEPROM = false);

    void send(uint8_t byte);
    uint8_t read();

    int16_t request_convert(bool wait);
    bool convert_result(volatile bool& sign, volatile int8_t& temp, volatile int8_t& fraction);

    inline void setPort(volatile uint8_t* _port)
    {
        port = _port;
    }

    inline void setDir(volatile uint8_t* _dir)
    {
        dir = _dir;
    }

    inline void setPin(volatile uint8_t* _pin)
    {
        pin = _pin;
    }

    inline void setPinNum(const uint8_t _pinnum)
    {
        pinnum = _pinnum;
    }

private:
    volatile uint8_t* port;
    volatile uint8_t* dir;
    volatile uint8_t* pin;
    uint8_t pinnum;
    uint8_t resolution;
};

#endif /* ONE_WIRE_DEV_HPP_ */
