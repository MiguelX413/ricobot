# Third-party licensing audit

This audit records what the source notices, Git history, and upstream materials
establish. Compatibility and redistribution rights for the combined plugin
remain unresolved.

## Components and terms

### Original Ricobot

Seven inherited source and header files retain Wei Mingzhi’s 2004 three-clause BSD-style notice. The [full notice](LICENSES/Ricobot-BSD-3-Clause.txt) must accompany binary redistribution in documentation or other materials. See the [upstream Ricobot repository](https://github.com/Bots-United/ricobot). The existing binary notice copy reproduces the inherited notice and disclaimer. The source grant also prohibits using the author’s or contributors’ names to endorse derived products without prior written permission.

### Metamod and Metamod-P

The vendored headers are documented as [Metamod-P v1.21p109](https://github.com/Bots-United/metamod-p/releases/tag/v1.21p109). Preserve the per-file GPL notices and verbatim exceptions. For example, [meta_api.h](metamod/meta_api.h) states GPL v2 or later and grants a specific exception for the Valve engine and Valve MODs. See the [bundled GPLv2](metamod/GPL.txt) and [GPLv3 text](LICENSES/GPL-3.0.txt).

### Valve Half-Life SDK

`hlsdk/`, [util.cpp](util.cpp), and [util.hpp](util.hpp) carry legacy SDK notices, including Valve and id Software attribution where applicable. Preserve the [legacy notice](LICENSES/Valve-Legacy-SDK-Notice.txt) and the [published Half-Life 1 SDK license](LICENSES/Valve-Half-Life-1-SDK.txt). The legacy notice limits use to non-commercial enhancements and the current upstream license specifies free distribution. The published license text is included for reference and does not establish that it supersedes the notices on this particular SDK snapshot. An unmodified copy of the published text also resides at [hlsdk/LICENSE](hlsdk/LICENSE), so an archive of the vendored SDK subtree carries the license with it. Both copies were compared byte for byte with [Valve’s published text](https://github.com/ValveSoftware/halflife/blob/master/LICENSE). The compiled plugin uses SDK headers and contains the SDK-derived utility implementation. Include both the published SDK text and the legacy notice with the binary documentation, and preserve the full notices in the utility sources when shipping source.

### HPB Bot Template #3

[Original Ricobot’s README](https://github.com/Bots-United/ricobot) identifies botman’s template as its base. The [HPB Bot archive](https://github.com/Bots-United/HPB-bot) is evidence of the project lineage. The exact Template #3 snapshot, its applicable terms, and the scope of surviving copied code still need verification.

### POD-Bot

[bot.cpp](bot.cpp) attributes the two functions following its source comment, currently `IsDeadlyDrop` and `BotFacePosition`, to POD-Bot. [Count Floyd’s POD-Bot 2.6 source release](https://github.com/Bots-United/podbot26) contains an informal reuse statement and upstream credits. The exact source revision and applicable grant for both routines have not been verified. Terms from a later PODBot MM fork cannot establish the terms of this earlier copy.

### RCBot and later RicoBot changes

RicoBot commit `9dc3d97` and source comments credit Cheeseh for fixes. The exact RCBot source and permission for any copied implementation are unverified. Ryan Hunter / APGRoboCop’s historical changes and any other inherited contributions retain their existing terms. No new grant is made on behalf of those authors.

## Source tracing for v0.1.0

### HPB Bot

The [original Ricobot snapshot `df471f3`](https://github.com/Bots-United/ricobot/tree/df471f371a3c55c354c93e75b4e38803ced79d96) identifies **Template #3**. The earliest revision of the [available Template #4 mirror](https://github.com/Bots-United/HPB_bot_template/tree/031d005) already describes the March 2004 Steam update. Its README includes botman's February 2001 notice permitting use subject to credit in the distributed README and on the distribution website.

**Still open.** The mirror is a useful lead for the attribution condition. Locate the original Template #3 archive and its README, identify the surviving template code, and confirm that the same grant applies to that copy. The project README and CREDITS.md directly credit botman and Template #3. Retain that credit on any separate download page if the verified grant requires it.

### POD-Bot

[POD-Bot 2.6 snapshot `715f3cd`](https://github.com/Bots-United/podbot26/blob/715f3cd60c71bba01e3dff5daa86816cf69bcd40/podbot/dlls/bot.cpp) contains a close source match for `IsDeadlyDrop`, including the same sequence, thresholds, and distinctive comments. It also contains a matching core of `BotFacePosition`; Ricobot adds angle clamping and omits punch-angle adjustment. The attribution already appears in the original Ricobot snapshot. [The 2003 release note](https://github.com/Bots-United/podbot26/blob/715f3cd60c71bba01e3dff5daa86816cf69bcd40/readme.txt) expressly encourages reuse in informal language.

**Still open.** The match establishes lineage, while the precise archive Ricobot used and the scope of the grant for all relevant contributors remain uncertain. Confirm that the grant covers modification and source and binary redistribution as part of this combination.

### RCBot

[RicoBot commit `9dc3d97`](https://github.com/MiguelX413/ricobot/commit/9dc3d97f82fd8b537deca2a9392c30bebda5960b) introduced the Cheeseh-credited movement timing. The [RCBot mirror `d13a8ac`](https://github.com/APGRoboCop/rcbotold/tree/d13a8ac36409b204657c26251ecc0b13760b76b5) computes movement milliseconds from elapsed time in `CBot::UpdateMsec`. It does not match RicoBot's variable names or establish the source of the 60 Hz update logic. The mirror has a GPLv3 license file, and its `dlls/bot.cpp` has a GPLv2-or-later header with a Valve exception.

**Still open.** Determine whether Cheeseh supplied a standalone patch, whether implementation was copied, and which permission or file-level terms cover the 2020 change. The later mirror does not establish that grant.

### HLSDK

Historical Makefile commit `9aca24b` builds against `hlsdk-2.3-p4/multiplayer`, with Ricochet-specific include paths. Commit `ae9d8a8` imported the current header tree. Of its 92 headers, 49 match a same-named file in [AlliedModders' SDK mirror `a0edb77`](https://github.com/alliedmodders/hlsdk/tree/a0edb7792a96998d349325bebab8ea41ec5cb239) and 24 match [Valve's repository `b1b5cf5`](https://github.com/ValveSoftware/halflife/tree/b1b5cf5892918535619b2937bb927e46cb097ba1), normalizing only CRLF to LF. Neither snapshot matches the complete set. The vendored `cbase.h` also matches [the Template #4 mirror](https://github.com/Bots-United/HPB_bot_template/tree/816b34a4f417e4f2d97a16c601c80abe54d16136), which shows shared SDK material without establishing where RicoBot acquired the file.

**Still open.** The old build path is evidence of a historical dependency, rather than proof that the 2023 import came from that archive. The attempted historical p4 downloads did not yield a usable archive. Identify the exact archive or a documented replacement, trace local changes and the top-level Valve-derived utility files, and verify applicable redistribution terms.

## AGPL scope and packaging

[CMakeLists.txt](CMakeLists.txt) and [file_handle.hpp](file_handle.hpp) carry
explicit AGPL-3.0-only
SPDX identifiers. [LICENSE.md](LICENSE.md) describes the grant for MiguelX413’s
original contributions, including independently licensable changes in
mixed-origin files. The inherited Ricobot, SDK, and Metamod files retain their
source notices. The repository and compiled plugin have no blanket
AGPL-3.0-only designation.

CMake builds the plugin and currently defines no installation, CPack, or
release target. GitHub source archives include the tracked `hlsdk/LICENSE`
with the SDK. When preparing a separate source package, retain that file,
the source notices, and the repository’s attribution and license files.

The existing Linux x86 and Windows Win32 binary uploads include the license scope,
this audit, credits, the texts under `LICENSES/`, and
`metamod/GPL.txt`. The complete SDK is excluded from the binary package.
Debug symbols remain a separate download. When preparing a release,
inspect the source and binary archives for the applicable notices.
Legal clearance requires a separate review.

## Before redistribution

- Establish the provenance and applicable grants for the Template #3, POD-Bot
  and RCBot-derived code, and the exact Ricochet SDK snapshot. Record any
  permission obtained or replace code whose rights cannot be established.
- Review how the license terms apply to the combined source and plugin. The
  Metamod exception has a specific scope, and the [AGPL section 13 combination
  provision](https://www.gnu.org/licenses/agpl-3.0.html#section13) addresses
  combinations with GPLv3 code. Neither resolves the applicability of Valve’s
  terms or grants rights to separately borrowed code.
- Include the applicable copyright notices, license texts, and any required
  corresponding source and source offers with distributions. Evaluate AGPL
  section 13’s network-source obligations where applicable. The existing CI
  uploads are build outputs and do not establish redistribution clearance.
