#include "CDebugInterfaceMenuItemView.h"
#include "CGuiMain.h"
#include "CGuiView.h"

CDebugInterfaceMenuItemView::CDebugInterfaceMenuItemView(CGuiView *view, const char *menuItemStr)
: CDebugInterfaceMenuItem(menuItemStr)
{
	this->view = view;
}

CDebugInterfaceMenuItemView::~CDebugInterfaceMenuItemView()
{
}

void CDebugInterfaceMenuItemView::RenderImGui()
{
	bool isVisible = view->visible;
	if (ImGui::MenuItem(view->name, "", &isVisible))
	{
		view->SetVisible(true);
		guiMain->SetFocus(view);
		guiMain->StoreLayoutInSettingsAtEndOfThisFrame();
	}
}
