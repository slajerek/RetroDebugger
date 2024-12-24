#ifndef _CDebugBreakpointRasterLine_h_
#define _CDebugBreakpointRasterLine_h_

#include "CDebugBreakpointAddr.h"

class CDebugBreakpointRasterLine : public CDebugBreakpointAddr
{
public:
	CDebugBreakpointRasterLine(int rasterLine);
	CDebugBreakpointRasterLine(CDebugSymbols *debugSymbols, int rasterLine);

	virtual void GetDetailsJson(nlohmann::json &j);
};


#endif
