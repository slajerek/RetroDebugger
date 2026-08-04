#include "CDebuggerServerApiVice.h"
#include "CDebugInterfaceVice.h"
#include "CDebugInterfaceC64.h"
#include "CDebuggerApiVice.h"
#include "CDebuggerServer.h"
#include "CDebugSymbolsDrive1541.h"
#include "CDebugSymbolsSegment.h"
#include "CDebugBreakpointsAddr.h"
#include "CDebugBreakpointAddr.h"

extern "C" {
	unsigned long c64d_get_vice_drivecpu_clk(int driveNum);
}

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
	const char *plat = debugInterface->GetPlatformNameEndpointString();

	// save PRG
	{
		sprintf(buf, "%s/savePrg", plat);
		EndpointDescriptor desc;
		desc.fn = buf;
		desc.platform = plat;
		desc.category = "media";
		desc.description = "Save memory range as PRG file, optionally with Exomizer compression";
		server->AddEndpointFunction(desc, [this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
		{
			bool ret;
			{
				CDebugInterfaceMutexGuard lock(debugInterfaceVice);

				string fileName = params.at("path").get<string>();
				u16 fromAddr = FUN_JsonValueDecOrHexStrWithPrefixToU64(params.at("fromAddr"));
				u16 toAddr = FUN_JsonValueDecOrHexStrWithPrefixToU64(params.at("toAddr"));
				bool exomize = false;
				if (params.contains("exomize"))
				{
					exomize = params.at("exomize").get<bool>();
				}

				if (exomize)
				{
					u16 jmpAddr = FUN_JsonValueDecOrHexStrWithPrefixToU64(params.at("jmpAddr"));
					ret = debuggerApiVice->SaveExomizerPRG(fromAddr, toAddr, jmpAddr, fileName.c_str());
				}
				else
				{
					ret = debuggerApiVice->SavePRG(fromAddr, toAddr, fileName.c_str());
				}
			}
			if (ret)
			{
				return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
			}
			return server->PrepareResult(HTTP_FORBIDDEN, token, json(), NULL, 0);
		});
	}

	// VIC
	{
		sprintf(buf, "%s/vic/write", plat);
		EndpointDescriptor desc;
		desc.fn = buf;
		desc.platform = plat;
		desc.category = "chips";
		desc.description = "Write VIC-II registers";
		server->AddEndpointFunction(desc, [this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
		{
			// Accept both shapes: the object {"27": 0} and the list of pairs
			// [[27, 0], ...] that vic/read hands back. Read-modify-write is the
			// natural cycle for VIC registers, so feeding a read result straight
			// back in has to work.
			json registerPairs = json::array();
			{
				const json &requested = params["registers"];
				if (requested.is_array())
				{
					for (const auto& entry : requested)
					{
						if (!entry.is_array() || entry.size() != 2)
						{
							return server->PrepareResult(HTTP_NOT_ACCEPTABLE, token, json(), NULL, 0);
						}
						registerPairs.push_back(entry);
					}
				}
				else if (requested.is_object())
				{
					for (auto& [key, value] : requested.items())
					{
						registerPairs.push_back(json::array({key, value}));
					}
				}
				else
				{
					// anything else (scalar, null, missing) is a client mistake, not a
					// server error -- say so instead of throwing out of the handler
					return server->PrepareResult(HTTP_NOT_ACCEPTABLE, token, json(), NULL, 0);
				}
			}

			{
				CDebugInterfaceMutexGuard lock(debugInterfaceVice);
				for (const auto& pair : registerPairs)
				{
					u64 registerNum = FUN_JsonValueDecOrHexStrWithPrefixToU64(pair[0]);
					if (registerNum >= 0xD000 && registerNum < 0xD040)
					{
						registerNum -= 0xD000;
					}
					if (registerNum > 0x3F)
					{
						return server->PrepareResult(HTTP_NOT_ACCEPTABLE, token, json(), NULL, 0);
					}
					u64 registerValue = FUN_JsonValueDecOrHexStrWithPrefixToU64(pair[1]);
					debuggerApiVice->SetVicRegister(registerNum, registerValue);
				}
			}

			return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
		});
	}

	{
		sprintf(buf, "%s/vic/read", plat);
		EndpointDescriptor desc;
		desc.fn = buf;
		desc.platform = plat;
		desc.category = "chips";
		desc.description = "Read VIC-II registers";
		server->AddEndpointFunction(desc, [this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
		{
			json j;
			{
				CDebugInterfaceMutexGuard lock(debugInterfaceVice);

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
						return server->PrepareResult(HTTP_NOT_ACCEPTABLE, token, json(), NULL, 0);
					}

					u8 registerValue = debugInterfaceVice->GetVicRegister(registerNum);
					registers[registerNum] = registerValue;
				}

				j["registers"] = registers;
			}
			return server->PrepareResult(HTTP_OK, token, j, NULL, 0);
		});
	}

	{
		sprintf(buf, "%s/vic/breakpoint/add", plat);
		EndpointDescriptor desc;
		desc.fn = buf;
		desc.platform = plat;
		desc.category = "chips";
		desc.description = "Add a VIC-II raster line breakpoint";
		server->AddEndpointFunction(desc, [this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
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
	}

	{
		sprintf(buf, "%s/vic/breakpoint/remove", plat);
		EndpointDescriptor desc;
		desc.fn = buf;
		desc.platform = plat;
		desc.category = "chips";
		desc.description = "Remove a VIC-II raster line breakpoint";
		server->AddEndpointFunction(desc, [this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
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
	}

	// CIA
	{
		sprintf(buf, "%s/cia/write", plat);
		EndpointDescriptor desc;
		desc.fn = buf;
		desc.platform = plat;
		desc.category = "chips";
		desc.description = "Write CIA registers (CIA1 or CIA2)";
		server->AddEndpointFunction(desc, [this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
		{
			{
				CDebugInterfaceMutexGuard lock(debugInterfaceVice);

				int selectedCiaNum = 0;
				if (params.contains("num"))
				{
					selectedCiaNum = params.at("num").get<int>();
					if (selectedCiaNum < 0 || selectedCiaNum > 1)
					{
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
						return server->PrepareResult(HTTP_NOT_ACCEPTABLE, token, json(), NULL, 0);
					}
					u64 registerValue = FUN_JsonValueDecOrHexStrWithPrefixToU64(value);
					debuggerApiVice->SetCiaRegister(ciaNum, registerNum, registerValue);
				}
			}

			return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
		});
	}

	{
		sprintf(buf, "%s/cia/read", plat);
		EndpointDescriptor desc;
		desc.fn = buf;
		desc.platform = plat;
		desc.category = "chips";
		desc.description = "Read CIA registers (CIA1 or CIA2)";
		server->AddEndpointFunction(desc, [this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
		{
			json j;
			{
				CDebugInterfaceMutexGuard lock(debugInterfaceVice);

				int selectedCiaNum = 0;
				if (params.contains("num"))
				{
					selectedCiaNum = params.at("num").get<int>();
					if (selectedCiaNum < 0 || selectedCiaNum > 1)
					{
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
						return server->PrepareResult(HTTP_NOT_ACCEPTABLE, token, json(), NULL, 0);
					}

					u8 registerValue = debuggerApiVice->GetCiaRegister(ciaNum, registerNum);
					registers[registerNum] = registerValue;
				}

				j["registers"] = registers;
			}
			return server->PrepareResult(HTTP_OK, token, j, NULL, 0);
		});
	}

	// SID
	{
		sprintf(buf, "%s/sid/write", plat);
		EndpointDescriptor desc;
		desc.fn = buf;
		desc.platform = plat;
		desc.category = "chips";
		desc.description = "Write SID registers with burst-write to avoid side-effects";
		server->AddEndpointFunction(desc, [this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
		{
			{
				CDebugInterfaceMutexGuard lock(debugInterfaceVice);

				// Note: we need to burst-write to SID registers at once to avoid side-effects
				// make all regs to not set SID
				for (int sidNum = 0; sidNum < C64D_SID_DATA_MAX_SIDS; sidNum++)
				{
					for (int reg = 0; reg < C64_NUM_SID_REGISTERS; reg++)
					{
						sidData->shouldSetSidReg[sidNum][reg] = false;
					}
				}

				for (auto& [key, jsonSidData] : params["sids"].items())
				{
					int sidNum = jsonSidData.at("num").get<int>();
					if (sidNum < 0 || sidNum >= C64D_SID_DATA_MAX_SIDS)
					{
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
			}

			return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
		});
	}

	{
		sprintf(buf, "%s/sid/read", plat);
		EndpointDescriptor desc;
		desc.fn = buf;
		desc.platform = plat;
		desc.category = "chips";
		desc.description = "Read SID registers";
		server->AddEndpointFunction(desc, [this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
		{
			json j;
			{
				CDebugInterfaceMutexGuard lock(debugInterfaceVice);

				int sidNum = 0;
				if (params.contains("num"))
				{
					sidNum = params.at("num").get<int>();
					if (sidNum < 0 || sidNum >= C64D_SID_DATA_MAX_SIDS)
					{
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

				j["registers"] = registers;
			}
			return server->PrepareResult(HTTP_OK, token, j, NULL, 0);
		});
	}

	// Drive 1541 memory with I/O
	{
		sprintf(buf, "%s/drive1541/cpu/memory/writeBlock", plat);
		EndpointDescriptor desc;
		desc.fn = buf;
		desc.platform = plat;
		desc.category = "memory";
		desc.description = "Write a block of bytes to 1541 drive CPU memory (with I/O)";
		desc.supportsBinaryInput = true;
		server->AddEndpointFunction(desc, [this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
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
	}

	{
		sprintf(buf, "%s/drive1541/cpu/memory/readBlock", plat);
		EndpointDescriptor desc;
		desc.fn = buf;
		desc.platform = plat;
		desc.category = "memory";
		desc.description = "Read a block of bytes from 1541 drive CPU memory (with I/O)";
		desc.supportsBinaryOutput = true;
		server->AddEndpointFunction(desc, [this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
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
			auto *resp = server->PrepareResult(HTTP_OK, token, json(), resultBinaryData, size);
			delete[] resultBinaryData;
			return resp;
		});
	}

	// Drive 1541 direct RAM
	{
		sprintf(buf, "%s/drive1541/ram/clear", plat);
		EndpointDescriptor desc;
		desc.fn = buf;
		desc.platform = plat;
		desc.category = "memory";
		desc.description = "Clear a range of 1541 drive direct RAM to a fill value";
		server->AddEndpointFunction(desc, [this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
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
	}

	{
		sprintf(buf, "%s/drive1541/ram/writeBlock", plat);
		EndpointDescriptor desc;
		desc.fn = buf;
		desc.platform = plat;
		desc.category = "memory";
		desc.description = "Write a block of bytes to 1541 drive direct RAM";
		desc.supportsBinaryInput = true;
		server->AddEndpointFunction(desc, [this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
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
	}

	{
		sprintf(buf, "%s/drive1541/ram/readBlock", plat);
		EndpointDescriptor desc;
		desc.fn = buf;
		desc.platform = plat;
		desc.category = "memory";
		desc.description = "Read a block of bytes from 1541 drive direct RAM";
		desc.supportsBinaryOutput = true;
		server->AddEndpointFunction(desc, [this, server](const string token, json params, unsigned char* binaryData, int binaryDataSize) -> vector<char>*
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
			auto *resp = server->PrepareResult(HTTP_OK, token, json(), resultBinaryData, size);
			delete[] resultBinaryData;
			return resp;
		});
	}

	// Drive1541 VIA
	{
		sprintf(buf, "%s/drive1541/via/write", plat);
		EndpointDescriptor desc;
		desc.fn = buf;
		desc.platform = plat;
		desc.category = "chips";
		desc.description = "Write 1541 drive VIA registers (VIA1 or VIA2)";
		server->AddEndpointFunction(desc, [this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
		{
			{
				CDebugInterfaceMutexGuard lock(debugInterfaceVice);

				int selectedViaNum = 0;
				if (params.contains("num"))
				{
					selectedViaNum = params.at("num").get<int>();
					if (selectedViaNum < 0 || selectedViaNum > 1)
					{
						return server->PrepareResult(HTTP_NOT_ACCEPTABLE, token, json(), NULL, 0);
					}
				}

				int selectedDriveNum = 0;
				if (params.contains("drive"))
				{
					selectedDriveNum = params.at("drive").get<int>();
					if (selectedDriveNum < 0 || selectedDriveNum > MAX_DRIVE_NUM)
					{
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
						return server->PrepareResult(HTTP_NOT_ACCEPTABLE, token, json(), NULL, 0);
					}
					u64 registerValue = FUN_JsonValueDecOrHexStrWithPrefixToU64(value);
					debuggerApiVice->SetDrive1541ViaRegister(selectedDriveNum, viaNum, registerNum, registerValue);
				}
			}

			return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
		});
	}

	{
		sprintf(buf, "%s/drive1541/via/read", plat);
		EndpointDescriptor desc;
		desc.fn = buf;
		desc.platform = plat;
		desc.category = "chips";
		desc.description = "Read 1541 drive VIA registers (VIA1 or VIA2)";
		server->AddEndpointFunction(desc, [this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
		{
			json j;
			{
				CDebugInterfaceMutexGuard lock(debugInterfaceVice);

				int selectedViaNum = 0;
				if (params.contains("num"))
				{
					selectedViaNum = params.at("num").get<int>();
					if (selectedViaNum < 0 || selectedViaNum > 1)
					{
						return server->PrepareResult(HTTP_NOT_ACCEPTABLE, token, json(), NULL, 0);
					}
				}

				int selectedDriveNum = 0;
				if (params.contains("drive"))
				{
					selectedDriveNum = params.at("drive").get<int>();
					if (selectedDriveNum < 0 || selectedDriveNum > MAX_DRIVE_NUM)
					{
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
						return server->PrepareResult(HTTP_NOT_ACCEPTABLE, token, json(), NULL, 0);
					}

					u8 registerValue = debuggerApiVice->GetDrive1541ViaRegister(selectedDriveNum, viaNum, registerNum);
					registers[registerNum] = registerValue;
				}

				j["registers"] = registers;
			}
			return server->PrepareResult(HTTP_OK, token, j, NULL, 0);
		});
	}

	// Drive 1541 CPU status
	{
		sprintf(buf, "%s/drive1541/cpu/status", plat);
		EndpointDescriptor desc;
		desc.fn = buf;
		desc.platform = plat;
		desc.category = "cpu";
		desc.description = "Get 1541 drive CPU status (PC, A, X, Y, SP, flags)";
		server->AddEndpointFunction(desc, [this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
		{
			json result;
			{
				CDebugInterfaceMutexGuard lock(debugInterfaceVice);

				C64StateCPU state;
				debugInterfaceVice->GetDrive1541CpuState(&state);
				result["pc"] = state.pc;
				result["a"] = state.a;
				result["x"] = state.x;
				result["y"] = state.y;
				result["sp"] = state.sp;
				result["flags"] = state.processorFlags;
			}
			return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
		});
	}

	// Drive 1541 cycle counter
	{
		sprintf(buf, "%s/drive1541/cpu/counters/read", plat);
		EndpointDescriptor desc;
		desc.fn = buf; desc.platform = plat; desc.category = "cpu";
		desc.description = "Read drive 1541 CPU cycle counter";
		server->AddEndpointFunction(desc, [this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
		{
			json result;
			result["cycle"] = (u64)c64d_get_vice_drivecpu_clk(0);
			return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
		});
	}

	// Drive 1541 state
	{
		sprintf(buf, "%s/drive1541/state", plat);
		EndpointDescriptor desc;
		desc.fn = buf;
		desc.platform = plat;
		desc.category = "cpu";
		desc.description = "Get 1541 drive state (PC and RAM size)";
		server->AddEndpointFunction(desc, [this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
		{
			json result;
			{
				CDebugInterfaceMutexGuard lock(debugInterfaceVice);

				result["pc"] = debugInterfaceVice->GetDrive1541PC();

				CDataAdapter *da = debuggerApiVice->GetDataAdapterDrive1541MemoryDirectRAM();
				if (da)
					result["ramSize"] = da->AdapterGetDataLength();
			}
			return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
		});
	}

	// Drive 1541 RAM clear (duplicate registration overrides earlier one)
	{
		sprintf(buf, "%s/drive1541/ram/clear", plat);
		EndpointDescriptor desc;
		desc.fn = buf;
		desc.platform = plat;
		desc.category = "memory";
		desc.description = "Clear a range of 1541 drive direct RAM to a fill value";
		server->AddEndpointFunction(desc, [this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
		{
			int address = params.at("address").get<int>();
			int size = params.at("size").get<int>();
			u8 value = 0;
			if (params.contains("value"))
				value = params.at("value").get<int>();

			CDataAdapter *dataAdapter = debuggerApiVice->GetDataAdapterDrive1541MemoryDirectRAM();
			if (address + size > dataAdapter->AdapterGetDataLength())
			{
				return server->PrepareResult(HTTP_PAYLOAD_TOO_LARGE, token, json(), NULL, 0);
			}

			for (int i = 0; i < size; i++)
			{
				dataAdapter->AdapterWriteByte(address + i, value);
			}
			return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
		});
	}

	// Drive 1541 MakeJmp
	{
		sprintf(buf, "%s/drive1541/cpu/makejmp", plat);
		EndpointDescriptor desc;
		desc.fn = buf; desc.platform = plat; desc.category = "cpu";
		desc.description = "Set drive 1541 CPU program counter";
		server->AddEndpointFunction(desc, [this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
		{
			int address = params.at("address").get<int>();
			{
				CDebugInterfaceMutexGuard lock(debugInterfaceVice);
				debugInterfaceVice->MakeJmpNoReset1541(address);
			}
			return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
		});
	}

	// Drive 1541 register write
	{
		sprintf(buf, "%s/drive1541/cpu/registers/write", plat);
		EndpointDescriptor desc;
		desc.fn = buf; desc.platform = plat; desc.category = "cpu";
		desc.description = "Write drive 1541 CPU registers (a, x, y, sp, flags)";
		server->AddEndpointFunction(desc, [this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
		{
			{
				CDebugInterfaceMutexGuard lock(debugInterfaceVice);
				if (params.contains("a")) debugInterfaceVice->SetRegisterA1541(params["a"].get<int>());
				if (params.contains("x")) debugInterfaceVice->SetRegisterX1541(params["x"].get<int>());
				if (params.contains("y")) debugInterfaceVice->SetRegisterY1541(params["y"].get<int>());
				if (params.contains("sp")) debugInterfaceVice->SetStackPointer1541(params["sp"].get<int>());
				if (params.contains("flags")) debugInterfaceVice->SetRegisterP1541(params["flags"].get<int>());
			}
			return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
		});
	}

	// Drive 1541 breakpoints (use symbolsDrive1541 instead of main symbols)
	{
		sprintf(buf, "%s/drive1541/cpu/breakpoint/add", plat);
		EndpointDescriptor desc;
		desc.fn = buf; desc.platform = plat; desc.category = "breakpoints";
		desc.description = "Add CPU breakpoint on drive 1541";
		server->AddEndpointFunction(desc, [this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
		{
			int addr = params.at("addr").get<int>();
			{
				CDebugInterfaceMutexGuard lock(debugInterfaceVice);
				CDebugInterfaceC64 *diC64 = (CDebugInterfaceC64 *)debugInterfaceVice;
				if (diC64->symbolsDrive1541 && diC64->symbolsDrive1541->currentSegment)
				{
					diC64->symbolsDrive1541->currentSegment->AddBreakpointPC(addr);
				}
			}
			return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
		});
	}

	{
		sprintf(buf, "%s/drive1541/cpu/breakpoint/remove", plat);
		EndpointDescriptor desc;
		desc.fn = buf; desc.platform = plat; desc.category = "breakpoints";
		desc.description = "Remove CPU breakpoint on drive 1541";
		server->AddEndpointFunction(desc, [this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
		{
			int addr = params.at("addr").get<int>();
			{
				CDebugInterfaceMutexGuard lock(debugInterfaceVice);
				CDebugInterfaceC64 *diC64 = (CDebugInterfaceC64 *)debugInterfaceVice;
				if (diC64->symbolsDrive1541 && diC64->symbolsDrive1541->currentSegment)
				{
					CDebugBreakpointsAddr *bps = diC64->symbolsDrive1541->currentSegment->breakpointsPC;
					if (bps)
					{
						auto it = bps->breakpoints.find(addr);
						if (it != bps->breakpoints.end())
						{
							bps->RemoveBreakpoint(it->second);
						}
					}
				}
			}
			return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
		});
	}

	{
		sprintf(buf, "%s/drive1541/cpu/breakpoint/list", plat);
		EndpointDescriptor desc;
		desc.fn = buf; desc.platform = plat; desc.category = "breakpoints";
		desc.description = "List CPU breakpoints on drive 1541";
		server->AddEndpointFunction(desc, [this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
		{
			json result;
			json bpList = json::array();
			{
				CDebugInterfaceMutexGuard lock(debugInterfaceVice);
				CDebugInterfaceC64 *diC64 = (CDebugInterfaceC64 *)debugInterfaceVice;
				if (diC64->symbolsDrive1541 && diC64->symbolsDrive1541->currentSegment)
				{
					CDebugBreakpointsAddr *bps = diC64->symbolsDrive1541->currentSegment->breakpointsPC;
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
	}

	SYS_ReleaseCharBuf(buf);
}
