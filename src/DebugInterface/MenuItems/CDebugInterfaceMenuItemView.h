#ifndef _CDebugInterfaceMenuItemView_h_
#define _CDebugInterfaceMenuItemView_h_

#include "CDebugInterfaceMenuItem.h"

class CGuiView;

class CDebugInterfaceMenuItemView : public CDebugInterfaceMenuItem
{
public:
	CDebugInterfaceMenuItemView(CGuiView *view, const char *menuItemStr);
	~CDebugInterfaceMenuItemView();
	
	CGuiView *view;

	virtual void RenderImGui();
};

#endif
