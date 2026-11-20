#ifndef RICOBOT_BOT_CONFIG_HPP
#define RICOBOT_BOT_CONFIG_HPP

#include <array>
#include <cstddef>
#include <cstdio>
#include <optional>

namespace bot_config {

inline constexpr std::size_t line_capacity = 256;

struct Line {
  std::array<char, line_capacity> text;
  std::array<char, line_capacity + 1> server_command;
  std::optional<std::size_t> first_arg;
  std::optional<std::size_t> second_arg;
  bool at_end;
};

// Reads one physical line. An empty command may also report end-of-file.
Line ReadLine(std::FILE &file);
std::optional<int> ParseInt(const char *argument);

} // namespace bot_config

#endif
