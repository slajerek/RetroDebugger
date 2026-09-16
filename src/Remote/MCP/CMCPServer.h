#ifndef _CMCPServer_h_
#define _CMCPServer_h_

#include "SYS_Defs.h"
#include "SYS_Threading.h"
#include "json.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <mutex>
#include <set>
#include <atomic>

class CDebuggerServer;
class CMCPBridgeClient;

// MCP tool handler: takes params JSON, returns result JSON
typedef std::function<nlohmann::json(const nlohmann::json &params)> MCPToolHandler;

struct MCPToolDescriptor
{
	std::string name;
	std::string description;
	std::string platform;           // "" = always visible, "c64"/"atari800"/"nes" = only when that platform is running
	nlohmann::json inputSchema;     // JSON Schema for tool params
	MCPToolHandler handler;
};

struct MCPResourceDescriptor
{
	std::string uri;
	std::string name;
	std::string description;
	std::string mimeType;
	std::function<std::string()> readHandler;
};

struct MCPPromptDescriptor
{
	std::string name;
	std::string description;
	nlohmann::json arguments;       // prompt argument definitions
	std::function<nlohmann::json(const nlohmann::json &args)> getHandler;
};

class CMCPServer : public CSlrThread
{
public:
	CMCPServer();
	virtual ~CMCPServer();

	void Start();
	void Stop();

	virtual void ThreadRun(void *passData);

	// Registration
	void RegisterTool(const MCPToolDescriptor &tool);
	void RegisterResource(const MCPResourceDescriptor &resource);
	void RegisterPrompt(const MCPPromptDescriptor &prompt);

	// Register all default tools/resources from the debugger server
	void RegisterDebuggerTools(CDebuggerServer *server);
	void RegisterStaticResources();
	void RegisterPrompts();
	void RegisterBridgeLocalTools();

	// The debugger server is published before its endpoint registry is ready.
	// MCP callers may only use the pointer returned by GetReadyDebuggerServer().
	void SetDebuggerServer(CDebuggerServer *server);
	CDebuggerServer *GetReadyDebuggerServer();

	// Bridge mode
	void SetBridgeMode(CMCPBridgeClient *bridge);
	void OnBridgeStateChanged(int oldState, int newState);
	void ClearDebuggerTools();
	CMCPBridgeClient *bridgeClient;
	bool isBridgeMode;
	bool debuggerToolsRegistered;

	bool isRunning;

	// MCP method dispatch (public for testing)
	nlohmann::json HandleRequest(const nlohmann::json &request);

	// Notifications
	void SendNotification(const std::string &method, const nlohmann::json &params = nlohmann::json::object());
	void CheckPlatformStateChanges();

private:
	// JSON-RPC over stdio
	std::string ReadMessage();
	void WriteMessage(const nlohmann::json &msg);
	std::mutex writeMutex;  // protects stdout writes
	nlohmann::json HandleInitialize(const nlohmann::json &params);
	nlohmann::json HandleToolsList();
	nlohmann::json HandleToolsCall(const nlohmann::json &params);
	void EnsureToolsRegistered();
	nlohmann::json HandleResourcesList();
	nlohmann::json HandleResourcesRead(const nlohmann::json &params);
	nlohmann::json HandlePromptsList();
	nlohmann::json HandlePromptsGet(const nlohmann::json &params);

	// Error helpers
	nlohmann::json MakeError(int code, const std::string &message);
	nlohmann::json MakeResult(const nlohmann::json &result);

	// Registries — toolsMutex guards tools (written by bridge thread, read by MCP thread)
	std::mutex toolsMutex;
	std::vector<MCPToolDescriptor> tools;
	std::vector<MCPResourceDescriptor> resources;
	std::vector<MCPPromptDescriptor> prompts;

	bool initialized;
	bool shouldStop;
	bool toolsRegistered;
	std::atomic<CDebuggerServer *> debuggerServer;

	// Platform state tracking for change detection
	std::set<std::string> lastActivePlatforms;
};

// Global MCP server instance
extern CMCPServer *mcpServer;

void MCP_ServerStart();
void MCP_ServerStop();
void MCP_BridgeStart(const char *host, int port, const char *path);

#endif
