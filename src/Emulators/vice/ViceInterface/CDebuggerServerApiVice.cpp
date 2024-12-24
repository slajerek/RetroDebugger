#include "CDebuggerServerApiVice.h"
#include "CDebugInterfaceVice.h"
#include "CDebuggerApiVice.h"
#include "CDebuggerServer.h"

using namespace std;
using namespace nlohmann;

CDebuggerServerApiVice::CDebuggerServerApiVice(CDebugInterface *debugInterface)
: CDebuggerServerApi(debugInterface)
{
	this->debugInterfaceVice = (CDebugInterfaceVice *)debugInterface;
	this->debuggerApiVice = (CDebuggerApiVice *)debugInterface->GetDebuggerApi();
	this->sidData = new CSidData();
}

void CDebuggerServerApiVice::RegisterEndpoints(CDebuggerServer *server)
{
	CDebuggerServerApi::RegisterEndpoints(server);

	char *buf = SYS_GetCharBuf();
	
	// save PRG
	sprintf(buf, "%s/savePrg", debugInterface->GetPlatformNameEndpointString());
	server->AddEndpointFunction(buf, [this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		debugInterfaceVice->LockMutex();
		
		string fileName = params.at("path").get<string>();
		u16 fromAddr = FUN_JsonValueDecOrHexStrWithPrefixToU64(params.at("fromAddr"));
		u16 toAddr = FUN_JsonValueDecOrHexStrWithPrefixToU64(params.at("toAddr"));
		bool exomize = false;
		if (params.contains("exomize"))
		{
			exomize = params.at("exomize").get<bool>();
		}

		bool ret;
		if (exomize)
		{
			u16 jmpAddr = FUN_JsonValueDecOrHexStrWithPrefixToU64(params.at("jmpAddr"));
			ret = debuggerApiVice->SaveExomizerPRG(fromAddr, toAddr, jmpAddr, fileName.c_str());
		}
		else
		{
			ret = debuggerApiVice->SavePRG(fromAddr, toAddr, fileName.c_str());
		}
		
		debugInterfaceVice->UnlockMutex();
		if (ret)
		{
			return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
		}
		return server->PrepareResult(HTTP_FORBIDDEN, token, json(), NULL, 0);
	});
	
	// VIC
	sprintf(buf, "%s/vic/write", debugInterface->GetPlatformNameEndpointString());
	server->AddEndpointFunction(buf, [this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		debugInterfaceVice->LockMutex();
		for (auto& [key, value] : params["registers"].items())
		{
			u64 registerNum = FUN_DecOrHexStrWithPrefixToU64(key.c_str());
			if (registerNum >= 0xD000 && registerNum < 0xD040)
			{
				registerNum -= 0xD000;
			}
			if (registerNum > 0x3F)
			{
				debugInterfaceVice->UnlockMutex();
				return server->PrepareResult(HTTP_NOT_ACCEPTABLE, token, json(), NULL, 0);
			}
			u64 registerValue = FUN_JsonValueDecOrHexStrWithPrefixToU64(value);
			debuggerApiVice->SetVicRegister(registerNum, registerValue);
		}
		debugInterfaceVice->UnlockMutex();
		
		return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
	});

	sprintf(buf, "%s/vic/read", debugInterface->GetPlatformNameEndpointString());
	server->AddEndpointFunction(buf, [this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		debugInterfaceVice->LockMutex();
		
		std::unordered_map<u64, u8> registers;
		for (const auto& reg : params["registers"])
		{
			u64 registerNum = FUN_JsonValueDecOrHexStrWithPrefixToU64(reg);
			if (registerNum >= 0xD000 && registerNum < 0xD040)
			{
				registerNum -= 0xD000;
			}
			if (registerNum > 0x3F)
			{
				debugInterfaceVice->UnlockMutex();
				return server->PrepareResult(HTTP_NOT_ACCEPTABLE, token, json(), NULL, 0);
			}

			u8 registerValue = debugInterfaceVice->GetVicRegister(registerNum);
			registers[registerNum] = registerValue;
		}
		
		json j;
		j["registers"] = registers;

		debugInterfaceVice->UnlockMutex();
		return server->PrepareResult(HTTP_OK, token, j, NULL, 0);
	});
	
	sprintf(buf, "%s/vic/breakpoint/add", debugInterface->GetPlatformNameEndpointString());
	server->AddEndpointFunction(buf, [this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
	{
		u64 rasterLine = FUN_JsonValueDecOrHexStrWithPrefixToU64(params.at("rasterLine"));
		json j;

		u64 breakpointId = debuggerApiVice->AddBreakpointRasterLine((int)rasterLine);
		if (breakpointId != UNKNOWN_BREAKPOINT_ID)
		{
			j["breakpointId"] = breakpointId;
			return server->PrepareResult(HTTP_OK, token, j, NULL, 0);
		}
		
		return server->PrepareResult(HTTP_NOT_ACCEPTABLE, token, j, NULL, 0);
	});

	sprintf(buf, "%s/vic/breakpoint/remove", debugInterface->GetPlatformNameEndpointString());
	server->AddEndpointFunction(buf, [this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
	{
		u64 rasterLine = FUN_JsonValueDecOrHexStrWithPrefixToU64(params.at("rasterLine"));
		json j;

		u64 breakpointId = debuggerApiVice->RemoveBreakpointRasterLine((int)rasterLine);
		if (breakpointId != UNKNOWN_BREAKPOINT_ID)
		{
			j["breakpointId"] = breakpointId;
			return server->PrepareResult(HTTP_OK, token, j, NULL, 0);
		}
		
		return server->PrepareResult(HTTP_NOT_ACCEPTABLE, token, j, NULL, 0);
	});
	
	// CIA
	sprintf(buf, "%s/cia/write", debugInterface->GetPlatformNameEndpointString());
	server->AddEndpointFunction(buf, [this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		debugInterfaceVice->LockMutex();
		
		int selectedCiaNum = 0;
		if (params.contains("num"))
		{
			selectedCiaNum = params.at("num").get<int>();
			if (selectedCiaNum < 0 || selectedCiaNum > 1)
			{
				debugInterfaceVice->UnlockMutex();
				return server->PrepareResult(HTTP_NOT_ACCEPTABLE, token, json(), NULL, 0);
			}
		}
		
		for (auto& [key, value] : params["registers"].items())
		{
			int ciaNum = selectedCiaNum;
			u64 registerNum = FUN_DecOrHexStrWithPrefixToU64(key.c_str());
			if (registerNum >= 0xDC00 && registerNum < 0xDC10)
			{
				registerNum -= 0xDC00;
				ciaNum = 0;
			}
			if (registerNum >= 0xDD00 && registerNum < 0xDD10)
			{
				registerNum -= 0xDD00;
				ciaNum = 1;
			}
			if (registerNum > 0x0F)
			{
				debugInterfaceVice->UnlockMutex();
				return server->PrepareResult(HTTP_NOT_ACCEPTABLE, token, json(), NULL, 0);
			}
			u64 registerValue = FUN_JsonValueDecOrHexStrWithPrefixToU64(value);
			debuggerApiVice->SetCiaRegister(ciaNum, registerNum, registerValue);
		}
		debugInterfaceVice->UnlockMutex();
		
		return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
	});

	sprintf(buf, "%s/cia/read", debugInterface->GetPlatformNameEndpointString());
	server->AddEndpointFunction(buf, [this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		debugInterfaceVice->LockMutex();
		
		int selectedCiaNum = 0;
		if (params.contains("num"))
		{
			selectedCiaNum = params.at("num").get<int>();
			if (selectedCiaNum < 0 || selectedCiaNum > 1)
			{
				debugInterfaceVice->UnlockMutex();
				return server->PrepareResult(HTTP_NOT_ACCEPTABLE, token, json(), NULL, 0);
			}
		}

		std::unordered_map<u64, u8> registers;
		for (const auto& reg : params["registers"])
		{
			int ciaNum = selectedCiaNum;
			u64 registerNum = FUN_JsonValueDecOrHexStrWithPrefixToU64(reg);
			if (registerNum >= 0xDC00 && registerNum < 0xDC10)
			{
				registerNum -= 0xDC00;
				ciaNum = 0;
			}
			if (registerNum >= 0xDD00 && registerNum < 0xDD10)
			{
				registerNum -= 0xDD00;
				ciaNum = 1;
			}
			if (registerNum > 0x0F)
			{
				debugInterfaceVice->UnlockMutex();
				return server->PrepareResult(HTTP_NOT_ACCEPTABLE, token, json(), NULL, 0);
			}

			u8 registerValue = debuggerApiVice->GetCiaRegister(ciaNum, registerNum);
			registers[registerNum] = registerValue;
		}
		
		json j;
		j["registers"] = registers;

		debugInterfaceVice->UnlockMutex();
		return server->PrepareResult(HTTP_OK, token, j, NULL, 0);
	});
	
	// SID
	sprintf(buf, "%s/sid/write", debugInterface->GetPlatformNameEndpointString());
	server->AddEndpointFunction(buf, [this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		debugInterfaceVice->LockMutex();
		
		// Note: we need to burst-write to SID registers at once to avoid side-effects
		// make all regs to not set SID
		for (int sidNum = 0; sidNum < SOUND_SIDS_MAX; sidNum++)
		{
			for (int reg = 0; reg < C64_NUM_SID_REGISTERS; reg++)
			{
				sidData->shouldSetSidReg[sidNum][reg] = false;
			}
		}

		for (auto& [key, jsonSidData] : params["sids"].items())
		{
			int sidNum = jsonSidData.at("num").get<int>();
			if (sidNum < 0 || sidNum > SOUND_SIDS_MAX)
			{
				debugInterfaceVice->UnlockMutex();
				return server->PrepareResult(HTTP_NOT_ACCEPTABLE, token, json(), NULL, 0);
			}

			for (auto& [key, value] : jsonSidData["registers"].items())
			{
				u64 registerNum = FUN_DecOrHexStrWithPrefixToU64(key.c_str());
				// TODO: subtract selected sid# address  --> debugInterfaceVice->GetSidStereoAddress(sidNum)
//				if (registerNum >= 0xD400 && registerNum < 0xDFFF)
//				{
//					registerNum -= 0xD400;
//				}
				
				u64 registerValue = FUN_JsonValueDecOrHexStrWithPrefixToU64(value);
				sidData->sidRegs[sidNum][registerNum] = registerValue;
				sidData->shouldSetSidReg[sidNum][registerNum] = true;
			}
		}
		
		// write to SIDs at once (i.e. now when paused, or in next cycle when running)
		debuggerApiVice->SetSid(sidData);

		debugInterfaceVice->UnlockMutex();
		
		return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
	});

	sprintf(buf, "%s/sid/read", debugInterface->GetPlatformNameEndpointString());
	server->AddEndpointFunction(buf, [this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		debugInterfaceVice->LockMutex();
		
		int sidNum = 0;
		if (params.contains("num"))
		{
			sidNum = params.at("num").get<int>();
			if (sidNum < 0 || sidNum > SOUND_SIDS_MAX)
			{
				debugInterfaceVice->UnlockMutex();
				return server->PrepareResult(HTTP_NOT_ACCEPTABLE, token, json(), NULL, 0);
			}
		}

		std::unordered_map<u64, u8> registers;
		for (const auto& reg : params["registers"])
		{
			u64 registerNum = FUN_JsonValueDecOrHexStrWithPrefixToU64(reg);
			// TODO: subtract selected sid# address  --> debugInterfaceVice->GetSidStereoAddress(sidNum)
//			if (registerNum >= 0xD400 && registerNum < 0xDFFF)
//			{
//				registerNum -= 0xD400;
//			}
//			if (registerNum > 0x0F)
//			{
//				return server->PrepareResult(HTTP_NOT_ACCEPTABLE, token, json(), NULL, 0);
//			}

			u8 registerValue = debuggerApiVice->GetSidRegister(sidNum, registerNum);
			registers[registerNum] = registerValue;
		}
		
		json j;
		j["registers"] = registers;

		debugInterfaceVice->UnlockMutex();
		return server->PrepareResult(HTTP_OK, token, j, NULL, 0);
	});
	
	//
	sprintf(buf, "%s/drive1541/cpu/memory/writeBlock", debugInterface->GetPlatformNameEndpointString());
	server->AddEndpointFunction(buf, [this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
	{
		int address = params.at("address").get<int>();
//		LOGD("cpu/memory/writeBlock: address=%d size=%d", address, binaryDataSize);
//		LOG_PrintHexArray(binaryData, binaryDataSize);

		CDataAdapter *dataAdapter = debuggerApiVice->GetDataAdapterDrive1541MemoryWithIO();
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
	
	sprintf(buf, "%s/drive1541/cpu/memory/readBlock", debugInterface->GetPlatformNameEndpointString());
	server->AddEndpointFunction(buf, [this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
	{
		int address = params.at("address").get<int>();
		int size = params.at("size").get<int>();
//		LOGD("cpu/memory/readBlock: address=%d size=%d", address, size);
		
		CDataAdapter *dataAdapter = debuggerApiVice->GetDataAdapterDrive1541MemoryWithIO();
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
	
	sprintf(buf, "%s/drive1541/ram/clear", debugInterface->GetPlatformNameEndpointString());
	server->AddEndpointFunction(buf, [this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
	{
		int address = params.at("address").get<int>();
		int size = params.at("size").get<int>();
		int value = 0;
		if (params.contains("value"))
		{
			value = params.at("value").get<int>();
		}
		
		CDataAdapter *dataAdapter = debuggerApiVice->GetDataAdapterDrive1541MemoryDirectRAM();
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
	
	sprintf(buf, "%s/drive1541/ram/writeBlock", debugInterface->GetPlatformNameEndpointString());
	server->AddEndpointFunction(buf, [this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
	{
		int address = params.at("address").get<int>();
//		LOGD("%s/ram/writeBlock: address=%d size=%d", debugInterface->GetPlatformNameEndpointString(), address, binaryDataSize);
//		LOG_PrintHexArray(binaryData, binaryDataSize);

		CDataAdapter *dataAdapter = debuggerApiVice->GetDataAdapterDrive1541MemoryDirectRAM();
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
	
	sprintf(buf, "%s/drive1541/ram/readBlock", debugInterface->GetPlatformNameEndpointString());
	server->AddEndpointFunction(buf, [this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
	{
		int address = params.at("address").get<int>();
		int size = params.at("size").get<int>();
//		LOGD("readBlock: address=%d size=%d", address, size);
		
		CDataAdapter *dataAdapter = debuggerApiVice->GetDataAdapterDrive1541MemoryDirectRAM();
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
	
	// Drive1541 VIA
	sprintf(buf, "%s/drive1541/via/write", debugInterface->GetPlatformNameEndpointString());
	server->AddEndpointFunction(buf, [this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		debugInterfaceVice->LockMutex();
		
		int selectedViaNum = 0;
		if (params.contains("num"))
		{
			selectedViaNum = params.at("num").get<int>();
			if (selectedViaNum < 0 || selectedViaNum > 1)
			{
				debugInterfaceVice->UnlockMutex();
				return server->PrepareResult(HTTP_NOT_ACCEPTABLE, token, json(), NULL, 0);
			}
		}

		int selectedDriveNum = 0;
		if (params.contains("drive"))
		{
			selectedDriveNum = params.at("drive").get<int>();
			if (selectedDriveNum < 0 || selectedDriveNum > MAX_DRIVE_NUM)
			{
				debugInterfaceVice->UnlockMutex();
				return server->PrepareResult(HTTP_NOT_ACCEPTABLE, token, json(), NULL, 0);
			}
		}

		for (auto& [key, value] : params["registers"].items())
		{
			int viaNum = selectedViaNum;
			u64 registerNum = FUN_DecOrHexStrWithPrefixToU64(key.c_str());
			if (registerNum >= 0x1800 && registerNum < 0x1810)
			{
				registerNum -= 0x1800;
				viaNum = 0;
			}
			if (registerNum >= 0x1C00 && registerNum < 0x1C10)
			{
				registerNum -= 0x1C00;
				viaNum = 1;
			}
			if (registerNum > 0x0F)
			{
				debugInterfaceVice->UnlockMutex();
				return server->PrepareResult(HTTP_NOT_ACCEPTABLE, token, json(), NULL, 0);
			}
			u64 registerValue = FUN_JsonValueDecOrHexStrWithPrefixToU64(value);
			debuggerApiVice->SetDrive1541ViaRegister(selectedDriveNum, viaNum, registerNum, registerValue);
		}
		debugInterfaceVice->UnlockMutex();
		
		return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
	});

	sprintf(buf, "%s/drive1541/via/read", debugInterface->GetPlatformNameEndpointString());
	server->AddEndpointFunction(buf, [this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		debugInterfaceVice->LockMutex();

		int selectedViaNum = 0;
		if (params.contains("num"))
		{
			selectedViaNum = params.at("num").get<int>();
			if (selectedViaNum < 0 || selectedViaNum > 1)
			{
				debugInterfaceVice->UnlockMutex();
				return server->PrepareResult(HTTP_NOT_ACCEPTABLE, token, json(), NULL, 0);
			}
		}

		int selectedDriveNum = 0;
		if (params.contains("drive"))
		{
			selectedDriveNum = params.at("drive").get<int>();
			if (selectedDriveNum < 0 || selectedDriveNum > MAX_DRIVE_NUM)
			{
				debugInterfaceVice->UnlockMutex();
				return server->PrepareResult(HTTP_NOT_ACCEPTABLE, token, json(), NULL, 0);
			}
		}

		std::unordered_map<u64, u8> registers;
		for (const auto& reg : params["registers"])
		{
			int viaNum = selectedViaNum;
			u64 registerNum = FUN_JsonValueDecOrHexStrWithPrefixToU64(reg);
			if (registerNum >= 0x1800 && registerNum < 0x1810)
			{
				registerNum -= 0x1800;
				viaNum = 0;
			}
			if (registerNum >= 0x1C00 && registerNum < 0x1C10)
			{
				registerNum -= 0x1C00;
				viaNum = 1;
			}
			if (registerNum > 0x0F)
			{
				debugInterfaceVice->UnlockMutex();
				return server->PrepareResult(HTTP_NOT_ACCEPTABLE, token, json(), NULL, 0);
			}

			u8 registerValue = debuggerApiVice->GetDrive1541ViaRegister(selectedDriveNum, viaNum, registerNum);
			registers[registerNum] = registerValue;
		}
		
		json j;
		j["registers"] = registers;

		debugInterfaceVice->UnlockMutex();
		return server->PrepareResult(HTTP_OK, token, j, NULL, 0);
	});

	SYS_ReleaseCharBuf(buf);
}
