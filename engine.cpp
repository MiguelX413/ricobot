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
// engine.cpp
//

#include "bot.hpp"
#include "bot_message.hpp"

static BotMessage bot_message;

void pfnClientCommand(edict_t *pEdict, char *szFmt, ...) {
  if (!pEdict || !(pEdict->v.flags & FL_FAKECLIENT))
    RETURN_META(MRES_IGNORED);
  RETURN_META(MRES_SUPERCEDE);
}

void pfnMessageBegin(int msg_dest, int msg_type, const float *pOrigin,
                     edict_t *ed) {
  bot_message = BotMessage(msg_dest, msg_type, ed);
  RETURN_META(MRES_IGNORED);
}

void pfnMessageEnd() {
  bot_message = BotMessage{};
  RETURN_META(MRES_IGNORED);
}

static void ParseBotMessageInteger(int value) {
  if (gpGlobals->deathmatch)
    bot_message.WriteInteger(value);
}

void pfnWriteByte(int iValue) {
  ParseBotMessageInteger(iValue);
  RETURN_META(MRES_IGNORED);
}

void pfnWriteChar(int iValue) {
  ParseBotMessageInteger(iValue);
  RETURN_META(MRES_IGNORED);
}

void pfnWriteShort(int iValue) {
  ParseBotMessageInteger(iValue);
  RETURN_META(MRES_IGNORED);
}

void pfnWriteLong(int iValue) {
  ParseBotMessageInteger(iValue);
  RETURN_META(MRES_IGNORED);
}

void pfnWriteEntity(int iValue) {
  ParseBotMessageInteger(iValue);
  RETURN_META(MRES_IGNORED);
}

void pfnWriteAngle(float) { RETURN_META(MRES_IGNORED); }

void pfnWriteCoord(float) { RETURN_META(MRES_IGNORED); }

void pfnWriteString(const char *) { RETURN_META(MRES_IGNORED); }

C_DLLEXPORT int GetEngineFunctions(enginefuncs_t *pengfuncsFromEngine,
                                   int *interfaceVersion) {
  if (!interfaceVersion)
    return FALSE;
  if (*interfaceVersion != ENGINE_INTERFACE_VERSION) {
    *interfaceVersion = ENGINE_INTERFACE_VERSION;
    return FALSE;
  }
  if (!pengfuncsFromEngine)
    return FALSE;
  *pengfuncsFromEngine = enginefuncs_t{};

  pengfuncsFromEngine->pfnClientCommand = pfnClientCommand;
  pengfuncsFromEngine->pfnMessageBegin = pfnMessageBegin;
  pengfuncsFromEngine->pfnMessageEnd = pfnMessageEnd;
  pengfuncsFromEngine->pfnWriteByte = pfnWriteByte;
  pengfuncsFromEngine->pfnWriteChar = pfnWriteChar;
  pengfuncsFromEngine->pfnWriteShort = pfnWriteShort;
  pengfuncsFromEngine->pfnWriteLong = pfnWriteLong;
  pengfuncsFromEngine->pfnWriteAngle = pfnWriteAngle;
  pengfuncsFromEngine->pfnWriteCoord = pfnWriteCoord;
  pengfuncsFromEngine->pfnWriteString = pfnWriteString;
  pengfuncsFromEngine->pfnWriteEntity = pfnWriteEntity;

  return TRUE;
}
