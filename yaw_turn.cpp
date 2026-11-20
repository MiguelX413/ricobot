/*
Ricobot (C) Copyright 2004, Wei Mingzhi
All rights reserved.

Redistribution and use in source and binary forms with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

3. Neither the name of the author nor the names of its contributors may be
used to endorse or promote products derived from this software without
specific prior written permission.

THIS SOFTWARE IS PROVIDED BY WEI MINGZHI "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
EVENT SHALL THE CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "yaw_turn.hpp"

#include <cmath>

float TurnYaw(float current, float ideal, float speed) {
  if (!std::isfinite(current))
    current = 0.0f;

  float current_180;
  // find the difference in the current and ideal angle
  const float diff = std::fabs(current - ideal);

  // check if difference is less than the max degrees per turn
  if (diff < speed)
    speed = diff; // just need to turn a little bit (less than max)

  // here we have four cases, both angle positive, one positive and
  // the other negative, one negative and the other positive, or
  // both negative.  handle each case separately...
  if (current >= 0 && ideal >= 0) // both positive
  {
    if (current > ideal)
      speed = -speed;
  } else if (current >= 0 && ideal < 0) {
    current_180 = current - 180;
    if (current_180 <= ideal)
      speed = -speed;
  } else if (current < 0 && ideal >= 0) {
    current_180 = current + 180;
    if (current_180 <= ideal)
      speed = -speed;
  } else // (current < 0) && (ideal < 0)  both negative
  {
    if (current > ideal)
      speed = -speed;
  }

  return current + speed;
}
