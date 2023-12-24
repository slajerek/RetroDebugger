#ifndef _CViewDebugEventsHistory_h_
#define _CViewDebugEventsHistory_h_

#include "CGuiView.h"

class CDebugInterface;

class CViewDebugEventsHistory : public CGuiView
{
public:
	CViewDebugEventsHistory(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterface *debugInterface);
	virtual ~CViewDebugEventsHistory();

	virtual void RenderImGui();

	virtual bool HasContextMenuItems();
	virtual void RenderContextMenuItems();

	virtual void ActivateView();
	virtual void DeactivateView();
	
	CDebugInterface *debugInterface;
};

#endif //_CViewDebugEventsHistory_h_
