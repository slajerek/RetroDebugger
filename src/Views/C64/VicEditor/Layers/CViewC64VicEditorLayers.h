#ifndef _CViewVicEditorLayers_H_
#define _CViewVicEditorLayers_H_

#include "SYS_Defs.h"
#include "CGuiView.h"
#include "CGuiEditHex.h"
#include "CGuiViewFrame.h"
#include "CGuiList.h"
#include "CGuiButtonSwitch.h"
#include <vector>
#include <list>

class CSlrFont;
class CDataAdapter;
class CViewDataMap;
class CSlrMutex;
class CDebugInterfaceC64;
class CViewC64VicEditor;
class CVicEditorLayer;

class CViewC64VicEditorLayers : public CGuiView, public CGuiListCallback, CGuiButtonSwitchCallback
{
public:
	CViewC64VicEditorLayers(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CViewC64VicEditor *vicEditor);
	
	virtual bool KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	
	virtual bool DoTap(float x, float y);
	
	virtual bool DoRightClick(float x, float y);
//	virtual bool DoFinishRightClick(float x, float y);

	virtual bool DoMove(float x, float y, float distX, float distY, float diffX, float diffY);

	virtual void SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY);
	
	virtual void Render();
	virtual void RenderImGui();
	virtual void DoLogic();
	
	//
	CViewC64VicEditor *vicEditor;
	
	CGuiList *lstLayers;
	std::vector<CGuiButtonSwitch *> btnsVisible;
	
	void RefreshLayers();
	void UpdateVisibleSwitchButtons();
	
	CSlrFont *font;
	float fontScale;
	float scale;
	
	virtual bool ButtonSwitchChanged(CGuiButtonSwitch *button);
	virtual void ListElementSelected(CGuiList *listBox);
	void SelectLayer(CVicEditorLayer *layer);

	void SelectNextLayer();

	void SetLayerVisible(CVicEditorLayer *layer, bool isVisible);
};


#endif

