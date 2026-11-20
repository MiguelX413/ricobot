#ifndef RICOBOT_YAW_TURN_HPP
#define RICOBOT_YAW_TURN_HPP

// The ideal yaw is already normalized. The caller wraps the resulting yaw.
float TurnYaw(float current, float ideal, float speed);

#endif
