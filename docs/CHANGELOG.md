# Retro Debugger — changelog

This file records what changed in each release. The full release history
starting from the first public releases lives in
[docs/release-notes.txt](release-notes.txt).

## 1.0.1 (current, on the `devel` branch)

- MCP (Model Context Protocol) server: exposes `retro_*` tools over stdio and a
  background reconnecting bridge; readiness and endpoint-failure semantics are
  documented in [docs/mcp](mcp/retrodebugger-mcp-skill.md)
- Remote debugging overhaul: WebSockets server hardening (crash on menu-enable,
  16 kB payload cap, descriptor duplicates), canonical register records for
  Vice and Atari, `-segment`/`-seg` start argument, `Warp Speed` read endpoint,
  `$` hex literals and hex patterns in the assemble/search paths
- CVE-2019-17544 fixed in the vendored VICE monitor's `unescape()` (out-of-bounds
  read on hostile input)
- Bundled libpng headers take precedence over a system GTK3's libpng16 build
- Core upgrade to the VICE 3.10 record: CPU, VIC-II, CIA, VIA, SID, 1541 drive
  and snapshot handling ported and covered by regression tests
- Snapshot save/restore boundary regression tests; Windows and Linux build
  fixes; macOS CI now builds SDL through the engine's vendored source
- Many bug fixes across the C64, Atari XL/XE and NES debugger views
