#pragma once

namespace GuiControl
{
// A lazily created test engine has no frame signal until its NewFrame hook runs.
// Queue only after that hook has observed the current, already-started GUI frame.
inline bool TestEngineFrameReady(int engineFrame, int guiFrame)
{
	return engineFrame > 0 && engineFrame == guiFrame;
}
}
