#ifndef _VIEWVICEDITORPREVIEW_H_
#define _VIEWVICEDITORPREVIEW_H_

#include "CViewC64VicDisplay.h"

class CViewC64VicEditor;
class CSlrString;

class CViewC64VicEditorDisplayPreview : public CViewC64VicDisplay
{
public:
	CViewC64VicEditorDisplayPreview(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY,
								 CDebugInterfaceC64 *debugInterface, CViewC64VicEditor *vicEditor);

	CViewC64VicEditor *vicEditor;
	vicii_cycle_state_t *viciiState;

	virtual void SetViciiState(vicii_cycle_state_t *viciiState);
	
	virtual void Render();
};

#endif
