# Audio Mixer VU Meters & Waveforms — Design Spec

**Date:** 2026-03-25
**Status:** Approved design, pending implementation

## Goal

Add vertical VU meters and vertical scrolling waveform displays to the generic Audio Mixer view (`CViewAudioMixer` in MTEngineSDL). Each audio channel shows: volume slider, VU meter, waveform — side by side. A new ring buffer in `CAudioChannel` provides sample data for visualization across all channel types (VICE, Atari, NES, GoatTracker, C64U).

## Layout

Per channel, left to right:

```
| Volume Slider | VU Meter | Waveform  |   gap   | (next channel...)
|  (existing)   |  20px w  |  ~60px w  |  8px    |
```

All elements scale vertically with the resizable window. The mixer window is resizable (user drags to desired height).

## Ring Buffer in CAudioChannel (MTEngineSDL)

### New Members

```cpp
static const int PEEK_BUFFER_SIZE = 2048;  // ~46ms at 44100 Hz

float peekBuffer[PEEK_BUFFER_SIZE];  // mono samples in [-1, 1]
int peekWritePos;                     // next write position (wraps)
```

### Sample Capture

During `MixIn()`, after applying volume, compute mono mix from the L/R sample pair and write to the ring buffer:

```cpp
float monoSample = ((float)sampleL + (float)sampleR) / 65536.0f;  // int16 stereo → float mono
peekBuffer[peekWritePos] = monoSample;
peekWritePos = (peekWritePos + 1) % PEEK_BUFFER_SIZE;
```

This runs on the audio thread. The ring buffer is single-writer (audio thread) and single-reader (UI thread). A torn read on one sample produces at most a one-pixel visual glitch — acceptable, no mutex needed.

### PeekRecentSamples

```cpp
int CAudioChannel::PeekRecentSamples(float *outSamples, int numSamples) const
```

Copies the last `numSamples` samples from the ring buffer into `outSamples` in chronological order (oldest first, newest last). Returns the number of samples actually copied (may be less than requested if buffer not yet full). Caller provides the output array.

### Initialization

In `CAudioChannel` constructor:
- `memset(peekBuffer, 0, sizeof(peekBuffer))`
- `peekWritePos = 0`

## VU Meter

### Data

- Read last ~960 samples (~20ms at 44100 Hz) via `PeekRecentSamples()`
- Compute **RMS**: `sqrt(mean(sample^2))` over the window
- Compute **Peak**: `max(|sample|)` over the window

### dB Conversion

```cpp
float toNormDb(float linear) {
    if (linear < 0.001f) return 0.0f;
    float db = 20.0f * log10f(linear);
    if (db < -60.0f) db = -60.0f;
    return (db + 60.0f) / 60.0f;  // normalized [0, 1]
}
```

### Rendering

- **Bar**: 20px wide, full channel height. Filled from bottom to RMS level.
- **Color gradient**: Green (below -12 dB normalized = 0.8), Yellow (-12 to -3 dB = 0.8–0.95), Red (above -3 dB = 0.95+)
- **Peak hold line**: 2px white horizontal line at peak level. Decays at ~20 dB/sec (subtract `deltaTime * 0.33` from normalized peak each frame; clamp to 0).
- Rendered via `ImDrawList::AddRectFilled()` and `ImDrawList::AddLine()`.

### Per-Channel State

The mixer view needs to track peak hold per channel. Since channels are dynamic, use a `std::map<CAudioChannel*, float>` for `peakHoldLevel`. Clean up stale entries when channels are removed.

## Waveform

### Data

- Read last N samples where N = waveform pixel height (varies with window resize)
- Use `PeekRecentSamples()` with N = available pixel height

### Rendering

- **Orientation**: Vertical. Y=bottom is the newest sample, Y=top is oldest (waveform scrolls upward as new samples arrive).
- **Width**: ~60px per channel.
- **Center line**: Thin grey vertical line at horizontal center of the waveform area (represents 0V / silence).
- **Waveform line**: For each row (sample), compute `x = centerX + sample * halfWidth` where `halfWidth = waveformWidth * 0.45`. Connect adjacent rows with `ImDrawList::AddLine()`.
- **Color**: Greenish tint (`ImVec4(0.4, 1.0, 0.4, 0.8)`).

### Clipping

Samples outside [-1, 1] are clamped before rendering. The `* 0.45` factor leaves a small margin to prevent drawing outside the waveform area.

## Modified Files

| File | Location | Change |
|---|---|---|
| `CAudioChannel.h` | `MTEngineSDL/src/Engine/Audio/` | Add `peekBuffer[]`, `peekWritePos`, `PeekRecentSamples()` |
| `CAudioChannel.cpp` | `MTEngineSDL/src/Engine/Audio/` | Initialize buffer, write during `MixIn()`, implement `PeekRecentSamples()` |
| `CViewAudioMixer.h` | `MTEngineSDL/src/Engine/GUI/AudioMixer/` | Add peak hold state map |
| `CViewAudioMixer.cpp` | `MTEngineSDL/src/Engine/GUI/AudioMixer/` | Add VU meter + waveform rendering columns |

No changes to RetroDebugger's `src/` — this is entirely an MTEngineSDL feature.

## Build System

MTEngineSDL file modifications only — no new files. Xcode, CMake, and Visual Studio projects do not need updating.

## Future Extensibility

- Stereo waveform (L/R split) — change `peekBuffer` to dual float arrays
- Per-channel color customization
- Configurable waveform width via right-click context menu
- Frequency spectrum / FFT display column
