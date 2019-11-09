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
#include "BaseClock.hpp"

BaseClock::BaseClock() : mTicks(0), mPaused(true)
{
}

void BaseClock::tick(int32_t t /*= 1*/)
{
    mTicks += t;

    if(mTicks < 0)
    {
        mTicks += 86400;
    }

    mTicks %= 86400;
}

void BaseClock::set(int32_t t)
{
    mTicks = 0;
    tick(t);
}

int32_t BaseClock::get() const
{
    return mTicks;
}

int8_t BaseClock::digit(int8_t d)
{
    switch(d)
    {
        case 0:
        {
            int16_t result = (int16_t)((((mTicks / 3600) % 24) / 10) % 10);
            return result == 0 ? -1 : result;
        }
        case 1:
        {
            return ((mTicks / 3600) % 24) % 10;
        }
        case 2:
        {
            return (((mTicks / 60) % 60) / 10) % 10;
        }
        case 3:
        {
            return ((mTicks / 60) % 60) % 10;
        }
    }

    return -1;
}

bool BaseClock::paused() const
{
    return mPaused;
}

void BaseClock::setPaused(bool paused)
{
    mPaused = paused;
}

void BaseClock::normalize()
{
    mTicks -= mTicks % 60;
}
