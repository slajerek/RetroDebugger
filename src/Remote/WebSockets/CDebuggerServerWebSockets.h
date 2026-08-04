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
	// set synchronously by Start(), unlike serverStarted/isRunning which are only set
	// once the server thread is already up — see the comment in Start()
	bool startRequested;
	int numConnectedClients;

	virtual void AddEndpointFunction(const std::string& endpointName, std::function<std::vector<char> *(const std::string, const nlohmann::json, u8 *, int)> func);
	virtual void AddEndpointFunction(const EndpointDescriptor &desc, EndpointHandlerV1 handler);
	virtual std::vector<char> *RunEndpointFunction(const std::string& endpointName, const std::string token, nlohmann::json params, u8 *binaryData, int binaryDataSize);
	virtual std::vector<char> *PrepareResult(int status, const std::string token, nlohmann::json resultJson, u8 *binaryData, int binaryDataSize);
	virtual std::vector<char> *RunEndpointFunction(const std::string& endpointName, const RequestContext &ctx, nlohmann::json params, u8 *binaryData, int binaryDataSize);
	virtual std::vector<char> *PrepareResult(int status, const RequestContext &ctx, nlohmann::json resultJson, u8 *binaryData, int binaryDataSize);
	virtual std::vector<EndpointDescriptor> GetEndpointDescriptors();
	
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
	std::vector<EndpointDescriptor> endpointDescriptors;
};

struct SocketData {
	int protocolVersion;   // 1 = legacy (all events via broadcast), 2 = selective
	SocketData() : protocolVersion(1) {}
};

#endif
