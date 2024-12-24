#ifdef WIN32
#include <winsock2.h>
#endif

#include "DBG_Log.h"
#include "CDebuggerServerWebSockets.h"
#include "CViewC64.h"
#include "CDebugInterface.h"
#include "CDebuggerServerApi.h"
#include "json.hpp"

using namespace std;
using namespace nlohmann;

CDebuggerServer *REMOTE_CreateDebuggerServerWebSockets(int port)
{
	return new CDebuggerServerWebSockets(port);
}

void REMOTE_DebuggerServerWebSocketsSetPort(CDebuggerServer *debuggerServer, int port)
{
	((CDebuggerServerWebSockets*)debuggerServer)->SetPort(port);
}

CDebuggerServerWebSockets::CDebuggerServerWebSockets(int port)
: port(port)
{
	app = NULL;
	serverStarted = false;
	numConnectedClients = 0;
}

void CDebuggerServerWebSockets::SetPort(int port)
{
	this->port = port;
}

void CDebuggerServerWebSockets::Start()
{
	if (serverStarted)
	{
		LOGError("CDebuggerServerWebSockets: server already started");
		return;
	}

	SYS_StartThread(this);
}

// Note, Stop will not shutdown server immediately, appLoop will still run if clients are connected
// TODO: force disconnect clients in CDebuggerServerWebSockets
void CDebuggerServerWebSockets::Stop()
{
	if (!serverStarted)
	{
		LOGError("CDebuggerServerWebSockets: server is not running");
		return;
	}
	
	if (!listenSocket)
	{
		LOGError("CDebuggerServerWebSockets: listenSocket is NULL");
		serverStarted = false;
		return;
	}
	
	appLoop->defer([this]()
	{
		us_listen_socket_close(0, listenSocket);
		listenSocket = nullptr;
		numConnectedClients = 0;
	});
}

bool CDebuggerServerWebSockets::AreClientsConnected()
{
	if (serverStarted && numConnectedClients > 0)
		return true;
	return false;
}

void CDebuggerServerWebSockets::ThreadRun(void *passData)
{
	ThreadSetName("WSDebugServer");
		
	numConnectedClients = 0;
	
	app = new uWS::App();
	app->listen(this->port, [this](auto *token)
	{
		if (token)
		{
			this->listenSocket = token;
			LOGM("WebSockets debugger server listening on port %d", port);
		}
		else
		{
			LOGError("WebSockets debugger server failed to listen on port %d", port);
			return;
		}
	});
	
	appLoop = app->getLoop();

	//
	AddEndpointFunction("load", [this](string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
	{
		string fileName = params.at("path").get<string>();
		CSlrString *str = new CSlrString(StringToUtf16(string(fileName)));
		viewC64->mainMenuHelper->LoadFile(str);
		delete str;
		return PrepareResult(HTTP_OK, token, json(), NULL, 0);
	});
	
	// register endpoints for all emulators
	for (auto it = viewC64->debugInterfaces.begin(); it != viewC64->debugInterfaces.end(); it++)
	{
		CDebugInterface *debugInterface = *it;
		CDebuggerServerApi *webSocketsApi = debugInterface->GetDebuggerServerApi();
		webSocketsApi->RegisterEndpoints(this);
	}

	app->ws<SocketData>("/stream", {
		.open = [this](auto* ws)
		{
			numConnectedClients++;
			LOGD("WebSocket connection opened (numConnectedClients=%d)", numConnectedClients);
			ws->subscribe("broadcast");
		},
		.message = [this](auto* ws, string_view message, uWS::OpCode opCode) 
		{
			string jsonStr;

			try
			{
				auto endPos = message.find('\0');
				bool hasBinary = false;
				int binaryDataSize = 0;
				
				if (endPos != string_view::npos)
				{
					jsonStr = message.substr(0, endPos);
					binaryDataSize = (int)message.size() - (int)endPos - 1;

					if (binaryDataSize > 0)
						hasBinary = true;
				}
				else
				{
					jsonStr = message;
				}
				
//				LOGD("/stream JSON:")
//				cout << message << endl;
				
				json j = json::parse(jsonStr);
				
				string fn = j["fn"].get<string>();
				string token;
				
				if (j.contains("token"))
				{
					token = j["token"].get<string>();
				}

				vector<char> *result;
				if (hasBinary == false)
				{
					result = RunEndpointFunction(fn, token, j["params"], nullptr, 0);
				}
				else
				{
					unsigned char* binaryData = nullptr;
					
					if (binaryDataSize > 0 && message.size() > endPos + 1)
					{
						binaryData = new unsigned char[binaryDataSize];
						memcpy(binaryData, message.data() + endPos + 1, binaryDataSize);
					}
					result = RunEndpointFunction(fn, token, j["params"], binaryData, binaryDataSize);
					delete[] binaryData;
				}
				
				string_view resultStr(result->data(), result->size());

//				cout << "Result: " << resultStr << endl;
				
				if (!resultStr.empty())
				{
					ws->send(resultStr, uWS::BINARY, (resultStr.length() > 512));
				}
				
				delete result;
			}
			catch (const exception &e) 
			{
				LOGError("CDebuggerServerWebSockets: invalid JSON format %s", e.what());
				LOGError("CDebuggerServerWebSockets: json=%s", jsonStr.c_str());
				json j;
				j["status"] = HTTP_BAD_REQUEST;
				j["error"] = e.what();
				ws->send(j.dump(), uWS::TEXT);
			}
		},
		.close = [this](auto* ws, int code, string_view message)
		{
			numConnectedClients--;
			LOGD("WebSocket connection closed (numConnectedClients=%d)", numConnectedClients);
		}
	});

	LOGD("CDebuggerServerWebSockets: run");
	serverStarted = true;
	app->run();
	
	LOGM("WebSockets debugger server shutdown");
	delete app;
	app = NULL;
	serverStarted = false;
}

void CDebuggerServerWebSockets::AddEndpointFunction(const string& functionName, function<vector<char>*(const string, const json, u8 *, int)> func)
{
	endpointFunctions[functionName] = func;
}

vector<char> *CDebuggerServerWebSockets::RunEndpointFunction(const string& functionName, const string token, json params, u8 *binaryData, int binaryDataSize)
{
	auto it = endpointFunctions.find(functionName);
	if (it != endpointFunctions.end()) 
	{
		// Execute the lambda associated with the endpoint
		return it->second(token, params, binaryData, binaryDataSize);
	}
	else
	{
		LOGError("CDebuggerServerWebSockets::RunEndpoint: endpoint %s not found", functionName.c_str());
		return new vector<char>();
	}
}

vector<char> *CDebuggerServerWebSockets::PrepareResult(int status, const string token, json resultJson, u8 *binaryData, int binaryDataSize)
{
	LOGD("CDebuggerServerWebSockets::PrepareResult");
	json sendJson;
	sendJson["status"] = status;
	if (!token.empty())
	{
		sendJson["token"] = token;
	}
	if (!resultJson.empty())
	{
		sendJson["result"] = resultJson;
	}
	
	string sendJsonStr = sendJson.dump();

	vector<char> *outBuffer = new vector<char>();
	outBuffer->reserve(sendJsonStr.length() + binaryDataSize + 2);

	outBuffer->insert(outBuffer->end(), sendJsonStr.begin(), sendJsonStr.end());
	
	if (binaryData)
	{
		outBuffer->push_back(0);
		outBuffer->insert(outBuffer->end(), binaryData, binaryData + binaryDataSize);
	}
	
	return outBuffer;
}

void CDebuggerServerWebSockets::BroadcastEvent(const char *eventName, nlohmann::json j)
{
	LOGD("WebSocketsDebuggerServer::BroadcastEvent");
	
	j["event"] = eventName;	
	appLoop->defer([this, j]()
	{
		app->publish("broadcast", j.dump(), uWS::OpCode::TEXT);
	});
}
