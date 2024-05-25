#ifndef __CViewDataWatch__
#define __CViewDataWatch__

#include "SYS_Defs.h"
#include "DebuggerDefs.h"
#include "GUI_Main.h"
#include "CDebugSymbols.h"
#include "imguiComboFilter.h"
#include "CGuiView.h"
#include <map>

class C64;
class CImageData;
class CSlrImage;
class CViewDataDump;
class CDebugInterface;
class CSlrFont;
class CViewDataMap;
class CDataAdapter;
class CDebugSymbols;
class CDebugSymbolsDataWatch;


class CViewDataWatch : public CGuiView, public ImGui::ComboFilterCallback
{
public:
	CViewDataWatch(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY,
				   CDebugSymbols *symbols, CViewDataMap *viewMemoryMap);
	~CViewDataWatch();
	
	virtual void DoLogic();
	virtual void RenderImGui();

	CDebugSymbols *symbols;
	CDebugInterface *debugInterface;
	CDataAdapter *dataAdapter;
	CViewDataMap *viewMemoryMap;
	
	CViewDataDump *viewDataDump;
	void SetViewC64DataDump(CViewDataDump *viewDataDump);
	
	virtual bool ComboFilterShouldOpenPopupCallback(const char *label, char *buffer, int bufferlen,
													const char **hints, int num_hints, ImGui::ComboFilterState *s);

protected:
	int addWatchPopupAddr;
	char addWatchPopupSymbol[256];
	int imGuiColumnsWidthWorkaroundFrame;
	int imGuiOpenPopupFrame;
	ImGui::ComboFilterState comboFilterState = {0, false};
	char comboFilterTextBuf[MAX_LABEL_TEXT_BUFFER_SIZE];

};

#endif
