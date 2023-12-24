#ifndef _CViewDataMonitor_h_
#define _CViewDataMonitor_h_

#include "CGuiView.h"

class CSlrFont;
class CSlrMutex;
class CDataAdapter;
class CViewMemoryMap;
class CDebugInterface;
class CViewDisassembly;
class CDebugSymbols;
class CDebugInterface;

class CDataCaptureState
{
public:
	CDataCaptureState(u64 cycle, u32 frame, u8 *dataCopy);
	~CDataCaptureState();
	char *name;
	u64 cycle;
	u32 frame;
	u8 *dataCopy;
};

class CViewDataMonitor : public CGuiView
{
public:
	CViewDataMonitor(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY,
					 CDebugSymbols *symbols, CDataAdapter *dataAdapter, CViewMemoryMap *viewMemoryMap, CViewDisassembly *viewDisassembly);
	virtual ~CViewDataMonitor();

	virtual void RenderImGui();

	virtual bool HasContextMenuItems();
	virtual void RenderContextMenuItems();

	virtual void ActivateView();
	virtual void DeactivateView();
	
	CDebugSymbols *symbols;
	CDebugInterface *debugInterface;
	CDataAdapter *dataAdapter;
	CDataAdapter *dataAdapterCode;
	CViewMemoryMap *viewMemoryMap;
	CViewDisassembly *viewDisassembly;
	
	// cycle#, u8 *data
	std::map<u64, CDataCaptureState *> dataCaptures;
	CDataCaptureState *currentDataCapture;
	
	int dataLen;
	u8 *data;
	
	// filters
	bool filterShowOnlyChanged;
	bool filterShowOnlyWhenMatchesPrevValue;
	int filterPrevValue;
	bool filterShowOnlyWhenMatchesCurrentValue;
	int filterCurrentValue;
	
	char buf[MAX_STRING_LENGTH];
	
	int *clipperAddresses;
};

#endif //_CViewDataMonitor_h_
