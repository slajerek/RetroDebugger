#ifndef _CViewDataPlot_h_
#define _CViewDataPlot_h_

#include "CGuiView.h"
#include "implot.h"

class CDebugSymbols;
class CDataAdapter;

enum DataPlotFormat : int {
	DataPlotFormat_U4_LE = 0,
	DataPlotFormat_U4_BE,
	DataPlotFormat_U8,
	DataPlotFormat_S8,
	DataPlotFormat_U16_LE,
	DataPlotFormat_S16_LE,
	DataPlotFormat_U16_BE,
	DataPlotFormat_S16_BE
};

class CViewDataPlot : public CGuiView, public ImPlotCallback
{
public:
	CViewDataPlot(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY,
				  CDebugSymbols *symbols, CDataAdapter *dataAdapter);
	virtual ~CViewDataPlot();

	virtual void RenderImGui();

	virtual bool HasContextMenuItems();
	virtual void RenderContextMenuItems();

	virtual void ActivateView();
	virtual void DeactivateView();
	
	void SetPlotDataLimits(int min, int max);
	
	//
	CDebugSymbols *symbols;
	CDataAdapter *dataAdapter;
	
	int dataIndexMin;
	int dataIndexMax;
	int dataLen;
	
	double *dataIndexes;
	u8 *dataBuffer;
	
	char *settingNameStrDataPlotFormat;
	DataPlotFormat dataPlotFormat;
	
	//
	virtual void ImPlotContextMenuRenderCallback();

};

#endif //_CViewDataPlot_h_
