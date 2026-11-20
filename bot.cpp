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
// bot.cpp
//

#include <algorithm>
#include <iterator>
#include <limits>
#include <string_view>

#include "bot.hpp"
#include "goldsrc.hpp"
#include "jump_target.hpp"
#include "yaw_turn.hpp"

bool b_observer_mode = false;
bool b_botdontshoot = false;
int bot_chat_percent = 10;

void BotSpawnInit(bot_t *pBot) {
  // Fix by Cheeseh (RCBot)
  pBot->fUpdateTime = gpGlobals->time;
  pBot->fLastRunPlayerMoveTime = gpGlobals->time;

  // pBot->msecnum = 0;
  // pBot->msecdel = 0.0;
  // pBot->msecval = 0.0;

  pBot->pBotEnemy = nullptr;
  pBot->f_bot_see_enemy_time = gpGlobals->time;
  pBot->f_bot_find_enemy_time = gpGlobals->time;
  pBot->b_bot_say_killed = false;
  pBot->f_bot_say_killed = 0.0f;
}

void BotChangePitch(bot_t *pBot, float speed) {
  edict_t *pEdict = pBot->pEdict;
  float current_180; // current +/- 180 degrees

  ClampAngle(pEdict->v.idealpitch);
  if (!std::isfinite(pEdict->v.v_angle.x))
    pEdict->v.v_angle.x = 0.0f;

  // turn from the current v_angle pitch to the idealpitch by selecting
  // the quickest way to turn to face that direction
  float current = pEdict->v.v_angle.x;

  const float ideal = pEdict->v.idealpitch;

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

  current += speed;
  pEdict->v.v_angle.x = current;
}

void BotChangeYaw(bot_t *pBot, float speed) {
  auto &state = pBot->pEdict->v;
  ClampAngle(state.ideal_yaw);
  state.v_angle.y = TurnYaw(state.v_angle.y, state.ideal_yaw, speed);
}

// The next 2 functions are taken from POD-Bot source

bool IsDeadlyDrop(bot_t *pBot, const Vector &vecTargetPos) {
  // Returns if given location would hurt Bot with falling damage

  edict_t *pEdict = pBot->pEdict;
  const Vector vecBot = pEdict->v.origin;
  TraceResult tr;

  Vector vecMove = UTIL_VecToAngles(vecTargetPos - vecBot);
  vecMove.x = 0; // reset pitch to 0 (level horizontally)
  vecMove.z = 0; // reset roll to 0 (straight up and down)
  MAKE_VECTORS(vecMove);

  const Vector v_direction = (vecTargetPos - vecBot).Normalize(); // 1 unit long
  Vector v_check = vecBot;
  Vector v_down = vecBot;

  v_down.z = v_down.z - 1000.0f; // straight down 1000 units

  TRACE_HULL(v_check, v_down, ignore_monsters, head_hull, pEdict, &tr);

  // We're not on ground anymore ?
  if (tr.flFraction > 0.036f)
    tr.flFraction = 0.036f;

  float last_height = tr.flFraction * 1000.0f;        // height from ground
  float distance = (vecTargetPos - v_check).Length(); // distance from goal

  while (distance > 16.0f) {
    // move 10 units closer to the goal...
    v_check = v_check + v_direction * 16.0f;

    v_down = v_check;
    v_down.z = v_down.z - 1000.0f; // straight down 1000 units

    TRACE_HULL(v_check, v_down, ignore_monsters, head_hull, pEdict, &tr);

    // Wall blocking ?
    if (tr.fStartSolid)
      return false;

    const float height = tr.flFraction * 1000.0f; // height from ground

    // Drops more than 100 Units ?
    if (last_height < height - 100)
      return true;

    last_height = height;

    distance = (vecTargetPos - v_check).Length(); // distance from goal
  }

  return false;
}

// Adjust all Bot Body and View Angles to face an absolute Vector
void BotFacePosition(bot_t *pBot, Vector const &vecPos) {
  edict_t *pEdict = pBot->pEdict;
  Vector vecDirection = UTIL_VecToAngles(vecPos - GetGunPosition(pEdict));
  vecDirection.x = -vecDirection.x;
  ClampAngles(vecDirection);

  pEdict->v.ideal_yaw = vecDirection.y;
  pEdict->v.idealpitch = vecDirection.x;
}

void BotMoveToPosition(bot_t *pBot, Vector const &vecPos) {
  const edict_t *pEdict = pBot->pEdict;

  if (pBot->pBotEnemy == nullptr) {
    // No enemy, just head towards the position
    pBot->vecLookAt = vecPos;
    pBot->f_move_speed = pEdict->v.maxspeed;
  } else {
    // Otherwise we need to do strafing...
    const float f1 = pEdict->v.angles.y - VEC_TO_YAW(vecPos - pEdict->v.origin);
    const float flCos = std::cos(f1 * M_PI / 180.0);
    const float flSin = std::sin(f1 * M_PI / 180.0);
    pBot->f_move_speed = pEdict->v.maxspeed * flCos;
    pBot->f_sidemove_speed = pEdict->v.maxspeed * flSin;
  }
}

edict_t *BotFindEnemy(bot_t *pBot) {
  Vector vecEnd;
  edict_t *pNewEnemy = nullptr;

  edict_t *pEdict = pBot->pEdict;

  if (pBot->pBotEnemy != nullptr) // does the bot already have an enemy?
  {
    vecEnd = pBot->pBotEnemy->v.origin + pBot->pBotEnemy->v.view_ofs;

    // if the enemy is dead?
    if (!goldsrc::IsAlive(
            *pBot->pBotEnemy)) // is the enemy dead?, assume bot killed it
      pBot->pBotEnemy =
          nullptr; // don't have an enemy anymore so null out the pointer...
    else if (FInViewCone(&vecEnd, pEdict) && FVisible(vecEnd, pEdict) &&
             std::fabs(pBot->pEdict->v.origin.z - pBot->pBotEnemy->v.origin.z) <
                 72)
      return pBot->pBotEnemy; // if enemy is still visible and in field of view,
                              // keep it
  }

  float nearestdistance = 2500.0f;

  // search the world for players...
  for (int i = 1; i <= gpGlobals->maxClients; i++) {
    edict_t *pPlayer = goldsrc::ClientEdict(i);

    // skip invalid players and skip self (i.e. this bot)
    if (pPlayer && pPlayer != pEdict) {
      if (!goldsrc::IsAlive(*pPlayer))
        continue; // skip this player if not alive (i.e. dead or dying)

      if (b_observer_mode && !(pPlayer->v.flags & FL_FAKECLIENT))
        continue;

      if (pPlayer->v.playerclass != 0)
        continue; // skip this player if he is spectating

      if (pPlayer->v.groupinfo != pEdict->v.groupinfo)
        continue; // skip this player is he is in a different arena

      // see if the enemy is at the same height as the bot
      if (std::fabs(pBot->pEdict->v.origin.z - pPlayer->v.origin.z) >= 72)
        continue;

      vecEnd = pPlayer->v.origin + pPlayer->v.view_ofs;

      // see if bot can see the player...
      if (FInViewCone(&vecEnd, pEdict) && FVisible(vecEnd, pEdict)) {
        const float distance = (pPlayer->v.origin - pEdict->v.origin).Length();
        if (distance < nearestdistance) {
          nearestdistance = distance;
          pNewEnemy = pPlayer;
        }
      }
    }
  }
  return pNewEnemy;
}

void BotShootAtEnemy(bot_t *pBot) {
  edict_t *pEdict = pBot->pEdict;

  if (FNullEnt(pBot->pBotEnemy))
    return; // no enemy, no need to shoot

  // is the bot's crosshair on the enemy yet?
  const Vector vecDir =
      (pBot->vecLookAt - GetGunPosition(pBot->pEdict)).Normalize();
  UTIL_MakeVectors(pEdict->v.v_angle);

  if (DotProduct(gpGlobals->v_forward, vecDir) > 0.9f) {
    // we will likely hit the enemy, FIRE!!
    // TODO: bounce/teleport attack
    const float distance =
        (pEdict->v.origin - pBot->pBotEnemy->v.origin).Length();
    // if enemy is near enough, we have enough discs,
    // and we haven't "Power Shot" powerup...
    if (distance < 128 && pBot->disc_number >= 3 &&
        !(pBot->m_iPowerups & POW_HARD) &&
        RANDOM_LONG(1, 100) < (6 - pBot->bot_skill) * 20)
      pEdict->v.button |= IN_ATTACK2; // Decapitate him!!!!
    else
      pEdict->v.button |= IN_ATTACK;
  }
}

static Vector QueryJumpTarget(bot_t &bot) {
  const edict_t &edict = *bot.pEdict;
  Vector target = bot.vecTargetPos;

  // jump to another disc randomly. TODO: make this much more advanced
  if (target == Vector(0, 0, 0)) {
    // find a jump arrow randomly.
    // TODO: let bots go for powerups or find paths on purpose
    int iCount = 0;
    Vector vecWPT[8];

    edict_t *p = nullptr;
    while ((p = UTIL_FindEntityInSphere(p, edict.v.origin, 240.0f)) !=
           nullptr) {
      if (strcmp(STRING(p->v.classname), "trigger_jump") == 0) {
        const Vector v_dest = VecBModelOrigin(p);
        if (jump_target::IsJumpHeight(edict.v.absmin.z, p->v.absmax.z) &&
            !IsDeadlyDrop(&bot, v_dest)) {
          vecWPT[iCount++] = v_dest;
          if (iCount >= 8)
            break;
        }
      }
    }

    if (iCount > 0)
      target = jump_target::Select(
          target, std::span{vecWPT}.first(static_cast<std::size_t>(iCount)),
          static_cast<std::size_t>(RANDOM_LONG(0, iCount - 1)));
  }

  // try not to knock into other players
  if (target != Vector(0, 0, 0)) {
    for (int i = 1; i <= gpGlobals->maxClients; i++) {
      const edict_t *pPlayer = goldsrc::ClientEdict(i);

      if (!pPlayer)
        continue;

      if (pPlayer == &edict || pPlayer->v.playerclass != 0 ||
          pPlayer->v.groupinfo != edict.v.groupinfo ||
          pPlayer->v.flags & (FL_ONGROUND | FL_PARTIALGROUND))
        continue;

      if (jump_target::IsBlocked(target, edict.v.origin, pPlayer->v.origin)) {
        target = Vector(0, 0, 0);
        break;
      }
    }
  }

  return target;
}

void BotThink(bot_t *pBot) {
  // Fix by Cheeseh (RCBot)
  const float now = gpGlobals->time;
  const float last_move_time = pBot->fLastRunPlayerMoveTime;
  pBot->fLastRunPlayerMoveTime = now;

  constexpr float max_move_msec = std::numeric_limits<byte>::max();
  const float elapsed_msec = std::isfinite(now) && std::isfinite(last_move_time)
                                 ? (now - last_move_time) * 1000.0f
                                 : 0.0f;
  // A map clock reset yields zero elapsed time for this move.
  const auto msecval =
      static_cast<byte>(std::clamp(elapsed_msec, 0.0f, max_move_msec));

  constexpr float fUpdateInterval = 1.0f / 60.0f; // update at 60 fps
  pBot->fUpdateTime = now + fUpdateInterval;

  Vector v_diff; // vector from previous to current location
  TraceResult tr;

  edict_t *pEdict = pBot->pEdict;

  pEdict->v.flags |= FL_FAKECLIENT;

  if (pBot->name.empty())
    pBot->name = std::string_view(STRING(pBot->pEdict->v.netname))
                     .substr(0, BOT_NAME_LEN);

  // TheFatal - START from Advanced Bot Framework (Thanks Rich!)
  /*
  // adjust the millisecond delay based on the frame rate interval...
  if (pBot->msecdel <= gpGlobals->time)
  {
         pBot->msecdel = gpGlobals->time + 0.5;
         if (pBot->msecnum > 0)
                pBot->msecval = 450.0 / pBot->msecnum;
         pBot->msecnum = 0;
  }
  else
         pBot->msecnum++;

  if (pBot->msecval < 1)    // don't allow msec to be less than 1...
         pBot->msecval = 1;

  if (pBot->msecval > 100)  // ...or greater than 100
         pBot->msecval = 100;

// TheFatal - END
*/
  pEdict->v.button = 0;
  pBot->f_move_speed = 0.0f;
  pBot->f_sidemove_speed = 0.0f;

  if (FNullEnt(pBot->killer_edict) || pBot->killer_edict->free) {
    pBot->killer_edict = nullptr;
    pBot->b_bot_say_killed = false;
  }

  if (pBot->b_bot_say_killed && pBot->f_bot_say_killed < gpGlobals->time) {
    int whine_index = 0;
    pBot->b_bot_say_killed = false;

    for (std::size_t attempt = 0; attempt < std::size(recent_bot_whine);
         ++attempt) {
      whine_index = RANDOM_LONG(0, whine_count - 1);
      if (std::ranges::find(recent_bot_whine, whine_index) ==
          std::ranges::end(recent_bot_whine))
        break;
    }

    std::ranges::copy_backward(std::ranges::begin(recent_bot_whine),
                               std::ranges::end(recent_bot_whine) - 1,
                               std::ranges::end(recent_bot_whine));
    recent_bot_whine[0] = whine_index;

    std::string message = bot_whine[whine_index];
    const auto player_placeholder = message.find("%s");
    if (pBot->killer_edict != nullptr &&
        player_placeholder != std::string::npos)
      message.replace(player_placeholder, 2,
                      std::string_view(STRING(pBot->killer_edict->v.netname))
                          .substr(0, BOT_NAME_LEN));

    constexpr std::size_t max_chat_length = 119;
    if (message.size() > max_chat_length)
      message.resize(max_chat_length);
    UTIL_HostSay(pEdict, message.c_str());
  }

  // if the bot is dead, randomly press fire to respawn...
  if (pEdict->v.health < 1 || pEdict->v.deadflag != DEAD_NO) {
    if (pBot->need_to_initialize) {
      BotSpawnInit(pBot);

      // did another player kill this bot AND bot whine messages loaded
      if (pBot->killer_edict != nullptr && whine_count > 0) {
        if (RANDOM_LONG(1, 100) <= bot_chat_percent) {
          pBot->b_bot_say_killed = true;
          pBot->f_bot_say_killed = gpGlobals->time + RANDOM_FLOAT(0.5f, 2.5f);
        }
      }

      pBot->need_to_initialize = false;
    }

    if (RANDOM_LONG(1, 100) > 50)
      pEdict->v.button = IN_ATTACK;

    g_engfuncs.pfnRunPlayerMove(pEdict, pEdict->v.v_angle, 0, 0, 0,
                                pEdict->v.button, 0, msecval);

    return;
  }

  // set this for the next time the bot dies so it will initialize stuff
  if (pBot->need_to_initialize == false)
    pBot->need_to_initialize = true;

  const float frame_time = gpGlobals->frametime;
  const float target_fps = CVAR_GET_FLOAT("fps_max");
  const double speed_factor =
      std::isfinite(frame_time) && frame_time > 0.0f &&
              std::isfinite(target_fps) && target_fps > 0.0f
          ? std::max(1.0, static_cast<double>(frame_time) * target_fps)
          : 1.0;
  const auto turn_speed = [speed_factor](float speed) {
    if (!std::isfinite(speed) || speed <= 0.0f)
      return 0.0f;
    return static_cast<float>(
        std::min(180.0, static_cast<double>(speed) * speed_factor));
  };

  BotChangeYaw(pBot, turn_speed(pEdict->v.yaw_speed));
  BotChangePitch(pBot, turn_speed(pEdict->v.pitch_speed));

  // set the body angles to point the gun correctly
  pEdict->v.angles.x = -pEdict->v.v_angle.x / 3;
  pEdict->v.angles.y = pEdict->v.v_angle.y;
  ClampAngles(pEdict->v.angles);
  ClampAngles(pEdict->v.v_angle);

  // handle movement related actions...
  if (b_botdontshoot == 0)
    pBot->pBotEnemy = BotFindEnemy(pBot);
  else
    pBot->pBotEnemy = nullptr; // clear enemy pointer (no ememy for you!)

  if (pBot->pBotEnemy != nullptr) // does an enemy exist?
  {
    // Aim at the enemy
    pBot->vecLookAt = pBot->pBotEnemy->v.origin;
    // TODO: bounce/teleport attack
    BotShootAtEnemy(pBot); // shoot at the enemy
  }

  if (pEdict->v.flags & (FL_ONGROUND | FL_PARTIALGROUND)) {
    pBot->vecTargetPos = QueryJumpTarget(*pBot);

    // move to the destination position
    if (pBot->vecTargetPos != Vector(0, 0, 0))
      BotMoveToPosition(pBot, pBot->vecTargetPos);
  } else
    pBot->vecTargetPos = Vector(0, 0, 0);

  BotFacePosition(pBot, pBot->vecLookAt);

  g_engfuncs.pfnRunPlayerMove(pEdict, pEdict->v.v_angle, pBot->f_move_speed,
                              pBot->f_sidemove_speed, 0, pEdict->v.button,
                              pEdict->v.impulse, msecval);
}
