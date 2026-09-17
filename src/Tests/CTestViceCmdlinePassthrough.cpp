#include "CTestViceCmdlinePassthrough.h"
#include "EmulatorsConfig.h"
#include "SYS_Main.h"
#include <cstring>
#include <cstdlib>
#include <string>

#if defined(RUN_COMMODORE64)
extern "C" {
#include "vice.h"       // must precede resources.h: defines VICE_ATTR_RESPRINTF
#include "cmdline.h"
#include "resources.h"
}
#endif

// RetroDebugger and VICE parse the SAME argv: VICE first (inside
// vice_main_program), then C64CommandLine.cpp, then the other emulators.
// VICE used to abort at the first option it did not recognize, which silently
// dropped every VICE option after it -- the bug behind the IDE64 USB server
// report, where "-c64" separators killed the -IDE64USB* options that followed.
//
// This asserts both halves of the fix:
//   1. VICE options are applied even when non-VICE options are interleaved
//   2. the non-VICE options SURVIVE in argv for the parsers that run later
//      (argv is compacted in place, so consuming them would starve Atari800)

void CTestViceCmdlinePassthrough::Run(ITestCallback *cb)
{
	this->callback = cb;
	this->isRunning = true;

#if !defined(RUN_COMMODORE64)
	TestCompleted(true, "C64 emulator disabled, skipping");
	return;
#else
	int step = 0;

	// preserve the resources this test writes
	int oldVersion = 0;
	resources_get_int("IDE64Version", &oldVersion);
	const char *oldAddrRaw = NULL;
	resources_get_string("IDE64USBServerAddress", &oldAddrRaw);
	std::string oldAddr = (oldAddrRaw != NULL) ? oldAddrRaw : "";

	// A mixed command line modelled on the user report. -c64, -prg and
	// -unpause are RetroDebugger options; VICE must skip them and still apply
	// every VICE option that follows. strdup because cmdline_parse may rewrite
	// tokens in place (the "--long" kludge).
	const char *tokens[] = {
		"retrodebugger",
		"-c64",
		"-IDE64version", "1",
		"-prg", "foo.prg",
		"-unpause",
		"-IDE64USBAddress", "ip4://127.0.0.1:64247"
	};
	const int tokenCount = 9;

	char *argvTest[16];
	char *allocated[16];   // cmdline_parse compacts argv in place, so keep the
						   // original pointers to free -- freeing the compacted
						   // array would double-free and leak
	for (int i = 0; i < tokenCount; i++)
	{
		argvTest[i] = strdup(tokens[i]);
		allocated[i] = argvTest[i];
	}
	argvTest[tokenCount] = NULL;

	int argc = tokenCount;
	int ret = cmdline_parse(&argc, argvTest);

	step++;
	StepCompleted(step, ret == 0, ret == 0
				  ? "cmdline_parse survived interleaved non-VICE options"
				  : "cmdline_parse aborted on a non-VICE option");

	int version = -1;
	resources_get_int("IDE64Version", &version);
	step++;
	StepCompleted(step, version == 1, version == 1
				  ? "IDE64Version applied through a mixed command line"
				  : "IDE64Version was not applied");

	const char *addr = NULL;
	resources_get_string("IDE64USBServerAddress", &addr);
	bool addrOk = (addr != NULL) && !strcmp(addr, "ip4://127.0.0.1:64247");
	step++;
	StepCompleted(step, addrOk, addrOk
				  ? "IDE64USBServerAddress applied after skipped options (incl. -prg's argument)"
				  : "IDE64USBServerAddress was not applied");

	// the non-VICE tokens must still be there for the parsers that run later
	const char *expectedKept[4] = { "-c64", "-prg", "foo.prg", "-unpause" };
	bool keptOk = (argc == 5);
	for (int k = 0; keptOk && k < 4; k++)
	{
		keptOk = (argvTest[k + 1] != NULL) && !strcmp(argvTest[k + 1], expectedKept[k]);
	}
	step++;
	StepCompleted(step, keptOk, keptOk
				  ? "non-VICE options retained in argv (-c64 -prg foo.prg -unpause)"
				  : "non-VICE options were consumed -- later parsers would be starved");

	// restore
	resources_set_int("IDE64Version", oldVersion);
	resources_set_string("IDE64USBServerAddress", oldAddr.c_str());

	for (int i = 0; i < tokenCount; i++)
	{
		free(allocated[i]);
	}

	bool allOk = (ret == 0) && (version == 1) && addrOk && keptOk;
	TestCompleted(allOk, allOk
				  ? "VICE cmdline passthrough works with interleaved RetroDebugger options"
				  : "VICE cmdline passthrough is broken");
#endif
}

void CTestViceCmdlinePassthrough::Cancel()
{
	isRunning = false;
}
