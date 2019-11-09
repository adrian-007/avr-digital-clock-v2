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

#ifndef BUFFEREDDISPLAYS_H_
#define BUFFEREDDISPLAYS_H_

#include <stdint.h>

#define DISPLAY_LENGTH 8

class BufferedDisplays
{
public:
    BufferedDisplays();

    void setDisplay(int8_t d);
    void setChar(int8_t c, bool dot = false);

    void setEnabled(int8_t d, bool enabled = false);
    bool getEnabled(int8_t d) const;

    void nextDisplay();

    void setBuffer(int8_t pos, int8_t value, bool dot = false);
    int8_t getBuffer(int8_t pos, bool dot = false);

private:
    enum
    {
        DIGIT = 0,
        DOT,
        STATE,
        LAST
    };

    volatile int8_t displayBuffer[LAST][DISPLAY_LENGTH];
    volatile int16_t currentDisplay;
};

#endif /* BUFFEREDDISPLAYS_H_ */
