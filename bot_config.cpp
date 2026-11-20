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

#include "bot_config.hpp"

#include <cerrno>
#include <cstdlib>
#include <limits>

namespace bot_config {

std::optional<int> ParseInt(const char *argument) {
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

Line ReadLine(std::FILE &file) {
  Line result{};
  auto &cmd_line = result.text;
  std::size_t cmd_index = 0;
  bool line_too_long = false;

  int ch = std::fgetc(&file);

  // skip any leading blanks
  while (ch == ' ')
    ch = std::fgetc(&file);

  while (ch != EOF && ch != '\r' && ch != '\n') {
    if (ch == '\t') // convert tabs to spaces
      ch = ' ';

    if (cmd_index < cmd_line.size() - 1)
      cmd_line[cmd_index++] = static_cast<char>(ch);
    else
      line_too_long = true;

    ch = std::fgetc(&file);

    // skip multiple spaces in input file
    while (cmd_index != 0 && cmd_line[cmd_index - 1] == ' ' && ch == ' ' &&
           !line_too_long)
      ch = std::fgetc(&file);
  }

  if (ch == '\r') // is it a carriage return?
  {
    ch = std::fgetc(&file); // skip the linefeed
  }

  result.at_end = ch == EOF;
  if (line_too_long || cmd_line[0] == '#') {
    cmd_line[0] = '\0';
    return result;
  }

  cmd_line[cmd_index] = '\0';
  if (cmd_line[0] == '\0')
    return result;

  std::snprintf(result.server_command.data(), result.server_command.size(),
                "%s\n", cmd_line.data());

  cmd_index = 0;
  while (cmd_line[cmd_index] != ' ' && cmd_line[cmd_index] != '\0')
    ++cmd_index;

  if (cmd_line[cmd_index] == ' ') {
    cmd_line[cmd_index++] = '\0';
    result.first_arg = cmd_index;

    while (cmd_line[cmd_index] != ' ' && cmd_line[cmd_index] != '\0')
      ++cmd_index;

    if (cmd_line[cmd_index] == ' ') {
      cmd_line[cmd_index++] = '\0';
      result.second_arg = cmd_index;
    }
  }

  return result;
}

} // namespace bot_config
