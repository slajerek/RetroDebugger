#pragma once

#include "CTest.h"
#include "SYS_Defs.h"
#include "json.hpp"
#include <string>
#include <vector>

class CDebugInterface;
class CDebugSymbols;
class CDebugDatabase;
class CSlrString;

// RD-015: the step-trace runner. Replays every trace listed in
// tests/source-debug/expectations/index.json (spec #9.5 format plus the
// machine (c64, atari, nes), the Tier 1 artifacts, the entry point and an
// optional fast interrupt -- C64 CIA timer A, Atari POKEY timer 1, NES DMC --
// whose handler can call a fixture function): loads
// the binary (a C64 .crt is attached) and its debug info into scratch symbols, runs to the start line
// through a source breakpoint, then issues each source step and compares
// (file, line, function, depth, reason). A failing trace writes the actual
// trace next to the expected one to
// /tmp/source-step-trace-<compiler>-<machine>-<fixture>.txt.
class CTestSourceStepTrace : public CTest
{
public:
	virtual const char *GetName() override { return "SourceStepTrace"; }
	virtual void Run(ITestCallback *callback) override;
	virtual void Cancel() override { isRunning = false; }

private:
	int stepNum = 1;
	std::string failure;
	bool Check(bool condition, const std::string &message);

	// an emulator a trace ran on: started by the test or not, its own symbols
	struct Machine
	{
		std::string name;
		CDebugInterface *di = NULL;
		bool started = false;
		CDebugSymbols *liveSymbols = NULL;
		u8 savedVector[2] = { 0, 0 };	// C64 $0314
		u8 savedPort[2] = { 0x2F, 0x37 };	// C64 6510 port: cc65's crt0 banks BASIC out
		// C64: a trace's .crt attached, the user's cartridge setting kept aside
		bool c64Cartridge = false;
		CSlrString *savedCartridgePath = NULL;
		// Atari: the interrupt banks the OS out and owns the CPU vectors
		bool atariInterrupt = false;
		u8 atariPortb = 0xFF, atariPbctl = 0, atariSkctl = 0;
		u8 atariVectors[6] = {};
	};
	std::vector<Machine> machines;
	Machine *GetMachine(const std::string &name);
	void RestoreMachines();

	bool RunTrace(const std::string &path);
	bool LaunchC64(CDebugInterface *di, const nlohmann::json &trace, const std::vector<u8> &binary,
				   const CDebugDatabase *database, const std::string &name);
	bool LaunchC64Cartridge(Machine &machine, const std::string &binaryPath, u16 startAddress, const std::string &name);
	bool DetachC64Cartridge(Machine &machine);
	bool LaunchAtari(CDebugInterface *di, const nlohmann::json &trace, const std::vector<u8> &binary,
					 const std::string &name);
	bool LaunchNes(CDebugInterface *di, const nlohmann::json &trace, const std::string &binaryPath,
				   const std::string &name);
	bool InstallInterrupt(Machine &machine, const nlohmann::json &trace, const CDebugDatabase *database,
						  u16 startAddress, const std::string &name);
	void RemoveAtariInterrupt(Machine &machine);
};
