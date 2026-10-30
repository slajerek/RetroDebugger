#ifndef _CDebuggerGuiControl_h_
#define _CDebuggerGuiControl_h_

class CDebuggerServer;

namespace GuiControl
{
	void RegisterEndpoint(CDebuggerServer *server);
	void PostFrame();
}

#endif
