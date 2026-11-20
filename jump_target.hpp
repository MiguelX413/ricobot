#ifndef RICOBOT_JUMP_TARGET_HPP
#define RICOBOT_JUMP_TARGET_HPP

#include <cstddef>
#include <span>

class Vector;

namespace jump_target {

Vector Select(const Vector &current, std::span<const Vector> candidates,
              std::size_t choice);
bool IsJumpHeight(float bot_bottom, float arrow_top);
bool IsBlocked(const Vector &target, const Vector &origin,
               const Vector &player_origin);

} // namespace jump_target

#endif
