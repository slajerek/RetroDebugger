# Chunk 2 Merge Methodology — VICE 3.1 → 3.10 (team brief)

You are a worker on the `vice310-merge` team. Read this in full before touching files.

## What we're doing

RetroDebugger embeds a fork of VICE. We are upgrading the embedded VICE from 3.1 to
3.10 while preserving slajerek's debugger integration (the `c64d_*` hooks). For each
"pending" file, the LIVE file in the tree is currently **VICE 3.1 + RD hooks**. Your
job is to transform it into **VICE 3.10 + the same RD hooks** — i.e. re-apply the RD
integration on top of the 3.10 upstream content.

Working dir: `/Users/garion/Projects/RetroDebugger`
Tree root for VICE: `src/Emulators/vice/`

## Reference trees (read-only, already on disk)

- Vanilla VICE 3.1  (the BASE):   `~/vice-ref/3.1/src/<rel>`
- Vanilla VICE 3.10 (the THEIRS): `~/vice-ref/3.10/src/<rel>`

`<rel>` is the path **relative to `src/`** in vanilla. The live file's `<rel>` is given
to you in your task. For `root/` files, the vanilla path drops the `root/` prefix
(e.g. live `root/log.c` ↔ vanilla `log.c`). The file map
`docs/superpowers/notes/phase3-data/chunk1-filemap.tsv` has col2 = vanilla rel path
for every file if you need to look one up.

## The rename you must account for

RD renamed `types.h` → `vicetypes.h`. Live files `#include "vicetypes.h"`; vanilla files
`#include "types.h"`. So before diffing/merging, normalize: in the BASE and THEIRS copies,
replace `#include "types.h"` → `#include "vicetypes.h"`. (Scripts below do this.) Do NOT
change RD's `#include "vicetypes.h"` back to types.h in the live tree.

## The 3-way merge recipe

Use a **per-agent scratch namespace** so parallel workers never collide. Replace `NAME`
with your teammate name (e.g. `cartw`):

```bash
NAME=cartw            # <-- your name
REL=c64/cart/reu.c    # <-- the file's vanilla-rel path
LIVE=src/Emulators/vice/c64/cart/reu.c   # <-- the live path (root/X has root/ prefix)

sed 's/#include "types.h"/#include "vicetypes.h"/' ~/vice-ref/3.1/src/$REL  > /tmp/${NAME}_base
sed 's/#include "types.h"/#include "vicetypes.h"/' ~/vice-ref/3.10/src/$REL > /tmp/${NAME}_theirs
cp "$LIVE" /tmp/${NAME}_ours

git merge-file -p --diff3 /tmp/${NAME}_ours /tmp/${NAME}_base /tmp/${NAME}_theirs > /tmp/${NAME}_merged
grep -c '^<<<<<<<' /tmp/${NAME}_merged   # number of conflict hunks to resolve
```

`--diff3` shows three sections per conflict:
```
<<<<<<< /tmp/NAME_ours      (RD's 3.1+hooks version)
||||||| /tmp/NAME_base      (vanilla 3.1 — the common ancestor)
=======                     (boundary)
>>>>>>> /tmp/NAME_theirs    (vanilla 3.10 — upstream's new version)
```

### Conflict resolution rule (the core judgment)

For each conflict, you are reconciling "RD's change to 3.1" against "upstream's change
3.1→3.10". Resolve by **taking the 3.10 (theirs) content as the base truth, then
re-applying RD's hook/integration delta on top**:

1. Compare `ours` vs `base`: what did RD add/change relative to vanilla 3.1? That delta
   is almost always (a) a `VICE_HOOK_*(...)` call, (b) a direct `c64d_*(...)` call,
   (c) a `#include "ViceWrapper.h"` / `"vice_debugger_hook.h"` / RD header, (d) an API
   extension (e.g. `save_reu_data`, `mem_ram`, `isPaused`), or (e) a `VICE_DEBUG` rename.
2. Compare `theirs` vs `base`: what did upstream change 3.1→3.10? Take ALL of it.
3. Write the resolved hunk = **3.10 content + RD's delta re-applied**, adapted to any
   3.10 signature/type changes (see gotchas). Delete the `<<<`/`|||`/`===`/`>>>` markers.

If RD's delta and the 3.10 change are in unrelated spots inside the hunk, keep both.
If 3.10 deleted the exact line RD hooked, the hook usually moves to the nearest
equivalent site; if there's no equivalent, flag it (see "When to stop and ask").

## The RD hook model (what to preserve)

- Hooks are centralized in `src/Emulators/vice/vice_debugger_hook.h`: `VICE_HOOK_*(...)`
  macros expand to `c64d_*` calls when `RETRODEBUGGER` is defined, and to `((void)0)` /
  constants when not. **Call sites use the macro.** Preserve every `VICE_HOOK_*` call.
- Some `c64d_*` calls and declarations are direct (not macro-routed) — preserve them too.
- Preserve RD `#include` additions: `ViceWrapper.h`, `ViceLogWrapper.h`, `SYS_Types.h`,
  `vice_debugger_hook.h`.
- Preserve RD API extensions and the `DEBUG`→`VICE_DEBUG` rename.
- Preserve `LOGD(...)`, `LOG_LOCK`, `LOGError(...)` logging additions.

## 3.10 gotchas (apply while resolving)

- `CLOCK` is now `uint64_t` (was 32-bit). Print formats / masks may have changed upstream
  — take the 3.10 form.
- The **translation subsystem is removed** in 3.10: no `translate.h`, no `IDCLS_*` IDs.
  Command-line option tables now use literal description strings. If a file `#include`s
  `translate.h` or uses `IDCLS_*`, take the 3.10 form (which won't). Do NOT reintroduce
  translate.* .
- 3.10 adds `VICE_ATTR_*` printf-attribute macros (already added to RD's `root/vice.h`).
- `diskunit_context` dual-drive model, CIA SDR delay-line rewrite, SID 2→8 — take 3.10
  forms wholesale; re-apply hooks on top.

## VERIFICATION GATE (mandatory before you report a file done)

After writing the merged result back to the LIVE path, prove you dropped **zero** upstream
3.10 content:

```bash
sed 's/#include "vicetypes.h"/#include "types.h"/' "$LIVE" > /tmp/${NAME}_chk
diff /tmp/${NAME}_chk ~/vice-ref/3.10/src/$REL > /tmp/${NAME}_diff
grep -c '^> ' /tmp/${NAME}_diff    # MUST be 0  (0 = no 3.10 line is missing)
grep -c '^< ' /tmp/${NAME}_diff    # >0 expected = your RD additions (hooks/includes/etc.)
```

- `> ` lines = 3.10 content NOT present in your merged file = **content you dropped. BAD.**
  Keep resolving until this is 0. (Rare legitimate exception: a 3.10 line RD intentionally
  replaced — e.g. `DEBUG`→`VICE_DEBUG`. If you believe a `>` line is an intentional RD
  replacement, list it explicitly in your report so the lead can confirm.)
- `< ` lines = lines in your file but not in 3.10 = your preserved RD additions. Eyeball
  them: every one should be a hook/include/API-ext/log/VICE_DEBUG line, NOT leftover 3.1
  code. If you see leftover 3.1 logic here, you failed to take 3.10's version — fix it.
- Also: `grep -c '^<<<<<<<\|^=======\|^>>>>>>>' "$LIVE"` MUST be 0 (no leftover markers).
- The file must still be syntactically plausible C/C++ (balanced braces; includes intact).

## DO NOT TOUCH (RD-custom config — merging toward vanilla destroys them)

`root/vice.h`, `root/vice_sdl.h`, `arch/vicetypes.h`, anything under `arch/` that is RD
platform glue, and `vice_debugger_hook.h`, `ViceWrapper.*`, `ViceLogWrapper.*`. If your
batch seems to require editing one of these, STOP and message the lead.

## Cross-cutting conventions (coordinate via SendMessage)

- **Snapshot/REU extension**: RD adds `save_reu_data` / `store_reu_data` and cart-ROM
  snapshot extensions threading through `snapshot.c`, `c64-snapshot.c`,
  `c64memsnapshot.c`, `cart/reu.c`, `drive-snapshot.c`. If your file calls a snapshot
  helper whose signature you're unsure of, message the owner of that file (or the lead)
  rather than guessing. Keep signatures identical to what the live 3.1+RD tree already used.
- **VICE_DEBUG rename**: apply consistently; it's RD-wide.

## TEAM RULES (strict)

1. **Edit ONLY the files in your assigned task.** Never touch another batch's files.
2. **Do NOT run any git command that changes state** (no add/commit/checkout/reset/stash/
   branch). The lead reviews your working-tree edits and commits them. Read-only git
   (`git diff <your files>`, `git status`) is fine.
3. Use `/tmp/<yourname>_*` for all scratch. Never use bare `/tmp/_b` etc.
4. When your task is done, mark it completed via TaskUpdate and SendMessage the lead a
   concise per-file report: for each file, the conflict count you resolved and the final
   `> `-line count (must be 0) and a one-line note on any judgment call.
5. If a file is genuinely too hard / deeply RD-customized / you'd be guessing — do NOT
   guess. Mark it and SendMessage the lead with specifics. Partial-but-correct beats
   complete-but-wrong. Per project mandate: do it right.
6. Then check TaskList for the next available task (lowest ID first) and continue.
