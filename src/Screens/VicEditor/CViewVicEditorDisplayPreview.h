#ifndef _VIEWVICEDITORPREVIEW_H_
#define _VIEWVICEDITORPREVIEW_H_

#include "CViewC64VicDisplay.h"

class CViewVicEditor;
class CSlrString;

class CViewVicEditorDisplayPreview : public CViewC64VicDisplay
{
public:
	CViewVicEditorDisplayPreview(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY,
								 CDebugInterfaceC64 *debugInterface, CSlrString *windowName, CViewVicEditor *vicEditor);

	CViewVicEditor *vicEditor;
	vicii_cycle_state_t *viciiState;

	virtual void SetViciiState(vicii_cycle_state_t *viciiState);
	
	virtual void Render();
};

#endif
