#include "CDebugInterfaceMenuItemFolder.h"
#include "CGuiView.h"

CDebugInterfaceMenuItemFolder::CDebugInterfaceMenuItemFolder(const char *menuItemStr)
: CDebugInterfaceMenuItem(menuItemStr)
{
	
}

CDebugInterfaceMenuItemFolder::~CDebugInterfaceMenuItemFolder()
{
}

void CDebugInterfaceMenuItemFolder::AddMenuItem(CDebugInterfaceMenuItem *menuItem)
{
	menuItems.push_back(menuItem);
}

void CDebugInterfaceMenuItemFolder::RenderImGui()
{
	if (ImGui::BeginMenu(menuItemStr))
	{
		for (std::list<CDebugInterfaceMenuItem *>::iterator it = menuItems.begin(); it != menuItems.end(); it++)
		{
			CDebugInterfaceMenuItem *menuItem = *it;
			menuItem->RenderImGui();
		}
		ImGui::EndMenu();
	}
}
