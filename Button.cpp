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
#include <avr/delay.h>
#include "Button.hpp"

Button::Button() : mProbeNum(0), mPin(0), mPinnum(0)
{
    mProbe[0] = mProbe[1] = false;
}

void Button::init(volatile uint8_t* pin, volatile uint8_t pinnum)
{
    mPin = pin;
    mPinnum = pinnum;
}

void Button::probe()
{
    mProbe[mProbeNum] = bit_is_clear(*mPin, mPinnum) ? true : false;

    ++mProbeNum;
    mProbeNum %= 2;
}

bool Button::pressed() const
{
    return mProbeNum == 1 && mProbe[0] && mProbe[1];
}

bool Button::waitForRelease()
{
    while(pressed())
    {
        _delay_ms(10);
    }
    return true;
}
