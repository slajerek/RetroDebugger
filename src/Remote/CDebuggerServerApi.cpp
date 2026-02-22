#include "SYS_Main.h"
#include "CDebuggerServerApi.h"
#include "CDebuggerServer.h"
#include "CDebugInterface.h"
#include "CDebuggerApi.h"
#include "CDebugSymbolsSegment.h"

using namespace std;
using namespace nlohmann;

CDebuggerServerApi::CDebuggerServerApi(CDebugInterface *debugInterface)
: debugInterface(debugInterface)
{
	debuggerApi = debugInterface->GetDebuggerApi();
}

void CDebuggerServerApi::RegisterEndpoints(CDebuggerServer *server)
{
	LOGD("CDebuggerServerApi::RegisterEndpoints: /%s", debugInterface->GetPlatformNameEndpointString());
	char *buf = SYS_GetCharBuf();
	
	sprintf(buf, "%s/reset/hard", debugInterface->GetPlatformNameEndpointString());
	server->AddEndpointFunction(buf, [this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		debuggerApi->ResetMachine(true);
		return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
	});

	sprintf(buf, "%s/reset/soft", debugInterface->GetPlatformNameEndpointString());
	server->AddEndpointFunction(buf, [this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		debuggerApi->ResetMachine(false);
		return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
	});

	sprintf(buf, "%s/detachEverything", debugInterface->GetPlatformNameEndpointString());
	server->AddEndpointFunction(buf, [this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		debuggerApi->DetachEverything();
		return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
	});

	sprintf(buf, "%s/warp/set", debugInterface->GetPlatformNameEndpointString());
	server->AddEndpointFunction(buf, [this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		bool warp = params.at("warp").get<bool>();
		debuggerApi->SetWarpSpeed(warp);
		return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
	});

	sprintf(buf, "%s/pause", debugInterface->GetPlatformNameEndpointString());
	server->AddEndpointFunction(buf, [this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		debuggerApi->PauseEmulation();
		return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
	});

	sprintf(buf, "%s/continue", debugInterface->GetPlatformNameEndpointString());
	server->AddEndpointFunction(buf, [this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		debuggerApi->UnPauseEmulation();
		return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
	});

	sprintf(buf, "%s/step/cycle", debugInterface->GetPlatformNameEndpointString());
	server->AddEndpointFunction(buf, [this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		debuggerApi->StepOneCycle();
		return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
	});

	sprintf(buf, "%s/step/instruction", debugInterface->GetPlatformNameEndpointString());
	server->AddEndpointFunction(buf, [this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		debuggerApi->StepOverInstruction();
		return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
	});

	sprintf(buf, "%s/step/subroutine", debugInterface->GetPlatformNameEndpointString());
	server->AddEndpointFunction(buf, [this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		debuggerApi->StepOverSubroutine();
		return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
	});
	
	sprintf(buf, "%s/cpu/status", debugInterface->GetPlatformNameEndpointString());
	server->AddEndpointFunction(buf, [this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>* 
	{
		json cpuStatus = debuggerApi->GetCpuStatusJson();
		return server->PrepareResult(HTTP_OK, token, cpuStatus, NULL, 0);
	});
	
	sprintf(buf, "%s/cpu/makejmp", debugInterface->GetPlatformNameEndpointString());
	server->AddEndpointFunction(buf, [this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		int address = params.at("address").get<int>();
		debuggerApi->MakeJmp(address);
		return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
	});
	
	sprintf(buf, "%s/cpu/counters/read", debugInterface->GetPlatformNameEndpointString());
	server->AddEndpointFunction(buf, [this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		json counters;
		counters["cycle"] = debuggerApi->GetMainCpuCycleCounter();
		counters["instruction"] = debuggerApi->GetMainCpuInstructionCycleCounter();
		counters["frame"] = debuggerApi->GetEmulationFrameNumber();
		return server->PrepareResult(HTTP_OK, token, counters, NULL, 0);
	});

	sprintf(buf, "%s/cpu/memory/writeBlock", debugInterface->GetPlatformNameEndpointString());
	server->AddEndpointFunction(buf, [this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
	{
		int address = params.at("address").get<int>();
//		LOGD("cpu/memory/writeBlock: address=%d size=%d", address, binaryDataSize);
//		LOG_PrintHexArray(binaryData, binaryDataSize);

		CDataAdapter *dataAdapter = debuggerApi->GetDataAdapterMemoryWithIO();
		if (address + binaryDataSize > dataAdapter->AdapterGetDataLength())
		{
			return server->PrepareResult(HTTP_PAYLOAD_TOO_LARGE, token, json(), NULL, 0);
		}
		
		for (int i = 0; i < binaryDataSize; i++)
		{
			dataAdapter->AdapterWriteByte(address++, binaryData[i]);
		}

		return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
	});
	
	sprintf(buf, "%s/cpu/memory/readBlock", debugInterface->GetPlatformNameEndpointString());
	server->AddEndpointFunction(buf, [this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
	{
		int address = params.at("address").get<int>();
		int size = params.at("size").get<int>();
//		LOGD("cpu/memory/readBlock: address=%d size=%d", address, size);
		
		CDataAdapter *dataAdapter = debuggerApi->GetDataAdapterMemoryWithIO();
		if (address + size > dataAdapter->AdapterGetDataLength())
		{
			return server->PrepareResult(HTTP_PAYLOAD_TOO_LARGE, token, json(), NULL, 0);
		}

		u8 *resultBinaryData = new u8[size];

		for (int i = 0; i < size; i++)
		{
			dataAdapter->AdapterReadByte(address++, &resultBinaryData[i]);
		}
		return server->PrepareResult(HTTP_OK, token, json(), resultBinaryData, size);
	});
	
	sprintf(buf, "%s/ram/clear", debugInterface->GetPlatformNameEndpointString());
	server->AddEndpointFunction(buf, [this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
	{
		int address = params.at("address").get<int>();
		int size = params.at("size").get<int>();
		int value = 0;
		if (params.contains("value"))
		{
			value = params.at("value").get<int>();
		}
		
		CDataAdapter *dataAdapter = debuggerApi->GetDataAdapterMemoryDirectRAM();
		if (address + size > dataAdapter->AdapterGetDataLength())
		{
			return server->PrepareResult(HTTP_PAYLOAD_TOO_LARGE, token, json(), NULL, 0);
		}

		for (int i = 0; i < size; i++)
		{
			dataAdapter->AdapterWriteByte(address++, value);
		}
		return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
	});
	
	sprintf(buf, "%s/ram/writeBlock", debugInterface->GetPlatformNameEndpointString());
	server->AddEndpointFunction(buf, [this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
	{
		int address = params.at("address").get<int>();
//		LOGD("%s/ram/writeBlock: address=%d size=%d", debugInterface->GetPlatformNameEndpointString(), address, binaryDataSize);
//		LOG_PrintHexArray(binaryData, binaryDataSize);

		CDataAdapter *dataAdapter = debuggerApi->GetDataAdapterMemoryDirectRAM();
		if (address + binaryDataSize > dataAdapter->AdapterGetDataLength())
		{
			return server->PrepareResult(HTTP_PAYLOAD_TOO_LARGE, token, json(), NULL, 0);
		}
		
		for (int i = 0; i < binaryDataSize; i++)
		{
			dataAdapter->AdapterWriteByte(address++, binaryData[i]);
		}

		return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
	});
	
	sprintf(buf, "%s/ram/readBlock", debugInterface->GetPlatformNameEndpointString());
	server->AddEndpointFunction(buf, [this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
	{
		int address = params.at("address").get<int>();
		int size = params.at("size").get<int>();
//		LOGD("readBlock: address=%d size=%d", address, size);
		
		CDataAdapter *dataAdapter = debuggerApi->GetDataAdapterMemoryDirectRAM();
		if (address + size > dataAdapter->AdapterGetDataLength())
		{
			return server->PrepareResult(HTTP_PAYLOAD_TOO_LARGE, token, json(), NULL, 0);
		}

		u8 *resultBinaryData = new u8[size];

		for (int i = 0; i < size; i++)
		{
			dataAdapter->AdapterReadByte(address++, &resultBinaryData[i]);
		}
		return server->PrepareResult(HTTP_OK, token, json(), resultBinaryData, size);
	});
	
	sprintf(buf, "%s/input/key/down", debugInterface->GetPlatformNameEndpointString());
	server->AddEndpointFunction(buf, [this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
	{
		int mtKeyCode = params.at("keyCode").get<int>();
		bool res = debuggerApi->KeyboardDown(mtKeyCode);
		return server->PrepareResult(res ? HTTP_OK : HTTP_NOT_ACCEPTABLE, token, json(), NULL, 0);
	});

	sprintf(buf, "%s/input/key/up", debugInterface->GetPlatformNameEndpointString());
	server->AddEndpointFunction(buf, [this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
	{
		int mtKeyCode = params.at("keyCode").get<int>();
		bool res = debuggerApi->KeyboardUp(mtKeyCode);
		return server->PrepareResult(res ? HTTP_OK : HTTP_NOT_ACCEPTABLE, token, json(), NULL, 0);
	});

	sprintf(buf, "%s/input/joystick/down", debugInterface->GetPlatformNameEndpointString());
	server->AddEndpointFunction(buf, [this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
	{
		u32 axis = CDebuggerApi::JoypadAxisNameToAxisCode(params.at("axis"));
		int port = params.at("port");
		debuggerApi->JoystickDown(port, axis);
		return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
	});

	sprintf(buf, "%s/input/joystick/up", debugInterface->GetPlatformNameEndpointString());
	server->AddEndpointFunction(buf, [this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
	{
		u32 axis = CDebuggerApi::JoypadAxisNameToAxisCode(params.at("axis"));
		int port = params.at("port");
		debuggerApi->JoystickUp(port, axis);
		return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
	});

	// breakpoints
	sprintf(buf, "%s/cpu/breakpoint/add", debugInterface->GetPlatformNameEndpointString());
	server->AddEndpointFunction(buf, [this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
	{
		u64 addr = FUN_JsonValueDecOrHexStrWithPrefixToU64(params.at("addr"));
		json j;

		u64 breakpointId = debuggerApi->AddBreakpointPC((int)addr);
		if (breakpointId != UNKNOWN_BREAKPOINT_ID)
		{
			j["breakpointId"] = breakpointId;
			char *strSegment = debugInterface->symbols->currentSegment->name->GetStdASCII();
			j["segment"] = strSegment;
			delete [] strSegment;
			return server->PrepareResult(HTTP_OK, token, j, NULL, 0);
		}
		
		return server->PrepareResult(HTTP_NOT_ACCEPTABLE, token, j, NULL, 0);
	});

	sprintf(buf, "%s/cpu/breakpoint/remove", debugInterface->GetPlatformNameEndpointString());
	server->AddEndpointFunction(buf, [this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
	{
		u64 addr = FUN_JsonValueDecOrHexStrWithPrefixToU64(params.at("addr"));
		json j;

		u64 breakpointId = debuggerApi->RemoveBreakpointPC((int)addr);
		if (breakpointId != UNKNOWN_BREAKPOINT_ID)
		{
			j["breakpointId"] = breakpointId;
			char *strSegment = debugInterface->symbols->currentSegment->name->GetStdASCII();
			j["segment"] = strSegment;
			delete [] strSegment;
			return server->PrepareResult(HTTP_OK, token, j, NULL, 0);
		}
		
		return server->PrepareResult(HTTP_NOT_ACCEPTABLE, token, j, NULL, 0);
	});

	sprintf(buf, "%s/cpu/memory/breakpoint/add", debugInterface->GetPlatformNameEndpointString());
	server->AddEndpointFunction(buf, [this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
	{
		u64 addr = FUN_JsonValueDecOrHexStrWithPrefixToU64(params.at("addr"));
		u64 value = FUN_JsonValueDecOrHexStrWithPrefixToU64(params.at("value"));
		
		json j;

		u32 memoryAccess = MEMORY_BREAKPOINT_ACCESS_WRITE;
		if (params.contains("access"))
		{
			if (params.at("access") == "read")
			{
				memoryAccess = MEMORY_BREAKPOINT_ACCESS_READ;
			}
			else if (params.at("access") == "write")
			{
				memoryAccess = MEMORY_BREAKPOINT_ACCESS_WRITE;
			}
			else return server->PrepareResult(HTTP_BAD_REQUEST, token, j, NULL, 0);
		}
		
		DataBreakpointComparison comparison = DataBreakpointComparison::MEMORY_BREAKPOINT_EQUAL;
		if (params.contains("comparison"))
		{
			string comparisonStr = params.at("comparison");
			comparison = CDebugBreakpointsData::StrToDataBreakpointComparison(comparisonStr.c_str());
		}
		
		u64 breakpointId = debuggerApi->AddBreakpointMemory((int)addr, memoryAccess, comparison, (int)value);
		if (breakpointId != UNKNOWN_BREAKPOINT_ID)
		{
			j["breakpointId"] = breakpointId;
			char *strSegment = debugInterface->symbols->currentSegment->name->GetStdASCII();
			j["segment"] = strSegment;
			delete [] strSegment;
			return server->PrepareResult(HTTP_OK, token, j, NULL, 0);
		}
		
		return server->PrepareResult(HTTP_NOT_ACCEPTABLE, token, j, NULL, 0);
	});

	sprintf(buf, "%s/cpu/memory/breakpoint/remove", debugInterface->GetPlatformNameEndpointString());
	server->AddEndpointFunction(buf, [this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
	{
		u64 addr = FUN_JsonValueDecOrHexStrWithPrefixToU64(params.at("addr"));
		json j;

		u64 breakpointId = debuggerApi->RemoveBreakpointMemory((int)addr);
		if (breakpointId != UNKNOWN_BREAKPOINT_ID)
		{
			j["breakpointId"] = breakpointId;
			char *strSegment = debugInterface->symbols->currentSegment->name->GetStdASCII();
			j["segment"] = strSegment;
			delete [] strSegment;
			return server->PrepareResult(HTTP_OK, token, j, NULL, 0);
		}
		
		return server->PrepareResult(HTTP_NOT_ACCEPTABLE, token, j, NULL, 0);
	});
	
	// set/get segment
	sprintf(buf, "%s/segment/read", debugInterface->GetPlatformNameEndpointString());
	server->AddEndpointFunction(buf, [this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
	{
		json j;
		CSlrString *segmentNameStr = debuggerApi->GetCurrentSegmentName();
		if (segmentNameStr == NULL)
		{
			j["segment"] = "";
		}
		else
		{
			const char *name = segmentNameStr->GetStdASCII();
			j["segment"] = name;
			delete [] name;
		}
		
		return server->PrepareResult(HTTP_OK, token, j, NULL, 0);
	});

	sprintf(buf, "%s/segment/write", debugInterface->GetPlatformNameEndpointString());
	server->AddEndpointFunction(buf, [this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
	{
		string segmentName = params["segment"];
		CSlrString *segmentNameStr = new CSlrString(segmentName.c_str());
		bool ret = debuggerApi->SetCurrentSegment(segmentNameStr);
		delete segmentNameStr;
		
		if (ret)
		{
			return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
		}
		else
		{
			return server->PrepareResult(HTTP_NOT_FOUND, token, json(), NULL, 0);
		}
		
	});

	
	// TODO: store/restore snapshot (binary blob)
	
	// Symbols:
	// DONE? add breakpoint	+ breakpoint callback
	// TODO: Breakpoint VSync callback
	// void CDebugInterface::DoFrame()	 -- is raster line ok? (no, because it's only for c64)

	// TODO: add watch
	// TODO: get current source file path / line / column
	
	SYS_ReleaseCharBuf(buf);
}
