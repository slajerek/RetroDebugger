#ifndef _CDebuggerServerWebSockets_h_
#define _CDebuggerServerWebSockets_h_

#include "CDebuggerServer.h"
#include "App.h"

class CDebugBreakpoint;

CDebuggerServer *REMOTE_CreateDebuggerServerWebSockets(int port);

class CDebuggerServerWebSockets : public CDebuggerServer
{
public:
	CDebuggerServerWebSockets(int port);
	
	virtual void Start();
	virtual void Stop();
	
	bool serverStarted;
	int numConnectedClients;

	virtual void AddEndpointFunction(const std::string& endpointName, std::function<std::vector<char> *(const std::string, const nlohmann::json, u8 *, int)> func);
	virtual std::vector<char> *RunEndpointFunction(const std::string& endpointName, const std::string token, nlohmann::json params, u8 *binaryData, int binaryDataSize);
	virtual std::vector<char> *PrepareResult(int status, const std::string token, nlohmann::json resultJson, u8 *binaryData, int binaryDataSize);
	
	virtual void ThreadRun(void *passData);

	virtual void BroadcastEvent(const char *eventName, nlohmann::json j);
		
	virtual bool AreClientsConnected();
	
	virtual void SetPort(int port);
	
private:
	int port;
	uWS::App *app;
	uWS::Loop *appLoop;
	us_listen_socket_t *listenSocket = nullptr;
	std::unordered_map<std::string, std::function<std::vector<char> *(const std::string token, const nlohmann::json, u8 *, int)>> endpointFunctions;
};

struct SocketData {};

#endif
