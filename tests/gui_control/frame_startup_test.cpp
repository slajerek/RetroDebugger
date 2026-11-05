#include "GuiControlFrameGate.h"
#include <cstdio>

int main()
{
	struct Case { int engineFrame; int guiFrame; bool ready; };
	const Case cases[] = {
		{0, 120, false}, // Lazy engine in an already-running GUI: do not queue.
		{0, 0, false},   // Neither context has received a frame signal yet.
		{-1, 120, false},
		{118, 120, false},
		{119, 120, false},
		{121, 120, false},
		{1, 1, true},
		{120, 120, true},
		{121, 121, true}
	};
	for (const Case &item : cases)
	{
		if (GuiControl::TestEngineFrameReady(item.engineFrame, item.guiFrame) != item.ready)
		{
			std::fprintf(stderr, "frame gate failed: engine=%d GUI=%d\n", item.engineFrame, item.guiFrame);
			return 1;
		}
	}
	std::puts("PASS: 9 frame-readiness cases (no native GUI acceptance)");
	return 0;
}
