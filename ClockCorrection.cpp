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
#include "ClockCorrection.hpp"

ClockCorrection::ClockCorrection() : BaseClock(), correction(0)
{
}

void ClockCorrection::tick(int32_t t /*= 1*/)
{
    if(t == 0)
    {
        return;
    }

    mTicks += t > 0 ? 1 : -1;

    if(mTicks > 999)
    {
        mTicks = -999;
    }
    else if(mTicks < -999)
    {
        mTicks = 999;
    }
}

int8_t ClockCorrection::digit(int8_t d)
{
    int32_t val = mTicks;

    if(val < 0)
    {
        val *= -1;
    }

    switch(d)
    {
        case 0:
        {
            if(mTicks < 0)
            {
                return '-';
            }
            return -1;
        }
        case 1:
        {
            return ((val / 100) % 10);
        }
        case 2:
        {
            return ((val / 10) % 10);
        }
        case 3:
        {
            return (val % 10);
        }
    }

    return -1;
}

int32_t ClockCorrection::checkCorrection()
{
    if(mTicks == 0)
    {
        return 0;
    }

    const int32_t abs = mTicks < 0 ? (mTicks * -1) : mTicks;

    if(correction++ >= abs)
    {
        correction = 0;

        return mTicks < 0 ? -1 : 1;
    }

    return 0;
}
