#ifndef _CViewDrive1541BreakpointsIrq_h_
#define _CViewDrive1541BreakpointsIrq_h_

#include "CGuiView.h"

class CDebugSymbolsDrive1541;

class CViewDrive1541BreakpointsIrq : public CGuiView
{
public:
	CViewDrive1541BreakpointsIrq(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugSymbolsDrive1541 *debugSymbolsDrive1541);
	virtual ~CViewDrive1541BreakpointsIrq();

	virtual void RenderImGui();

	virtual bool HasContextMenuItems();
	virtual void RenderContextMenuItems();

	virtual void ActivateView();
	virtual void DeactivateView();
	
	CDebugSymbolsDrive1541 *debugSymbolsDrive1541;
};

#endif //_CViewC64IrqBreakpoints_h_
