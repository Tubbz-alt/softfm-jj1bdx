///////////////////////////////////////////////////////////////////////////////////
// SoftFM - Software decoder for FM broadcast radio with stereo support          //
//                                                                               //
// Copyright (C) 2015 Edouard Griffiths, F4EXB                                   //
//                                                                               //
// This program is free software; you can redistribute it and/or modify          //
// it under the terms of the GNU General Public License as published by          //
// the Free Software Foundation as version 3 of the License, or                  //
//                                                                               //
// This program is distributed in the hope that it will be useful,               //
// but WITHOUT ANY WARRANTY; without even the implied warranty of                //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                  //
// GNU General Public License V3 for more details.                               //
//                                                                               //
// You should have received a copy of the GNU General Public License             //
// along with this program. If not, see <http://www.gnu.org/licenses/>.          //
///////////////////////////////////////////////////////////////////////////////////

#ifndef INCLUDE_FASTATAN2_H_
#define INCLUDE_FASTATAN2_H_

#include <cmath>
#include <cstdint>

// Fast arctan2
// See https://www.dsprelated.com/showarticle/1052.php
// for the algorithm

inline float fastatan2(float y, float x)
{
    const float pi = (float)(std::acos(-1.0));
    const float pi_2 = pi/2.0f;
    const float a1 = -0.060317f;
    const float a2 = -0.198146f;
    const float a3 = 1.044261f;
    const float a4 = -0.002178f;
    float result = 0.0f;
    if (x != 0.0f)
    {
        const union { float flVal; uint32_t nVal; } tYSign = { y };
        const union { float flVal; uint32_t nVal; } tXSign = { x };
        if (fabsf(x) >= fabsf(y))
        {
            union { float flVal; uint32_t nVal; } tOffset = { pi };
            // Add or subtract PI based on y's sign.
            tOffset.nVal |= tYSign.nVal & 0x80000000u;
            // No offset if x is positive, so multiply by 0 or based on x's sign.
            tOffset.nVal *= tXSign.nVal >> 31;
            result = tOffset.flVal;
            const float z = y / x;
            result += (((((a1 * z) + a2) * z) + a3) * z) + a4;
        }
        else // Use atan(y/x) = pi/2 - atan(x/y) if |y/x| > 1.
        {
            union { float flVal; uint32_t nVal; } tOffset = { pi_2 };
            // Add or subtract PI/2 based on y's sign.
            tOffset.nVal |= tYSign.nVal & 0x80000000u;
            result = tOffset.flVal;
            const float z = x / y;
            result -= (((((a1 * z) + a2) * z) + a3) * z) + a4;
        }
    }
    else if (y > 0.0f)
    {
        result = pi_2;
    }
    else if (y < 0.0f)
    {
        result = -pi_2;
    }
    return result;
}

#endif /* INCLUDE_FASTATAN2_H_ */
