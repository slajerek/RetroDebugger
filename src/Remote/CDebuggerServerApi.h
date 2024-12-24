#ifndef _CDebuggerServerApi_h_
#define _CDebuggerServerApi_h_

// generic server api
class CDebugInterface;
class CDebuggerApi;
class CDebuggerServer;

class CDebuggerServerApi
{
public:
	CDebuggerServerApi(CDebugInterface *debugInterface);
	CDebugInterface *debugInterface;
	CDebuggerApi *debuggerApi;
	
	virtual void RegisterEndpoints(CDebuggerServer *server);
};

#endif
