#ifndef _C64DUiScale_h_
#define _C64DUiScale_h_

//
// RetroDebugger's HiDPI policy layer.
//
// THE SCALE ITSELF LIVES IN THE ENGINE, in MTEngineSDL/src/Engine/GUI/
// MT_UiScale.h -- MT_GetUiScale(), MT_UiScaled(), MT_SetUiScale(),
// MT_DetectDisplayUiScale(). It belongs there because the thing that makes
// scaling necessary is the engine's own SDL/ImGui integration (SDL3 declares
// PER_MONITOR_AWARE_V2, so Windows and Linux hand over physical pixels while
// macOS hands over points), and because the engine's own legacy views carry the
// same kind of fixed pixel constants this app's do. Scale a constant with
// MT_UiScaled(); do NOT add an app-side wrapper for it.
//
// The engine also owns APPLYING the scale to the ImGui style, from
// VID_FinishStyleChange -- the tail every style change passes through -- so
// nothing here has to notice a theme switch or a macOS appearance flip and
// re-assert the scale afterwards.
//
// What is left here is only what is genuinely RetroDebugger's:
//
//   * the CONFIG POLICY -- auto versus a value the user picked, and which key
//     remembers what;
//   * the MIGRATION of geometry this app has already persisted, layouts.dat and
//     imgui.ini, which is where compatibility with existing users lives;
//   * the vocabulary of which layout parameters are pixel sizes.
//
// Design notes: the HiDPI UI scaling design notes.
//

#include "SYS_Defs.h"
#include "MT_UiScale.h"

class CGuiView;
class CByteBuffer;

// ---------------------------------------------------------------------------
// Startup -- both halves run inside MT_PostInit, and the order matters
// ---------------------------------------------------------------------------

// BEFORE any view is constructed: resolves the scale from config (or the
// display) and hands it to the engine, so MT_UiScaled() is already answering
// correctly while CViewC64 builds its views.
void C64D_UiScaleInitEarly();

// AFTER every view exists: migrates persisted geometry (imgui.ini on disk and
// the in-memory layouts.dat buffers) when the settings folder was written at a
// different scale. Needs the live views because the layout wire format stores a
// parameter's name and value but not its type.
void C64D_UiScaleMigratePersistedGeometry();

// ---------------------------------------------------------------------------
// The Settings > UI > UI Scale menu
// ---------------------------------------------------------------------------

// True when the scale in force came from the display rather than from a value
// the user picked. Drives the "Auto" tick.
bool C64D_IsUiScaleAuto();

// Applies the new scale, rescales the live UI (views, ImGui windows, dock
// nodes) and every stored workspace, and persists the choice. `isAuto` records
// where the value came from.
void C64D_UiScaleSet(float newScale, bool isAuto);

// ---------------------------------------------------------------------------
// Pieces, exposed for the tests
// ---------------------------------------------------------------------------

// Scales the geometry fields of an ImGui .ini text (Pos, Size, SizeRef,
// ViewportPos) and returns a newly allocated NUL-terminated string; the caller
// frees with delete[]. Ids and dock/selection handles are left alone.
char *C64D_UiScaleTransformImGuiIni(const char *ini, float factor);

// Rewrites one serialized layout buffer in place. Returns false and leaves the
// buffer untouched if it cannot be parsed.
bool C64D_UiScaleTransformLayoutBuffer(CByteBuffer *buffer, float factor);

// Multiplies the size-like float layout parameters of every live view.
void C64D_UiScaleRescaleLiveViewParameters(float factor);

// True for the layout parameter names that carry a pixel size.
bool C64D_UiScaleIsSizeLikeLayoutParameter(const char *parameterName);

#endif
