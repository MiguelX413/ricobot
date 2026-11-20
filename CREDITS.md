# Credits

## RicoBot and RicoBot NG

- **Wei Mingzhi (Whistler)** created the original Ricobot for Half-Life:
  Ricochet, as documented in the [original README](https://github.com/Bots-United/ricobot)
  and the 2004 copyright notices in the inherited source.
- **botman and the HPB Bot contributors** created the HPB Bot Template.
  Whistler credits Template #3 as Ricobot’s base. The
  [HPB Bot source archive](https://github.com/Bots-United/HPB-bot) documents the
  project. See the [HPB Bot attribution evidence](LICENSING.md#hpb-bot)
  for the template notice and the remaining provenance question.
- **Ryan Hunter (APGRoboCop)** maintained this fork before the NG effort. Git
  records SSE2 and performance work (`2b34241`, `057caac`), AMBuild and
  Linux support (`ae9d8a8`, `6807cc4`), angle work (`c73aa76`), and
  buffer-overflow mitigation (`7ecd712`).
- **Cheeseh and the RCBot project** are credited for bot fixes in
  [bot.cpp](bot.cpp) and [bot.hpp](bot.hpp). The changes are recorded in
  `9dc3d97`, committed by Ryan Hunter under the title “Fix by Cheeseh.”
  The [RCBot README](https://github.com/APGRoboCop/rcbotold) credits Cheeseh
  as founder. The exact source revision of the changes is still unverified.
- **Markus “Count Floyd” Klinge and the POD-Bot contributors** created and
  contributed to POD-Bot. RicoBot attributes two movement routines to that
  project. The [original 2.6 release](https://github.com/Bots-United/podbot26)
  credits botman, Ditlew, Vasily Pupkin, Another1, Aspirin, Paulo La-Fritte,
  Killaruna, RedFox, William van der Sterren, Polymorph, and the Nuclearbox
  community for ideas, code, or both. That upstream credit does not establish
  individual authorship of the routines in RicoBot.
- **MiguelX413** maintains RicoBot Next Generation. Git records the CMake
  and CI migration (`6efe0e4`), formatting and analysis setup
  (`b390a98`), C++20 modernization and Win32 compatibility work
  (`0fa3c5f`), RAII file handling (`d18719c`), and shared command
  validation (`c9aa980`, `fb715ed`).

## Engine and plugin infrastructure

- **Will Day and the Metamod contributors** developed the original Metamod
  plugin interface, credited in the [vendored API header](metamod/meta_api.h).
- **Jussi Kivilinna and the Metamod-P contributors** extended and maintained
  Metamod-P. RicoBot NG includes headers from their
  [v1.21p109 release](https://github.com/Bots-United/metamod-p/releases/tag/v1.21p109).
- **Valve** developed Half-Life and GoldSrc and published the
  [Half-Life SDK](https://github.com/ValveSoftware/halflife). Valve and
  **id Software** are credited in the inherited SDK notices for the SDK
  and underlying licensed technology, respectively. The SDK license is included
  with the [vendored headers](hlsdk/LICENSE), and the
  [legacy notice](LICENSES/Valve-Legacy-SDK-Notice.txt) is preserved.

See [the licensing audit](LICENSING.md) for the evidence and open questions
about borrowed code.
