#ifndef RICOBOT_BOT_MESSAGE_HPP
#define RICOBOT_BOT_MESSAGE_HPP

struct edict_s;

class BotMessage {
public:
  BotMessage() = default;
  BotMessage(int destination, int message_id, edict_s *recipient);

  void WriteInteger(int value);

private:
  enum class Type { Ignored, Death, Ammo, Powerup };

  Type type_ = Type::Ignored;
  int bot_index_ = -1;
  int fields_ = 0;
  int killer_index_ = 0;
};

#endif
