#ifndef _CDebuggerServerApi_h_
#define _CDebuggerServerApi_h_

#include "CDebugInterface.h"

// RAII mutex guard for endpoint handlers — ensures unlock on any exit path
// (normal return, early return, or exception)
class CDebugInterfaceMutexGuard
{
public:
	CDebugInterfaceMutexGuard(CDebugInterface *di) : di(di) { di->LockMutex(); }
	~CDebugInterfaceMutexGuard() { di->UnlockMutex(); }
private:
	CDebugInterface *di;
	CDebugInterfaceMutexGuard(const CDebugInterfaceMutexGuard&) = delete;
	CDebugInterfaceMutexGuard& operator=(const CDebugInterfaceMutexGuard&) = delete;
};

class CDebuggerApi;
class CDebuggerServer;

class CDebuggerServerApi
{
public:
	CDebuggerServerApi(CDebugInterface *debugInterface);
	CDebugInterface *debugInterface;
	CDebuggerApi *debuggerApi;
	
	virtual void RegisterEndpoints(CDebuggerServer *server);
	static void RegisterGuiEndpoint(CDebuggerServer *server);
	static void GuiPostFrame();
};

#endif
