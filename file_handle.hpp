#ifndef RICOBOT_FILE_HANDLE_HPP
#define RICOBOT_FILE_HANDLE_HPP

#include <cstdio>
#include <memory>

struct FileCloser {
  void operator()(std::FILE *file) const noexcept { std::fclose(file); }
};

using FileHandle = std::unique_ptr<std::FILE, FileCloser>;

#endif
