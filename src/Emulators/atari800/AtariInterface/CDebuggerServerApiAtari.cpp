#include "CDebuggerServerApiAtari.h"
#include "CDebugInterfaceAtari.h"
#include "CDebuggerServer.h"
#include "SYS_Funct.h"

using namespace std;
using namespace nlohmann;

CDebuggerServerApiAtari::CDebuggerServerApiAtari(CDebugInterface *debugInterface)
: CDebuggerServerApi(debugInterface)
{
	this->debugInterfaceAtari = (CDebugInterfaceAtari *)debugInterface;
}

void CDebuggerServerApiAtari::RegisterEndpoints(CDebuggerServer *server)
{
	// Register generic cross-platform endpoints first
	CDebuggerServerApi::RegisterEndpoints(server);

	char *buf = SYS_GetCharBuf();

	// Atari machine state
	sprintf(buf, "%s/machine/state", debugInterface->GetPlatformNameEndpointString());
	server->AddEndpointFunction(buf, [this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
	{
		json result;
		{
			CDebugInterfaceMutexGuard lock(debugInterfaceAtari);
			result["platform"] = "atari800";
			result["isRunning"] = debugInterfaceAtari->isRunning;
			result["pokeyStereo"] = debugInterfaceAtari->IsPokeyStereo();
		}
		return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
	});

	const char *plat = debugInterface->GetPlatformNameEndpointString();

	// Cartridge insert
	{
		sprintf(buf, "%s/media/cart/insert", plat);
		EndpointDescriptor desc;
		desc.fn = buf; desc.platform = plat; desc.category = "media";
		desc.description = "Insert Atari cartridge ROM file";
		server->AddEndpointFunction(desc, [this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
		{
			string path = params.at("path").get<string>();
			bool readOnly = params.value("readOnly", true);
			bool success = debugInterfaceAtari->InsertCartridge((char *)path.c_str(), readOnly);
			if (success)
				return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
			return server->PrepareResult(HTTP_INTERNAL_SERVER_ERROR, token, json(), NULL, 0);
		});
	}

	// ANTIC register read
	{
		sprintf(buf, "%s/antic/read", plat);
		EndpointDescriptor desc;
		desc.fn = buf; desc.platform = plat; desc.category = "chips";
		desc.description = "Read ANTIC display processor registers";
		server->AddEndpointFunction(desc, [this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
		{
			json result;
			json regs;
			{
				CDebugInterfaceMutexGuard lock(debugInterfaceAtari);
				for (const auto &reg : params["registers"])
				{
					int regNum = reg.get<int>();
					regs[to_string(regNum)] = debugInterfaceAtari->GetAnticRegister(regNum);
				}
			}
			result["registers"] = regs;
			return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
		});
	}

	// GTIA register read
	{
		sprintf(buf, "%s/gtia/read", plat);
		EndpointDescriptor desc;
		desc.fn = buf; desc.platform = plat; desc.category = "chips";
		desc.description = "Read GTIA graphics registers";
		server->AddEndpointFunction(desc, [this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
		{
			json result;
			json regs;
			{
				CDebugInterfaceMutexGuard lock(debugInterfaceAtari);
				for (const auto &reg : params["registers"])
				{
					int regNum = reg.get<int>();
					regs[to_string(regNum)] = debugInterfaceAtari->GetGtiaRegister(regNum);
				}
			}
			result["registers"] = regs;
			return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
		});
	}

	// POKEY register read
	{
		sprintf(buf, "%s/pokey/read", plat);
		EndpointDescriptor desc;
		desc.fn = buf; desc.platform = plat; desc.category = "chips";
		desc.description = "Read POKEY sound/IO registers";
		server->AddEndpointFunction(desc, [this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
		{
			json result;
			json regs;
			{
				CDebugInterfaceMutexGuard lock(debugInterfaceAtari);
				for (const auto &reg : params["registers"])
				{
					int regNum = reg.get<int>();
					regs[to_string(regNum)] = debugInterfaceAtari->GetPokeyRegister(regNum);
				}
			}
			result["registers"] = regs;
			return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
		});
	}

	// PIA register read
	{
		sprintf(buf, "%s/pia/read", plat);
		EndpointDescriptor desc;
		desc.fn = buf; desc.platform = plat; desc.category = "chips";
		desc.description = "Read PIA joystick/banking registers";
		server->AddEndpointFunction(desc, [this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
		{
			json result;
			json regs;
			{
				CDebugInterfaceMutexGuard lock(debugInterfaceAtari);
				for (const auto &reg : params["registers"])
				{
					int regNum = reg.get<int>();
					regs[to_string(regNum)] = debugInterfaceAtari->GetPiaRegister(regNum);
				}
			}
			result["registers"] = regs;
			return server->PrepareResult(HTTP_OK, token, result, NULL, 0);
		});
	}

	// ANTIC register write
	{
		sprintf(buf, "%s/antic/write", plat);
		EndpointDescriptor desc;
		desc.fn = buf; desc.platform = plat; desc.category = "chips";
		desc.description = "Write ANTIC display processor registers. Params: registers {regNum: value, ...}";
		server->AddEndpointFunction(desc, [this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
		{
			{
				CDebugInterfaceMutexGuard lock(debugInterfaceAtari);
				for (auto &[key, value] : params["registers"].items())
				{
					// stoi() is base-10 only: "0x18" silently parsed as register 0,
					// landing the write on the wrong register with no error. Use the
					// same dec/hex parser the C64 endpoints use.
					int regNum = (int)FUN_DecOrHexStrWithPrefixToU64(key.c_str());
					u8 val = value.get<int>();
					debugInterfaceAtari->SetAnticRegister(regNum, val);
				}
			}
			return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
		});
	}

	// GTIA register write
	{
		sprintf(buf, "%s/gtia/write", plat);
		EndpointDescriptor desc;
		desc.fn = buf; desc.platform = plat; desc.category = "chips";
		desc.description = "Write GTIA graphics registers. Params: registers {regNum: value, ...}";
		server->AddEndpointFunction(desc, [this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
		{
			{
				CDebugInterfaceMutexGuard lock(debugInterfaceAtari);
				for (auto &[key, value] : params["registers"].items())
				{
					// stoi() is base-10 only: "0x18" silently parsed as register 0,
					// landing the write on the wrong register with no error. Use the
					// same dec/hex parser the C64 endpoints use.
					int regNum = (int)FUN_DecOrHexStrWithPrefixToU64(key.c_str());
					u8 val = value.get<int>();
					debugInterfaceAtari->SetGtiaRegister(regNum, val);
				}
			}
			return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
		});
	}

	// POKEY register write
	{
		sprintf(buf, "%s/pokey/write", plat);
		EndpointDescriptor desc;
		desc.fn = buf; desc.platform = plat; desc.category = "chips";
		desc.description = "Write POKEY sound/IO registers. Params: registers {regNum: value, ...}";
		server->AddEndpointFunction(desc, [this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
		{
			{
				CDebugInterfaceMutexGuard lock(debugInterfaceAtari);
				for (auto &[key, value] : params["registers"].items())
				{
					// stoi() is base-10 only: "0x18" silently parsed as register 0,
					// landing the write on the wrong register with no error. Use the
					// same dec/hex parser the C64 endpoints use.
					int regNum = (int)FUN_DecOrHexStrWithPrefixToU64(key.c_str());
					u8 val = value.get<int>();
					debugInterfaceAtari->SetPokeyRegister(regNum, val);
				}
			}
			return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
		});
	}

	// PIA register write
	{
		sprintf(buf, "%s/pia/write", plat);
		EndpointDescriptor desc;
		desc.fn = buf; desc.platform = plat; desc.category = "chips";
		desc.description = "Write PIA joystick/banking registers. Params: registers {regNum: value, ...}";
		server->AddEndpointFunction(desc, [this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
		{
			{
				CDebugInterfaceMutexGuard lock(debugInterfaceAtari);
				for (auto &[key, value] : params["registers"].items())
				{
					// stoi() is base-10 only: "0x18" silently parsed as register 0,
					// landing the write on the wrong register with no error. Use the
					// same dec/hex parser the C64 endpoints use.
					int regNum = (int)FUN_DecOrHexStrWithPrefixToU64(key.c_str());
					u8 val = value.get<int>();
					debugInterfaceAtari->SetPiaRegister(regNum, val);
				}
			}
			return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
		});
	}

	// Disk attach
	{
		sprintf(buf, "%s/media/disk/attach", plat);
		EndpointDescriptor desc;
		desc.fn = buf; desc.platform = plat; desc.category = "media";
		desc.description = "Attach Atari disk image (ATR/XFD). Params: path, diskNo (1-based), readOnly (default true)";
		server->AddEndpointFunction(desc, [this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
		{
			string path = params.at("path").get<string>();
			int diskNo = params.value("diskNo", 1);
			bool readOnly = params.value("readOnly", true);
			bool success = debugInterfaceAtari->MountDisk((char *)path.c_str(), diskNo, readOnly);
			if (success)
				return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
			return server->PrepareResult(HTTP_INTERNAL_SERVER_ERROR, token, json(), NULL, 0);
		});
	}

	// Tape attach
	{
		sprintf(buf, "%s/media/tape/attach", plat);
		EndpointDescriptor desc;
		desc.fn = buf; desc.platform = plat; desc.category = "media";
		desc.description = "Attach Atari cassette tape image. Params: path, readOnly (default true)";
		server->AddEndpointFunction(desc, [this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
		{
			string path = params.at("path").get<string>();
			bool readOnly = params.value("readOnly", true);
			bool success = debugInterfaceAtari->AttachTape((char *)path.c_str(), readOnly);
			if (success)
				return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
			return server->PrepareResult(HTTP_INTERNAL_SERVER_ERROR, token, json(), NULL, 0);
		});
	}

	// Load executable (XEX)
	{
		sprintf(buf, "%s/media/load", plat);
		EndpointDescriptor desc;
		desc.fn = buf; desc.platform = plat; desc.category = "media";
		desc.description = "Load Atari executable file (XEX/COM/EXE). Params: path";
		server->AddEndpointFunction(desc, [this, server](const string token, json params, unsigned char *binaryData, int binaryDataSize) -> vector<char>*
		{
			string path = params.at("path").get<string>();
			bool success = debugInterfaceAtari->LoadExecutable((char *)path.c_str());
			if (success)
				return server->PrepareResult(HTTP_OK, token, json(), NULL, 0);
			return server->PrepareResult(HTTP_INTERNAL_SERVER_ERROR, token, json(), NULL, 0);
		});
	}

	SYS_ReleaseCharBuf(buf);
}
