#include "CDebuggerServer.h"

CDebuggerServer::CDebuggerServer()
{
	endpointRegistryReady.store(false, std::memory_order_release);
}

void CDebuggerServer::Start()
{
}

void CDebuggerServer::Stop()
{
}

void CDebuggerServer::AddEndpointFunction(const std::string& endpointName, std::function<std::vector<char> *(const std::string, const nlohmann::json, u8 *, int)> func)
{
}

void CDebuggerServer::AddEndpointFunction(const EndpointDescriptor &desc, EndpointHandlerV1 handler)
{
}

std::vector<char> *CDebuggerServer::RunEndpointFunction(const std::string& endpointName, const std::string token, nlohmann::json params, u8 *binaryData, int binaryDataSize)
{
	return NULL;
}

std::vector<char> *CDebuggerServer::PrepareResult(int status, const std::string token, nlohmann::json resultJson, u8 *binaryData, int binaryDataSize)
{
	return NULL;
}

std::vector<char> *CDebuggerServer::RunEndpointFunction(const std::string& endpointName, const RequestContext &ctx, nlohmann::json params, u8 *binaryData, int binaryDataSize)
{
	// Default: delegate to v1 path using token from context
	return RunEndpointFunction(endpointName, ctx.token, params, binaryData, binaryDataSize);
}

std::vector<char> *CDebuggerServer::PrepareResult(int status, const RequestContext &ctx, nlohmann::json resultJson, u8 *binaryData, int binaryDataSize)
{
	return NULL;
}

void CDebuggerServer::ThreadRun(void *passData)
{
}

void CDebuggerServer::BroadcastEvent(const char *eventName, nlohmann::json j)
{
}

bool CDebuggerServer::AreClientsConnected()
{
	return false;
}

std::vector<EndpointDescriptor> CDebuggerServer::GetEndpointDescriptors()
{
	return std::vector<EndpointDescriptor>();
}

void CDebuggerServer::SetEndpointRegistryReady(bool isReady)
{
	endpointRegistryReady.store(isReady, std::memory_order_release);
}

bool CDebuggerServer::IsEndpointRegistryReady()
{
	return endpointRegistryReady.load(std::memory_order_acquire);
}
