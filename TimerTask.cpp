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
#include "TimerTask.hpp"

TimerTask::TimerTask() : mElapsed(0), mId(0)
{
}

void TimerTask::setId(int8_t id)
{
    mId = id;
}

void TimerTask::setElapsed(uint32_t elapsed)
{
    mElapsed = elapsed;
}

void TimerTask::tick()
{
    if(mElapsed > 0)
    {
        --mElapsed;
    }
}

int8_t TimerTask::id()
{
    return mId;
}

uint32_t TimerTask::elapsed()
{
    return mElapsed;
}

bool TimerTask::finished() const
{
    return mElapsed == 0;
}
