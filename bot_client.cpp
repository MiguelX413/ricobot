// This is an open source non-commercial project. Dear PVS-Studio, please check
// it. PVS-Studio Static Code Analyzer for C, C++, C#, and Java:
// http://www.viva64.com
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

//
// Ricobot - a bot example for Valve Software's Ricochet MOD
//
// bot_client.cpp
//

#include <iterator>

#include "bot.hpp"

static bot_t *ActiveBotAt(int index) {
  if (index < 0 || index >= std::ssize(bots))
    return nullptr;
  auto &bot = bots[index];
  return bot.is_used && bot.pEdict && !bot.pEdict->free ? &bot : nullptr;
}

void BotClient_Ricochet_DeathMsg(const int *value, int) {
  static int fields = 0;
  static int killer_index = 0;
  if (!value) {
    fields = 0;
    killer_index = 0;
    return;
  }

  if (fields == 0) {
    killer_index = *value;
    ++fields;
    return;
  }
  if (fields != 1)
    return;
  ++fields;

  const int victim_index = *value;
  if (victim_index < 1 || victim_index > gpGlobals->maxClients)
    return;

  if (auto *victim = UTIL_GetBotPointer(INDEXENT(victim_index))) {
    victim->killer_edict = killer_index >= 1 &&
                                   killer_index <= gpGlobals->maxClients &&
                                   killer_index != victim_index
                               ? INDEXENT(killer_index)
                               : nullptr;
  }
}

void BotClient_Ricochet_AmmoX(const int *value, int bot_index) {
  static int fields = 0;
  if (!value) {
    fields = 0;
    return;
  }

  if (fields >= 2 || ++fields != 2)
    return;
  if (auto *bot = ActiveBotAt(bot_index))
    bot->disc_number = *value;
}

void BotClient_Ricochet_Powerup(const int *value, int bot_index) {
  if (value) {
    if (auto *bot = ActiveBotAt(bot_index))
      bot->m_iPowerups = *value;
  }
}
