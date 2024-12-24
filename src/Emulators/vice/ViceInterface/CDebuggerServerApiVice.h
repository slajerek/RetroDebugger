#ifndef _CDebuggerServerApiVice_h_
#define _CDebuggerServerApiVice_h_

#include "CDebuggerServerApi.h"
#include "drive.h"

class CDebugInterfaceVice;
class CDebuggerApiVice;
class CSidData;

const int MAX_DRIVE_NUM = DRIVE_NUM - 1;

class CDebuggerServerApiVice : public CDebuggerServerApi
{
public:
	CDebuggerServerApiVice(CDebugInterface *debugInterface);
	CDebugInterfaceVice *debugInterfaceVice;
	CDebuggerApiVice *debuggerApiVice;
	
	virtual void RegisterEndpoints(CDebuggerServer *server);
	
	//
	CSidData *sidData;
};

#endif
