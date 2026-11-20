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
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <iterator>
#include <limits>
#include <optional>
#include <utility>
#include <vector>

#include "bot.hpp"
#include "file_handle.hpp"
#include "usercmd.h"

char welcome_msg[] = "Welcome to RicoBot NG v" RICOBOT_NG_VERSION "\n"
                     "Based on Ricobot by Wei Mingzhi\n"
                     "https://github.com/MiguelX413/ricobot\n"
                     "Compile date: " __DATE__ "\n";

DLL_GLOBAL const Vector g_vecZero = Vector(0, 0, 0);

int default_bot_skill = 2;
float bot_check_time = 30.0f;
int min_bots = -1;
int max_bots = -1;
int num_bots = 0;
int prev_num_bots = 0;
bool g_GameRules = false;
edict_t *listenserver_edict = nullptr;
float welcome_time = 0.0;
bool welcome_sent = false;

static FileHandle bot_cfg_fp;
bool need_to_open_cfg = true;
float bot_cfg_pause_time = 0.0f;
float respawn_time = 0.0f;
bool spawn_time_reset = false;
static bool map_started = false;
using IdleClock = std::chrono::steady_clock;
static constexpr auto sleep_timeout = std::chrono::minutes{10};
static IdleClock::time_point last_player_activity;
// Startup still runs bot.cfg and population setup before the first human joins.
enum class BotActivity { Startup, Awake, Idle };
static BotActivity bot_activity = BotActivity::Startup;
static std::vector<std::optional<std::array<float, 3>>> player_view_angles;

struct SleepingBot {
  edict_t *disconnecting_client;
  std::string name;
  int skill;
};
static std::vector<SleepingBot> sleeping_bots;
static IdleClock::time_point next_sleep_recreation;
static IdleClock::time_point sleep_kick_deadline;

static bool IsHumanClient(const edict_t *player) {
  return player && !player->free && (player->v.flags & FL_CLIENT) &&
         !(player->v.flags & (FL_FAKECLIENT | FL_PROXY));
}

static void RecordPlayerActivity() { last_player_activity = IdleClock::now(); }

std::array<std::string, MAX_BOT_WHINE> bot_whine;
int whine_count;
int recent_bot_whine[5];

void BotNameInit();
void ProcessBotCfgFile();

void ServerCommand();

void GameDLLInit() {
  char filename[256];
  last_player_activity = IdleClock::now();
  bot_activity = BotActivity::Startup;
  player_view_angles.clear();
  sleeping_bots.clear();

  // Register server command
  static char command_name[] = "bot";
  (*g_engfuncs.pfnAddServerCommand)(command_name, ServerCommand);

  whine_count = 0;

  // initialize the bots array of structures...
  for (auto &bot : bots) {
    bot = bot_t{};
    bot.kick_time = -1.0f;
  }

  std::ranges::fill(recent_bot_whine, -1);

  BotNameInit();

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
                             plugin_log_tag,   PT_STARTUP,    PT_ANYTIME};

// Metamod requesting info about this plugin
C_DLLEXPORT int Meta_Query(char *, plugin_info_t **pPlugInfo,
                           mutil_funcs_t *pMetaUtilFuncs) {
  *pPlugInfo = &Plugin_info;        // Give metamod our plugin_info struct
  gpMetaUtilFuncs = pMetaUtilFuncs; // Get metamod utility function table.
  return TRUE;
}

// Metamod attaching plugin to the server
C_DLLEXPORT int Meta_Attach(PLUG_LOADTIME, META_FUNCTIONS *pFunctionTable,
                            meta_globals_t *pMGlobals,
                            gamedll_funcs_t *pGamedllFuncs) {
  if (!pMGlobals) {
    LOG_ERROR(PLID, "Meta_Attach called with null pMGlobals");
    return FALSE;
  }

  gpMetaGlobals = pMGlobals;

  if (!pFunctionTable) {
    LOG_ERROR(PLID, "Meta_Attach called with null pFunctionTable");
    return FALSE;
  }

  memcpy(pFunctionTable, &gMetaFunctionTable, sizeof(META_FUNCTIONS));
  gpGamedllFuncs = pGamedllFuncs;

  GameDLLInit();

  return TRUE;
}

// Metamod detaching plugin from the server
C_DLLEXPORT int Meta_Detach(PLUG_LOADTIME, PL_UNLOAD_REASON) { return TRUE; }

static void AddBotFromCommand(edict_t *requester, const char *name,
                              const char *skill, bool pause_config) {
  if (bot_activity == BotActivity::Idle)
    return;
  BotCreate(requester, name, skill);
  // Give the engine time to initialize the bot before processing more config.
  if (pause_config)
    bot_cfg_pause_time = gpGlobals->time + 2.0f;
  bot_check_time = gpGlobals->time + 5.0f;
}

static int NormalizeBotLimit(int value) {
  return value < 0 || value > 31 ? 1 : value;
}

void ServerCommand() {
  if (strcmp(CMD_ARGV(1), "addbot") == 0) {
    AddBotFromCommand(nullptr, CMD_ARGV(2), CMD_ARGV(3),
                      /*pause_config=*/false);
  } else if (strcmp(CMD_ARGV(1), "min_bots") == 0) {
    min_bots = NormalizeBotLimit(atoi(CMD_ARGV(2)));

    printf("min_bots set to %d\n", min_bots);
  } else if (strcmp(CMD_ARGV(1), "max_bots") == 0) {
    max_bots = NormalizeBotLimit(atoi(CMD_ARGV(2)));

    printf("max_bots set to %d\n", max_bots);
  }
}

int DispatchSpawn(edict_t *pent) {
  if (gpGlobals->deathmatch) {
    const char *pClassname = const_cast<char *>(STRING(pent->v.classname));

    if (strcmp(pClassname, "worldspawn") == 0) {
      g_GameRules = true;
      map_started = true;
      player_view_angles.clear();

      bot_cfg_pause_time = 0.0f;
      respawn_time = 0.0f;
      spawn_time_reset = false;

      prev_num_bots = num_bots;
      num_bots = 0;

      bot_check_time = gpGlobals->time + 30.0f;
    }
  }

  RETURN_META_VALUE(MRES_IGNORED, 0);
}

BOOL ClientConnect(edict_t *pEntity, const char *pszName,
                   const char *pszAddress, char szRejectReason[128]) {
  if (gpGlobals->deathmatch) {
    // check if this client is the listen server client
    if (strcmp(pszAddress, "loopback") == 0)
      listenserver_edict =
          pEntity; // save the edict of the listen server client...

    // check if this is NOT a bot joining the server...
    if (strcmp(pszAddress, "127.0.0.1") != 0) {
      int count = 0;
      int i;
      // don't try to add bots for 60 seconds, give client time to get added
      bot_check_time = gpGlobals->time + 60.0f;

      for (i = 0; i < std::ssize(bots); i++) {
        if (bots[i].is_used) // count the number of bots in use
          count++;
      }

      // if there are currently more than the minimum number of bots running
      // then kick one of the bots off the server...
      if (count > min_bots && min_bots != -1) {
        for (i = 0; i < std::ssize(bots); i++) {
          if (bots[i].is_used && bots[i].pEdict && !bots[i].pEdict->free) {
            char cmd[80];
            const int length = snprintf(cmd, sizeof(cmd), "kick \"%s\"\n",
                                        STRING(bots[i].pEdict->v.netname));
            if (length >= 0 && static_cast<std::size_t>(length) < sizeof(cmd)) {
              SERVER_COMMAND(cmd); // kick the bot using (kick "name")
            }
            break;
          }
        }
      }
    }
  }

  RETURN_META_VALUE(MRES_IGNORED, 0);
}

static void ClientPutInServer(edict_t *player) {
  if (gpGlobals->deathmatch && player && !player->free &&
      !(player->v.flags & (FL_FAKECLIENT | FL_PROXY))) {
    RecordPlayerActivity();
    if (bot_activity != BotActivity::Awake) {
      bot_activity = BotActivity::Awake;
      for (auto &bot : bots)
        bot.fLastRunPlayerMoveTime = gpGlobals->time;
    }
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

    bool sleep_reset = false;
    for (auto &bot : sleeping_bots) {
      if (bot.disconnecting_client == pEntity) {
        bot.disconnecting_client = nullptr;
        sleep_reset = true;
        break;
      }
    }

    for (bot_t &bot : bots) {
      if (bot.pBotEnemy == pEntity)
        bot.pBotEnemy = nullptr;
      if (bot.killer_edict == pEntity) {
        bot.killer_edict = nullptr;
        bot.b_bot_say_killed = false;
      }
      if (bot.pEdict == pEntity) {
        bot.pEdict = nullptr;
        if (bot.is_used)
          bot.kick_time = sleep_reset ? -1.0f : gpGlobals->time;
        bot.is_used = false;
        bot.respawn_state = RespawnState::Unused;
      }
    }
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
    RecordPlayerActivity();

  // only allow custom commands if deathmatch mode and NOT dedicated server and
  // client sending command is the listen server client...

  if ((gpGlobals->deathmatch) && (!IS_DEDICATED_SERVER()) &&
      (pEntity == listenserver_edict)) {
    const char *pcmd = CMD_ARGV(0);
    const char *arg1 = CMD_ARGV(1);
    const char *arg2 = CMD_ARGV(2);
    char msg[80];

    if (FStrEq(pcmd, "addbot")) {
      if (IsHumanClient(pEntity))
        RecordPlayerActivity();
      AddBotFromCommand(pEntity, arg1, arg2, /*pause_config=*/false);

      RETURN_META(MRES_SUPERCEDE);
    }

    const auto console_value = [arg1]() -> std::optional<int> {
      if (arg1 && *arg1)
        return atoi(arg1);
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
        RecordPlayerActivity();
    }
  }
  RETURN_META(MRES_IGNORED);
}

static void BeginBotSleep() {
  bot_activity = BotActivity::Idle;
  next_sleep_recreation = IdleClock::now();
  sleep_kick_deadline = next_sleep_recreation + std::chrono::seconds{5};

  for (const auto &bot : bots) {
    if (!bot.is_used || !bot.pEdict || bot.pEdict->free)
      continue;

    const int user_id = GETPLAYERUSERID(bot.pEdict);
    if (user_id <= 0) {
      LOG_ERROR(PLID, "Could not reset sleeping bot: missing client user ID");
      continue;
    }

    sleeping_bots.push_back(
        {bot.pEdict, STRING(bot.pEdict->v.netname), bot.bot_skill + 1});
    char command[32];
    snprintf(command, sizeof(command), "kick #%d\n", user_id);
    SERVER_COMMAND(command);
  }
}

static void RecreateSleepingBots() {
  const auto now = IdleClock::now();
  const auto waiting_for_disconnect = [](const SleepingBot &bot) {
    return bot.disconnecting_client != nullptr;
  };
  if (std::ranges::any_of(sleeping_bots, waiting_for_disconnect)) {
    if (now < sleep_kick_deadline)
      return;
    std::erase_if(sleeping_bots, waiting_for_disconnect);
    LOG_ERROR(PLID, "Timed out waiting for sleeping bots to disconnect");
  }
  if (sleeping_bots.empty() || now < next_sleep_recreation)
    return;

  auto bot = std::move(sleeping_bots.back());
  sleeping_bots.pop_back();
  const std::string skill = std::to_string(bot.skill);
  BotCreate(nullptr, bot.name.c_str(), skill.c_str());
  next_sleep_recreation = now + std::chrono::seconds{2};
}

void StartFrame() {
  if (gpGlobals->deathmatch) {
    static int i, index;
    static float previous_time = -1.0f;

    int count;

    // if a new map has started then (MUST BE FIRST IN StartFrame)...
    if (previous_time >= 0.0f &&
        (map_started || gpGlobals->time + 0.1f < previous_time)) {
      bot_cfg_pause_time = 0.0f;
      respawn_time = 0.0f;
      spawn_time_reset = false;
      sleeping_bots.clear();
      for (auto &bot : bots) {
        bot.pEdict = nullptr;
        bot.pBotEnemy = nullptr;
        bot.killer_edict = nullptr;
        bot.b_bot_say_killed = false;
      }
      bot_cfg_fp = OpenMapConfig();

      if (bot_cfg_fp != nullptr) {
        for (index = 0; index < std::ssize(bots); index++) {
          bots[index].is_used = false;
          bots[index].respawn_state = RespawnState::Unused;
          bots[index].kick_time = -1.0f;
        }

        if (IS_DEDICATED_SERVER())
          bot_cfg_pause_time = gpGlobals->time + 5.0f;
        else
          bot_cfg_pause_time = gpGlobals->time + 20.0f;
      } else {
        count = 0;

        // mark the bots as needing to be respawned...
        for (index = 0; index < std::ssize(bots); index++) {
          if (count >= prev_num_bots) {
            bots[index].is_used = false;
            bots[index].respawn_state = RespawnState::Unused;
            bots[index].kick_time = -1.0f;
          }

          if (bots[index].is_used) // is this slot used?
          {
            bots[index].respawn_state = RespawnState::Pending;
            count++;
          }

          // check for any bots that were very recently kicked...
          const float kick_time = bots[index].kick_time;
          if (!bots[index].is_used && kick_time >= 0.0f &&
              kick_time <= previous_time && kick_time + 5.0f > previous_time) {
            bots[index].respawn_state = RespawnState::Pending;
            count++;
          } else if (!bots[index].is_used) {
            bots[index].respawn_state = RespawnState::Unused;
          }
          bots[index].kick_time = -1.0f;
        }

        // set the respawn time
        if (IS_DEDICATED_SERVER())
          respawn_time = gpGlobals->time + 5.0f;
        else
          respawn_time = gpGlobals->time + 20.0f;
      }

      bot_check_time = gpGlobals->time + 30.0f;
    }
    map_started = false;

    if (bot_activity == BotActivity::Awake && sleeping_bots.empty() &&
        IdleClock::now() - last_player_activity >= sleep_timeout)
      BeginBotSleep();

    if (!sleeping_bots.empty()) {
      RecreateSleepingBots();
      previous_time = gpGlobals->time;
      RETURN_META(MRES_IGNORED);
    }
    if (bot_activity == BotActivity::Idle) {
      previous_time = gpGlobals->time;
      RETURN_META(MRES_IGNORED);
    }

    if (!IS_DEDICATED_SERVER()) {
      if (listenserver_edict != nullptr && welcome_sent == false &&
          welcome_time < 1.0f) {
        // are they out of observer mode yet?
        if (IsAlive(listenserver_edict))
          welcome_time = gpGlobals->time + 5.0f; // welcome in 5 seconds
      }

      if (welcome_time > 0.0f && welcome_time < gpGlobals->time &&
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

    count = 0;

    for (auto &bot : bots) {
      if (bot.is_used && bot.respawn_state == RespawnState::Idle) {
        if (bot_activity == BotActivity::Awake)
          BotThink(&bot);

        count++;
      }
    }

    if (count > num_bots)
      num_bots = count;

    // are we currently respawning bots and is it time to spawn one yet?
    if (respawn_time > 1.0f && respawn_time <= gpGlobals->time) {
      index = 0;

      // find bot needing to be respawned...
      while (index < std::ssize(bots) &&
             bots[index].respawn_state != RespawnState::Pending)
        index++;

      if (index < std::ssize(bots)) {
        bots[index].respawn_state = RespawnState::InProgress;
        bots[index].is_used = false; // free up this slot

        // respawn 1 bot then wait a while (otherwise engine crashes)
        char c_skill[2];
        snprintf(c_skill, sizeof(c_skill), "%d", bots[index].bot_skill + 1);
        BotCreate(nullptr, bots[index].name.c_str(), c_skill);
        if (bots[index].respawn_state == RespawnState::InProgress)
          bots[index].respawn_state = RespawnState::Unused;

        respawn_time = gpGlobals->time + 2.0f; // set next respawn time

        bot_check_time = gpGlobals->time + 5.0f;
      } else
        respawn_time = 0.0f;
    }

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
          if (IsAlive(listenserver_edict)) {
            spawn_time_reset = true;

            if (respawn_time >= 1.0f)
              respawn_time = std::fmin(respawn_time, gpGlobals->time + 1.0f);

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

    // check if time to see if a bot needs to be created...
    if (bot_check_time < gpGlobals->time) {
      count = 0;
      bot_check_time = gpGlobals->time + 5.0f;

      for (i = 1; i <= gpGlobals->maxClients; i++) {
        const edict_t *pPlayer = INDEXENT(i);
        if (!FNullEnt(pPlayer) && !pPlayer->free &&
            pPlayer->v.flags & (FL_CLIENT | FL_FAKECLIENT))
          count++;
      }

      // if there are currently less than the maximum number of "players"
      // then add another bot using the default skill level...
      if (count < max_bots && max_bots != -1)
        BotCreate(nullptr, nullptr, nullptr);
    }

    previous_time = gpGlobals->time;
  }

  RETURN_META(MRES_IGNORED);
}

C_DLLEXPORT int GetEntityAPI(DLL_FUNCTIONS *pFunctionTable,
                             int interfaceVersion) {
  memset(pFunctionTable, 0, sizeof(DLL_FUNCTIONS));

  pFunctionTable->pfnSpawn = DispatchSpawn;
  pFunctionTable->pfnClientConnect = ClientConnect;
  pFunctionTable->pfnClientPutInServer = ClientPutInServer;
  pFunctionTable->pfnClientDisconnect = ClientDisconnect;
  pFunctionTable->pfnClientCommand = ClientCommand;
  pFunctionTable->pfnStartFrame = StartFrame;
  pFunctionTable->pfnCmdStart = CmdStart;

  return TRUE;
}

static std::optional<int> ParseConfigInt(const char *argument) {
  if (argument == nullptr)
    return std::nullopt;

  char *end = nullptr;
  errno = 0;
  const long value = std::strtol(argument, &end, 10);
  if (end == argument || *end != '\0' || errno == ERANGE ||
      value < std::numeric_limits<int>::min() ||
      value > std::numeric_limits<int>::max())
    return std::nullopt;

  return static_cast<int>(value);
}

void ProcessBotCfgFile() {
  constexpr std::size_t config_line_capacity = 256;
  std::array<char, config_line_capacity> cmd_line;
  std::size_t cmd_index = 0;
  bool line_too_long = false;
  char *cmd, *arg1, *arg2;

  if (bot_cfg_pause_time > gpGlobals->time)
    return;

  if (bot_cfg_fp == nullptr)
    return;

  int ch = fgetc(bot_cfg_fp.get());

  // skip any leading blanks
  while (ch == ' ')
    ch = fgetc(bot_cfg_fp.get());

  while (ch != EOF && ch != '\r' && ch != '\n') {
    if (ch == '\t') // convert tabs to spaces
      ch = ' ';

    if (cmd_index < cmd_line.size() - 1)
      cmd_line[cmd_index++] = static_cast<char>(ch);
    else
      line_too_long = true;

    ch = fgetc(bot_cfg_fp.get());

    // skip multiple spaces in input file
    while (cmd_index != 0 && cmd_line[cmd_index - 1] == ' ' && ch == ' ' &&
           !line_too_long)
      ch = fgetc(bot_cfg_fp.get());
  }

  if (ch == '\r') // is it a carriage return?
  {
    ch = fgetc(bot_cfg_fp.get()); // skip the linefeed
  }

  // if reached end of file, then close it
  if (ch == EOF) {
    bot_cfg_fp.reset();

    bot_cfg_pause_time = 0.0f;
  }

  if (line_too_long)
    return;

  cmd_line[cmd_index] = 0; // terminate the command line
  static std::array<char, config_line_capacity + 1> server_cmd;

  // copy the command line to a server command buffer...
  snprintf(server_cmd.data(), server_cmd.size(), "%s\n", cmd_line.data());

  cmd_index = 0;
  cmd = cmd_line.data();
  arg1 = arg2 = nullptr;

  // skip to blank or end of string...
  while (cmd_line[cmd_index] != ' ' && cmd_line[cmd_index] != 0)
    cmd_index++;

  if (cmd_line[cmd_index] == ' ') {
    cmd_line[cmd_index++] = 0;
    arg1 = &cmd_line[cmd_index];

    // skip to blank or end of string...
    while (cmd_line[cmd_index] != ' ' && cmd_line[cmd_index] != 0)
      cmd_index++;

    if (cmd_line[cmd_index] == ' ') {
      cmd_line[cmd_index++] = 0;
      arg2 = &cmd_line[cmd_index];
    }
  }

  if (cmd_line[0] == '#' || cmd_line[0] == 0)
    return; // return if comment or blank line

  if (strcmp(cmd, "addbot") == 0) {
    AddBotFromCommand(nullptr, arg1, arg2, /*pause_config=*/true);
    return;
  }

  const auto value = ParseConfigInt(arg1);

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

  SERVER_COMMAND(server_cmd.data());
}
