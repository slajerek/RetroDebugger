#pragma once

#include "CTest.h"

// RD-027: the emulator-core accessors the memory mapping providers will read
// (VICE banking state and cartridge buffers, Atari800 banking state/attrib/
// backing RAM, Nestopia PRG mapping and cartridge info) return sane values on
// the default machines, plus regressions for the three core defects fixed with
// them: the NES safe peek no longer reads past the 2 KB RAM array, the C64
// cartridge peek goes through VICE's per-cartridge monitor peek, and the C64
// cartridge data adapter honours its memory type.
class CTestCoreMappingAccessors : public CTest
{
public:
	virtual const char *GetName() override { return "CoreMappingAccessors"; }
	virtual void Run(ITestCallback *callback) override;
	virtual void Cancel() override { isRunning = false; }
};
