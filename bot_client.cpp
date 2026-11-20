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
#include "bot_message.hpp"
#include "goldsrc.hpp"

static bot_t *ActiveBotAt(int index) {
  if (index < 0 || index >= std::ssize(bots))
    return nullptr;
  auto &bot = bots[index];
  return bot.is_used && bot.pEdict && !bot.pEdict->free ? &bot : nullptr;
}

BotMessage::BotMessage(int destination, int message_id, edict_t *recipient) {
  if (!gpGlobals->deathmatch || message_id <= 0)
    return;

  bot_index_ = UTIL_GetBotIndex(recipient);
  const bool broadcast = destination == MSG_BROADCAST || destination == MSG_ALL;

  if ((broadcast || bot_index_ >= 0) &&
      message_id == GET_USER_MSG_ID(PLID, "DeathMsg", nullptr)) {
    type_ = Type::Death;
  } else if (bot_index_ >= 0) {
    if (message_id == GET_USER_MSG_ID(PLID, "AmmoX", nullptr))
      type_ = Type::Ammo;
    else if (message_id == GET_USER_MSG_ID(PLID, "Powerup", nullptr))
      type_ = Type::Powerup;
  }
}

void BotMessage::WriteInteger(int value) {
  switch (type_) {
  case Type::Ignored:
    return;
  case Type::Death:
    if (fields_ == 0) {
      killer_index_ = value;
      ++fields_;
    } else if (fields_ == 1) {
      ++fields_;
      if (auto *victim = UTIL_GetBotPointer(goldsrc::ClientEdict(value))) {
        victim->killer_edict = killer_index_ != value
                                   ? goldsrc::ClientEdict(killer_index_)
                                   : nullptr;
      }
    }
    return;
  case Type::Ammo:
    if (fields_ < 2 && ++fields_ == 2) {
      if (auto *bot = ActiveBotAt(bot_index_))
        bot->disc_number = value;
    }
    return;
  case Type::Powerup:
    if (auto *bot = ActiveBotAt(bot_index_))
      bot->m_iPowerups = value;
    return;
  }
}
