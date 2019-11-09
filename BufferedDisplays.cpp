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
#include "BufferedDisplays.hpp"

/**
 * Wiring layout in regard to 7 segments of signle digit on the display:
 *
 * a  - PC0
 * b  - PD6
 * c  - PD1
 * d  - PD3
 * e  - PD4
 * f  - PD7
 * g  - PD0
 * dp - PD2
 * dc - PD5
 *
 *      a
 *    ------
 * f |      | b
 *   |  g   |
 *    ------
 * e |      | c
 *   |      |
 *    ------ . dp
 *      d
 *
 */

BufferedDisplays::BufferedDisplays() : currentDisplay(0)
{
    for(uint8_t i = 0; i < LAST; ++i)
    {
        for(uint8_t j = 0; j < DISPLAY_LENGTH; ++j)
        {
            displayBuffer[i][j] = i == 2 ? 1 : -1;
        }
    }
}

void BufferedDisplays::setDisplay(int8_t d)
{
    PORTA |= 1 << 0 | 1 << 1 | 1 << 2 | 1 << 3;
    PORTC |= 1 << 2 | 1 << 3 | 1 << 4 | 1 << 5;

    switch(d)
    {
        case 0:
        {
            PORTC &= ~(1 << 5);
            break;
        }
        case 1:
        {
            PORTC &= ~(1 << 4);
            break;
        }
        case 2:
        {
            PORTC &= ~(1 << 3);
            break;
        }
        case 3:
        {
            PORTC &= ~(1 << 2);
            break;
        }
        case 4:
        {
            PORTA &= ~(1 << 0);
            break;
        }
        case 5:
        {
            PORTA &= ~(1 << 1);
            break;
        }
        case 6:
        {
            PORTA &= ~(1 << 2);
            break;
        }
        case 7:
        {
            PORTA &= ~(1 << 3);
            break;
        }
        default:
        {
            break;
        }
    }
}

void BufferedDisplays::setChar(int8_t c, bool dot /*= false*/)
{
    PORTD = ~(1 << 5); // without colon
    PORTC |= 1 << 0;

    switch(c)
    {
        //case 'O':
        case 0:
        {
            PORTC &= ~(1 << 0);
            PORTD &= ~(1 << 6 | 1 << 1 | 1 << 3 | 1 << 3 | 1 << 4 | 1 << 7);
            break;
        }
        case 1:
        {
            PORTD &= ~(1 << 6 | 1 << 1);
            break;
        }
        case 2:
        {
            PORTC &= ~(1 << 0);
            PORTD &= ~(1 << 6 | 1 << 0 | 1 << 4 | 1 << 3);
            break;
        }
        case 3:
        {
            PORTC &= ~(1 << 0);
            PORTD &= ~(1 << 6 | 1 << 0 | 1 << 1 | 1 << 3);
            break;
        }
        case 4:
        {
            PORTD &= ~(1 << 7 | 1 << 0 | 1 << 6 | 1 << 1);
            break;
        }
        case 5:
        {
            PORTC &= ~(1 << 0);
            PORTD &= ~(1 << 7 | 1 << 0 | 1 << 1 | 1 << 3);
            break;
        }
        case 6:
        {
            PORTC &= ~(1 << 0);
            PORTD &= ~(1 << 7 | 1 << 0 | 1 << 1 | 1 << 3 | 1 << 4);
            break;
        }
        case 7:
        {
            PORTC &= ~(1 << 0);
            PORTD &= ~(1 << 6 | 1 << 1);
            break;
        }
        case 8:
        {
            PORTC &= ~(1 << 0);
            PORTD &= ~(1 << 6 | 1 << 7 | 1 << 0 | 1 << 1 | 1 << 3 | 1 << 4);
            break;
        }
        case 9:
        {
            PORTC &= ~(1 << 0);
            PORTD &= ~(1 << 6 | 1 << 7 | 1 << 0 | 1 << 1 | 1 << 3);
            break;
        }
        case '-':
        {
            PORTD &= ~(1 << 0);
            break;
        }
        case ':':
        {
            PORTD &= ~(1 << 5);
            break;
        }
        /*case 'A':
        {
            PORTC &= ~(1 << 0);
            PORTD &= ~(1 << 6 | 1 << 7 | 1 << 0 | 1 << 1 | 1 << 4);
            break;
        }
        case 'F':
        {
            PORTC &= ~(1 << 0);
            PORTD &= ~(1 << 7 | 1 << 4 | 1 << 0);
            break;
        }
        case 'C':
        {
            PORTC &= ~(1 << 0);
            PORTD &= ~(1 << 7 | 1 << 4 | 1 << 3);
            break;
        }
        case 'c':
        {
            PORTD &= ~(1 << 0 | 1 << 3 | 1 << 4);
            break;
        }
        case 'o':
        {
            PORTD &= ~(1 << 0 | 1 << 1 | 1 << 3 | 1 << 4);
            break;
        }
        case 'n':
        {
            PORTD &= ~(1 << 0 | 1 << 1 | 1 << 4);
            break;
        }
        case 'f':
        {
            PORTD &= ~(1 << 0 | 1 << 4);
            break;
        }*/
        default:
        {
            break;
        }
    }

    if(dot)
    {
        PORTD &= ~(1 << 2);
    }
}

void BufferedDisplays::setEnabled(int8_t d, bool enabled /*= false*/)
{
    displayBuffer[STATE][d] = enabled ? 1 : -1;
}

bool BufferedDisplays::getEnabled(int8_t d) const
{
    return displayBuffer[STATE][d] > 0;
}

void BufferedDisplays::nextDisplay()
{
    setDisplay(-1);

    if(displayBuffer[STATE][currentDisplay] > 0)
    {
        setChar(displayBuffer[DIGIT][currentDisplay], displayBuffer[DOT][currentDisplay] > 0);
        setDisplay(currentDisplay);
    }
    else
    {
        setChar(-1);
    }

    currentDisplay++;
    currentDisplay %= DISPLAY_LENGTH;
}

void BufferedDisplays::setBuffer(int8_t pos, int8_t value, bool dot /*= false*/)
{
    displayBuffer[dot ? DOT : DIGIT][pos] = value;
}

int8_t BufferedDisplays::getBuffer(int8_t pos, bool dot /*= false*/)
{
    return displayBuffer[dot ? DOT : DIGIT][pos];
}
