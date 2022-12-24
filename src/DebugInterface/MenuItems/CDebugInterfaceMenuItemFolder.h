#ifndef _CDebugInterfaceMenuItemFolder_h_
#define _CDebugInterfaceMenuItemFolder_h_

#include "CDebugInterfaceMenuItem.h"
#include <list>

class CDebugInterfaceMenuItemFolder : public CDebugInterfaceMenuItem
{
public:
	CDebugInterfaceMenuItemFolder(const char *menuItemStr);
	~CDebugInterfaceMenuItemFolder();
	
	std::list<CDebugInterfaceMenuItem *> menuItems;
	void AddMenuItem(CDebugInterfaceMenuItem *menuItem);
	
	virtual void RenderImGui();
};

#endif
