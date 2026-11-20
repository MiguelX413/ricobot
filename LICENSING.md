# Third-party licensing audit

This audit records what the source notices, Git history, and upstream materials
establish. Compatibility and redistribution rights for the combined plugin
remain unresolved.

| Component | Evidence and terms |
|---|---|
| Original Ricobot | Seven inherited source and header files retain Wei Mingzhi’s 2004 three-clause BSD-style notice. The [full notice](LICENSES/Ricobot-BSD-3-Clause.txt) must accompany binary redistribution in documentation or other materials. See the [upstream Ricobot repository](https://github.com/Bots-United/ricobot). |
| Metamod and Metamod-P | The vendored headers are documented as [Metamod-P v1.21p109](https://github.com/Bots-United/metamod-p/releases/tag/v1.21p109). Preserve the per-file GPL notices and verbatim exceptions. For example, [meta_api.h](metamod/meta_api.h) states GPL v2 or later and grants a specific exception for the Valve engine and Valve MODs. See the [bundled GPLv2](metamod/GPL.txt) and [GPLv3 text](LICENSES/GPL-3.0.txt). |
| Valve Half-Life SDK | `hlsdk/`, [util.cpp](util.cpp), and [util.hpp](util.hpp) carry legacy SDK notices, including Valve and id Software attribution where applicable. Preserve the [legacy notice](LICENSES/Valve-Legacy-SDK-Notice.txt) and the [published Half-Life 1 SDK license](LICENSES/Valve-Half-Life-1-SDK.txt). The legacy notice limits use to non-commercial enhancements and the current upstream license specifies free distribution. The published license text is included for reference and does not establish that it supersedes the notices on this particular SDK snapshot. |
| HPB Bot Template #3 | [Original Ricobot’s README](https://github.com/Bots-United/ricobot) identifies botman’s template as its base. The [HPB Bot archive](https://github.com/Bots-United/HPB-bot) is evidence of the project lineage. The exact Template #3 snapshot, its applicable terms, and the scope of surviving copied code still need verification. |
| POD-Bot | [bot.cpp](bot.cpp) attributes the two functions following its source comment, currently `IsDeadlyDrop` and `BotFacePosition`, to POD-Bot. [Count Floyd’s POD-Bot 2.6 source release](https://github.com/Bots-United/podbot26) contains an informal reuse statement and upstream credits. The exact source revision and applicable grant for both routines have not been verified. Terms from a later PODBot MM fork cannot establish the terms of this earlier copy. |
| RCBot and later RicoBot changes | RicoBot commit `9dc3d97` and source comments credit Cheeseh for fixes. The exact RCBot source and permission for any copied implementation are unverified. Ryan Hunter / APGRoboCop’s historical changes and any other inherited contributions retain their existing terms. No new grant is made on behalf of those authors. |

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
