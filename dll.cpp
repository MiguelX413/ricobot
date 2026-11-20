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
// dll.cpp
//

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <optional>
#include <utility>
#include <vector>

#include "bot.hpp"
#include "bot_config.hpp"
#include "bot_lifecycle.hpp"
#include "file_handle.hpp"
#include "goldsrc.hpp"
#include "usercmd.h"

char welcome_msg[] = "Welcome to RicoBot NG v" RICOBOT_NG_VERSION "\n"
                     "Based on Ricobot by Wei Mingzhi\n"
                     "https://github.com/MiguelX413/ricobot\n"
                     "Compile date: " __DATE__ "\n";

DLL_GLOBAL const Vector g_vecZero = Vector(0, 0, 0);

int default_bot_skill = 2;
int min_bots = -1;
int max_bots = -1;
bool g_GameRules = false;
edict_t *listenserver_edict = nullptr;
float welcome_time = 0.0;
bool welcome_sent = false;

static FileHandle bot_cfg_fp;
bool need_to_open_cfg = true;
float bot_cfg_pause_time = 0.0f;
bool spawn_time_reset = false;
static bool map_started = false;
static std::vector<std::optional<std::array<float, 3>>> player_view_angles;

static bool IsHumanClient(const edict_t *player) {
  return player && !player->free && (player->v.flags & FL_CLIENT) &&
         !(player->v.flags & (FL_FAKECLIENT | FL_PROXY));
}

std::array<std::string, MAX_BOT_WHINE> bot_whine;
int whine_count;
int recent_bot_whine[5];

static void ProcessBotCfgFile();

void ServerCommand();

void GameDLLInit() {
  char filename[256];
  player_view_angles.clear();

  // Register server command
  static char command_name[] = "bot";
  (*g_engfuncs.pfnAddServerCommand)(command_name, ServerCommand);
  bot_lifecycle::Initialize();

  whine_count = 0;

  std::ranges::fill(recent_bot_whine, -1);

  if (!UTIL_BuildFileName(filename, "bot_whine.txt", nullptr))
    return;

  FileHandle bfp{fopen(filename, "r")};

  if (bfp != nullptr) {
    std::array<char, 80> buffer;
    while (whine_count < MAX_BOT_WHINE &&
           fgets(buffer.data(), buffer.size(), bfp.get()) != nullptr) {
      std::string message = buffer.data();
      if (!message.empty() && message.back() == '\n')
        message.pop_back();

      const auto player_placeholder = message.find("%n");
      if (player_placeholder != std::string::npos)
        message[player_placeholder + 1] = 's';

      if (!message.empty())
        bot_whine[whine_count++] = std::move(message);
    }
  }
}

gamedll_funcs_t *gpGamedllFuncs;
mutil_funcs_t *gpMetaUtilFuncs;
meta_globals_t *gpMetaGlobals;

META_FUNCTIONS gMetaFunctionTable = {
    GetEntityAPI,       nullptr, nullptr, nullptr, nullptr, nullptr,
    GetEngineFunctions, nullptr};

// Metamod's plugin info stores mutable pointers to these strings.
static char plugin_interface[] = META_INTERFACE_VERSION;
static char plugin_name[] = "RicoBot Next Generation";
static char plugin_version[] = RICOBOT_NG_VERSION;
static char plugin_date[] = __DATE__;
static char plugin_author[] = "Wei Mingzhi and MiguelX413";
static char plugin_url[] = "https://github.com/MiguelX413/ricobot";
static char plugin_log_tag[] = "RICOBOT";

plugin_info_t Plugin_info = {plugin_interface, plugin_name,   plugin_version,
                             plugin_date,      plugin_author, plugin_url,
                             plugin_log_tag,   PT_STARTUP,    PT_NEVER};

// Metamod negotiates the version advertised in Plugin_info.ifvers.
C_DLLEXPORT int Meta_Query(char *, plugin_info_t **pPlugInfo,
                           mutil_funcs_t *pMetaUtilFuncs) {
  if (!pPlugInfo || !pMetaUtilFuncs || !pMetaUtilFuncs->pfnCallGameEntity ||
      !pMetaUtilFuncs->pfnGetUserMsgID || !pMetaUtilFuncs->pfnLogError)
    return FALSE;

  *pPlugInfo = &Plugin_info;        // Give metamod our plugin_info struct
  gpMetaUtilFuncs = pMetaUtilFuncs; // Get metamod utility function table.
  return TRUE;
}

// Metamod attaching plugin to the server
C_DLLEXPORT int Meta_Attach(PLUG_LOADTIME, META_FUNCTIONS *pFunctionTable,
                            meta_globals_t *pMGlobals,
                            gamedll_funcs_t *pGamedllFuncs) {
  if (!gpGlobals || !gpMetaUtilFuncs || !pMGlobals || !pFunctionTable ||
      !pGamedllFuncs || !pGamedllFuncs->dllapi_table ||
      !pGamedllFuncs->dllapi_table->pfnClientConnect ||
      !pGamedllFuncs->dllapi_table->pfnClientPutInServer)
    return FALSE;

  gpMetaGlobals = pMGlobals;
  *pFunctionTable = gMetaFunctionTable;
  gpGamedllFuncs = pGamedllFuncs;

  GameDLLInit();

  return TRUE;
}

// The engine registration API has no matching removal operation. Keep the
// plugin resident across maps and replace it by restarting the server.
C_DLLEXPORT int Meta_Detach(PLUG_LOADTIME, PL_UNLOAD_REASON) { return FALSE; }

static void AddBotFromCommand(edict_t *requester, const char *name,
                              const char *skill, bool pause_config) {
  if (!bot_lifecycle::CanAddBot())
    return;
  BotCreate(requester, name, skill);
  // Give the engine time to initialize the bot before processing more config.
  if (pause_config)
    bot_cfg_pause_time = gpGlobals->time + 2.0f;
  bot_lifecycle::DelayPopulation(5.0f);
}

static int NormalizeBotLimit(int value) {
  return value < 0 || value > 31 ? 1 : value;
}

void ServerCommand() {
  if (CMD_ARGC() < 2)
    return;

  if (strcmp(CMD_ARGV(1), "addbot") == 0) {
    AddBotFromCommand(nullptr, CMD_ARGV(2), CMD_ARGV(3),
                      /*pause_config=*/false);
  } else if (strcmp(CMD_ARGV(1), "min_bots") == 0) {
    min_bots = NormalizeBotLimit(bot_config::ParseInt(CMD_ARGV(2)).value_or(0));

    printf("min_bots set to %d\n", min_bots);
  } else if (strcmp(CMD_ARGV(1), "max_bots") == 0) {
    max_bots = NormalizeBotLimit(bot_config::ParseInt(CMD_ARGV(2)).value_or(0));

    printf("max_bots set to %d\n", max_bots);
  }
}

int DispatchSpawn(edict_t *pent) {
  if (gpGlobals->deathmatch && pent && !pent->free) {
    const char *pClassname = STRING(pent->v.classname);

    if (strcmp(pClassname, "worldspawn") == 0) {
      g_GameRules = true;
      map_started = true;
      player_view_angles.clear();

      bot_cfg_pause_time = 0.0f;
      spawn_time_reset = false;
      bot_lifecycle::OnWorldSpawn();
    }
  }

  RETURN_META_VALUE(MRES_IGNORED, 0);
}

BOOL ClientConnect(edict_t *pEntity, const char *pszName,
                   const char *pszAddress, char szRejectReason[128]) {
  if (gpGlobals->deathmatch && pEntity && pszAddress) {
    // check if this client is the listen server client
    if (strcmp(pszAddress, "loopback") == 0)
      listenserver_edict =
          pEntity; // save the edict of the listen server client...

    // check if this is NOT a bot joining the server...
    if (strcmp(pszAddress, "127.0.0.1") != 0) {
      bot_lifecycle::HumanConnecting(min_bots);
    }
  }

  RETURN_META_VALUE(MRES_IGNORED, 0);
}

static void ClientPutInServer(edict_t *player) {
  if (gpGlobals->deathmatch && player && !player->free &&
      !(player->v.flags & (FL_FAKECLIENT | FL_PROXY))) {
    bot_lifecycle::HumanJoined();
  }

  RETURN_META(MRES_IGNORED);
}

void ClientDisconnect(edict_t *pEntity) {
  if (pEntity) {
    const int client_index = ENTINDEX(pEntity);
    if (client_index > 0 &&
        static_cast<std::size_t>(client_index) < player_view_angles.size())
      player_view_angles[client_index].reset();

    if (listenserver_edict == pEntity) {
      listenserver_edict = nullptr;
      welcome_time = 0.0f;
    }

    bot_lifecycle::ClientDisconnected(*pEntity);
  }

  RETURN_META(MRES_IGNORED);
}

static void SetBotToggle(bool &setting, std::optional<int> value) {
  if (value)
    setting = *value != 0;
}

static bool SetBotSetting(int &setting, std::optional<int> value, int minimum,
                          int maximum) {
  if (!value)
    return true;
  if (*value < minimum || *value > maximum)
    return false;
  setting = *value;
  return true;
}

static bool SetBotSkill(std::optional<int> value) {
  return SetBotSetting(default_bot_skill, value, 1, 5);
}

static bool SetBotChatPercent(std::optional<int> value) {
  return SetBotSetting(bot_chat_percent, value, 0, 100);
}

void ClientCommand(edict_t *pEntity) {
  if (gpGlobals->deathmatch && IsHumanClient(pEntity) && CMD_ARGC() > 1 &&
      (FStrEq(CMD_ARGV(0), "say") || FStrEq(CMD_ARGV(0), "say_team")))
    bot_lifecycle::RecordPlayerActivity();

  // only allow custom commands if deathmatch mode and NOT dedicated server and
  // client sending command is the listen server client...

  if (gpGlobals->deathmatch && pEntity && !pEntity->free &&
      !IS_DEDICATED_SERVER() && (pEntity == listenserver_edict)) {
    const char *pcmd = CMD_ARGV(0);
    const char *arg1 = CMD_ARGV(1);
    const char *arg2 = CMD_ARGV(2);
    char msg[80];

    if (FStrEq(pcmd, "addbot")) {
      if (IsHumanClient(pEntity))
        bot_lifecycle::RecordPlayerActivity();
      AddBotFromCommand(pEntity, arg1, arg2, /*pause_config=*/false);

      RETURN_META(MRES_SUPERCEDE);
    }

    const auto console_value = [arg1]() -> std::optional<int> {
      if (arg1 && *arg1)
        return bot_config::ParseInt(arg1).value_or(0);
      return std::nullopt;
    };

    if (FStrEq(pcmd, "observer")) {
      SetBotToggle(b_observer_mode, console_value());

      if (b_observer_mode)
        ClientPrint(pEntity, HUD_PRINTNOTIFY, "observer mode ENABLED\n");
      else
        ClientPrint(pEntity, HUD_PRINTNOTIFY, "observer mode DISABLED\n");

      RETURN_META(MRES_SUPERCEDE);
    } else if (FStrEq(pcmd, "botskill")) {
      if (!SetBotSkill(console_value()))
        ClientPrint(pEntity, HUD_PRINTNOTIFY, "invalid botskill value!\n");

      snprintf(msg, sizeof(msg), "botskill is %d\n", default_bot_skill);
      ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

      RETURN_META(MRES_SUPERCEDE);
    } else if (FStrEq(pcmd, "botdontshoot")) {
      SetBotToggle(b_botdontshoot, console_value());

      if (b_botdontshoot)
        ClientPrint(pEntity, HUD_PRINTNOTIFY, "botdontshoot ENABLED\n");
      else
        ClientPrint(pEntity, HUD_PRINTNOTIFY, "botdontshoot DISABLED\n");

      RETURN_META(MRES_SUPERCEDE);
    } else if (FStrEq(pcmd, "bot_chat_percent")) {
      if (!SetBotChatPercent(console_value()))
        ClientPrint(pEntity, HUD_PRINTNOTIFY,
                    "invalid bot_chat_percent value!\n");

      snprintf(msg, sizeof(msg), "bot_chat_percent is %d\n", bot_chat_percent);
      ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

      RETURN_META(MRES_SUPERCEDE);
    }
  }

  RETURN_META(MRES_IGNORED);
}

static FileHandle OpenMapConfig() {
  std::string map_config = STRING(gpGlobals->mapname);
  map_config += "_bot.cfg";
  char filename[256];
  if (!UTIL_BuildFileName(filename, "maps", map_config.c_str()))
    return {};
  return FileHandle{fopen(filename, "r")};
}

static void CmdStart(const edict_t *player, const usercmd_t *cmd,
                     unsigned int) {
  if (gpGlobals->deathmatch && cmd && IsHumanClient(player)) {
    const int index = g_engfuncs.pfnIndexOfEdict(player);
    if (index > 0 && index <= gpGlobals->maxClients) {
      if (player_view_angles.size() <= static_cast<std::size_t>(index))
        player_view_angles.resize(static_cast<std::size_t>(index) + 1);

      const std::array angles{cmd->viewangles[0], cmd->viewangles[1],
                              cmd->viewangles[2]};
      auto &previous = player_view_angles[index];
      const bool turned = previous && *previous != angles;
      previous = angles;

      if (turned || cmd->buttons || cmd->impulse || cmd->weaponselect ||
          cmd->forwardmove || cmd->sidemove || cmd->upmove)
        bot_lifecycle::RecordPlayerActivity();
    }
  }
  RETURN_META(MRES_IGNORED);
}

void StartFrame() {
  if (gpGlobals->deathmatch) {
    static float previous_time = -1.0f;

    // if a new map has started then (MUST BE FIRST IN StartFrame)...
    if (previous_time >= 0.0f &&
        (map_started || gpGlobals->time + 0.1f < previous_time)) {
      bot_cfg_pause_time = 0.0f;
      spawn_time_reset = false;
      bot_cfg_fp = OpenMapConfig();
      bot_lifecycle::OnMapChange(previous_time, bot_cfg_fp != nullptr);

      if (bot_cfg_fp != nullptr) {
        if (IS_DEDICATED_SERVER())
          bot_cfg_pause_time = gpGlobals->time + 5.0f;
        else
          bot_cfg_pause_time = gpGlobals->time + 20.0f;
      }
    }
    map_started = false;

    if (bot_lifecycle::PauseFrame()) {
      previous_time = gpGlobals->time;
      RETURN_META(MRES_IGNORED);
    }

    if (!IS_DEDICATED_SERVER()) {
      if (listenserver_edict != nullptr && welcome_sent == false &&
          welcome_time < 1.0f) {
        // are they out of observer mode yet?
        if (goldsrc::IsAlive(*listenserver_edict))
          welcome_time = gpGlobals->time + 5.0f; // welcome in 5 seconds
      }

      if (listenserver_edict && !listenserver_edict->free &&
          welcome_time > 0.0f && welcome_time < gpGlobals->time &&
          welcome_sent == false) {
        // send the welcome message to this client
        MESSAGE_BEGIN(MSG_ONE_UNRELIABLE, SVC_TEMPENTITY, nullptr,
                      listenserver_edict);
        WRITE_BYTE(TE_TEXTMESSAGE);
        WRITE_BYTE(1);
        WRITE_SHORT(-1);
        WRITE_SHORT(0);
        WRITE_BYTE(0);
        WRITE_BYTE(RANDOM_LONG(10, 255));
        WRITE_BYTE(RANDOM_LONG(10, 255));
        WRITE_BYTE(RANDOM_LONG(10, 255));
        WRITE_BYTE(1);
        WRITE_BYTE(RANDOM_LONG(10, 255));
        WRITE_BYTE(RANDOM_LONG(10, 255));
        WRITE_BYTE(RANDOM_LONG(10, 255));
        WRITE_BYTE(1);
        WRITE_SHORT(256);
        WRITE_SHORT(512);
        WRITE_SHORT(512);
        WRITE_STRING(welcome_msg);
        MESSAGE_END();
        char welcome_sound[] = "speak barney/guyresponsible\n";
        SERVER_COMMAND(welcome_sound);
        welcome_sent = true; // clear this so we only do it once
      }
    }

    bot_lifecycle::Think();
    bot_lifecycle::Respawn();

    if (g_GameRules) {
      if (need_to_open_cfg) // have we open bot.cfg file yet?
      {
        need_to_open_cfg = false; // only do this once!!!

        bot_cfg_fp = OpenMapConfig();
        char filename[256];
        if (bot_cfg_fp == nullptr &&
            UTIL_BuildFileName(filename, "bot.cfg", nullptr)) {
          bot_cfg_fp.reset(fopen(filename, "r"));
        }

        if (IS_DEDICATED_SERVER())
          bot_cfg_pause_time = gpGlobals->time + 5.0f;
        else
          bot_cfg_pause_time = gpGlobals->time + 20.0f;
      }

      if (!IS_DEDICATED_SERVER() && !spawn_time_reset) {
        if (listenserver_edict != nullptr) {
          if (goldsrc::IsAlive(*listenserver_edict)) {
            spawn_time_reset = true;

            bot_lifecycle::ExpediteRespawn();

            if (bot_cfg_pause_time >= 1.0f)
              bot_cfg_pause_time =
                  std::fmin(bot_cfg_pause_time, gpGlobals->time + 1.0f);
          }
        }
      }

      if (bot_cfg_fp && bot_cfg_pause_time >= 1.0f &&
          bot_cfg_pause_time <= gpGlobals->time) {
        // process bot.cfg file options...
        ProcessBotCfgFile();
      }
    }

    bot_lifecycle::Populate(max_bots);

    previous_time = gpGlobals->time;
  }

  RETURN_META(MRES_IGNORED);
}

C_DLLEXPORT int GetEntityAPI(DLL_FUNCTIONS *pFunctionTable,
                             int interfaceVersion) {
  if (!pFunctionTable || interfaceVersion != INTERFACE_VERSION)
    return FALSE;
  *pFunctionTable = DLL_FUNCTIONS{};

  pFunctionTable->pfnSpawn = DispatchSpawn;
  pFunctionTable->pfnClientConnect = ClientConnect;
  pFunctionTable->pfnClientPutInServer = ClientPutInServer;
  pFunctionTable->pfnClientDisconnect = ClientDisconnect;
  pFunctionTable->pfnClientCommand = ClientCommand;
  pFunctionTable->pfnStartFrame = StartFrame;
  pFunctionTable->pfnCmdStart = CmdStart;

  return TRUE;
}

static void ProcessBotCfgFile() {
  auto line = bot_config::ReadLine(*bot_cfg_fp);
  if (line.at_end) {
    bot_cfg_fp.reset();
    bot_cfg_pause_time = 0.0f;
  }
  if (line.text[0] == '\0')
    return;

  const char *cmd = line.text.data();
  const char *arg1 = line.first_arg ? cmd + *line.first_arg : nullptr;
  const char *arg2 = line.second_arg ? cmd + *line.second_arg : nullptr;

  if (strcmp(cmd, "addbot") == 0) {
    AddBotFromCommand(nullptr, arg1, arg2, /*pause_config=*/true);
    return;
  }

  const auto value = bot_config::ParseInt(arg1);

  if (strcmp(cmd, "botskill") == 0) {
    SetBotSkill(value);
    return;
  }

  if (strcmp(cmd, "observer") == 0) {
    SetBotToggle(b_observer_mode, value);
    return;
  }

  if (strcmp(cmd, "botdontshoot") == 0) {
    SetBotToggle(b_botdontshoot, value);
    return;
  }

  if (strcmp(cmd, "min_bots") == 0) {
    if (!value)
      return;
    min_bots = NormalizeBotLimit(*value);
    if (IS_DEDICATED_SERVER())
      printf("min_bots set to %d\n", min_bots);
    return;
  }

  if (strcmp(cmd, "max_bots") == 0) {
    if (!value)
      return;
    max_bots = NormalizeBotLimit(*value);
    if (IS_DEDICATED_SERVER())
      printf("max_bots set to %d\n", max_bots);
    return;
  }

  if (strcmp(cmd, "pause") == 0) {
    if (value)
      bot_cfg_pause_time = gpGlobals->time + *value;
    return;
  }

  if (strcmp(cmd, "bot_chat_percent") == 0) {
    SetBotChatPercent(value);
    return;
  }

  SERVER_COMMAND(line.server_command.data());
}
