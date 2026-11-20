# RicoBot Next Generation (RicoBot NG)

**v0.1.0** is the first RicoBot NG version. RicoBot NG continues Wei Mingzhi’s
Ricobot, a bot for Half-Life: Ricochet that runs as a 32-bit GoldSrc Metamod
plugin.

## Build

Use CMake 3.20 or newer and a C++20 toolchain. Linux requires 32-bit C++
headers and libraries. On Windows, use Visual Studio with the Win32 C++
toolchain.

**Linux**

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build --parallel
```

**Windows**

```powershell
cmake -S . -B build -A Win32
cmake --build build --config RelWithDebInfo --parallel
```

The plugin is still named `ricobot_mm.so` on Linux and `ricobot_mm.dll` on
Windows. For an existing installation, stop the server and replace the plugin
at the path already listed in Metamod’s `plugins.ini`. For a new installation,
put the matching binary under the Ricochet game directory and register its
relative path in Metamod’s `plugins.ini`, using `linux` or `win32` as
appropriate. Check `meta list` after starting the server.

## Usage

- On a dedicated server, use `bot addbot [name] [skill]`, `bot min_bots <n>`,
  and `bot max_bots <n>`.
- On a listen server, the host can use `addbot [name] [skill]`,
  `observer <0|1>`, `botdontshoot <0|1>`, `botskill <1–5>`, and
  `bot_chat_percent <0–100>` from the client console in deathmatch.
- The initial config lookup checks `maps/<map>_bot.cfg` and falls back to
  `bot.cfg` in the Ricochet game directory. Map transitions look for the
  map-specific config. Config commands such as `addbot` and `pause` retain
  their existing timing.
- Bot names and chat can be customized with `bot_names.txt` and
  `bot_whine.txt` in the Ricochet game directory.

## Project information

- [Changelog](CHANGELOG.md)
- [Roadmap](ROADMAP.md)
- [Credits](CREDITS.md)
- [License scope](LICENSE.md) and [third-party licensing audit](LICENSING.md)

MiguelX413’s original contributions are offered under AGPL-3.0-only where
legally permissible. Inherited materials carry their own terms. Combined
redistribution questions remain open in the audit.
