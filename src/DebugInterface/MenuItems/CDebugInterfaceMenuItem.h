#ifndef _CDebugInterfaceMenuItem_h_
#define _CDebugInterfaceMenuItem_h_

// menu items to hold views and folders in emulator's menu
class CDebugInterfaceMenuItem
{
public:
	CDebugInterfaceMenuItem(const char *menuItemStr);
	~CDebugInterfaceMenuItem();
	
	const char *menuItemStr;
	
	virtual void RenderImGui();
};

#endif
