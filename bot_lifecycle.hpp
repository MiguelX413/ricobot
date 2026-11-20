#ifndef RICOBOT_BOT_LIFECYCLE_HPP
#define RICOBOT_BOT_LIFECYCLE_HPP

struct edict_s;

namespace bot_lifecycle {

void Initialize();
void RecordPlayerActivity();
void HumanJoined();
void ClientDisconnected(edict_s &player);
void HumanConnecting(int min_bots);

bool CanAddBot();
void DelayPopulation(float seconds);
void OnWorldSpawn();
void OnMapChange(float previous_time, bool has_map_config);
bool PauseFrame();
void Think();
void Respawn();
void ExpediteRespawn();
void Populate(int max_bots);

} // namespace bot_lifecycle

#endif
