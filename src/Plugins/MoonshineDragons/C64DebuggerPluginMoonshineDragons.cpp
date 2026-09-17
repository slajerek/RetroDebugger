#include "C64DebuggerPluginMoonshineDragons.h"
#include "CByteBuffer.h"
#include "SYS_FileSystem.h"
#include "CGuiMain.h"
#include "CDebugInterfaceVice.h"
#include "CViewC64.h"
#include "DBG_Log.h"
#include <stdio.h>

C64DebuggerPluginMoonshineDragons *pluginMoonshineDragons = NULL;

void PLUGIN_MoonshineDragonsInit()
{
	if (pluginMoonshineDragons == NULL)
	{
		pluginMoonshineDragons = new C64DebuggerPluginMoonshineDragons(320, 60, 520, 240);
		CDebuggerEmulatorPlugin::RegisterPlugin(pluginMoonshineDragons);
	}
}

void PLUGIN_MoonshineDragonsSetVisible(bool isVisible)
{
	if (pluginMoonshineDragons != NULL)
	{
		pluginMoonshineDragons->SetVisible(isVisible);
	}
}

C64DebuggerPluginMoonshineDragons::C64DebuggerPluginMoonshineDragons(float posX, float posY, float sizeX, float sizeY)
: CGuiView(posX, posY, -1, sizeX, sizeY)
{
	InitImGuiView("Moonshine Dragons");

	extensionsPRG.push_back(new CSlrString("prg"));

	strcpy(statusText, "Idle.");
}

C64DebuggerPluginMoonshineDragons::~C64DebuggerPluginMoonshineDragons()
{
}

void C64DebuggerPluginMoonshineDragons::Init()
{
	api->AddView(this);
}

void C64DebuggerPluginMoonshineDragons::RenderImGui()
{
	PreRenderImGui();

	ImGui::TextWrapped("Minimal demo: displays \"MOONSHINE DRAGONS\" in text mode with a color-wave animation on the row beneath it.");
	ImGui::Separator();

	if (ImGui::Button("Generate & Run", ImVec2(160, 30)))
	{
		GenerateAndRun();
	}
	ImGui::SameLine();
	if (ImGui::Button("Save PRG...", ImVec2(140, 30)))
	{
		RequestSavePrgDialog();
	}

	ImGui::Separator();
	ImGui::TextUnformatted("Status:");
	ImGui::SameLine();
	ImGui::TextUnformatted(statusText);

	PostRenderImGui();
}

// 6502 program: clears screen, writes "MOONSHINE DRAGONS" at row 12 col 11,
// then in an infinite loop waits for raster $FB, increments a frame counter,
// and rewrites the 17-byte color RAM strip under the text with (x + frame) & 0x0F
// to produce a cyclic rainbow wave across the title.
std::vector<u8> C64DebuggerPluginMoonshineDragons::BuildPRG()
{
	std::vector<u8> prg;

	// PRG load address: $0801 (little-endian).
	prg.push_back(0x01); prg.push_back(0x08);

	// BASIC stub "10 SYS 2061" — same shape as the Fireworks plugin.
	// Resumes at $080D.
	static const u8 BASIC_STUB[] = {
		0x0B, 0x08,             // link to next BASIC line ($080B)
		0x0A, 0x00,             // line number 10
		0x9E,                   // SYS token
		0x32, 0x30, 0x36, 0x31, // "2061"
		0x00,                   // end-of-line
		0x00, 0x00              // end-of-program
	};
	for (u8 b : BASIC_STUB) prg.push_back(b);

	// 6502 program starting at $080D. Hand-assembled — offsets relative to
	// the listing in the comment above (see WritePrg comment for the full
	// disassembly).
	static const u8 CODE[] = {
		// $080D: clear-screen + paint title
		0x78,                   // SEI
		0xA9, 0x00,             // LDA #$00
		0x8D, 0x20, 0xD0,       // STA $D020
		0x8D, 0x21, 0xD0,       // STA $D021
		0xA2, 0x00,             // LDX #$00
		0xA9, 0x20,             // LDA #$20  (space, screen code)
		// $081A: clear loop (4 banks of 256)
		0x9D, 0x00, 0x04,       // STA $0400,X
		0x9D, 0x00, 0x05,       // STA $0500,X
		0x9D, 0x00, 0x06,       // STA $0600,X
		0x9D, 0xE8, 0x06,       // STA $06E8,X  (covers the tail to $07E7)
		0xE8,                   // INX
		0xD0, 0xF1,             // BNE $081A
		// $0829: write 17-char title to $05EB (row 12, col 11)
		0xA2, 0x10,             // LDX #$10  (16 → 0)
		0xBD, 0x50, 0x08,       // LDA $0850,X  (TEXT)
		0x9D, 0xEB, 0x05,       // STA $05EB,X
		0xCA,                   // DEX
		0x10, 0xF7,             // BPL $082B
		// $0834: main loop — raster wait + color wave
		0xA9, 0xFB,             // LDA #$FB
		0xCD, 0x12, 0xD0,       // CMP $D012
		0xD0, 0xFB,             // BNE $0836
		0xEE, 0x61, 0x08,       // INC $0861 (FRAME)
		0xA2, 0x10,             // LDX #$10
		// $0840: color loop
		0x8A,                   // TXA
		0x18,                   // CLC
		0x6D, 0x61, 0x08,       // ADC $0861 (FRAME)
		0x29, 0x0F,             // AND #$0F
		0x9D, 0xEB, 0xD9,       // STA $D9EB,X  (color RAM under title)
		0xCA,                   // DEX
		0x10, 0xF3,             // BPL $0840
		0x4C, 0x34, 0x08,       // JMP $0834
		// $0850: TEXT — screen codes for "MOONSHINE DRAGONS"
		0x0D, 0x0F, 0x0F, 0x0E, 0x13, 0x08, 0x09, 0x0E, 0x05,
		0x20,
		0x04, 0x12, 0x01, 0x07, 0x0F, 0x0E, 0x13,
		// $0861: FRAME counter
		0x00
	};
	for (u8 b : CODE) prg.push_back(b);

	return prg;
}

void C64DebuggerPluginMoonshineDragons::GenerateAndRun()
{
	if (api == NULL)
	{
		strcpy(statusText, "API unavailable.");
		return;
	}

	std::vector<u8> prg = BuildPRG();

	CByteBuffer *buffer = new CByteBuffer();
	buffer->PutBytes(prg.data(), (unsigned int)prg.size());
	// PutBytes leaves the read index at end-of-buffer. LoadPRG reads from
	// index, so without Rewind() it reads zero bytes and the autostart
	// silently does nothing (logs "LoadPRG: ..loaded till=0000").
	buffer->Rewind();
	LOGM("MoonshineDragons::GenerateAndRun: prg.size=%d buffer->length=%d buffer->index=%d",
		(int)prg.size(), (int)buffer->length, (int)buffer->index);

	api->LoadPRG(buffer, /*autoStart*/ true, /*forceFastReset*/ true);

	// LoadPRG takes ownership of nothing — free our local buffer.
	delete buffer;

	snprintf(statusText, sizeof(statusText),
		"Built %d bytes ($0801-$%04X) and loaded.",
		(int)prg.size(), (int)(0x0801 + (prg.size() - 2) - 1));
}

void C64DebuggerPluginMoonshineDragons::RequestSavePrgDialog()
{
	SYS_DialogSaveFile(this, &extensionsPRG, NULL, NULL, NULL);
}

void C64DebuggerPluginMoonshineDragons::WritePrgToFile(const char *filePath)
{
	std::vector<u8> prg = BuildPRG();

	FILE *fp = fopen(filePath, "wb");
	if (fp == NULL)
	{
		snprintf(statusText, sizeof(statusText),
			"Save failed: cannot open '%s'.", filePath);
		return;
	}
	fwrite(prg.data(), 1, prg.size(), fp);
	fclose(fp);

	snprintf(statusText, sizeof(statusText),
		"Saved %d bytes to '%s'.", (int)prg.size(), filePath);
}

void C64DebuggerPluginMoonshineDragons::SystemDialogFileSaveSelected(CSlrString *path)
{
	guiMain->LockMutex();
	char *cstr = path->GetStdASCII();
	WritePrgToFile(cstr);
	delete [] cstr;
	guiMain->UnlockMutex();
}

void C64DebuggerPluginMoonshineDragons::SystemDialogFileSaveCancelled()
{
}
