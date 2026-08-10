#include "SYS_Main.h"
#include "SYS_FileSystem.h"
#include "CDebuggerServerApi.h"
#include "CDebuggerServer.h"
#include "CDebugInterface.h"
#include "CDebuggerApi.h"
#include "CDebugSymbols.h"
#include "CDebugSymbolsSegment.h"
#include "CDebugBreakpointsAddr.h"
#include "CDebugBreakpointsData.h"
#include "CDebugBreakpointAddr.h"
#include "CDebugSymbolsDataWatch.h"
#include "CDataAdapter.h"
#include "CByteBuffer.h"
#include "DebuggerDefs.h"
#include "CViewC64.h"
#include "CImageData.h"
#include "png.h"

using namespace std;
using namespace nlohmann;

// PNG in-memory write helpers for screen/snapshot endpoint
static void PngWriteCallback(png_structp png_ptr, png_bytep data, png_size_t length)
{
	vector<uint8_t> *buf = (vector<uint8_t> *)png_get_io_ptr(png_ptr);
	buf->insert(buf->end(), data, data + length);
}

static void PngFlushCallback(png_structp png_ptr)
{
	(void)png_ptr;
}

CDebuggerServerApi::CDebuggerServerApi(CDebugInterface *debugInterface)
: debugInterface(debugInterface)
{
	debuggerApi = debugInterface->GetDebuggerApi();
	// debuggerApi may be NULL for backends that don't implement it (e.g. C64U)
}

// Helper: register endpoint with both handler AND descriptor metadata
static void RegisterEndpoint(CDebuggerServer *server, const char *fn, const char *platform,
							 const char *category, const char *description,
							 EndpointHandlerV1 handler,
							 bool binaryIn = false, bool binaryOut = false)
{
	EndpointDescriptor desc;
	desc.fn = fn;
	desc.platform = platform;
	desc.category = category;
	desc.description = description;
	desc.supportsBinaryInput = binaryIn;
	desc.supportsBinaryOutput = binaryOut;
	server->AddEndpointFunction(desc, handler);
}

void CDebuggerServerApi::RegisterEndpoints(CDebuggerServer *server)
{
	LOGD("CDebuggerServerApi::RegisterEndpoints: /%s", debugInterface->GetPlatformNameEndpointString());

	// Skip endpoint registration for backends without a debugger API (e.g. C64U stub)
	if (!debuggerApi)
	{
		LOGD("CDebuggerServerApi::RegisterEndpoints: debuggerApi is NULL for /%s, skipping", debugInterface->GetPlatformNameEndpointString());
		return;
	}

	char *buf = SYS_GetCharBuf();
	const char *plat = debugInterface->GetPlatformNameEndpointString();
	
	sprintf(buf, "%s/reset/hard", plat);
	RegisterEndpoint(server, buf, plat, "control", "Hard reset the machine",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		debuggerApi->ResetMachine(true);
		json ev;
		ev["platform"] = debugInterface->GetPlatformNameEndpointString();
		ev["hard"] = true;
		server->BroadcastEvent("emulation.reset", ev);
		return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
	});

	sprintf(buf, "%s/reset/soft", plat);
	RegisterEndpoint(server, buf, plat, "control", "Soft reset the machine",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		debuggerApi->ResetMachine(false);
		json ev;
		ev["platform"] = debugInterface->GetPlatformNameEndpointString();
		ev["hard"] = false;
		server->BroadcastEvent("emulation.reset", ev);
		return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
	});

	sprintf(buf, "%s/detachEverything", plat);
	RegisterEndpoint(server, buf, plat, "control", "Detach all cartridges, disks, and tapes",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		debuggerApi->DetachEverything();
		return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
	});

	sprintf(buf, "%s/warp/set", plat);
	RegisterEndpoint(server, buf, plat, "control", "Enable or disable warp speed",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		bool warp = params.at("warp").get<bool>();
		debuggerApi->SetWarpSpeed(warp);
		// read the state back so a client can verify its own action in one round trip
		json result;
		result["warp"] = debuggerApi->GetWarpSpeed();
		return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
	});

	sprintf(buf, "%s/warp/get", plat);
	RegisterEndpoint(server, buf, plat, "control", "Get current warp speed state",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		json result;
		result["warp"] = debuggerApi->GetWarpSpeed();
		return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
	});

	sprintf(buf, "%s/pause", plat);
	RegisterEndpoint(server, buf, plat, "control", "Pause emulation",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		debuggerApi->PauseEmulation();
		json ev;
		ev["platform"] = debugInterface->GetPlatformNameEndpointString();
		server->BroadcastEvent("emulation.paused", ev);
		return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
	});

	sprintf(buf, "%s/continue", plat);
	RegisterEndpoint(server, buf, plat, "control", "Continue emulation after pause",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		debuggerApi->UnPauseEmulation();
		json ev;
		ev["platform"] = debugInterface->GetPlatformNameEndpointString();
		server->BroadcastEvent("emulation.continued", ev);
		return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
	});

	// Deterministic replay: run exactly N frames then pause. Blocks until finished,
	// so the reply arriving means the frames have already run (frame = final counter).
	sprintf(buf, "%s/run/frames", plat);
	RegisterEndpoint(server, buf, plat, "control", "Run exactly N frames then pause (blocks until finished; count 1..100000)",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		int count = 1;
		if (params.is_object() && params.contains("count"))
		{
			count = params.at("count").get<int>();
		}
		if (count < 1 || count > 100000)
		{
			return server->PrepareResult(HTTP_NOT_ACCEPTABLE, token, json(), NULL, 0);
		}
		if (debugInterface->GetDebugMode() != DEBUGGER_MODE_PAUSED)
		{
			json errorJson;
			errorJson["error"] = "machine must be paused; current+N would race the emulation thread";
			return server->PrepareResult(HTTP_NOT_ACCEPTABLE, token, errorJson, NULL, 0);
		}
		unsigned int frameStart = debuggerApi->GetEmulationFrameNumber();
		bool completed = debugInterface->RunEmulationForFrames((uint32)count);
		json result;
		result["completed"] = completed;
		result["count"] = count;
		result["frameStart"] = frameStart;
		result["frame"] = debuggerApi->GetEmulationFrameNumber();
		return server->PrepareResult(completed ? HTTP_OK : HTTP_NOT_ACCEPTABLE, token, result, NULL, 0);
	});

	sprintf(buf, "%s/step/cycle", plat);
	RegisterEndpoint(server, buf, plat, "cpu", "Step one CPU cycle",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		debuggerApi->StepOneCycle();
		return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
	});

	sprintf(buf, "%s/step/instruction", plat);
	RegisterEndpoint(server, buf, plat, "cpu", "Step one CPU instruction",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		debuggerApi->StepOverInstruction();
		return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
	});

	sprintf(buf, "%s/step/subroutine", plat);
	RegisterEndpoint(server, buf, plat, "cpu", "Step over subroutine call",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		debuggerApi->StepOverSubroutine();
		return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
	});
	
	sprintf(buf, "%s/cpu/status", plat);
	RegisterEndpoint(server, buf, plat, "cpu", "Get CPU registers and flags",
	[this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
	{
		json cpuStatus = debuggerApi->GetCpuStatusJson();
		return server->PrepareResult(HTTP_OK, token, cpuStatus, NULL, 0);
	});
	
	sprintf(buf, "%s/cpu/makejmp", plat);
	RegisterEndpoint(server, buf, plat, "cpu", "Set program counter to address",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		int address = params.at("address").get<int>();
		debuggerApi->MakeJmp(address);
		return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
	});
	
	sprintf(buf, "%s/cpu/counters/read", plat);
	RegisterEndpoint(server, buf, plat, "cpu", "Read cycle, instruction, and frame counters",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		json counters;
		counters["cycle"] = debuggerApi->GetMainCpuCycleCounter();
		counters["instruction"] = debuggerApi->GetMainCpuInstructionCycleCounter();
		counters["frame"] = debuggerApi->GetEmulationFrameNumber();
		return server->PrepareResult(HTTP_OK, token, counters, NULL, 0);
	});

	sprintf(buf, "%s/cpu/memory/writeBlock", plat);
	RegisterEndpoint(server, buf, plat, "memory", "Write binary block to CPU-visible memory",
	[this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
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
	}, true);
	
	sprintf(buf, "%s/cpu/memory/readBlock", plat);
	RegisterEndpoint(server, buf, plat, "memory", "Read binary block from CPU-visible memory",
	[this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
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
		auto *resp = server->PrepareResult(HTTP_OK, token, json(), resultBinaryData, size);
		delete[] resultBinaryData;
		return resp;
	}, false, true);

	sprintf(buf, "%s/ram/clear", plat);
	RegisterEndpoint(server, buf, plat, "memory", "Clear RAM region to a value",
	[this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
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
	
	sprintf(buf, "%s/ram/writeBlock", plat);
	RegisterEndpoint(server, buf, plat, "memory", "Write binary block to direct RAM",
	[this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
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
	}, true);
	
	sprintf(buf, "%s/ram/readBlock", plat);
	RegisterEndpoint(server, buf, plat, "memory", "Read binary block from direct RAM",
	[this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
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
		auto *resp = server->PrepareResult(HTTP_OK, token, json(), resultBinaryData, size);
		delete[] resultBinaryData;
		return resp;
	}, false, true);
	
	sprintf(buf, "%s/input/key/down", plat);
	RegisterEndpoint(server, buf, plat, "input", "Send key-down event",
	[this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
	{
		if (!params.is_object() || !params.contains("keyCode") || !params.at("keyCode").is_number_integer())
		{
			return server->PrepareResult(HTTP_NOT_ACCEPTABLE, token, json(), NULL, 0);
		}
		int mtKeyCode = params.at("keyCode").get<int>();
		debugInterface->LockIoMutex();
		bool res = debuggerApi->KeyboardDown(mtKeyCode);
		debugInterface->UnlockIoMutex();
		json result;
		result["queued"] = res;
		return server->PrepareResult(res ? HTTP_ACCEPTED : HTTP_NOT_ACCEPTABLE, token, result, NULL, 0);
	});

	sprintf(buf, "%s/input/key/up", plat);
	RegisterEndpoint(server, buf, plat, "input", "Send key-up event",
	[this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
	{
		if (!params.is_object() || !params.contains("keyCode") || !params.at("keyCode").is_number_integer())
		{
			return server->PrepareResult(HTTP_NOT_ACCEPTABLE, token, json(), NULL, 0);
		}
		int mtKeyCode = params.at("keyCode").get<int>();
		debugInterface->LockIoMutex();
		bool res = debuggerApi->KeyboardUp(mtKeyCode);
		debugInterface->UnlockIoMutex();
		json result;
		result["queued"] = res;
		return server->PrepareResult(res ? HTTP_ACCEPTED : HTTP_NOT_ACCEPTABLE, token, result, NULL, 0);
	});

	sprintf(buf, "%s/input/joystick/down", plat);
	RegisterEndpoint(server, buf, plat, "input", "Send joystick axis down event",
	[this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
	{
		u32 axis = CDebuggerApi::JoypadAxisNameToAxisCode(params.at("axis"));
		int port = params.at("port");
		debuggerApi->JoystickDown(port, axis);
		return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
	});

	sprintf(buf, "%s/input/joystick/up", plat);
	RegisterEndpoint(server, buf, plat, "input", "Send joystick axis up event",
	[this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
	{
		u32 axis = CDebuggerApi::JoypadAxisNameToAxisCode(params.at("axis"));
		int port = params.at("port");
		debuggerApi->JoystickUp(port, axis);
		return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
	});

	// breakpoints
	sprintf(buf, "%s/cpu/breakpoint/add", plat);
	RegisterEndpoint(server, buf, plat, "breakpoints", "Add CPU address breakpoint",
	[this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
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

	sprintf(buf, "%s/cpu/breakpoint/remove", plat);
	RegisterEndpoint(server, buf, plat, "breakpoints", "Remove CPU address breakpoint",
	[this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
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

	sprintf(buf, "%s/cpu/memory/breakpoint/add", plat);
	RegisterEndpoint(server, buf, plat, "breakpoints", "Add memory data breakpoint",
	[this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
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

	sprintf(buf, "%s/cpu/memory/breakpoint/remove", plat);
	RegisterEndpoint(server, buf, plat, "breakpoints", "Remove memory data breakpoint",
	[this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
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
	sprintf(buf, "%s/segment/read", plat);
	RegisterEndpoint(server, buf, plat, "segment", "Get current debug segment name",
	[this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
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

	sprintf(buf, "%s/segment/write", plat);
	RegisterEndpoint(server, buf, plat, "segment", "Set current debug segment by name",
	[this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
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

	
	// Breakpoint listing
	sprintf(buf, "%s/cpu/breakpoint/list", plat);
	RegisterEndpoint(server, buf, plat, "breakpoints", "List all CPU address breakpoints",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		json result;
		json bpList = json::array();

		{
			CDebugInterfaceMutexGuard lock(debugInterface);
			CDebugSymbols *symbols = debugInterface->symbols;
			if (symbols && symbols->currentSegment)
			{
				CDebugBreakpointsAddr *bps = symbols->currentSegment->breakpointsPC;
				if (bps)
				{
					for (auto &pair : bps->breakpoints)
					{
						json bp;
						bp["address"] = pair.first;
						bp["isActive"] = pair.second->isActive;
						bpList.push_back(bp);
					}
				}
			}
		}

		result["breakpoints"] = bpList;
		return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
	});

	sprintf(buf, "%s/cpu/memory/breakpoint/list", plat);
	RegisterEndpoint(server, buf, plat, "breakpoints", "List all memory data breakpoints",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		json result;
		json bpList = json::array();

		{
			CDebugInterfaceMutexGuard lock(debugInterface);
			CDebugSymbols *symbols = debugInterface->symbols;
			if (symbols && symbols->currentSegment)
			{
				CDebugBreakpointsData *bps = symbols->currentSegment->breakpointsData;
				if (bps)
				{
					for (auto &pair : bps->breakpoints)
					{
						json bp;
						bp["address"] = pair.first;
						bp["isActive"] = pair.second->isActive;
						bpList.push_back(bp);
					}
				}
			}
		}

		result["breakpoints"] = bpList;
		return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
	});

	// Machine state
	sprintf(buf, "%s/state", plat);
	RegisterEndpoint(server, buf, plat, "state", "Get machine state including run/pause status",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		json result;

		{
			CDebugInterfaceMutexGuard lock(debugInterface);
			result["platform"] = debugInterface->GetPlatformNameEndpointString();
			result["isRunning"] = debugInterface->isRunning;
			result["isPaused"] = debugInterface->GetDebugMode() != DEBUGGER_MODE_RUNNING;

			// CPU status
			CDataAdapter *da = debugInterface->GetDataAdapter();
			if (da)
			{
				result["cpuPC"] = debugInterface->GetCpuPC();
				result["dataAdapterSize"] = da->AdapterGetDataLength();
			}
		}

		return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
	});

	// Watch list
	sprintf(buf, "%s/watch/list", plat);
	RegisterEndpoint(server, buf, plat, "watch", "List all data watches",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		json result;
		json watchList = json::array();

		{
			CDebugInterfaceMutexGuard lock(debugInterface);
			CDebugSymbols *symbols = debugInterface->symbols;
			if (symbols && symbols->currentSegment)
			{
				for (auto &pair : symbols->currentSegment->watches)
				{
					CDebugSymbolsDataWatch *w = pair.second;
					json watch;
					watch["address"] = w->address;
					if (w->watchName)
						watch["name"] = w->watchName;
					watch["representation"] = w->representation;
					watch["numberOfValues"] = w->numberOfValues;
					watchList.push_back(watch);
				}
			}
		}

		result["watches"] = watchList;
		return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
	});

	// Watch add
	sprintf(buf, "%s/watch/add", plat);
	RegisterEndpoint(server, buf, plat, "watch", "Add a data watch at address",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		int address = params.at("address").get<int>();
		string name = params.value("name", "");
		int numValues = params.value("numberOfValues", 1);

		{
			CDebugInterfaceMutexGuard lock(debugInterface);
			CDebugSymbols *symbols = debugInterface->symbols;
			if (symbols && symbols->currentSegment)
			{
				char *nameBuf = NULL;
				if (!name.empty())
				{
					nameBuf = new char[name.size() + 1];
					strcpy(nameBuf, name.c_str());
				}
				symbols->currentSegment->AddWatch(address, nameBuf, 0, numValues);
				if (nameBuf) delete[] nameBuf;
			}
		}

		return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
	});

	// Watch remove
	sprintf(buf, "%s/watch/remove", plat);
	RegisterEndpoint(server, buf, plat, "watch", "Remove a data watch at address",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		int address = params.at("address").get<int>();

		{
			CDebugInterfaceMutexGuard lock(debugInterface);
			CDebugSymbols *symbols = debugInterface->symbols;
			if (symbols && symbols->currentSegment)
			{
				symbols->currentSegment->RemoveWatch(address);
			}
		}

		return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
	});

	// Snapshot save — returns snapshot as binary blob
	sprintf(buf, "%s/snapshot/save", plat);
	RegisterEndpoint(server, buf, plat, "snapshot", "Save emulator snapshot as binary blob",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		{
			CDebugInterfaceMutexGuard lock(debugInterface);

			CByteBuffer *byteBuffer = new CByteBuffer();
			bool success = debugInterface->SaveChipsSnapshotSynced(byteBuffer);

			if (success && byteBuffer->length > 0)
			{
				json result;
				result["size"] = byteBuffer->length;
				vector<char> *resp = server->PrepareResult(HTTP_OK, token, result, byteBuffer->data, byteBuffer->length);
				delete byteBuffer;
				return resp;
			}

			delete byteBuffer;
			return server->PrepareResult(HTTP_INTERNAL_SERVER_ERROR, token, json(), NULL, 0);
		}
	}, false, true);

	// Snapshot load — accepts snapshot binary blob
	sprintf(buf, "%s/snapshot/load", plat);
	RegisterEndpoint(server, buf, plat, "snapshot", "Load emulator snapshot from binary blob",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		if (!binaryData || binaryDataSize <= 0)
		{
			return server->PrepareResult(HTTP_BAD_REQUEST, token, json(), NULL, 0);
		}

		bool success;
		{
			CDebugInterfaceMutexGuard lock(debugInterface);

			CByteBuffer *byteBuffer = new CByteBuffer(binaryData, binaryDataSize);
			success = debugInterface->LoadChipsSnapshotSynced(byteBuffer);
			delete byteBuffer;
		}

		if (success)
			return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
		return server->PrepareResult(HTTP_INTERNAL_SERVER_ERROR, token, json(), NULL, 0);
	}, true);

	// Snapshot save to file — uses SaveChipsSnapshotSynced (synchronous) then writes to file
	sprintf(buf, "%s/snapshot/saveFile", plat);
	RegisterEndpoint(server, buf, plat, "snapshot", "Save emulator snapshot to a file path",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		string path = params.at("path").get<string>();

		CByteBuffer *byteBuffer = new CByteBuffer();
		bool success = debugInterface->SaveChipsSnapshotSynced(byteBuffer);

		if (!success || byteBuffer->length == 0)
		{
			delete byteBuffer;
			return server->PrepareResult(HTTP_INTERNAL_SERVER_ERROR, token, json(), NULL, 0);
		}

		FILE *f = fopen(path.c_str(), "wb");
		if (!f)
		{
			delete byteBuffer;
			return server->PrepareResult(HTTP_INTERNAL_SERVER_ERROR, token, json(), NULL, 0);
		}
		fwrite(byteBuffer->data, 1, byteBuffer->length, f);
		fclose(f);

		json result;
		result["path"] = path;
		result["size"] = byteBuffer->length;
		delete byteBuffer;
		return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
	});

	// Snapshot load from file — reads file then uses LoadChipsSnapshotSynced (synchronous)
	sprintf(buf, "%s/snapshot/loadFile", plat);
	RegisterEndpoint(server, buf, plat, "snapshot", "Load emulator snapshot from a file path",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		string path = params.at("path").get<string>();

		FILE *f = fopen(path.c_str(), "rb");
		if (!f)
			return server->PrepareResult(HTTP_NOT_FOUND, token, json(), NULL, 0);

		fseek(f, 0, SEEK_END);
		long fileSize = ftell(f);
		fseek(f, 0, SEEK_SET);

		if (fileSize <= 0)
		{
			fclose(f);
			return server->PrepareResult(HTTP_BAD_REQUEST, token, json(), NULL, 0);
		}

		u8 *data = new u8[fileSize];
		fread(data, 1, fileSize, f);
		fclose(f);

		// CByteBuffer takes ownership of 'data' and frees it in its destructor
		CByteBuffer *byteBuffer = new CByteBuffer(data, (int)fileSize);
		bool success = debugInterface->LoadChipsSnapshotSynced(byteBuffer);
		delete byteBuffer; // also frees 'data'

		if (success)
		{
			json result;
			result["path"] = path;
			result["size"] = (long)fileSize;
			return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
		}
		return server->PrepareResult(HTTP_INTERNAL_SERVER_ERROR, token, json(), NULL, 0);
	});

	// Quick snapshot save — saves to the standard quick-slot file (slot is 1-indexed, matching RetroDebugger UI)
	sprintf(buf, "%s/snapshot/quickSave", plat);
	RegisterEndpoint(server, buf, plat, "snapshot", "Save quick snapshot to a numbered slot (slot 1-N, matches RetroDebugger UI)",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		int slot = params.at("slot").get<int>();
		if (slot < 1)
			return server->PrepareResult(HTTP_BAD_REQUEST, token, json(), NULL, 0);

		int fileId = slot - 1; // UI is 1-indexed, files are 0-indexed
		const char *ext = nullptr;
		switch (debugInterface->GetEmulatorType())
		{
			case EMULATOR_TYPE_C64_VICE:  ext = "snap"; break;
			case EMULATOR_TYPE_ATARI800:  ext = "a8s";  break;
			case EMULATOR_TYPE_NESTOPIA:  ext = "sav";  break;
			default:
				return server->PrepareResult(HTTP_INTERNAL_SERVER_ERROR, token, json(), NULL, 0);
		}

		string path = gUTFPathToSettings->GetStdStringUTF8();
		char fname[64];
		sprintf(fname, "snapshot-%d.%s", fileId, ext);
		path += fname;

		CByteBuffer *byteBuffer = new CByteBuffer();
		bool success = debugInterface->SaveChipsSnapshotSynced(byteBuffer);

		if (!success || byteBuffer->length == 0)
		{
			delete byteBuffer;
			return server->PrepareResult(HTTP_INTERNAL_SERVER_ERROR, token, json(), NULL, 0);
		}

		FILE *f = fopen(path.c_str(), "wb");
		if (!f)
		{
			delete byteBuffer;
			return server->PrepareResult(HTTP_INTERNAL_SERVER_ERROR, token, json(), NULL, 0);
		}
		fwrite(byteBuffer->data, 1, byteBuffer->length, f);
		fclose(f);

		json result;
		result["slot"] = slot;
		result["path"] = path;
		result["size"] = byteBuffer->length;
		delete byteBuffer;
		return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
	});

	// Quick snapshot load — loads from the standard quick-slot file (slot is 1-indexed, matching RetroDebugger UI)
	sprintf(buf, "%s/snapshot/quickLoad", plat);
	RegisterEndpoint(server, buf, plat, "snapshot", "Load quick snapshot from a numbered slot (slot 1-N, matches RetroDebugger UI)",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		int slot = params.at("slot").get<int>();
		if (slot < 1)
			return server->PrepareResult(HTTP_BAD_REQUEST, token, json(), NULL, 0);

		int fileId = slot - 1;
		const char *ext = nullptr;
		switch (debugInterface->GetEmulatorType())
		{
			case EMULATOR_TYPE_C64_VICE:  ext = "snap"; break;
			case EMULATOR_TYPE_ATARI800:  ext = "a8s";  break;
			case EMULATOR_TYPE_NESTOPIA:  ext = "sav";  break;
			default:
				return server->PrepareResult(HTTP_INTERNAL_SERVER_ERROR, token, json(), NULL, 0);
		}

		string path = gUTFPathToSettings->GetStdStringUTF8();
		char fname[64];
		sprintf(fname, "snapshot-%d.%s", fileId, ext);
		path += fname;

		FILE *f = fopen(path.c_str(), "rb");
		if (!f)
			return server->PrepareResult(HTTP_NOT_FOUND, token, json(), NULL, 0);

		fseek(f, 0, SEEK_END);
		long fileSize = ftell(f);
		fseek(f, 0, SEEK_SET);

		if (fileSize <= 0)
		{
			fclose(f);
			return server->PrepareResult(HTTP_BAD_REQUEST, token, json(), NULL, 0);
		}

		u8 *data = new u8[fileSize];
		fread(data, 1, fileSize, f);
		fclose(f);

		CByteBuffer *byteBuffer = new CByteBuffer(data, (int)fileSize);
		bool success = debugInterface->LoadChipsSnapshotSynced(byteBuffer);
		delete byteBuffer;

		if (success)
		{
			json result;
			result["slot"] = slot;
			result["path"] = path;
			result["size"] = (long)fileSize;
			return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
		}
		return server->PrepareResult(HTTP_INTERNAL_SERVER_ERROR, token, json(), NULL, 0);
	});

	// --- Input injection ---

	// Joystick button/direction press
	sprintf(buf, "%s/input/joystickDown", plat);
	RegisterEndpoint(server, buf, plat, "input", "Press joystick directions/buttons (axes bitmask: N=1 S=2 W=4 E=8 FIRE=16 FIRE2=32 START=64 SELECT=128)",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		int port = params.value("port", 0);
		uint32_t axes = params.at("axes").get<uint32_t>();
		debugInterface->JoystickDown(port, axes);
		json result;
		result["port"] = port;
		result["axes"] = axes;
		return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
	});

	// Joystick button/direction release
	sprintf(buf, "%s/input/joystickUp", plat);
	RegisterEndpoint(server, buf, plat, "input", "Release joystick directions/buttons (axes bitmask: N=1 S=2 W=4 E=8 FIRE=16 FIRE2=32 START=64 SELECT=128)",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		int port = params.value("port", 0);
		uint32_t axes = params.at("axes").get<uint32_t>();
		debugInterface->JoystickUp(port, axes);
		json result;
		result["port"] = port;
		result["axes"] = axes;
		return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
	});

	// Read current joystick state
	sprintf(buf, "%s/input/joystickState", plat);
	RegisterEndpoint(server, buf, plat, "input", "Read current joystick state bitmask for a port",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		int port = params.value("port", 0);
		uint32_t state = debugInterface->GetJoystickState(port);
		json result;
		result["port"] = port;
		result["state"] = state;
		return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
	});

	// Keyboard key press
	sprintf(buf, "%s/input/keyDown", plat);
	RegisterEndpoint(server, buf, plat, "input", "Press a keyboard key (keyCode: MTKEY/SDL keycode; ASCII for printable chars)",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		if (!params.is_object() || !params.contains("keyCode") || !params.at("keyCode").is_number_integer())
		{
			return server->PrepareResult(HTTP_NOT_ACCEPTABLE, token, json(), NULL, 0);
		}
		uint32_t keyCode = params.at("keyCode").get<uint32_t>();
		debugInterface->LockIoMutex();
		bool queued = debugInterface->KeyboardDown(keyCode);
		debugInterface->UnlockIoMutex();
		json result;
		result["keyCode"] = keyCode;
		result["queued"] = queued;
		return server->PrepareResult(queued ? HTTP_ACCEPTED : HTTP_NOT_ACCEPTABLE, token, result, NULL, 0);
	});

	// Keyboard key release
	sprintf(buf, "%s/input/keyUp", plat);
	RegisterEndpoint(server, buf, plat, "input", "Release a keyboard key (keyCode: MTKEY/SDL keycode; ASCII for printable chars)",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		if (!params.is_object() || !params.contains("keyCode") || !params.at("keyCode").is_number_integer())
		{
			return server->PrepareResult(HTTP_NOT_ACCEPTABLE, token, json(), NULL, 0);
		}
		uint32_t keyCode = params.at("keyCode").get<uint32_t>();
		debugInterface->LockIoMutex();
		bool queued = debugInterface->KeyboardUp(keyCode);
		debugInterface->UnlockIoMutex();
		json result;
		result["keyCode"] = keyCode;
		result["queued"] = queued;
		return server->PrepareResult(queued ? HTTP_ACCEPTED : HTTP_NOT_ACCEPTABLE, token, result, NULL, 0);
	});

	// Emulator start — starts the emulation thread for this platform
	sprintf(buf, "%s/emulator/start", plat);
	RegisterEndpoint(server, buf, plat, "emulator", "Start the emulation thread for this platform",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		if (debugInterface->isRunning)
		{
			json result;
			result["status"] = "already_running";
			return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
		}
		viewC64->StartEmulationThreadSafe(debugInterface);
		json result;
		result["status"] = "started";
		return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
	});

	// Emulator stop — stops the emulation thread for this platform
	sprintf(buf, "%s/emulator/stop", plat);
	RegisterEndpoint(server, buf, plat, "emulator", "Stop the emulation thread for this platform",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		if (!debugInterface->isRunning)
		{
			json result;
			result["status"] = "already_stopped";
			return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
		}
		viewC64->StopEmulationThreadSafe(debugInterface);
		json result;
		result["status"] = "stopped";
		return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
	});

	// --- MCP analysis tools ---

	sprintf(buf, "%s/cpu/disassemble", plat);
	RegisterEndpoint(server, buf, plat, "analysis", "Disassemble memory as 6502 instructions",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		int address = params.value("address", 0);
		int count = params.value("count", 64);
		bool includeBytes = params.value("includeBytes", true);
		bool includeLabels = params.value("includeLabels", true);

		if (count > 512) count = 512;
		if (count < 1) count = 1;

		json result;
		{
			CDebugInterfaceMutexGuard lock(debugInterface);
			result = debuggerApi->DisassembleMemory(address, count, includeBytes, includeLabels);
		}
		return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
	});

	sprintf(buf, "%s/cpu/assemble", plat);
	RegisterEndpoint(server, buf, plat, "analysis", "Assemble 6502 instructions and write to memory",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		int address = params.at("address").get<int>();
		string code = params.at("code").get<string>();

		json result;
		{
			CDebugInterfaceMutexGuard lock(debugInterface);
			result = debuggerApi->AssembleCode(address, code);
		}

		if (result.contains("error"))
			return server->PrepareResult(HTTP_BAD_REQUEST, token, result, NULL, 0);
		return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
	});

	sprintf(buf, "%s/cpu/codemap", plat);
	RegisterEndpoint(server, buf, plat, "analysis", "Get memory regions identified as executable code",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		int startAddress = params.value("startAddress", 0);
		int endAddress = params.value("endAddress", 65535);

		json result;
		{
			CDebugInterfaceMutexGuard lock(debugInterface);
			result = debuggerApi->GetCodeMap(startAddress, endAddress);
		}
		return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
	});

	sprintf(buf, "%s/memory/search", plat);
	RegisterEndpoint(server, buf, plat, "memory", "Search RAM for a specific byte value",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		const auto &vj = params.at("value");
		int value = vj.is_string() ? std::stoi(vj.get<string>()) : vj.get<int>();
		int startAddress = params.value("startAddress", 0);
		int endAddress = params.value("endAddress", 65535);

		CDataAdapter *dataAdapter = debuggerApi->GetDataAdapterMemoryWithIO();
		int adapterLen = dataAdapter->AdapterGetDataLength();
		if (endAddress >= adapterLen) endAddress = adapterLen - 1;

		json matches = json::array();
		for (int addr = startAddress; addr <= endAddress; addr++)
		{
			u8 b = 0;
			dataAdapter->AdapterReadByte(addr, &b);
			if (b == (u8)value)
				matches.push_back(addr);
		}

		json result;
		result["value"] = value;
		result["startAddress"] = startAddress;
		result["endAddress"] = endAddress;
		result["matches"] = matches;
		result["count"] = (int)matches.size();
		return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
	});

	sprintf(buf, "%s/cpu/search", plat);
	RegisterEndpoint(server, buf, plat, "analysis", "Search memory for opcode patterns",
	[this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		string pattern = params.at("pattern").get<string>();
		int startAddress = params.value("startAddress", 0);
		int endAddress = params.value("endAddress", 65535);
		bool executedOnly = params.value("executedOnly", true);

		json result;
		{
			CDebugInterfaceMutexGuard lock(debugInterface);
			result = debuggerApi->SearchOpcodePattern(pattern, startAddress, endAddress, executedOnly);
		}

		if (result.contains("error"))
			return server->PrepareResult(HTTP_BAD_REQUEST, token, result, NULL, 0);
		return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
	});

	// Helper lambda: get screen pixels cropped to actual content size, encode as PNG.
	// Uses debuggerApi->GetScreenImage() which handles cropping per platform.
	auto encodeScreenPng = [this](int &outW, int &outH, string &errorText) -> vector<uint8_t>
	{
		errorText = "";
		CImageData *img = debuggerApi->GetScreenImage(&outW, &outH);
		if (!img || !img->resultData || outW <= 0 || outH <= 0)
		{
			// not an encoder problem: the emulator has no frame to hand out
			errorText = "Screen image not available";
			LOGError("encodeScreenPng: screen image not available (img=%s data=%s w=%d h=%d)",
					 img ? "ok" : "NULL", (img && img->resultData) ? "ok" : "NULL", outW, outH);
			return {};
		}

		uint8_t *pixels = img->resultData;

		vector<uint8_t> pngData;
		png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
		if (!png_ptr)
		{
			// most likely cause: png.h version does not match the linked libpng
			errorText = "PNG encoder init failed (libpng version mismatch?): header "
						PNG_LIBPNG_VER_STRING ", library ";
			errorText += png_get_libpng_ver(NULL);
			LOGError("encodeScreenPng: png_create_write_struct failed, header=%s library=%s",
					 PNG_LIBPNG_VER_STRING, png_get_libpng_ver(NULL));
			return {};
		}

		png_infop info_ptr = png_create_info_struct(png_ptr);
		if (!info_ptr)
		{
			errorText = "PNG info struct allocation failed";
			LOGError("encodeScreenPng: png_create_info_struct failed");
			png_destroy_write_struct(&png_ptr, NULL);
			return {};
		}

		if (setjmp(png_jmpbuf(png_ptr)))
		{
			errorText = "PNG encoding failed";
			LOGError("encodeScreenPng: libpng longjmp during encoding");
			png_destroy_write_struct(&png_ptr, &info_ptr);
			return {};
		}

		png_set_write_fn(png_ptr, &pngData, PngWriteCallback, PngFlushCallback);
		png_set_IHDR(png_ptr, info_ptr, outW, outH, 8, PNG_COLOR_TYPE_RGB_ALPHA,
					 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
		png_write_info(png_ptr, info_ptr);

		vector<png_bytep> rowPtrs(outH);
		for (int y = 0; y < outH; y++)
			rowPtrs[y] = pixels + y * outW * 4;
		png_write_image(png_ptr, rowPtrs.data());
		png_write_end(png_ptr, NULL);
		png_destroy_write_struct(&png_ptr, &info_ptr);

		return pngData;
	};

	sprintf(buf, "%s/screen/snapshot", plat);
	RegisterEndpoint(server, buf, plat, "screen", "Capture current emulator screen as PNG (binary payload)",
	[this, server, encodeScreenPng](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		int w = 0, h = 0;
		string errorText;
		vector<uint8_t> pngData = encodeScreenPng(w, h, errorText);
		if (pngData.empty())
		{
			json errorJson;
			errorJson["error"] = errorText;
			return server->PrepareResult(HTTP_INTERNAL_SERVER_ERROR, token, errorJson, NULL, 0);
		}
		json result;
		result["width"] = w;
		result["height"] = h;
		result["format"] = "png";
		return server->PrepareResult(HTTP_OK, token, result, pngData.data(), (int)pngData.size());
	});

	sprintf(buf, "%s/screen/save", plat);
	RegisterEndpoint(server, buf, plat, "screen", "Save current emulator screen as PNG file to a given path",
	[this, server, encodeScreenPng](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		string path = params.at("path").get<string>();

		int w = 0, h = 0;
		string errorText;
		vector<uint8_t> pngData = encodeScreenPng(w, h, errorText);
		if (pngData.empty())
		{
			json errorJson;
			errorJson["error"] = errorText;
			return server->PrepareResult(HTTP_INTERNAL_SERVER_ERROR, token, errorJson, NULL, 0);
		}

		FILE *fp = fopen(path.c_str(), "wb");
		if (!fp)
		{
			json errorJson;
			errorJson["error"] = "Cannot open file for writing: " + path;
			return server->PrepareResult(HTTP_INTERNAL_SERVER_ERROR, token, errorJson, NULL, 0);
		}
		fwrite(pngData.data(), 1, pngData.size(), fp);
		fclose(fp);

		json result;
		result["path"] = path;
		result["width"] = w;
		result["height"] = h;
		result["format"] = "png";
		result["byteCount"] = (int)pngData.size();
		return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
	});

	SYS_ReleaseCharBuf(buf);
}
