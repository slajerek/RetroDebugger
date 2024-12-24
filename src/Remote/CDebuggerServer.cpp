#include "CDebuggerServer.h"

CDebuggerServer::CDebuggerServer()
{
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

std::vector<char> *CDebuggerServer::RunEndpointFunction(const std::string& endpointName, const std::string token, nlohmann::json params, u8 *binaryData, int binaryDataSize)
{
	return NULL;
}

std::vector<char> *CDebuggerServer::PrepareResult(int status, const std::string token, nlohmann::json resultJson, u8 *binaryData, int binaryDataSize)
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
