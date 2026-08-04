#ifdef WIN32
#include <winsock2.h>
#endif

#include "DBG_Log.h"
#include "C64D_Version.h"
#include "CDebuggerServerWebSockets.h"
#include "CViewWSLog.h"
#include "CViewC64.h"
#include "CDebugInterface.h"
#include "CDebuggerServerApi.h"
#include "CGuiMain.h"
#include "SYS_FileSystem.h"
#include "json.hpp"
#include <set>

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
	startRequested = false;
	numConnectedClients = 0;
}

void CDebuggerServerWebSockets::SetPort(int port)
{
	this->port = port;
}

void CDebuggerServerWebSockets::Start()
{
	// serverStarted and isRunning are both set by the server thread once it is up, so
	// two Start() calls in a row (the menu handler used to do exactly that) both see
	// them false and the second SYS_StartThread hits SYS_FatalExit("thread is already
	// running") — the process dies instantly, with no dialog and no log entry. This
	// flag is set here, synchronously, so the second call is refused instead.
	if (startRequested || serverStarted)
	{
		LOGError("CDebuggerServerWebSockets: server already started");
		return;
	}

	startRequested = true;
	SYS_StartThread(this);
}

// Note, Stop will not shutdown server immediately, appLoop will still run if clients are connected
// TODO: force disconnect clients in CDebuggerServerWebSockets
void CDebuggerServerWebSockets::Stop()
{
	if (!serverStarted)
	{
		LOGError("CDebuggerServerWebSockets: server is not running");
		// a start that never reached the listening state must not block the next one
		startRequested = false;
		return;
	}

	if (!listenSocket)
	{
		LOGError("CDebuggerServerWebSockets: listenSocket is NULL");
		serverStarted = false;
		startRequested = false;
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

	// Pre-allocate hash map to avoid rehashing during bulk endpoint registration.
	// Each platform registers ~30 endpoints, with up to 4 platforms + ~10 server-level.
	endpointFunctions.reserve(256);
	endpointDescriptors.reserve(256);

	//
	AddEndpointFunction("load", [this](string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
	{
		string fileName = params.at("path").get<string>();

		if (!SYS_FileExists(fileName.c_str()))
		{
			json errorJson;
			errorJson["error"] = "File not found: " + fileName;
			return PrepareResult(HTTP_NOT_FOUND, token, errorJson, NULL, 0);
		}

		CSlrString *str = new CSlrString(StringToUtf16(string(fileName)));

		// LoadFile calls GL functions (e.g. image import fallback) — must run on the UI thread.
		// BroadcastEvent is also deferred here so "media.loaded" fires only after the load actually runs.
		struct LoadFileTask : public CUiThreadTaskCallback
		{
			CSlrString *path;
			CMainMenuHelper *helper;
			CDebuggerServerWebSockets *server;
			string filePath;
			LoadFileTask(CSlrString *path, CMainMenuHelper *helper, CDebuggerServerWebSockets *server, const string &filePath)
				: path(path), helper(helper), server(server), filePath(filePath) {}
			~LoadFileTask() { delete path; }
			void RunUIThreadTask() override
			{
				helper->LoadFile(path);
				json ev;
				ev["path"] = filePath;
				server->BroadcastEvent("media.loaded", ev);
			}
		};
		guiMain->AddUiThreadTask(new LoadFileTask(str, viewC64->mainMenuHelper, this, fileName));

		json result;
		result["status"] = "queued";
		result["path"] = fileName;
		return PrepareResult(HTTP_OK, token, result, NULL, 0);
	});
	
	// register endpoints for all emulators
	for (auto it = viewC64->debugInterfaces.begin(); it != viewC64->debugInterfaces.end(); it++)
	{
		CDebugInterface *debugInterface = *it;
		CDebuggerServerApi *webSocketsApi = debugInterface->GetDebuggerServerApi();
		webSocketsApi->RegisterEndpoints(this);
	}

	// Discovery endpoints (server-level, v2)
	AddEndpointFunction("server/hello", [this](string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
	{
		json result;
		result["protocolVersion"] = DEBUGGER_PROTOCOL_CURRENT;
		result["serverName"] = "RetroDebugger";
		result["serverVersion"] = RETRODEBUGGER_VERSION_STRING;
		return PrepareResult(HTTP_OK, token, result, NULL, 0);
	});

	AddEndpointFunction("server/platforms", [this](string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
	{
		json result;
		json platforms = json::array();
		for (auto *di : viewC64->debugInterfaces)
		{
			json p;
			p["name"] = di->GetPlatformNameEndpointString();
			p["running"] = di->isRunning;
			platforms.push_back(p);
		}
		result["platforms"] = platforms;
		return PrepareResult(HTTP_OK, token, result, NULL, 0);
	});

	AddEndpointFunction("server/capabilities", [this](string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
	{
		json result;
		json platforms = json::array();
		for (auto *di : viewC64->debugInterfaces)
		{
			json p;
			p["name"] = di->GetPlatformNameEndpointString();
			p["fullName"] = di->GetPlatformNameString();
			p["running"] = di->isRunning;

			// List categories of endpoints available for this platform
			json categories = json::array();
			string platPrefix = string(di->GetPlatformNameEndpointString()) + "/";
			std::set<string> catSet;
			for (const auto &desc : endpointDescriptors)
			{
				if (desc.platform == di->GetPlatformNameEndpointString() && !desc.category.empty())
					catSet.insert(desc.category);
			}
			for (const auto &cat : catSet)
				categories.push_back(cat);
			p["categories"] = categories;

			// Count endpoints
			int count = 0;
			int stubbed = 0;
			for (const auto &desc : endpointDescriptors)
			{
				if (desc.platform == di->GetPlatformNameEndpointString())
				{
					count++;
					if (desc.isStubbed) stubbed++;
				}
			}
			p["endpointCount"] = count;
			p["stubbedCount"] = stubbed;

			platforms.push_back(p);
		}
		result["platforms"] = platforms;
		result["protocolVersion"] = DEBUGGER_PROTOCOL_CURRENT;
		result["discoveryVersion"] = DEBUGGER_DISCOVERY_VERSION;
		result["features"] = json::array({"platforms", "endpointDescriptors", "binaryFraming"});
		result["serverName"] = "RetroDebugger";
		return PrepareResult(HTTP_OK, token, result, NULL, 0);
	});

	AddEndpointFunction("server/endpoints", [this](string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
	{
		json result;
		json endpoints = json::array();
		// List all registered endpoint names
		for (const auto &pair : endpointFunctions)
		{
			json ep;
			ep["fn"] = pair.first;
			endpoints.push_back(ep);
		}
		// Add descriptor details where available
		for (const auto &desc : endpointDescriptors)
		{
			// Find and enrich the matching entry
			for (auto &ep : endpoints)
			{
				if (ep["fn"] == desc.fn)
				{
					ep = desc.ToJson();
					break;
				}
			}
		}
		result["endpoints"] = endpoints;
		return PrepareResult(HTTP_OK, token, result, NULL, 0);
	});

	AddEndpointFunction("server/events", [this](string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
	{
		json result;
		json events = json::array();
		events.push_back({{"name", "breakpoint"}, {"description", "CPU or memory breakpoint hit"}});
		events.push_back({{"name", "emulation.paused"}, {"description", "Emulation paused"}});
		events.push_back({{"name", "emulation.continued"}, {"description", "Emulation resumed"}});
		events.push_back({{"name", "emulation.reset"}, {"description", "Machine reset (hard or soft)"}});
		events.push_back({{"name", "media.loaded"}, {"description", "File loaded into emulator"}});
		result["events"] = events;
		return PrepareResult(HTTP_OK, token, result, NULL, 0);
	});

	// Note: server/events/subscribe and server/events/unsubscribe are handled
	// directly in the message handler (above) because they need the ws pointer
	// for uWS topic management. They don't go through RunEndpointFunction.

	app->ws<SocketData>("/stream", {
		// uWS defaults to 16 kB, and anything above it is dropped by closing the socket
		// without an error or a close frame — so a client sending a whole PRG or disk
		// image in one writeBlock just loses the connection with no way to tell why.
		// 4 MB covers every realistic payload here (64 kB RAM, D64, D81, CRT) with room
		// to spare. Field order must match the declaration order in uWS::WebSocketBehavior.
		.maxPayloadLength = 4 * 1024 * 1024,
		.idleTimeout = 0,              // Disable uWS idle timeout — bridge has its own 30s socket timeout
		.sendPingsAutomatically = false, // No auto-pings when idle timeout is disabled
		.open = [this](auto* ws)
		{
			numConnectedClients++;
			LOGD("WebSocket connection opened (numConnectedClients=%d)", numConnectedClients);
			ws->subscribe("broadcast");
			CViewWSLog::LogConnection("connected");
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

				// Detect v2 protocol
				if (j.contains("protocolVersion"))
				{
					SocketData *sd = (SocketData *)ws->getUserData();
					int ver = j["protocolVersion"].get<int>();
					if (sd->protocolVersion < ver)
						sd->protocolVersion = ver;
				}

				// Intercept subscribe/unsubscribe — needs ws pointer
				if (fn == "server/events/subscribe")
				{
					string eventName = j["params"].value("event", "");
					if (!eventName.empty())
					{
						ws->subscribe("event:" + eventName);
						LOGD("WebSocket client subscribed to event:%s", eventName.c_str());
					}
					json resp;
					resp["status"] = HTTP_OK;
					if (!token.empty()) resp["token"] = token;
					resp["result"]["subscribed"] = eventName;
					string respStr = resp.dump();
					ws->send(respStr, uWS::TEXT);
					return;
				}
				if (fn == "server/events/unsubscribe")
				{
					string eventName = j["params"].value("event", "");
					if (!eventName.empty())
					{
						ws->unsubscribe("event:" + eventName);
						LOGD("WebSocket client unsubscribed from event:%s", eventName.c_str());
					}
					json resp;
					resp["status"] = HTTP_OK;
					if (!token.empty()) resp["token"] = token;
					resp["result"]["unsubscribed"] = eventName;
					string respStr = resp.dump();
					ws->send(respStr, uWS::TEXT);
					return;
				}

				{
					string paramsPreview = j["params"].dump();
					CViewWSLog::LogRequest(fn.c_str(), paramsPreview.c_str());
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

				if (result && !result->empty())
				{
					string_view resultStr(result->data(), result->size());
					ws->send(resultStr, uWS::BINARY, (resultStr.length() > 512));
					// Log response: parse status from result JSON
					try
					{
						size_t nullPos = resultStr.find('\0');
						string jsonPart(resultStr.data(), nullPos != string_view::npos ? nullPos : resultStr.size());
						json rj = json::parse(jsonPart);
						int status = rj.value("status", 0);
						std::string bodyStr;
						if (rj.contains("result") && !rj["result"].is_null())
							bodyStr = rj["result"].dump();
						CViewWSLog::LogResponse(status, fn.c_str(), (int)resultStr.size(), bodyStr.empty() ? nullptr : bodyStr.c_str());
					}
					catch (...) {}
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
			CViewWSLog::LogConnection("disconnected");
		}
	});

	LOGD("CDebuggerServerWebSockets: run");
	serverStarted = true;
	app->run();
	
	LOGM("WebSockets debugger server shutdown");
	delete app;
	app = NULL;
	serverStarted = false;
	startRequested = false;
}

void CDebuggerServerWebSockets::AddEndpointFunction(const string& functionName, function<vector<char>*(const string, const json, u8 *, int)> func)
{
	endpointFunctions[functionName] = func;
}

void CDebuggerServerWebSockets::AddEndpointFunction(const EndpointDescriptor &desc, EndpointHandlerV1 handler)
{
	endpointFunctions[desc.fn] = handler;

	// The handler map is keyed by fn, so a re-registration replaces it; the descriptor
	// vector has no such key and would keep a stale twin. That happens for real: a
	// duplicated registration, and every Stop()+Start() cycle, which re-runs the whole
	// registration and would otherwise grow the descriptor list without bound.
	for (size_t i = 0; i < endpointDescriptors.size(); i++)
	{
		if (endpointDescriptors[i].fn == desc.fn)
		{
			endpointDescriptors[i] = desc;
			return;
		}
	}

	endpointDescriptors.push_back(desc);
}

vector<EndpointDescriptor> CDebuggerServerWebSockets::GetEndpointDescriptors()
{
	return endpointDescriptors;
}

vector<char> *CDebuggerServerWebSockets::RunEndpointFunction(const string& functionName, const string token, json params, u8 *binaryData, int binaryDataSize)
{
	auto it = endpointFunctions.find(functionName);
	if (it != endpointFunctions.end())
	{
		try
		{
			return it->second(token, params, binaryData, binaryDataSize);
		}
		catch (const std::exception &e)
		{
			LOGError("CDebuggerServerWebSockets::RunEndpoint: %s threw: %s", functionName.c_str(), e.what());
			json errorJson;
			errorJson["error"] = string("endpoint error: ") + e.what();
			return PrepareResult(HTTP_INTERNAL_SERVER_ERROR, token, errorJson, NULL, 0);
		}
	}
	else
	{
		LOGError("CDebuggerServerWebSockets::RunEndpoint: endpoint %s not found", functionName.c_str());
		json errorJson;
		errorJson["error"] = string("endpoint not found: ") + functionName;
		return PrepareResult(HTTP_NOT_FOUND, token, errorJson, NULL, 0);
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

vector<char> *CDebuggerServerWebSockets::RunEndpointFunction(const string& functionName, const RequestContext &ctx, json params, u8 *binaryData, int binaryDataSize)
{
	// v2 dispatch: currently delegates to v1 handler (lambdas use token string)
	// The endpoint gets the token from context for backward compat
	return RunEndpointFunction(functionName, ctx.token, params, binaryData, binaryDataSize);
}

vector<char> *CDebuggerServerWebSockets::PrepareResult(int status, const RequestContext &ctx, json resultJson, u8 *binaryData, int binaryDataSize)
{
	LOGD("CDebuggerServerWebSockets::PrepareResult (v2 context)");

	// Use the protocol helper to build the response JSON
	json sendJson = DebuggerProtocol::MakeResponse(ctx, status, resultJson);

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
	LOGD("WebSocketsDebuggerServer::BroadcastEvent: %s", eventName);

	j["event"] = eventName;
	string eventTopic = string("event:") + eventName;
	appLoop->defer([this, j, eventTopic]()
	{
		string msg = j.dump();
		app->publish("broadcast", msg, uWS::OpCode::TEXT);     // v1 clients (all events)
		app->publish(eventTopic, msg, uWS::OpCode::TEXT);       // v2 clients (selective)
	});
}
