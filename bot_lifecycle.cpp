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

#include "bot_lifecycle.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iterator>
#include <string_view>
#include <utility>
#include <vector>

#include "bot.hpp"
#include "bot_config.hpp"
#include "file_handle.hpp"
#include "goldsrc.hpp"

bot_t bots[BOT_CAPACITY];

static int number_names = 0;
static std::array<std::string, MAX_BOT_NAMES> bot_names;

static void SanitizeBotName(std::string &name, bool allow_spaces) {
  std::erase_if(name, [allow_spaces](char character) {
    return character < ' ' || (!allow_spaces && character == ' ') ||
           character > '~' || character == '"';
  });
}

static void BotNameInit() {
  char bot_name_filename[256];

  if (!UTIL_BuildFileName(bot_name_filename, "bot_names.txt", nullptr))
    return;

  FileHandle bot_name_fp{fopen(bot_name_filename, "r")};

  if (bot_name_fp != nullptr) {
    char name_buffer[80];
    while (number_names < MAX_BOT_NAMES &&
           fgets(name_buffer, 80, bot_name_fp.get()) != nullptr) {
      std::string name = name_buffer;
      SanitizeBotName(name, /*allow_spaces=*/true);
      if (!name.empty()) {
        name.resize(
            std::min(name.size(), static_cast<std::size_t>(BOT_NAME_LEN)));
        bot_names[number_names++] = std::move(name);
      }
    }
  }
}

static std::string BotPickName() {
  int index;
  int attempts = 0;

  // see if a name exists from a kicked bot (if so, reuse it)
  for (const auto &bot : bots) {
    if (!bot.is_used && bot.respawn_state != RespawnState::Pending &&
        !bot.name.empty())
      return bot.name;
  }

  int name_index = RANDOM_LONG(1, number_names) - 1; // zero based

  // check make sure this name isn't used
  bool used = true;

  while (used) {
    used = false;

    for (index = 1; index <= gpGlobals->maxClients; index++) {
      const edict_t *pPlayer = goldsrc::ClientEdict(index);

      if (pPlayer) {
        if (bot_names[name_index] == STRING(pPlayer->v.netname)) {
          used = true;
          break;
        }
      }
    }

    if (used) {
      name_index++;

      if (name_index == number_names)
        name_index = 0;

      attempts++;

      if (attempts == number_names)
        used = false; // break out of loop even if already used
    }
  }

  return bot_names[name_index];
}

void BotCreate(edict_t *pPlayer, const char *pname, const char *pskill) {
  std::string name;
  if (pname != nullptr && *pname != '\0')
    name = std::string_view(pname).substr(0, BOT_NAME_LEN - 1);
  else
    name = number_names > 0 ? BotPickName() : "Bot";

  int skill = bot_config::ParseInt(pskill).value_or(default_bot_skill);
  if (skill < 1 || skill > 5)
    skill = default_bot_skill;

  SanitizeBotName(name, /*allow_spaces=*/false);

  int index = 0;
  while (index < std::ssize(bots) &&
         (bots[index].is_used ||
          bots[index].respawn_state == RespawnState::Pending))
    index++;

  if (index == std::ssize(bots)) {
    if (pPlayer)
      ClientPrint(pPlayer, HUD_PRINTNOTIFY, "Can't create bot!\n");
    return;
  }

  edict_t *BotEnt = (*g_engfuncs.pfnCreateFakeClient)(name.c_str());

  if (FNullEnt(BotEnt)) {
    if (pPlayer)
      ClientPrint(pPlayer, HUD_PRINTNOTIFY,
                  "Max. Players reached.  Can't create bot!\n");
  } else {
    char reject_reason[128]{};

    if (BotEnt->pvPrivateData != nullptr)
      FREE_PRIVATE(BotEnt);
    BotEnt->pvPrivateData = nullptr;
    BotEnt->v.frags = 0;

    // create the player entity by calling MOD's player function
    // (from LINK_ENTITY_TO_CLASS for player object)
    if (!CALL_GAME_ENTITY(PLID, "player", VARS(BotEnt)) ||
        !MDLL_ClientConnect(BotEnt, name.c_str(), "127.0.0.1", reject_reason)) {
      reject_reason[sizeof(reject_reason) - 1] = '\0';
      LOG_ERROR(PLID, "Could not connect fake client: %s",
                reject_reason[0] ? reject_reason
                                 : "game DLL initialization failed");

      // Drop the engine client slot through the server, using its unique ID.
      const int user_id = GETPLAYERUSERID(BotEnt);
      if (user_id > 0) {
        char command[32];
        snprintf(command, sizeof(command), "kick #%d\n", user_id);
        SERVER_COMMAND(command);
      } else {
        LOG_ERROR(PLID, "Could not get the rejected fake client's user ID");
      }
      if (pPlayer)
        ClientPrint(pPlayer, HUD_PRINTNOTIFY, "Can't create bot!\n");
      return;
    }
    MDLL_ClientPutInServer(BotEnt);

    BotEnt->v.flags |= FL_FAKECLIENT;

    bot_t *pBot = &bots[index]; // initialize all the variables for this bot...
    *pBot = bot_t{};

    pBot->is_used = true;
    pBot->respawn_state = RespawnState::Idle;
    pBot->create_time = gpGlobals->time;
    pBot->kick_time = -1.0f;

    pBot->pEdict = BotEnt;

    BotSpawnInit(pBot);

    BotEnt->v.idealpitch = BotEnt->v.v_angle.x;
    BotEnt->v.ideal_yaw = BotEnt->v.v_angle.y;
    BotEnt->v.pitch_speed = BOT_PITCH_SPEED;
    BotEnt->v.yaw_speed = BOT_YAW_SPEED;

    pBot->bot_skill = skill - 1; // 0 based for array indexes
  }
}

namespace bot_lifecycle {

static float bot_check_time = 30.0f;
static float respawn_time = 0.0f;
static int num_bots = 0;
static int prev_num_bots = 0;
using IdleClock = std::chrono::steady_clock;
static char sleep_minutes_name[] = "ricobot_sleep_minutes";
static char sleep_minutes_default[] = "10";
static cvar_t sleep_minutes{sleep_minutes_name, sleep_minutes_default,
                            FCVAR_EXTDLL, 10.0f, nullptr};
static IdleClock::time_point last_player_activity;
// Startup still runs bot.cfg and population setup before the first human joins.
enum class BotActivity { Startup, Awake, Idle };
static BotActivity bot_activity = BotActivity::Startup;

struct SleepingBot {
  edict_t *disconnecting_client;
  std::string name;
  int skill;
};
static std::vector<SleepingBot> sleeping_bots;
static IdleClock::time_point next_sleep_recreation;
static IdleClock::time_point sleep_kick_deadline;

static bool IdleTimeoutReached() {
  using Minutes = std::chrono::duration<double, std::ratio<60>>;
  const float timeout = CVAR_GET_FLOAT(sleep_minutes_name);
  return std::isfinite(timeout) && timeout > 0.0f &&
         IdleClock::now() - last_player_activity >= Minutes{timeout};
}

void RecordPlayerActivity() { last_player_activity = IdleClock::now(); }

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

void Initialize() {
  last_player_activity = IdleClock::now();
  bot_activity = BotActivity::Startup;
  sleeping_bots.clear();

  if (!CVAR_GET_POINTER(sleep_minutes_name))
    CVAR_REGISTER(&sleep_minutes);

  // initialize the bots array of structures...
  for (auto &bot : bots) {
    bot = bot_t{};
    bot.kick_time = -1.0f;
  }

  BotNameInit();
}

void HumanJoined() {
  RecordPlayerActivity();
  if (bot_activity != BotActivity::Awake) {
    bot_activity = BotActivity::Awake;
    for (auto &bot : bots)
      bot.fLastRunPlayerMoveTime = gpGlobals->time;
  }
}

void ClientDisconnected(edict_t &player) {
  bool sleep_reset = false;
  for (auto &bot : sleeping_bots) {
    if (bot.disconnecting_client == &player) {
      bot.disconnecting_client = nullptr;
      sleep_reset = true;
      break;
    }
  }

  for (bot_t &bot : bots) {
    if (bot.pBotEnemy == &player)
      bot.pBotEnemy = nullptr;
    if (bot.killer_edict == &player) {
      bot.killer_edict = nullptr;
      bot.b_bot_say_killed = false;
    }
    if (bot.pEdict == &player) {
      bot.pEdict = nullptr;
      if (bot.is_used)
        bot.kick_time = sleep_reset ? -1.0f : gpGlobals->time;
      bot.is_used = false;
      bot.respawn_state = RespawnState::Unused;
    }
  }
}

void HumanConnecting(int min_bots) {
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

bool CanAddBot() { return bot_activity != BotActivity::Idle; }

void DelayPopulation(float seconds) {
  bot_check_time = gpGlobals->time + seconds;
}

void OnWorldSpawn() {
  respawn_time = 0.0f;
  prev_num_bots = num_bots;
  num_bots = 0;

  bot_check_time = gpGlobals->time + 30.0f;
}

void OnMapChange(float previous_time, bool has_map_config) {
  respawn_time = 0.0f;
  int count;
  int index;
  sleeping_bots.clear();
  for (auto &bot : bots) {
    bot.pEdict = nullptr;
    bot.pBotEnemy = nullptr;
    bot.killer_edict = nullptr;
    bot.b_bot_say_killed = false;
  }
  if (has_map_config) {
    for (index = 0; index < std::ssize(bots); index++) {
      bots[index].is_used = false;
      bots[index].respawn_state = RespawnState::Unused;
      bots[index].kick_time = -1.0f;
    }
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

bool PauseFrame() {
  if (bot_activity == BotActivity::Awake && sleeping_bots.empty() &&
      IdleTimeoutReached())
    BeginBotSleep();

  if (!sleeping_bots.empty()) {
    RecreateSleepingBots();
    return true;
  }
  return bot_activity == BotActivity::Idle;
}

void Think() {
  int count = 0;

  for (auto &bot : bots) {
    if (bot.is_used && bot.respawn_state == RespawnState::Idle) {
      // Ricochet's Spawn can reset the fake-client flag.
      if (!bot.pEdict || bot.pEdict->free) {
        bot.pEdict = nullptr;
        bot.pBotEnemy = nullptr;
        bot.killer_edict = nullptr;
        bot.is_used = false;
        bot.respawn_state = RespawnState::Unused;
        bot.kick_time = -1.0f;
        continue;
      }
      if (bot_activity == BotActivity::Awake)
        BotThink(&bot);

      count++;
    }
  }

  if (count > num_bots)
    num_bots = count;
}

void Respawn() {
  // are we currently respawning bots and is it time to spawn one yet?
  if (respawn_time > 1.0f && respawn_time <= gpGlobals->time) {
    int index = 0;

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
}

void ExpediteRespawn() {
  if (respawn_time >= 1.0f)
    respawn_time = std::fmin(respawn_time, gpGlobals->time + 1.0f);
}

void Populate(int max_bots) {
  // check if time to see if a bot needs to be created...
  if (bot_check_time < gpGlobals->time) {
    int count = 0;
    bot_check_time = gpGlobals->time + 5.0f;

    for (int i = 1; i <= gpGlobals->maxClients; i++) {
      const edict_t *pPlayer = goldsrc::ClientEdict(i);
      if (pPlayer && pPlayer->v.flags & (FL_CLIENT | FL_FAKECLIENT))
        count++;
    }

    // if there are currently less than the maximum number of "players"
    // then add another bot using the default skill level...
    if (count < max_bots && max_bots != -1)
      BotCreate(nullptr, nullptr, nullptr);
  }
}

} // namespace bot_lifecycle
