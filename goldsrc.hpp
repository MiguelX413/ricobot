#ifndef RICOBOT_GOLDSRC_HPP
#define RICOBOT_GOLDSRC_HPP

struct edict_s;

namespace goldsrc {

// Returns null for a slot outside the client range or without an occupied
// edict.
edict_s *ClientEdict(int index);
bool IsAlive(const edict_s &player);

} // namespace goldsrc

#endif
