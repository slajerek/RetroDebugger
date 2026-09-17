#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#define CLOSE_SOCKET closesocket
#define POLL_FN WSAPoll
typedef int socklen_t;
typedef SSIZE_T ssize_t;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#define CLOSE_SOCKET close
#define POLL_FN poll
#endif

#include "CMCPBridgeClient.h"
#include "CMCPServer.h"
#include "DBG_Log.h"
#include "SYS_Main.h"
#include <SDL3/SDL.h>

#include <cstring>
#include <random>
#include <sstream>
#include <iomanip>
#include <algorithm>

// Minimal SHA-1 for WebSocket handshake (RFC 6455)
// Only used for the Sec-WebSocket-Accept validation
namespace {

struct SHA1Context
{
	uint32_t state[5];
	uint64_t count;
	uint8_t buffer[64];
};

static void SHA1Transform(uint32_t state[5], const uint8_t buffer[64])
{
	uint32_t a, b, c, d, e, w[80];
	for (int i = 0; i < 16; i++)
		w[i] = (buffer[i*4] << 24) | (buffer[i*4+1] << 16) | (buffer[i*4+2] << 8) | buffer[i*4+3];
	for (int i = 16; i < 80; i++)
	{
		uint32_t t = w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16];
		w[i] = (t << 1) | (t >> 31);
	}
	a = state[0]; b = state[1]; c = state[2]; d = state[3]; e = state[4];
	for (int i = 0; i < 80; i++)
	{
		uint32_t f, k;
		if (i < 20) { f = (b & c) | ((~b) & d); k = 0x5A827999; }
		else if (i < 40) { f = b ^ c ^ d; k = 0x6ED9EBA1; }
		else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8F1BBCDC; }
		else { f = b ^ c ^ d; k = 0xCA62C1D6; }
		uint32_t temp = ((a << 5) | (a >> 27)) + f + e + k + w[i];
		e = d; d = c; c = (b << 30) | (b >> 2); b = a; a = temp;
	}
	state[0] += a; state[1] += b; state[2] += c; state[3] += d; state[4] += e;
}

static void SHA1Init(SHA1Context *ctx)
{
	ctx->state[0] = 0x67452301; ctx->state[1] = 0xEFCDAB89;
	ctx->state[2] = 0x98BADCFE; ctx->state[3] = 0x10325476;
	ctx->state[4] = 0xC3D2E1F0;
	ctx->count = 0;
	memset(ctx->buffer, 0, 64);
}

static void SHA1Update(SHA1Context *ctx, const uint8_t *data, size_t len)
{
	size_t i = 0;
	size_t index = (size_t)(ctx->count & 63);
	ctx->count += len;
	if (index)
	{
		size_t part = 64 - index;
		if (len >= part)
		{
			memcpy(ctx->buffer + index, data, part);
			SHA1Transform(ctx->state, ctx->buffer);
			i = part;
		}
		else
		{
			memcpy(ctx->buffer + index, data, len);
			return;
		}
	}
	for (; i + 63 < len; i += 64)
		SHA1Transform(ctx->state, data + i);
	if (i < len)
		memcpy(ctx->buffer, data + i, len - i);
}

static void SHA1Final(SHA1Context *ctx, uint8_t digest[20])
{
	uint8_t finalcount[8];
	uint64_t bits = ctx->count * 8;
	for (int i = 0; i < 8; i++)
		finalcount[i] = (uint8_t)(bits >> ((7 - i) * 8));

	uint8_t pad = 0x80;
	SHA1Update(ctx, &pad, 1);
	pad = 0;
	while ((ctx->count & 63) != 56)
		SHA1Update(ctx, &pad, 1);
	SHA1Update(ctx, finalcount, 8);

	for (int i = 0; i < 5; i++)
	{
		digest[i*4]   = (uint8_t)(ctx->state[i] >> 24);
		digest[i*4+1] = (uint8_t)(ctx->state[i] >> 16);
		digest[i*4+2] = (uint8_t)(ctx->state[i] >> 8);
		digest[i*4+3] = (uint8_t)(ctx->state[i]);
	}
}

static const char b64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static std::string Base64Encode(const uint8_t *data, size_t len)
{
	std::string out;
	out.reserve(((len + 2) / 3) * 4);
	for (size_t i = 0; i < len; i += 3)
	{
		unsigned int n = ((unsigned int)data[i]) << 16;
		if (i + 1 < len) n |= ((unsigned int)data[i + 1]) << 8;
		if (i + 2 < len) n |= ((unsigned int)data[i + 2]);
		out += b64_table[(n >> 18) & 0x3F];
		out += b64_table[(n >> 12) & 0x3F];
		out += (i + 1 < len) ? b64_table[(n >> 6) & 0x3F] : '=';
		out += (i + 2 < len) ? b64_table[n & 0x3F] : '=';
	}
	return out;
}

} // anonymous namespace

using namespace std;
using namespace nlohmann;

CMCPBridgeClient::CMCPBridgeClient(const string &host, int port, const string &path)
: host(host), port(port), path(path)
{
	state = MCP_BRIDGE_DISCONNECTED;
	socketFd = -1;
	reconnectAttempts = 0;
	remoteProtocolVersion = 0;
	remoteDiscoveryVersion = 0;
	isRunning = false;
	stateCallback = nullptr;
}

void CMCPBridgeClient::SetStateCallback(MCPBridgeStateCallback cb)
{
	stateCallback = cb;
}

void CMCPBridgeClient::SetState(MCPBridgeState newState)
{
	MCPBridgeState oldState = state.load();
	if (oldState == newState) return;
	state = newState;
	if (stateCallback)
		stateCallback(oldState, newState);
}

CMCPBridgeClient::~CMCPBridgeClient()
{
	Stop();
	// Thread is done now — safe to clean up socket
	Disconnect();
}

void CMCPBridgeClient::Start()
{
	// Note: SYS_StartThread checks isRunning — we must NOT set it before the call.
	// The thread itself will be considered "running" once SYS_StartThread returns.
	SYS_StartThread(this);
}

void CMCPBridgeClient::Stop()
{
	if (!isRunning)
		return;

	isRunning = false;

	// Wait for the background thread to finish before touching any state.
	// The thread itself calls Disconnect() in its cleanup, so we must NOT
	// call Disconnect() here — that would race with the thread's own call
	// and potentially lock/destroy the mutex while the thread still holds it.
	if (thread)
	{
		SDL_WaitThread(thread, NULL);
		thread = NULL;
	}
}

void CMCPBridgeClient::ThreadRun(void *passData)
{
	ThreadSetName("MCPBridge");
	isRunning = true;
	LOGD("CMCPBridgeClient: background reconnect thread started");

	while (isRunning)
	{
		MCPBridgeState currentState = state.load();
		if (currentState == MCP_BRIDGE_DISCONNECTED || currentState == MCP_BRIDGE_STALE)
		{
			lock_guard<mutex> lock(connectMutex);
			// Re-check after acquiring lock
			currentState = state.load();
			if (currentState == MCP_BRIDGE_CONNECTED)
			{
				SYS_Sleep(2000);
				continue;
			}

			if (TryConnectInternal())
			{
				lock_guard<mutex> slock(socketMutex);
				if (RunDiscovery())
				{
					SetState(MCP_BRIDGE_CONNECTED);
					reconnectAttempts = 0;
					LOGM("CMCPBridgeClient: connected and discovery complete");
				}
				else
				{
					WebSocketClose();
					SetState(MCP_BRIDGE_STALE);
				}
			}
			else
			{
				reconnectAttempts++;
				int backoff = std::min(kInitialReconnectMs * (1 << std::min(reconnectAttempts, 5)), kMaxReconnectBackoffMs);
				SYS_Sleep(backoff);
				continue;
			}
		}
		else if (currentState == MCP_BRIDGE_CONNECTED)
		{
			// Poll platform state every 2s — use try_lock so we never block
			// a concurrent RunEndpointFunction (tool call) waiting on the mutex.
			// If a tool call is in flight, just skip this poll cycle.
			unique_lock<mutex> slock(socketMutex, std::try_to_lock);
			if (slock.owns_lock())
				PollPlatformState();
		}

		SYS_Sleep(2000);
	}

	Disconnect();
	LOGD("CMCPBridgeClient: background thread stopped");
}

MCPBridgeState CMCPBridgeClient::GetState()
{
	return state.load();
}

string CMCPBridgeClient::GetStateString()
{
	switch (state.load())
	{
		case MCP_BRIDGE_DISCONNECTED: return "disconnected";
		case MCP_BRIDGE_CONNECTING: return "connecting";
		case MCP_BRIDGE_CONNECTED: return "connected";
		case MCP_BRIDGE_STALE: return "stale";
	}
	return "unknown";
}

bool CMCPBridgeClient::EnsureConnected()
{
	if (state.load() == MCP_BRIDGE_CONNECTED)
		return true;

	// Request-triggered reconnect with bounded timeout
	lock_guard<mutex> clock(connectMutex);
	if (state.load() == MCP_BRIDGE_CONNECTED)
		return true;

	LOGD("CMCPBridgeClient::EnsureConnected: attempting request-triggered reconnect");
	if (TryConnectInternal())
	{
		lock_guard<mutex> slock(socketMutex);
		if (RunDiscovery())
		{
			SetState(MCP_BRIDGE_CONNECTED);
			reconnectAttempts = 0;
			return true;
		}
		WebSocketClose();
		SetState(MCP_BRIDGE_STALE);
	}

	return false;
}

bool CMCPBridgeClient::TryConnect()
{
	lock_guard<mutex> lock(connectMutex);
	return TryConnectInternal();
}

bool CMCPBridgeClient::TryConnectInternal()
{
	// Caller must hold connectMutex
	if (state.load() == MCP_BRIDGE_CONNECTED)
		return true;

	SetState(MCP_BRIDGE_CONNECTING);

	if (WebSocketConnect())
	{
		return true;
	}

	SetState(MCP_BRIDGE_DISCONNECTED);
	return false;
}

void CMCPBridgeClient::Disconnect()
{
	lock_guard<mutex> slock(socketMutex);
	WebSocketClose();
	MCPBridgeState prev = state.load();
	if (prev == MCP_BRIDGE_CONNECTED)
		SetState(MCP_BRIDGE_STALE);
	else
		SetState(MCP_BRIDGE_DISCONNECTED);
}

bool CMCPBridgeClient::RunDiscovery()
{
	LOGD("CMCPBridgeClient::RunDiscovery");

	// server/hello
	{
		vector<char> *resp = WebSocketSendRequest("server/hello", json::object(), nullptr, 0);
		if (!resp)
		{
			lastError = "server/hello failed";
			return false;
		}
		string raw(resp->data(), resp->size());
		delete resp;

		auto nullPos = raw.find('\0');
		if (nullPos != string::npos) raw = raw.substr(0, nullPos);

		try
		{
			json j = json::parse(raw);
			if (j.value("status", 0) != 200)
			{
				lastError = "server/hello returned non-200";
				return false;
			}
			remoteProtocolVersion = j["result"].value("protocolVersion", 0);
			remoteServerName = j["result"].value("serverName", "");
			remoteServerVersion = j["result"].value("serverVersion", "");

			if (remoteProtocolVersion < DEBUGGER_PROTOCOL_V2)
			{
				lastError = "incompatible protocol version: " + to_string(remoteProtocolVersion);
				return false;
			}
		}
		catch (const exception &e)
		{
			lastError = string("server/hello parse error: ") + e.what();
			return false;
		}
	}

	// server/capabilities
	{
		vector<char> *resp = WebSocketSendRequest("server/capabilities", json::object(), nullptr, 0);
		if (!resp)
		{
			lastError = "server/capabilities failed";
			return false;
		}
		string raw(resp->data(), resp->size());
		delete resp;

		auto nullPos = raw.find('\0');
		if (nullPos != string::npos) raw = raw.substr(0, nullPos);

		try
		{
			json j = json::parse(raw);
			if (j.value("status", 0) != 200)
			{
				lastError = "server/capabilities returned non-200";
				return false;
			}
			remoteDiscoveryVersion = j["result"].value("discoveryVersion", 0);
			remoteFeatures = j["result"].value("features", json::array());
			remotePlatforms = j["result"].value("platforms", json::array());

			if (remoteDiscoveryVersion < DEBUGGER_DISCOVERY_VERSION)
			{
				lastError = "incompatible discovery version: " + to_string(remoteDiscoveryVersion);
				return false;
			}

			// Validate required features
			bool hasPlatforms = false, hasEndpoints = false, hasBinary = false;
			for (const auto &f : remoteFeatures)
			{
				string fs = f.get<string>();
				if (fs == "platforms") hasPlatforms = true;
				if (fs == "endpointDescriptors") hasEndpoints = true;
				if (fs == "binaryFraming") hasBinary = true;
			}
			if (!hasPlatforms || !hasEndpoints || !hasBinary)
			{
				lastError = "missing required features";
				return false;
			}
		}
		catch (const exception &e)
		{
			lastError = string("server/capabilities parse error: ") + e.what();
			return false;
		}
	}

	// server/platforms
	{
		vector<char> *resp = WebSocketSendRequest("server/platforms", json::object(), nullptr, 0);
		if (!resp)
		{
			lastError = "server/platforms failed";
			return false;
		}
		string raw(resp->data(), resp->size());
		delete resp;

		auto nullPos = raw.find('\0');
		if (nullPos != string::npos) raw = raw.substr(0, nullPos);

		try
		{
			json j = json::parse(raw);
			lock_guard<mutex> plock(platformsMutex);
			remotePlatforms = j["result"].value("platforms", json::array());
		}
		catch (const exception &e)
		{
			lastError = string("server/platforms parse error: ") + e.what();
			return false;
		}
	}

	// server/endpoints
	{
		vector<char> *resp = WebSocketSendRequest("server/endpoints", json::object(), nullptr, 0);
		if (!resp)
		{
			lastError = "server/endpoints failed";
			return false;
		}
		string raw(resp->data(), resp->size());
		delete resp;

		auto nullPos = raw.find('\0');
		if (nullPos != string::npos) raw = raw.substr(0, nullPos);

		try
		{
			json j = json::parse(raw);
			remoteEndpoints.clear();
			if (j.contains("result") && j["result"].contains("endpoints"))
			{
				for (const auto &ep : j["result"]["endpoints"])
				{
					EndpointDescriptor desc;
					desc.fn = ep.value("fn", "");
					desc.platform = ep.value("platform", "");
					desc.category = ep.value("category", "");
					desc.description = ep.value("description", "");
					desc.supportsBinaryInput = ep.value("supportsBinaryInput", false);
					desc.supportsBinaryOutput = ep.value("supportsBinaryOutput", false);
					desc.isStubbed = ep.value("isStubbed", false);
					remoteEndpoints.push_back(desc);
				}
			}
		}
		catch (const exception &e)
		{
			lastError = string("server/endpoints parse error: ") + e.what();
			return false;
		}
	}

	LOGD("CMCPBridgeClient::RunDiscovery: success — %d endpoints, %d platforms",
		 (int)remoteEndpoints.size(), (int)remotePlatforms.size());
	return true;
}

bool CMCPBridgeClient::PollPlatformState()
{
	// Caller must hold socketMutex
	if (state.load() != MCP_BRIDGE_CONNECTED)
		return false;

	vector<char> *resp = WebSocketSendRequest("server/platforms", json::object(), nullptr, 0);
	if (!resp)
	{
		WebSocketClose();
		SetState(MCP_BRIDGE_STALE);
		return true; // changed (disconnected)
	}

	string raw(resp->data(), resp->size());
	delete resp;

	auto nullPos = raw.find('\0');
	if (nullPos != string::npos) raw = raw.substr(0, nullPos);

	try
	{
		json j = json::parse(raw);
		json newPlatforms = j["result"].value("platforms", json::array());
		lock_guard<mutex> plock(platformsMutex);
		if (newPlatforms != remotePlatforms)
		{
			remotePlatforms = newPlatforms;
			return true;
		}
	}
	catch (const exception &)
	{
		WebSocketClose();
		SetState(MCP_BRIDGE_STALE);
		return true;
	}

	return false;
}

json CMCPBridgeClient::GetDiagnostics()
{
	json diag;
	diag["state"] = GetStateString();
	diag["host"] = host;
	diag["port"] = port;
	diag["path"] = path;
	diag["reconnectAttempts"] = reconnectAttempts;
	if (!lastError.empty())
		diag["lastError"] = lastError;
	if (state.load() == MCP_BRIDGE_CONNECTED)
	{
		diag["remoteServerName"] = remoteServerName;
		diag["remoteServerVersion"] = remoteServerVersion;
		diag["remoteProtocolVersion"] = remoteProtocolVersion;
		diag["remoteDiscoveryVersion"] = remoteDiscoveryVersion;
		diag["remotePlatforms"] = remotePlatforms;
		diag["remoteEndpointCount"] = (int)remoteEndpoints.size();
	}
	return diag;
}

// --- CDebuggerServer interface ---

void CMCPBridgeClient::AddEndpointFunction(const string &endpointName, function<vector<char> *(const string, const json, u8 *, int)> func)
{
	// No-op for bridge client — endpoints live on the remote server
}

void CMCPBridgeClient::AddEndpointFunction(const EndpointDescriptor &desc, EndpointHandlerV1 handler)
{
	// No-op for bridge client
}

vector<char> *CMCPBridgeClient::RunEndpointFunction(const string &endpointName, const string token, json params, u8 *binaryData, int binaryDataSize)
{
	if (!EnsureConnected())
	{
		json errorJson;
		errorJson["status"] = HTTP_SERVICE_UNAVAILABLE;
		errorJson["error"] = "desktop_unavailable";
		errorJson["message"] = "No RetroDebugger desktop instance is currently attached";
		if (!lastError.empty())
			errorJson["details"] = lastError;
		string s = errorJson.dump();
		vector<char> *result = new vector<char>(s.begin(), s.end());
		return result;
	}

	lock_guard<mutex> lock(socketMutex);
	vector<char> *resp = WebSocketSendRequest(endpointName, params, binaryData, binaryDataSize);
	if (!resp)
	{
		// Connection died mid-request
		WebSocketClose();
		SetState(MCP_BRIDGE_STALE);
		json errorJson;
		errorJson["status"] = HTTP_SERVICE_UNAVAILABLE;
		errorJson["error"] = "transport_disconnected";
		errorJson["message"] = "Connection lost during request";
		string s = errorJson.dump();
		return new vector<char>(s.begin(), s.end());
	}
	return resp;
}

vector<char> *CMCPBridgeClient::PrepareResult(int status, const string token, json resultJson, u8 *binaryData, int binaryDataSize)
{
	json sendJson;
	sendJson["status"] = status;
	if (!token.empty()) sendJson["token"] = token;
	if (!resultJson.empty()) sendJson["result"] = resultJson;

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

vector<char> *CMCPBridgeClient::RunEndpointFunction(const string &endpointName, const RequestContext &ctx, json params, u8 *binaryData, int binaryDataSize)
{
	return RunEndpointFunction(endpointName, ctx.token, params, binaryData, binaryDataSize);
}

vector<char> *CMCPBridgeClient::PrepareResult(int status, const RequestContext &ctx, json resultJson, u8 *binaryData, int binaryDataSize)
{
	return PrepareResult(status, ctx.token, resultJson, binaryData, binaryDataSize);
}

void CMCPBridgeClient::BroadcastEvent(const char *eventName, json j)
{
	// Bridge doesn't broadcast events — they flow from the desktop app
}

bool CMCPBridgeClient::AreClientsConnected()
{
	return state.load() == MCP_BRIDGE_CONNECTED;
}

vector<EndpointDescriptor> CMCPBridgeClient::GetEndpointDescriptors()
{
	return remoteEndpoints;
}

// --- WebSocket low-level ---

string CMCPBridgeClient::GenerateWebSocketKey()
{
	uint8_t bytes[16];
	random_device rd;
	mt19937 gen(rd());
	uniform_int_distribution<int> dist(0, 255);
	for (int i = 0; i < 16; i++)
		bytes[i] = (uint8_t)dist(gen);
	return Base64Encode(bytes, 16);
}

bool CMCPBridgeClient::ValidateWebSocketAccept(const string &key, const string &acceptHeader)
{
	// RFC 6455: SHA-1(key + magic GUID), base64-encoded
	string magic = key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	SHA1Context ctx;
	SHA1Init(&ctx);
	SHA1Update(&ctx, (const uint8_t *)magic.data(), magic.size());
	uint8_t digest[20];
	SHA1Final(&ctx, digest);
	string expected = Base64Encode(digest, 20);
	return expected == acceptHeader;
}

bool CMCPBridgeClient::WebSocketConnect()
{
	LOGD("CMCPBridgeClient::WebSocketConnect to %s:%d%s", host.c_str(), port, path.c_str());

	// Resolve host
	struct addrinfo hints = {}, *res = nullptr;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	string portStr = to_string(port);

	int err = getaddrinfo(host.c_str(), portStr.c_str(), &hints, &res);
	if (err != 0 || !res)
	{
		lastError = string("DNS resolve failed: ") + gai_strerror(err);
		LOGD("CMCPBridgeClient: %s", lastError.c_str());
		return false;
	}

	socketFd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (socketFd < 0)
	{
		lastError = "socket() failed";
		freeaddrinfo(res);
		return false;
	}

	// Set a connect timeout via non-blocking + poll
#ifdef WIN32
	unsigned long nonBlocking = 1;
	ioctlsocket(socketFd, FIONBIO, &nonBlocking);
#else
	int flags = fcntl(socketFd, F_GETFL, 0);
	fcntl(socketFd, F_SETFL, flags | O_NONBLOCK);
#endif

	int connectResult = connect(socketFd, res->ai_addr, res->ai_addrlen);
	freeaddrinfo(res);

#ifdef WIN32
	if (connectResult < 0 && WSAGetLastError() != WSAEWOULDBLOCK)
#else
	if (connectResult < 0 && errno != EINPROGRESS)
#endif
	{
		lastError = "connect() failed";
		CLOSE_SOCKET(socketFd);
		socketFd = -1;
		return false;
	}

	// Wait for connection with timeout
	struct pollfd pfd = {};
	pfd.fd = socketFd;
	pfd.events = POLLOUT;
	int pollResult = POLL_FN(&pfd, 1, kRequestReconnectTimeoutMs);
	if (pollResult <= 0)
	{
		lastError = "connect() timed out";
		CLOSE_SOCKET(socketFd);
		socketFd = -1;
		return false;
	}

	// Check for connect error
	int sockErr = 0;
	socklen_t sockErrLen = sizeof(sockErr);
	getsockopt(socketFd, SOL_SOCKET, SO_ERROR, (char *)&sockErr, &sockErrLen);
	if (sockErr != 0)
	{
		lastError = "connect() socket error";
		CLOSE_SOCKET(socketFd);
		socketFd = -1;
		return false;
	}

	// Back to blocking mode for request/response
	// 30s timeout: snapshot save/load on large states can take several seconds
#ifdef WIN32
	nonBlocking = 0;
	ioctlsocket(socketFd, FIONBIO, &nonBlocking);
	DWORD timeout = 30000;
	setsockopt(socketFd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout));
	setsockopt(socketFd, SOL_SOCKET, SO_SNDTIMEO, (const char *)&timeout, sizeof(timeout));
#else
	fcntl(socketFd, F_SETFL, flags);
	struct timeval tv;
	tv.tv_sec = 30;
	tv.tv_usec = 0;
	setsockopt(socketFd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	setsockopt(socketFd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
#endif

	// WebSocket upgrade handshake
	string wsKey = GenerateWebSocketKey();
	ostringstream req;
	req << "GET " << path << " HTTP/1.1\r\n"
		<< "Host: " << host << ":" << port << "\r\n"
		<< "Upgrade: websocket\r\n"
		<< "Connection: Upgrade\r\n"
		<< "Sec-WebSocket-Key: " << wsKey << "\r\n"
		<< "Sec-WebSocket-Version: 13\r\n"
		<< "\r\n";

	string reqStr = req.str();
	ssize_t sent = send(socketFd, reqStr.data(), reqStr.size(), 0);
	if (sent != (ssize_t)reqStr.size())
	{
		lastError = "WebSocket handshake send failed";
		CLOSE_SOCKET(socketFd);
		socketFd = -1;
		return false;
	}

	// Read HTTP response
	char responseBuf[2048];
	string response;
	while (true)
	{
		ssize_t n = recv(socketFd, responseBuf, sizeof(responseBuf), 0);
		if (n <= 0)
		{
			lastError = "WebSocket handshake recv failed";
			CLOSE_SOCKET(socketFd);
			socketFd = -1;
			return false;
		}
		response.append(responseBuf, n);
		if (response.find("\r\n\r\n") != string::npos)
			break;
	}

	// Validate 101 response
	if (response.find("HTTP/1.1 101") == string::npos)
	{
		lastError = "WebSocket handshake: not 101";
		CLOSE_SOCKET(socketFd);
		socketFd = -1;
		return false;
	}

	// Validate Sec-WebSocket-Accept
	string acceptKey;
	auto pos = response.find("Sec-WebSocket-Accept: ");
	if (pos != string::npos)
	{
		auto endPos = response.find("\r\n", pos + 22);
		if (endPos != string::npos)
			acceptKey = response.substr(pos + 22, endPos - pos - 22);
	}

	if (!ValidateWebSocketAccept(wsKey, acceptKey))
	{
		lastError = "WebSocket handshake: invalid accept key";
		CLOSE_SOCKET(socketFd);
		socketFd = -1;
		return false;
	}

	LOGD("CMCPBridgeClient::WebSocketConnect: handshake complete");
	return true;
}

void CMCPBridgeClient::WebSocketClose()
{
	if (socketFd >= 0)
	{
		// Send close frame (opcode 0x8)
		uint8_t closeFrame[6];
		closeFrame[0] = 0x88; // FIN + close
		closeFrame[1] = 0x80; // masked, 0 payload
		// Mask key (4 bytes, can be anything)
		closeFrame[2] = 0; closeFrame[3] = 0; closeFrame[4] = 0; closeFrame[5] = 0;
		send(socketFd, (char*)closeFrame, 6, 0);
		CLOSE_SOCKET(socketFd);
		socketFd = -1;
	}
}

bool CMCPBridgeClient::WebSocketSendFrame(const vector<char> &data)
{
	if (socketFd < 0) return false;

	// Build masked binary frame
	size_t payloadLen = data.size();
	vector<uint8_t> frame;

	// Opcode: 0x2 (binary)
	frame.push_back(0x82); // FIN + binary

	// Payload length + mask bit
	if (payloadLen < 126)
	{
		frame.push_back(0x80 | (uint8_t)payloadLen);
	}
	else if (payloadLen < 65536)
	{
		frame.push_back(0x80 | 126);
		frame.push_back((uint8_t)(payloadLen >> 8));
		frame.push_back((uint8_t)(payloadLen & 0xFF));
	}
	else
	{
		frame.push_back(0x80 | 127);
		for (int i = 7; i >= 0; i--)
			frame.push_back((uint8_t)((payloadLen >> (i * 8)) & 0xFF));
	}

	// Mask key (RFC 6455 requires client-to-server masking)
	uint8_t mask[4] = {0x37, 0x42, 0x19, 0x84};
	frame.push_back(mask[0]);
	frame.push_back(mask[1]);
	frame.push_back(mask[2]);
	frame.push_back(mask[3]);

	// Masked payload
	for (size_t i = 0; i < payloadLen; i++)
		frame.push_back((uint8_t)data[i] ^ mask[i & 3]);

	ssize_t sent = 0;
	size_t total = frame.size();
	while (sent < (ssize_t)total)
	{
		ssize_t n = send(socketFd, (char*)frame.data() + sent, total - sent, 0);
		if (n <= 0) return false;
		sent += n;
	}

	return true;
}

vector<char> *CMCPBridgeClient::WebSocketReadFrame()
{
	if (socketFd < 0) return nullptr;

	// Read frame header (2 bytes)
	uint8_t header[2];
	ssize_t n = recv(socketFd, (char*)header, 2, MSG_WAITALL);
	if (n != 2) return nullptr;

	bool masked = (header[1] & 0x80) != 0;
	uint64_t payloadLen = header[1] & 0x7F;

	if (payloadLen == 126)
	{
		uint8_t ext[2];
		if (recv(socketFd, (char*)ext, 2, MSG_WAITALL) != 2) return nullptr;
		payloadLen = ((uint64_t)ext[0] << 8) | ext[1];
	}
	else if (payloadLen == 127)
	{
		uint8_t ext[8];
		if (recv(socketFd, (char*)ext, 8, MSG_WAITALL) != 8) return nullptr;
		payloadLen = 0;
		for (int i = 0; i < 8; i++)
			payloadLen = (payloadLen << 8) | ext[i];
	}

	uint8_t mask[4] = {};
	if (masked)
	{
		if (recv(socketFd, (char*)mask, 4, MSG_WAITALL) != 4) return nullptr;
	}

	// Read payload
	vector<char> *payload = new vector<char>(payloadLen);
	if (payloadLen > 0)
	{
		ssize_t totalRead = 0;
		while (totalRead < (ssize_t)payloadLen)
		{
			n = recv(socketFd, payload->data() + totalRead, payloadLen - totalRead, 0);
			if (n <= 0)
			{
				delete payload;
				return nullptr;
			}
			totalRead += n;
		}

		if (masked)
		{
			for (size_t i = 0; i < payloadLen; i++)
				(*payload)[i] ^= mask[i & 3];
		}
	}

	// Check for close frame (opcode 0x8)
	uint8_t opcode = header[0] & 0x0F;
	if (opcode == 0x08)
	{
		delete payload;
		return nullptr;
	}

	// Ping — respond with pong
	if (opcode == 0x09)
	{
		// Send pong with same payload
		if (payload && !payload->empty())
		{
			vector<char> pong;
			pong.push_back((char)0x8A); // FIN + pong
			pong.push_back((char)(0x80 | (payload->size() & 0x7F)));
			uint8_t pmask[4] = {0, 0, 0, 0};
			pong.insert(pong.end(), pmask, pmask + 4);
			pong.insert(pong.end(), payload->begin(), payload->end());
			send(socketFd, pong.data(), pong.size(), 0);
		}
		delete payload;
		// Read next real frame
		return WebSocketReadFrame();
	}

	// Skip broadcast event frames — server pushes these asynchronously and they
	// can arrive interleaved with request/response pairs, corrupting the stream.
	// Events have an "event" key; responses have a "status" key.
	if (payload && !payload->empty())
	{
		try
		{
			// Only scan the JSON prefix (strip any trailing binary blob after '\0')
			const char *data = payload->data();
			size_t len = payload->size();
			auto nullPos = (const char *)memchr(data, '\0', len);
			size_t jsonLen = nullPos ? (size_t)(nullPos - data) : len;

			json j = json::parse(data, data + jsonLen);
			if (j.contains("event"))
			{
				string eventName = j.value("event", "?");
				LOGD("CMCPBridgeClient: forwarding broadcast event: %s", eventName.c_str());
				if (mcpServer)
				{
					json notifParams;
					notifParams["level"] = "info";
					notifParams["logger"] = "retrodebugger";
					notifParams["data"] = j.dump();
					mcpServer->SendNotification("notifications/message", notifParams);
				}
				delete payload;
				return WebSocketReadFrame();
			}
		}
		catch (...) {}
	}

	return payload;
}

vector<char> *CMCPBridgeClient::WebSocketSendRequest(const string &fn, const json &params, u8 *binaryData, int binaryDataSize)
{
	// Build request JSON matching the debugger WebSocket protocol
	json request;
	request["fn"] = fn;
	request["params"] = params;
	request["protocolVersion"] = DEBUGGER_PROTOCOL_CURRENT;

	string jsonStr = request.dump();

	vector<char> frameData;
	frameData.insert(frameData.end(), jsonStr.begin(), jsonStr.end());

	if (binaryData && binaryDataSize > 0)
	{
		frameData.push_back('\0');
		frameData.insert(frameData.end(), binaryData, binaryData + binaryDataSize);
	}

	if (!WebSocketSendFrame(frameData))
		return nullptr;

	return WebSocketReadFrame();
}
