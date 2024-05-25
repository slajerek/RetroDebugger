#ifndef _CViewC64BreakpointsIrq_h_
#define _CViewC64BreakpointsIrq_h_

#include "CGuiView.h"

class CDebugSymbolsC64;

class CViewC64BreakpointsIrq : public CGuiView
{
public:
	CViewC64BreakpointsIrq(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugSymbolsC64 *debugSymbolsC64);
	virtual ~CViewC64BreakpointsIrq();

	virtual void RenderImGui();

	virtual bool HasContextMenuItems();
	virtual void RenderContextMenuItems();

	virtual void ActivateView();
	virtual void DeactivateView();
	
	CDebugSymbolsC64 *debugSymbolsC64;
};

#endif //_CViewC64IrqBreakpoints_h_
