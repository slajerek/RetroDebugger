# Renoise to GoatTracker 2 Import — Design Spec v2

**Date:** 2026-03-25
**Branch:** feature-c64u (will move to dedicated branch for implementation)
**Status:** Approved design, pending implementation

## Goal

Add a C++ import feature within RetroDebugger's GoatTracker 2 plugin that reads a Renoise song file (`.xrns`) and populates GT2's in-memory song structures (patterns, order list, metadata). The user maps N Renoise tracks down to 3 SID channels via an ImGui modal window.

## Format Comparison

| Property | Renoise (.xrns) | GoatTracker 2 (.sng) |
|---|---|---|
| Container | ZIP archive with Song.xml | Binary file |
| Pattern model | All tracks in one pattern | One pattern = one channel |
| Tracks | N (variable) | Fixed 3 (SID voices) |
| Max patterns | Unlimited | 208 (MAX_PATT) |
| Max pattern rows | Variable (typically 64) | 128 (MAX_PATTROWS) |
| Max instruments | Unlimited (VST/sample) | 63 (SID instruments) |
| Sequence | SequenceEntry list → pattern index | songorder[song][channel][pos] |
| Note format | String: "C-4", "OFF" | Byte: 0x60-0xBC, 0xBE=KEYOFF |
| Instrument ref | Hex string: "00"-"FF" | Byte: 0-63 |

## Architecture

### New Files

All new files go in `src/Plugins/GoatTracker/`:

| File | Purpose |
|---|---|
| `CRenoiseImporter.h` / `.cpp` | Core importer: parse .xrns ZIP, extract Song.xml, convert to GT2 data |
| `CViewRenoiseImport.h` / `.cpp` | ImGui modal window: file selection, track mapping UI, import trigger |

**Separation of concerns:** `CRenoiseImporter` is pure data processing (no UI dependency). `CViewRenoiseImport` owns the UI and delegates to the importer. This makes it straightforward to add other import sources later (e.g., ProTracker, SID files) by creating a new importer class and reusing the view pattern.

### Dependencies

- **XML parsing:** Use a lightweight C/C++ XML parser. TinyXML2 (single .h/.cpp) is preferred if not already in the codebase. Alternatively, use pugixml or a minimal custom parser.
- **ZIP extraction:** Use miniz, zlib, or platform ZIP APIs to extract `Song.xml` from the `.xrns` archive.
- Both libraries should be added to `src/Plugins/GoatTracker/` or a shared vendor location.

## Menu Integration

Add a menu item under the main menu bar:

**Plugins > GoatTracker > Import Renoise Song...**

This opens `CViewRenoiseImport` as an ImGui modal. Implementation in `CMainMenuBar.cpp` — add the item in the existing GoatTracker plugin submenu section.

## Import Window UI

```
┌─── Import Renoise Song ──────────────────────────┐
│                                                    │
│  File: example-renoise.xrns        [Browse...]    │
│  Tracks: 5  │  Patterns: 52  │  BPM: 131          │
│                                                    │
│  ── Track Mapping ──────────────────────────────── │
│  SID Channel 1: [Track 1: (name if any)  ▾]       │
│  SID Channel 2: [Track 2: (name if any)  ▾]       │
│  SID Channel 3: [Track 3: (name if any)  ▾]       │
│                                                    │
│  ☐ Keep existing instruments                       │
│                                                    │
│  [Import]  [Cancel]                                │
└────────────────────────────────────────────────────┘
```

### Behavior

- **File path persistence:** The last used file path is saved/loaded via RetroDebugger settings (`c64SettingsStorage`). On open, the window shows the previously used filename. The `[Browse...]` button opens a system file dialog filtered to `.xrns` files.
- **Auto-parse on file load:** When a file is selected, the importer parses the `.xrns` header to extract: track count, track names, pattern count, BPM. This info is displayed in the summary line.
- **Track mapping dropdowns:** Each SID channel (1-3) has a combo box listing all Renoise sequencer tracks by index and name, plus a "-- None --" option. Default: first 3 tracks map to SID channels 1-3 (or fewer if the song has fewer than 3 tracks — unmapped channels default to "-- None --"). A "None" mapping produces REST-filled patterns for that channel.
- **Keep existing instruments checkbox:** When checked, the import clears song data (patterns, order, metadata) but preserves GT2 instrument definitions. When unchecked (default), everything is cleared.
- **Import button:** Validates limits, then executes the full import pipeline.
- **Error display:** Validation errors (too many patterns, sequence too long) shown inline in the modal before import proceeds.

## Intermediate Data Model

The importer parses Renoise XML into an intermediate representation. This model is intentionally generic to support future extensibility.

```cpp
// Special note values
constexpr int RENOISE_NOTE_EMPTY = -1;
constexpr int RENOISE_NOTE_OFF   = -2;

struct RenoiseNote {
    int noteValue;      // MIDI note number (0-119), or RENOISE_NOTE_EMPTY / RENOISE_NOTE_OFF
    int instrument;     // -1 = none specified, 0+ = Renoise instrument index (0-based; +1 for GT2)
    // Future fields (not implemented now):
    // int volume;       // -1 = none, 0-128
    // int effectCmd;    // -1 = none
    // int effectVal;    // -1 = none
};

struct RenoisePattern {
    int numLines;                                    // Number of rows (typically 64)
    std::vector<std::vector<RenoiseNote>> tracks;    // tracks[trackIdx][lineIdx]
};

struct RenoiseSequenceEntry {
    int patternIndex;
};

struct RenoiseSong {
    std::string name;
    std::string artist;
    int bpm;
    int linesPerBeat;
    int numTracks;
    std::vector<std::string> trackNames;
    std::vector<RenoisePattern> patterns;
    std::vector<RenoiseSequenceEntry> sequence;
};
```

## Import Pipeline

### Step 1 — Parse .xrns

1. Open `.xrns` as ZIP archive.
2. Extract `Song.xml` into memory.
3. Parse XML into `RenoiseSong` intermediate structure.
4. Renoise lines are **sparse** (only lines with data have `<Line index="N">` elements). Fill gaps with `RENOISE_NOTE_EMPTY` entries.
5. Extract `GlobalSongData` for BPM, artist, song name. Truncate song name and artist to 31 characters (GT2 `MAX_STR` = 32 including null terminator).
6. Extract `PatternSequence > SequenceEntries` for the play order.

**Track filtering:** Renoise patterns contain multiple track types: `PatternTrack` (sequencer tracks), `PatternGroupTrack`, `PatternMasterTrack`, and `PatternSendTrack`. Only `PatternTrack` elements are extracted. Group, master, and send tracks are skipped. The track mapping dropdown only lists sequencer tracks.

**Multi-NoteColumn handling:** Each Renoise track may contain multiple `NoteColumn` elements per line (polyphonic tracks). The importer reads only the **first NoteColumn** per line; additional columns are ignored. Users should pre-arrange Renoise tracks to single-column monophonic parts for best results.

**Alias pattern resolution:** If a `PatternTrack` has `<AliasPatternIndex>` set to a non-negative value, its data comes from the corresponding track in the referenced pattern. The importer must resolve these aliases by reading track data from the referenced pattern instead.

### Step 2 — Validate Limits

Before modifying any GT2 state, check:

| Constraint | Limit | Error message |
|---|---|---|
| Unique patterns × 3 | ≤ 208 (MAX_PATT) | "Too many unique patterns (N). Max supported: 69." |
| Sequence length | ≤ 254 (MAX_SONGLEN) | "Sequence too long (N entries). Max: 254." |
| Instrument indices | ≤ 62 (maps to GT2 1-63) | Warning: "Renoise instruments above index 62 will be clamped to 63." |
| Pattern rows | ≤ 128 | Warning: "Pattern N has M rows, truncating to 128." |

### Step 3 — Build Compact Pattern Map

Walk the Renoise sequence. Collect unique pattern indices in order of first appearance.

```
Renoise sequence: [5, 12, 5, 3]
Unique (first-seen order): [5, 12, 3]
Compact map: {5 → 0, 12 → 1, 3 → 2}
```

Each compact index `C` maps to 3 GT2 patterns: `C*3+0`, `C*3+1`, `C*3+2` (one per SID channel).

### Step 4 — Convert Patterns

For each unique Renoise pattern, given user mapping SID ch0 ← Renoise track X, ch1 ← track Y, ch2 ← track Z:

**For each SID channel `ch` (0-2):**
1. Determine the source Renoise track index from the user's mapping.
2. Create GT2 pattern at index `compactIdx * 3 + ch`.
3. Set `pattlen[gt2PatIdx] = min(renoisePattern.numLines, 128)`.
4. For each row `r` in `0..pattlen-1`, write 4 bytes into `pattern[gt2PatIdx][r*4 .. r*4+3]`:

**Note conversion (byte 0):**

| Renoise | GT2 byte | Description |
|---|---|---|
| "C-0" | `0x60` (FIRSTNOTE) | Lowest note |
| "C#0" | `0x61` | |
| ... | +1 per semitone | |
| "G#7" | `0xBC` (LASTNOTE) | Highest playable note |
| "OFF" | `0xBE` (KEYOFF) | Release SID envelope (gate off) |
| Empty | `0xBD` (REST) | No note event on this row |

Formula: `gt2Note = 0x60 + (octave * 12) + semitoneOffset`

Where semitone offsets: C=0, C#=1, D=2, D#=3, E=4, F=5, F#=6, G=7, G#=8, A=9, A#=10, B=11.

**Note range:** GT2 supports C-0 (0x60) through G#7 (0xBC) — 93 chromatic notes. Renoise notes above G#7 (i.e., A-7, A#7, B-7, and octaves 8-9) must be clamped to G#7 with a warning. The values 0xBD (REST), 0xBE (KEYOFF), and 0xBF (KEYON) are reserved control bytes, not notes.

**Empty rows:** Rows without a note must use REST (0xBD), not 0x00. GT2's `clearpattern()` initializes all rows to REST. A byte value of 0x00 in the note position has special meaning in GT2's relocation code and would cause display corruption.

**Instrument (byte 1):**
- GT2 instruments are **1-based**: byte value 0 means "no instrument change", instruments 1-63 are valid.
- Renoise instruments are **0-based**: instrument "00" is the first instrument.
- Conversion: `gt2Instrument = renoiseInstrument + 1` (clamped to 1-63).
- Maximum importable Renoise instrument index: 62 (maps to GT2 instrument 63).
- If no instrument specified in Renoise row: write `0x00` (no instrument change).

**Command (byte 2) and Parameter (byte 3):**
- Always `0x00` for now. (See Future Extensibility for effect mapping plans.)

**Pattern terminator:** After writing all rows, write ENDPATT (0xFF) at position `pattlen * 4`:
```
pattern[gt2PatIdx][pattlen * 4] = ENDPATT;
```
GT2's `countpatternlengths()` scans for ENDPATT to determine pattern length. Without this terminator, the function would scan past the intended end into adjacent pattern data. Defensively zero-fill remaining rows with ENDPATT as well (matching `clearpattern()` behavior).

### Step 5 — Build GT2 Song Order

For each channel `ch` (0-2), walk the Renoise sequence:

```
For each sequenceEntry in renoiseSong.sequence:
    compactIdx = compactMap[sequenceEntry.patternIndex]
    gt2PatIdx = compactIdx * 3 + ch
    songorder[0][ch][pos++] = gt2PatIdx
songorder[0][ch][pos] = 0xFF    // LOOPSONG (end marker)
songorder[0][ch][pos + 1] = 0x00  // Loop destination (jump to start of sequence)
songlen[0][ch] = pos
```

All 3 channels share the same sequence length (Renoise patterns contain all tracks simultaneously).

### Step 6 — Clear & Write GT2 Globals

**Stop playback first:** Call `stopsong()` before modifying any GT2 state. GT2's global data structures are not thread-safe.

**Clear existing data using `clearsong(cs, cp, ci, ct, cn)`:**
- **"Keep instruments" unchecked (default):** `clearsong(1, 1, 1, 1, 1)` — clears everything (song order, patterns, instruments, tables, names).
- **"Keep instruments" checked:** `clearsong(1, 1, 0, 0, 1)` — clears song order, patterns, and names; preserves instruments and tables.

This properly resets all editor state (playback state, cursor positions, channel state) that a manual clear would miss.

**Write imported data:**
1. Write converted patterns to `pattern[]` (including ENDPATT terminators).
2. Write song order to `songorder[0][]`.
3. Copy song name to `songname[]`, artist to `authorname[]` (truncated to 31 chars).
4. Call `countpatternlengths()` to recalculate `pattlen[]`, `songlen[]`, `highestusedpattern`, and `highestusedinstr` from the written data.
5. Trigger GT2 display refresh.

### Pattern Reuse

Pattern reuse is preserved naturally. If Renoise pattern 5 appears at sequence positions 0, 2, and 7, the GT2 songorder entries at those positions all point to the same 3 GT2 patterns. No data is duplicated.

Example:
```
Renoise sequence: [5, 12, 5, 3]
Compact map: {5→0, 12→1, 3→2}

GT2 songorder[0][ch0]: [0, 3, 0, 6, 0xFF]  (patterns 0, 3, 0, 6, END)
GT2 songorder[0][ch1]: [1, 4, 1, 7, 0xFF]  (patterns 1, 4, 1, 7, END)
GT2 songorder[0][ch2]: [2, 5, 2, 8, 0xFF]  (patterns 2, 5, 2, 8, END)
```

Total GT2 patterns used: 9 (3 unique Renoise patterns × 3 SID channels).

## Settings Persistence

| Setting | Storage | Default |
|---|---|---|
| Last .xrns file path | `c64SettingsStorage` | Empty string |
| SID ch1 track mapping | Per-session (not persisted) | Track 0 |
| SID ch2 track mapping | Per-session (not persisted) | Track 1 |
| SID ch3 track mapping | Per-session (not persisted) | Track 2 |
| Keep existing instruments | Per-session (not persisted) | false |

## Build System Updates

New source files must be added to all three build systems:
- **Xcode:** `platform/MacOS/c64d.xcodeproj` — add to GoatTracker group
- **CMake:** `CMakeLists.txt` — add to source list
- **Visual Studio:** `platform/Windows/c64d/c64d.vcxproj` + `.vcxproj.filters`

XML parser and ZIP library (if not already present) also need build system entries.

## Future Extensibility (Not Implemented Now)

These features are explicitly deferred but the architecture supports them:

### Append Mode
Import without clearing existing song data. New patterns are allocated after `highestusedpattern`, and new sequence entries are appended to the existing song order. The `RenoiseNote` intermediate model and the compact pattern map already support arbitrary starting offsets — the importer just needs a `startPatternIdx` parameter.

### Effect Mapping
Map Renoise volume, panning, and effect columns to GT2 commands. The `RenoiseNote` struct has commented-out fields (`volume`, `effectCmd`, `effectVal`) ready to be populated. A mapping table would convert Renoise effect numbers to GT2 command bytes (e.g., Renoise volume slide → GT2 `CMD_SETAD`/`CMD_SETSR`).

### Multi-Song Import
Import into GT2 song slots other than slot 0. The song order write step already indexes by song number — changing `songorder[0]` to `songorder[N]` is the only modification needed.

### Tempo Mapping
Convert Renoise BPM + LinesPerBeat to GT2 tempo. GT2 tempo is set via `CMD_SETTEMPO` (command 0x0F) in pattern data or via the global tempo setting. The conversion formula would be: `gt2Tempo = 60 * tickRate / (bpm * linesPerBeat)` adjusted for PAL/NTSC frame rates.

### Additional Import Sources
The `CRenoiseImporter` / `CViewRenoiseImport` split establishes a pattern: importer class handles format-specific parsing into intermediate data, view class handles UI and GT2 integration. A ProTracker or MIDI importer would follow the same pattern with a different parser class.
