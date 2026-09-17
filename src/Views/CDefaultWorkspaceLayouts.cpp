#include "DBG_Log.h"
#include "CDefaultWorkspaceLayouts.h"
#include "CDebugInterface.h"
#include "CDebugInterfaceAtari.h"
#include "CDebugInterfaceC64.h"
#include "CDebugInterfaceNes.h"
#include "CGuiMain.h"
#include "CGuiView.h"
#include "CGuiViewDebugLog.h"
#include "CLayoutManager.h"
#include "CViewAtariScreen.h"
#include "CViewAtariStateANTIC.h"
#include "CViewAtariStateCPU.h"
#include "CViewAtariStateGTIA.h"
#include "CViewAtariStatePIA.h"
#include "CViewAtariStatePOKEY.h"
#include "CViewC64AllGraphicsBitmaps.h"
#include "CViewC64AllGraphicsCharsets.h"
#include "CViewC64AllGraphicsScreens.h"
#include "CViewC64AllGraphicsSprites.h"
#include "CViewC64Screen.h"
#include "CViewC64SidPianoKeyboard.h"
#include "CViewC64SidTrackerHistory.h"
#include "CViewC64StateCPU.h"
#include "CViewC64StateCIA.h"
#include "CViewC64StateREU.h"
#include "CViewC64StateSID.h"
#include "CViewC64StateVIC.h"
#include "CViewC64VicControl.h"
#include "CViewC64VicDisplay.h"
#include "CViewC64.h"
#include "CViewDataDump.h"
#include "CViewDataMap.h"
#include "CViewDisassembly.h"
#include "CViewDrive1541StateCPU.h"
#include "CViewDrive1541StateVIA.h"
#include "CViewEmulationCounters.h"
#include "CViewEmulationState.h"
#include "CViewMonitorConsole.h"
#include "CViewNesPianoKeyboard.h"
#include "CViewNesPpuNametables.h"
#include "CViewNesPpuPalette.h"
#include "CViewNesScreen.h"
#include "CViewNesStateAPU.h"
#include "CViewNesStateCPU.h"
#include "CViewNesStatePPU.h"
#include "CViewSourceCode.h"
#include "SYS_KeyCodes.h"
#include "CByteBuffer.h"
#include "CConfigStorageHjson.h"
#include "CLayoutParameter.h"
#include "imgui.h"
#include "imgui_internal.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <sstream>

void SDefaultWorkspacePopupState::StartShortcutCapture(EDefaultWorkspaceSlot slot)
{
	capturingShortcutSlot = slot;
	pendingKeyboardCallbackRemoval = false;
}

void SDefaultWorkspacePopupState::StopShortcutCapture(bool removeKeyboardCallback)
{
	capturingShortcutSlot = (EDefaultWorkspaceSlot)0;
	if (removeKeyboardCallback)
	{
		pendingKeyboardCallbackRemoval = true;
	}
}

bool SDefaultWorkspacePopupState::ConsumeKeyboardCallbackRemovalRequest()
{
	bool shouldRemove = pendingKeyboardCallbackRemoval;
	pendingKeyboardCallbackRemoval = false;
	return shouldRemove;
}

static const SDefaultWorkspaceViewPlacementSpec kC64OnlyPlacements[] = {
	{ "c64.screen", 35.102f, 0.0f, 508.235f, 360.0f }
};

static const SDefaultWorkspaceViewPlacementSpec kC64DataDumpPlacements[] = {
	{ "c64.screen", 317.416f, 10.5f, 259.584f, 183.872f },
	{ "c64.cpu_state", 317.416f, 0.0f, 0.0f, 0.0f },
	{ "c64.disassembly", 1.0f, 1.0f, 105.0f, 356.0f, 7.0f, true, false, false, false },
	{ "c64.memory_map", 112.0f, 1.0f, 199.0f, 192.0f },
	{ "c64.data_dump", 108.0f, 196.0f, 470.0f, 162.0f, 6.0f }
};

static const SDefaultWorkspaceViewPlacementSpec kC64DebuggerPlacements[] = {
	{ "c64.screen", 180.0f, 10.0f, 257.28f, 182.24f },
	{ "c64.cpu_state", 181.0f, 0.0f, 0.0f, 0.0f },
	{ "c64.disassembly", 1.0f, 1.0f, 175.0f, 356.0f, 7.0f, true, true, true, false },
	{ "c64.data_dump", 178.0f, 195.0f, 470.0f, 165.0f, 5.0f },
	{ "c64.vic_state", 440.0f, 0.0f, 136.0f, 192.0f }
};

static const SDefaultWorkspaceViewPlacementSpec kC64MemoryMapPlacements[] = {
	{ "c64.screen", 420.0f, 10.0f, 157.44f, 111.52f },
	{ "c64.cpu_state", 78.0f, 0.0f, 0.0f, 0.0f },
	{ "c64.disassembly", 0.5f, 0.5f, 75.0f, 359.0f, 5.0f, true, false, false, false },
	{ "c64.memory_map", 77.0f, 15.0f, 340.5f, 340.5f },
	{ "c64.data_dump", 421.0f, 125.0f, 470.0f, 230.0f, 5.0f }
};

static const SDefaultWorkspaceViewPlacementSpec kC64DriveDebuggerPlacements[] = {
	{ "c64.screen", 80.0f, 10.0f, 418.56f, 296.48f },
	{ "c64.cpu_state", 80.0f, 0.0f, 0.0f, 0.0f },
	{ "c64.disassembly", 0.5f, 0.5f, 75.0f, 359.0f, 5.0f, true, false, false, false },
	{ "drive1541.cpu_state", 350.0f, 0.0f, 0.0f, 0.0f },
	{ "drive1541.disassembly", 500.0f, 0.5f, 75.0f, 359.0f, 5.0f, true, false, false, false },
	{ "drive1541.via_state", 342.0f, 310.0f, 240.0f, 50.0f },
	{ "c64.memory_map", 80.25f, 309.25f, 50.0f, 50.0f },
	{ "drive1541.memory_map", 447.75f, 309.25f, 50.0f, 50.0f },
	{ "c64.cia_state", 135.0f, 310.0f, 380.0f, 58.0f }
};

static const SDefaultWorkspaceViewPlacementSpec kC64DriveMemoryMapPlacements[] = {
	{ "c64.screen", 190.0f, 0.0f, 201.6f, 152.8f },
	{ "c64.cpu_state", 0.5f, 0.0f, 178.5f, 10.0f, 3.5f },
	{ "c64.disassembly", 0.5f, 17.0f, 178.5f, 338.0f, 5.0f, true, false, false, false, 0.6f },
	{ "drive1541.cpu_state", 429.0f, 0.0f, 150.0f, 10.0f, 5.0f },
	{ "drive1541.disassembly", 429.0f, 17.0f, 150.0f, 338.0f, 5.0f, true, false, false, false, 0.6f },
	{ "c64.memory_map", 190.0f, 155.0f, 115.0f, 200.0f },
	{ "drive1541.memory_map", 306.0f, 155.0f, 115.0f, 200.0f }
};

static const SDefaultWorkspaceViewPlacementSpec kC64ShowStatesPlacements[] = {
	{ "c64.screen", 317.416f, 10.5f, 259.584f, 183.872f },
	{ "c64.cpu_state", 317.416f, 0.0f, 0.0f, 0.0f },
	{ "c64.vic_state", 13.0f, 13.0f, 290.0f, 160.0f },
	{ "c64.sid_state", 0.0f, 190.0f, 100.0f, 100.0f },
	{ "c64.cia_state", 190.0f, 200.0f, 380.0f, 58.0f },
	{ "c64.reu_state", 315.0f, 315.0f, 380.0f, 58.0f },
	{ "c64.emulation_counters", 496.0f, 335.0f, 380.0f, 58.0f },
	{ "drive1541.via_state", 190.0f, 265.0f, 240.0f, 50.0f },
	{ "c64.emulation_state", 371.0f, 350.0f, 100.0f, 100.0f }
};

static const SDefaultWorkspaceViewPlacementSpec kC64MonitorConsolePlacements[] = {
	{ "c64.screen", 317.416f, 10.5f, 259.584f, 183.872f },
	{ "c64.cpu_state", 317.416f, 0.0f, 0.0f, 0.0f },
	{ "c64.monitor_console", 1.0f, 1.0f, 310.0f, 194.372f },
	{ "c64.disassembly", 1.0f, 195.5f, 125.0f, 159.5f, 5.0f, true, true, true, false },
	{ "c64.data_dump", 128.0f, 195.5f, 252.0f, 165.0f, 5.0f },
	{ "c64.memory_map", 381.0f, 195.5f, 199.0f, 164.0f }
};

static const SDefaultWorkspaceViewPlacementSpec kC64FullScreenZoomPlacements[] = {
	{ "c64.screen", 35.882f, 0.0f, 508.235f, 360.0f }
};

static const SDefaultWorkspaceViewPlacementSpec kC64CyclerPlacements[] = {
	{ "c64.screen", 317.416f, 10.5f, 259.584f, 183.872f },
	{ "c64.cpu_state", 317.416f, 0.0f, 0.0f, 0.0f },
	{ "c64.disassembly", 1.0f, 1.0f, 315.0f, 208.0f, 7.0f, true, true, true, true },
	{ "c64.data_dump", 0.0f, 217.0f, 313.0f, 143.0f, 5.3f },
	{ "c64.vic_state", 320.0f, 330.0f, 256.0f, 28.0f }
};

static const SDefaultWorkspaceViewPlacementSpec kC64VicDisplayLitePlacements[] = {
	{ "c64.disassembly", 1.0f, 1.0f, 180.6f, 356.0f, 7.0f, true, false, true, true },
	{ "c64.cpu_state", 185.5f, 2.5f, 0.0f, 0.0f },
	{ "c64.data_dump", 185.5f, 260.0f, 277.0f, 97.0f, 5.0f },
	{ "c64.screen", 185.5f, 13.0f, 345.6f, 244.8f },
	{ "c64.memory_map", 467.5f, 260.0f, 110.0f, 99.0f }
};

static const SDefaultWorkspaceViewPlacementSpec kC64VicDisplayPlacements[] = {
	{ "c64.screen", 444.6f, 15.0f, 134.4f, 95.2f },
	{ "c64.cpu_state", 354.6f, 2.5f, 0.0f, 0.0f },
	{ "c64.disassembly", 1.0f, 1.0f, 180.6f, 356.0f, 7.0f, true, false, true, true },
	{ "c64.vic_state", 186.5f, 1.0f, 256.5f, 144.0f },
	{ "c64.data_dump", 449.5f, 113.0f, 125.0f, 34.0f, 5.0f },
	{ "c64.vic_display", 185.5f, 150.0f, 329.989f, 204.280f },
	{ "c64.vic_control", 521.5f, 150.0f, 200.0f, 200.0f }
};

static const SDefaultWorkspaceViewPlacementSpec kC64SourceCodePlacements[] = {
	{ "c64.disassembly", 1.0f, 1.0f, 110.6f, 356.0f, 7.0f, true, false, false, false },
	{ "c64.source_code", 114.1f, 15.0f, 463.9f, 341.0f },
	{ "c64.cpu_state", 220.0f, 2.5f, 0.0f, 0.0f },
	{ "c64.screen", 478.0f, 0.0f, 97.92f, 69.36f }
};

static const SDefaultWorkspaceViewPlacementSpec kC64AllGraphicsPlacements[] = {
	{ "c64.all_graphics_bitmaps", 0.0f, 0.0f, 169.0f, 180.0f },
	{ "c64.all_graphics_screens", 169.0f, 0.0f, 169.0f, 180.0f },
	{ "c64.all_graphics_charsets", 0.0f, 180.0f, 169.0f, 180.0f },
	{ "c64.all_graphics_sprites", 169.0f, 180.0f, 169.0f, 180.0f },
	{ "c64.screen", 458.0f, 15.0f, 115.2f, 81.6f }
};

static const SDefaultWorkspaceViewPlacementSpec kC64AllSidsPlacements[] = {
	{ "c64.sid_tracker_history", 0.0f, 0.0f, 350.0f, 126.0f },
	{ "c64.sid_piano_keyboard", 0.0f, 128.0f, 350.0f, 58.0f },
	{ "c64.sid_state", 0.0f, 190.0f, 350.0f, 168.0f },
	{ "c64.cpu_state", 350.0f, 2.5f, 0.0f, 0.0f },
	{ "c64.data_dump", 458.0f, 100.0f, 470.0f, 135.0f, 5.0f },
	{ "c64.memory_map", 442.5f, 239.0f, 130.0f, 119.0f },
	{ "c64.disassembly", 358.0f, 239.0f, 79.0f, 119.0f, 5.0f, true, false, false, false },
	{ "c64.screen", 458.0f, 15.0f, 115.2f, 81.6f }
};

static const SDefaultWorkspaceViewPlacementSpec kC64MemoryDebuggerPlacements[] = {
	{ "c64.cpu_state", 350.0f, 2.5f, 0.0f, 0.0f },
	{ "c64.disassembly", 1.0f, 2.0f, 65.6f, 180.0f, 4.0f, true, false, false, false },
	{ "c64.disassembly2", 1.0f, 186.0f, 65.6f, 167.0f, 4.0f, true, false, false, false },
	{ "c64.data_dump", 67.1f, 15.5f, 387.0f, 169.0f, 5.0f },
	{ "c64.data_dump2", 67.1f, 185.5f, 387.0f, 169.0f, 5.0f },
	{ "c64.data_dump3", 458.0f, 100.0f, 470.0f, 135.0f, 5.0f },
	{ "c64.memory_map", 460.0f, 239.0f, 112.0f, 119.0f },
	{ "c64.screen", 458.0f, 15.0f, 115.2f, 81.6f }
};

static const SDefaultWorkspaceViewPlacementSpec kAtariOnlyPlacements[] = {
	{ "atari.screen", 37.220f, 0.0f, 504.000f, 360.000f }
};

static const SDefaultWorkspaceViewPlacementSpec kAtariDataDumpPlacements[] = {
	{ "atari.screen", 349.864f, 10.5f, 227.136f, 162.240f },
	{ "atari.cpu_state", 349.864f, 0.0f, 0.0f, 0.0f },
	{ "atari.disassembly", 1.0f, 1.0f, 105.0f, 356.0f, 7.0f },
	{ "atari.memory_map", 112.0f, 1.0f, 199.0f, 192.0f },
	{ "atari.data_dump", 108.0f, 196.0f, 470.0f, 162.0f, 6.0f }
};

static const SDefaultWorkspaceViewPlacementSpec kAtariDebuggerPlacements[] = {
	{ "atari.screen", 180.0f, 10.0f, 225.120f, 160.800f },
	{ "atari.cpu_state", 181.0f, 0.0f, 0.0f, 0.0f },
	{ "atari.disassembly", 1.0f, 1.0f, 175.0f, 356.0f, 7.0f },
	{ "atari.data_dump", 178.0f, 195.0f, 470.0f, 165.0f, 5.0f }
};

static const SDefaultWorkspaceViewPlacementSpec kAtariShowStatesPlacements[] = {
	{ "atari.screen", 349.864f, 10.5f, 227.136f, 162.240f },
	{ "atari.cpu_state", 349.864f, 0.0f, 0.0f, 0.0f },
	{ "atari.antic_state", 0.0f, 0.0f, 350.0f, 185.0f },
	{ "atari.gtia_state", 0.0f, 190.0f, 390.0f, 105.0f },
	{ "atari.pia_state", 400.0f, 190.0f, 175.0f, 60.0f },
	{ "atari.pokey_state", 0.0f, 300.0f, 490.0f, 58.0f },
	{ "atari.emulation_counters", 496.0f, 335.0f, 140.0f, 43.0f },
	{ "atari.emulation_state", 371.0f, 350.0f, 100.0f, 100.0f }
};

static const SDefaultWorkspaceViewPlacementSpec kAtariMemoryMapPlacements[] = {
	{ "atari.screen", 420.0f, 10.0f, 137.760f, 98.400f },
	{ "atari.cpu_state", 78.0f, 0.0f, 0.0f, 0.0f },
	{ "atari.disassembly", 0.5f, 0.5f, 75.0f, 359.0f, 5.0f },
	{ "atari.memory_map", 77.0f, 15.0f, 340.5f, 340.5f },
	{ "atari.data_dump", 421.0f, 112.0f, 470.0f, 135.0f, 5.0f },
	{ "atari.gtia_state", 420.0f, 248.0f, 157.0f, 31.0f },
	{ "atari.antic_state", 420.0f, 279.0f, 157.0f, 22.0f },
	{ "atari.pokey_state", 420.0f, 301.0f, 157.0f, 57.0f }
};

static const SDefaultWorkspaceViewPlacementSpec kAtariMonitorConsolePlacements[] = {
	{ "atari.screen", 349.864f, 10.5f, 227.136f, 162.240f },
	{ "atari.cpu_state", 349.864f, 0.0f, 0.0f, 0.0f },
	{ "atari.monitor_console", 1.0f, 1.0f, 310.0f, 193.020f },
	{ "atari.disassembly", 1.0f, 195.5f, 125.0f, 159.5f, 5.0f },
	{ "atari.data_dump", 128.0f, 195.5f, 252.0f, 165.0f, 5.0f },
	{ "atari.memory_map", 381.0f, 195.5f, 199.0f, 164.0f }
};

static const SDefaultWorkspaceViewPlacementSpec kAtariCyclerPlacements[] = {
	{ "atari.screen", 349.864f, 10.5f, 227.136f, 162.240f },
	{ "atari.cpu_state", 349.864f, 0.0f, 0.0f, 0.0f },
	{ "atari.disassembly", 1.0f, 1.0f, 315.0f, 208.0f, 7.0f },
	{ "atari.data_dump", 0.0f, 217.0f, 353.0f, 143.0f, 5.3f },
	{ "atari.memory_map", 365.0f, 180.5f, 195.0f, 174.0f }
};

static const SDefaultWorkspaceViewPlacementSpec kAtariDisplayLitePlacements[] = {
	{ "atari.disassembly", 1.0f, 1.0f, 180.6f, 356.0f, 7.0f },
	{ "atari.screen", 185.5f, 13.0f, 302.4f, 216.0f },
	{ "atari.cpu_state", 185.5f, 2.5f, 0.0f, 0.0f },
	{ "atari.data_dump", 185.5f, 230.0f, 277.0f, 131.0f, 5.0f },
	{ "atari.memory_map", 465.5f, 230.0f, 112.0f, 130.0f }
};

static const SDefaultWorkspaceViewPlacementSpec kAtariSourceCodePlacements[] = {
	{ "atari.disassembly", 1.0f, 1.0f, 110.6f, 356.0f, 7.0f },
	{ "atari.source_code", 114.1f, 15.0f, 463.9f, 341.0f },
	{ "atari.cpu_state", 220.0f, 2.5f, 0.0f, 0.0f },
	{ "atari.screen", 478.0f, 0.0f, 85.680f, 61.200f }
};

static const SDefaultWorkspaceViewPlacementSpec kNesOnlyPlacements[] = {
	{ "nes.screen", 97.220f, 0.0f, 384.000f, 360.000f }
};

static const SDefaultWorkspaceViewPlacementSpec kNesDataDumpPlacements[] = {
	{ "nes.screen", 348.863f, 10.5f, 195.553f, 183.331f },
	{ "nes.cpu_state", 348.863f, 0.0f, 0.0f, 0.0f },
	{ "nes.disassembly", 1.0f, 1.0f, 105.0f, 356.0f, 7.0f },
	{ "nes.memory_map", 112.0f, 1.0f, 199.0f, 192.0f },
	{ "nes.data_dump", 108.0f, 196.0f, 470.0f, 162.0f, 6.0f },
	{ "nes.emulation_counters", 496.0f, 335.0f, 120.0f, 43.0f }
};

static const SDefaultWorkspaceViewPlacementSpec kNesDebuggerPlacements[] = {
	{ "nes.screen", 180.0f, 10.0f, 171.520f, 160.800f },
	{ "nes.cpu_state", 181.0f, 0.0f, 0.0f, 0.0f },
	{ "nes.disassembly", 1.0f, 1.0f, 175.0f, 356.0f, 7.0f },
	{ "nes.data_dump", 178.0f, 195.0f, 470.0f, 165.0f, 5.0f },
	{ "nes.emulation_counters", 496.0f, 335.0f, 120.0f, 43.0f }
};

static const SDefaultWorkspaceViewPlacementSpec kNesApuPlacements[] = {
	{ "nes.screen", 403.944f, 10.5f, 173.056f, 162.240f },
	{ "nes.cpu_state", 403.944f, 0.0f, 0.0f, 0.0f },
	{ "nes.piano_keyboard", 0.0f, 122.0f, 393.0f, 50.0f },
	{ "nes.apu_state", 0.0f, 175.0f, 490.0f, 160.0f },
	{ "nes.emulation_counters", 496.0f, 335.0f, 120.0f, 43.0f }
};

static const SDefaultWorkspaceViewPlacementSpec kNesShowStatesPlacements[] = {
	{ "nes.screen", 403.944f, 10.5f, 173.056f, 162.240f },
	{ "nes.cpu_state", 373.944f, 0.0f, 0.0f, 0.0f },
	{ "nes.disassembly", 1.0f, 1.0f, 130.0f, 175.0f, 5.0f },
	{ "nes.data_dump", 1.0f, 177.0f, 130.0f, 135.0f, 5.0f },
	{ "nes.ppu_state", 1.0f, 317.0f, 130.0f, 40.0f },
	{ "nes.ppu_nametables", 135.0f, 1.0f, 173.056f, 173.056f },
	{ "nes.ppu_nametable_data_dump", 135.0f, 177.0f, 425.0f, 135.0f },
	{ "nes.ppu_palette", 135.0f, 317.0f, 280.0f, 30.0f },
	{ "nes.emulation_counters", 496.0f, 335.0f, 120.0f, 43.0f }
};

static const SDefaultWorkspaceViewPlacementSpec kNesMemoryMapPlacements[] = {
	{ "nes.screen", 420.0f, 10.0f, 104.960f, 98.400f },
	{ "nes.cpu_state", 78.0f, 0.0f, 0.0f, 0.0f },
	{ "nes.disassembly", 0.5f, 0.5f, 75.0f, 359.0f, 5.0f },
	{ "nes.memory_map", 77.0f, 15.0f, 340.5f, 340.5f },
	{ "nes.data_dump", 421.0f, 125.0f, 470.0f, 230.0f, 5.0f }
};

static const SDefaultWorkspaceViewPlacementSpec kNesMonitorConsolePlacements[] = {
	{ "nes.screen", 403.944f, 10.5f, 173.056f, 162.240f },
	{ "nes.cpu_state", 403.944f, 0.0f, 0.0f, 0.0f },
	{ "nes.monitor_console", 1.0f, 1.0f, 310.0f, 193.020f },
	{ "nes.disassembly", 1.0f, 195.5f, 125.0f, 159.5f, 5.0f },
	{ "nes.data_dump", 128.0f, 195.5f, 252.0f, 165.0f, 5.0f },
	{ "nes.memory_map", 381.0f, 195.5f, 199.0f, 164.0f }
};

static const std::vector<SDefaultWorkspaceLayoutSpec> kC64DefaultWorkspaceSpecs = {
	{ DefaultWorkspacePlatform_C64, DefaultWorkspaceSlot_Only, "C64 Only", "retro.default.c64.only", kC64OnlyPlacements, sizeof(kC64OnlyPlacements) / sizeof(kC64OnlyPlacements[0]) },
	{ DefaultWorkspacePlatform_C64, DefaultWorkspaceSlot_DataDump, "C64 Data Dump", "retro.default.c64.data_dump", kC64DataDumpPlacements, sizeof(kC64DataDumpPlacements) / sizeof(kC64DataDumpPlacements[0]) },
	{ DefaultWorkspacePlatform_C64, DefaultWorkspaceSlot_Debugger, "C64 Debugger", "retro.default.c64.debugger", kC64DebuggerPlacements, sizeof(kC64DebuggerPlacements) / sizeof(kC64DebuggerPlacements[0]) },
	{ DefaultWorkspacePlatform_C64, DefaultWorkspaceSlot_1541MemoryMap, "C64 1541 Memory Map", "retro.default.c64.1541_memory_map", kC64DriveMemoryMapPlacements, sizeof(kC64DriveMemoryMapPlacements) / sizeof(kC64DriveMemoryMapPlacements[0]) },
	{ DefaultWorkspacePlatform_C64, DefaultWorkspaceSlot_ShowStates, "C64 Show States", "retro.default.c64.show_states", kC64ShowStatesPlacements, sizeof(kC64ShowStatesPlacements) / sizeof(kC64ShowStatesPlacements[0]) },
	{ DefaultWorkspacePlatform_C64, DefaultWorkspaceSlot_MemoryMap, "C64 Memory Map", "retro.default.c64.memory_map", kC64MemoryMapPlacements, sizeof(kC64MemoryMapPlacements) / sizeof(kC64MemoryMapPlacements[0]) },
	{ DefaultWorkspacePlatform_C64, DefaultWorkspaceSlot_1541Debugger, "C64 1541 Debugger", "retro.default.c64.1541_debugger", kC64DriveDebuggerPlacements, sizeof(kC64DriveDebuggerPlacements) / sizeof(kC64DriveDebuggerPlacements[0]) },
	{ DefaultWorkspacePlatform_C64, DefaultWorkspaceSlot_MonitorConsole, "C64 Monitor Console", "retro.default.c64.monitor_console", kC64MonitorConsolePlacements, sizeof(kC64MonitorConsolePlacements) / sizeof(kC64MonitorConsolePlacements[0]) },
	{ DefaultWorkspacePlatform_C64, DefaultWorkspaceSlot_FullScreenZoom, "C64 Full Screen Zoom", "retro.default.c64.full_screen_zoom", kC64FullScreenZoomPlacements, sizeof(kC64FullScreenZoomPlacements) / sizeof(kC64FullScreenZoomPlacements[0]) },
	{ DefaultWorkspacePlatform_C64, DefaultWorkspaceSlot_Cycler, "C64 Cycler", "retro.default.c64.cycler", kC64CyclerPlacements, sizeof(kC64CyclerPlacements) / sizeof(kC64CyclerPlacements[0]) },
	{ DefaultWorkspacePlatform_C64, DefaultWorkspaceSlot_VicDisplayLite, "C64 VIC Display Lite", "retro.default.c64.vic_display_lite", kC64VicDisplayLitePlacements, sizeof(kC64VicDisplayLitePlacements) / sizeof(kC64VicDisplayLitePlacements[0]) },
	{ DefaultWorkspacePlatform_C64, DefaultWorkspaceSlot_VicDisplay, "C64 VIC Display", "retro.default.c64.vic_display", kC64VicDisplayPlacements, sizeof(kC64VicDisplayPlacements) / sizeof(kC64VicDisplayPlacements[0]) },
	{ DefaultWorkspacePlatform_C64, DefaultWorkspaceSlot_SourceCode, "C64 Source Code", "retro.default.c64.source_code", kC64SourceCodePlacements, sizeof(kC64SourceCodePlacements) / sizeof(kC64SourceCodePlacements[0]) },
	{ DefaultWorkspacePlatform_C64, DefaultWorkspaceSlot_AllGraphics, "C64 All Graphics", "retro.default.c64.all_graphics", kC64AllGraphicsPlacements, sizeof(kC64AllGraphicsPlacements) / sizeof(kC64AllGraphicsPlacements[0]) },
	{ DefaultWorkspacePlatform_C64, DefaultWorkspaceSlot_AllSids, "C64 All SIDs", "retro.default.c64.all_sids", kC64AllSidsPlacements, sizeof(kC64AllSidsPlacements) / sizeof(kC64AllSidsPlacements[0]) },
	{ DefaultWorkspacePlatform_C64, DefaultWorkspaceSlot_MemoryDebugger, "C64 Memory Debugger", "retro.default.c64.memory_debugger", kC64MemoryDebuggerPlacements, sizeof(kC64MemoryDebuggerPlacements) / sizeof(kC64MemoryDebuggerPlacements[0]) }
};

static const std::vector<SDefaultWorkspaceLayoutSpec> kAtariDefaultWorkspaceSpecs = {
	{ DefaultWorkspacePlatform_Atari800, DefaultWorkspaceSlot_Only, "Atari800 Only", "retro.default.atari800.only", kAtariOnlyPlacements, sizeof(kAtariOnlyPlacements) / sizeof(kAtariOnlyPlacements[0]) },
	{ DefaultWorkspacePlatform_Atari800, DefaultWorkspaceSlot_DataDump, "Atari800 Data Dump", "retro.default.atari800.data_dump", kAtariDataDumpPlacements, sizeof(kAtariDataDumpPlacements) / sizeof(kAtariDataDumpPlacements[0]) },
	{ DefaultWorkspacePlatform_Atari800, DefaultWorkspaceSlot_Debugger, "Atari800 Debugger", "retro.default.atari800.debugger", kAtariDebuggerPlacements, sizeof(kAtariDebuggerPlacements) / sizeof(kAtariDebuggerPlacements[0]) },
	{ DefaultWorkspacePlatform_Atari800, DefaultWorkspaceSlot_ShowStates, "Atari800 Show States", "retro.default.atari800.show_states", kAtariShowStatesPlacements, sizeof(kAtariShowStatesPlacements) / sizeof(kAtariShowStatesPlacements[0]) },
	{ DefaultWorkspacePlatform_Atari800, DefaultWorkspaceSlot_MemoryMap, "Atari800 Memory Map", "retro.default.atari800.memory_map", kAtariMemoryMapPlacements, sizeof(kAtariMemoryMapPlacements) / sizeof(kAtariMemoryMapPlacements[0]) },
	{ DefaultWorkspacePlatform_Atari800, DefaultWorkspaceSlot_MonitorConsole, "Atari800 Monitor Console", "retro.default.atari800.monitor_console", kAtariMonitorConsolePlacements, sizeof(kAtariMonitorConsolePlacements) / sizeof(kAtariMonitorConsolePlacements[0]) },
	{ DefaultWorkspacePlatform_Atari800, DefaultWorkspaceSlot_Cycler, "Atari800 Cycler", "retro.default.atari800.cycler", kAtariCyclerPlacements, sizeof(kAtariCyclerPlacements) / sizeof(kAtariCyclerPlacements[0]) },
	{ DefaultWorkspacePlatform_Atari800, DefaultWorkspaceSlot_VicDisplayLite, "Atari800 Display Lite", "retro.default.atari800.display_lite", kAtariDisplayLitePlacements, sizeof(kAtariDisplayLitePlacements) / sizeof(kAtariDisplayLitePlacements[0]) },
	{ DefaultWorkspacePlatform_Atari800, DefaultWorkspaceSlot_SourceCode, "Atari800 Source Code", "retro.default.atari800.source_code", kAtariSourceCodePlacements, sizeof(kAtariSourceCodePlacements) / sizeof(kAtariSourceCodePlacements[0]) }
};

static const std::vector<SDefaultWorkspaceLayoutSpec> kNesDefaultWorkspaceSpecs = {
	{ DefaultWorkspacePlatform_NES, DefaultWorkspaceSlot_Only, "NES Only", "retro.default.nes.only", kNesOnlyPlacements, sizeof(kNesOnlyPlacements) / sizeof(kNesOnlyPlacements[0]) },
	{ DefaultWorkspacePlatform_NES, DefaultWorkspaceSlot_DataDump, "NES Data Dump", "retro.default.nes.data_dump", kNesDataDumpPlacements, sizeof(kNesDataDumpPlacements) / sizeof(kNesDataDumpPlacements[0]) },
	{ DefaultWorkspacePlatform_NES, DefaultWorkspaceSlot_Debugger, "NES Debugger", "retro.default.nes.debugger", kNesDebuggerPlacements, sizeof(kNesDebuggerPlacements) / sizeof(kNesDebuggerPlacements[0]) },
	{ DefaultWorkspacePlatform_NES, DefaultWorkspaceSlot_1541MemoryMap, "NES APU", "retro.default.nes.apu", kNesApuPlacements, sizeof(kNesApuPlacements) / sizeof(kNesApuPlacements[0]) },
	{ DefaultWorkspacePlatform_NES, DefaultWorkspaceSlot_ShowStates, "NES Show States", "retro.default.nes.show_states", kNesShowStatesPlacements, sizeof(kNesShowStatesPlacements) / sizeof(kNesShowStatesPlacements[0]) },
	{ DefaultWorkspacePlatform_NES, DefaultWorkspaceSlot_MemoryMap, "NES Memory Map", "retro.default.nes.memory_map", kNesMemoryMapPlacements, sizeof(kNesMemoryMapPlacements) / sizeof(kNesMemoryMapPlacements[0]) },
	{ DefaultWorkspacePlatform_NES, DefaultWorkspaceSlot_MonitorConsole, "NES Monitor Console", "retro.default.nes.monitor_console", kNesMonitorConsolePlacements, sizeof(kNesMonitorConsolePlacements) / sizeof(kNesMonitorConsolePlacements[0]) }
};

static const std::vector<SDefaultWorkspaceShortcutSlotSpec> kDefaultWorkspaceShortcutSlotSpecs = {
	{ DefaultWorkspaceSlot_Only, "Only", "Ctrl+F1", MTKEY_F1, false, false, true, false },
	{ DefaultWorkspaceSlot_DataDump, "Data Dump", "Ctrl+F2", MTKEY_F2, false, false, true, false },
	{ DefaultWorkspaceSlot_Debugger, "Debugger", "Ctrl+F3", MTKEY_F3, false, false, true, false },
	{ DefaultWorkspaceSlot_1541MemoryMap, "1541 Memory Map / APU", "Ctrl+F4", MTKEY_F4, false, false, true, false },
	{ DefaultWorkspaceSlot_ShowStates, "Show States", "Ctrl+F5", MTKEY_F5, false, false, true, false },
	{ DefaultWorkspaceSlot_MemoryMap, "Memory Map", "Ctrl+F6", MTKEY_F6, false, false, true, false },
	{ DefaultWorkspaceSlot_1541Debugger, "1541 Debugger", "Ctrl+F7", MTKEY_F7, false, false, true, false },
	{ DefaultWorkspaceSlot_MonitorConsole, "Monitor Console", "Ctrl+F8", MTKEY_F8, false, false, true, false },
	{ DefaultWorkspaceSlot_FullScreenZoom, "Full Screen Zoom", "Ctrl+Shift+F1", MTKEY_F1, true, false, true, false },
	{ DefaultWorkspaceSlot_Cycler, "Cycler", "Ctrl+Shift+F2", MTKEY_F2, true, false, true, false },
	{ DefaultWorkspaceSlot_VicDisplayLite, "Display Lite", "Ctrl+Shift+F4", MTKEY_F4, true, false, true, false },
	{ DefaultWorkspaceSlot_VicDisplay, "VIC Display", "Ctrl+Shift+F5", MTKEY_F5, true, false, true, false },
	{ DefaultWorkspaceSlot_SourceCode, "Source Code", "Ctrl+Shift+F3", MTKEY_F3, true, false, true, false },
	{ DefaultWorkspaceSlot_AllGraphics, "All Graphics", "Ctrl+Shift+F7", MTKEY_F7, true, false, true, false },
	{ DefaultWorkspaceSlot_AllSids, "All SIDs", "Ctrl+Shift+F8", MTKEY_F8, true, false, true, false },
	{ DefaultWorkspaceSlot_MemoryDebugger, "Memory Debugger", "Ctrl+Shift+F9", MTKEY_F9, true, false, true, false }
};

static const std::vector<SDefaultWorkspaceLayoutSpec> kEmptyDefaultWorkspaceSpecs;

static const char *kDefaultWorkspaceShortcutSlotsEnabledConfigKey = "DefaultWorkspaceContextShortcutsEnabled";
static const char *kDefaultWorkspaceShortcutSlotsConfigKey = "DefaultWorkspaceShortcutSlots";
static const char *kDefaultWorkspaceShortcutSlotsVersionConfigKey = "DefaultWorkspaceShortcutSlotsVersion";

static int GetHjsonInt(Hjson::Value hjsonValue, const char *name, int defaultValue)
{
	Hjson::Value hValue = hjsonValue[name];
	if (hValue == Hjson::Type::Undefined)
		return defaultValue;

	try
	{
		return static_cast<const int>(hValue);
	}
	catch (const std::exception &)
	{
		return defaultValue;
	}
}

static bool GetHjsonBool(Hjson::Value hjsonValue, const char *name, bool defaultValue)
{
	Hjson::Value hValue = hjsonValue[name];
	if (hValue == Hjson::Type::Undefined)
		return defaultValue;

	try
	{
		return static_cast<const bool>(hValue);
	}
	catch (const std::exception &)
	{
		return defaultValue;
	}
}

static bool SaveDefaultWorkspaceShortcutConfig(CConfigStorageHjson *config)
{
	if (config == NULL)
		return false;

	if (config->isFromSettings)
		return config->SaveConfig();

	std::stringstream ss;
	ss << Hjson::Marshal(config->hjsonRoot);
	std::string serializedConfig = ss.str();
	const char *cstrConfig = serializedConfig.c_str();

	CByteBuffer byteBuffer;
	byteBuffer.PutBytes((u8 *)cstrConfig, (unsigned int)strlen(cstrConfig));
	return byteBuffer.storeToFile(config->configFileName);
}

static void HideDebugInterfaceViews(CDebugInterface *debugInterface)
{
	if (debugInterface == NULL)
		return;

	for (std::list<CGuiView *>::iterator it = debugInterface->views.begin(); it != debugInterface->views.end(); ++it)
	{
		if (*it != NULL)
		{
			(*it)->SetVisible(false);
		}
	}
}

static void HideDefaultWorkspacePlatformViews(CViewC64 *viewC64)
{
	if (viewC64 == NULL)
		return;

	HideDebugInterfaceViews(viewC64->debugInterfaceC64);
	HideDebugInterfaceViews(viewC64->debugInterfaceAtari);
	HideDebugInterfaceViews(viewC64->debugInterfaceNes);
	if (guiViewDebugLog != NULL)
	{
		guiViewDebugLog->SetVisible(false);
	}
}

static void UndockHiddenDebugInterfaceViews(CDebugInterface *debugInterface)
{
	if (debugInterface == NULL)
		return;

	for (std::list<CGuiView *>::iterator it = debugInterface->views.begin(); it != debugInterface->views.end(); ++it)
	{
		CGuiView *view = *it;
		if (view != NULL && !view->visible && view->imGuiWindow != NULL)
		{
			view->imGuiWindow->DockId = 0;
		}
	}
}

static void UndockHiddenDefaultWorkspacePlatformViews(CViewC64 *viewC64)
{
	if (viewC64 == NULL)
		return;

	UndockHiddenDebugInterfaceViews(viewC64->debugInterfaceC64);
	UndockHiddenDebugInterfaceViews(viewC64->debugInterfaceAtari);
	UndockHiddenDebugInterfaceViews(viewC64->debugInterfaceNes);
	if (guiViewDebugLog != NULL && !guiViewDebugLog->visible && guiViewDebugLog->imGuiWindow != NULL)
	{
		guiViewDebugLog->imGuiWindow->DockId = 0;
	}
}

static void SetLayoutString(char **target, const char *value)
{
	if (*target != NULL)
	{
		STRFREE(*target);
		*target = NULL;
	}
	if (value != NULL)
	{
		*target = STRALLOC(value);
	}
}

static void SetLayoutName(CLayoutManager *layoutManager, CLayoutData *layoutData, const char *layoutName)
{
	if (layoutManager == NULL || layoutData == NULL || layoutName == NULL)
		return;

	if (layoutData->layoutName != NULL)
	{
		layoutManager->layoutsByHash.erase(GetHashCode64(layoutData->layoutName));
	}
	SetLayoutString(&layoutData->layoutName, layoutName);
	layoutManager->layoutsByHash[GetHashCode64(layoutData->layoutName)] = layoutData;
}

static void ClearLayoutShortcut(CLayoutManager *layoutManager, CLayoutData *layoutData)
{
	if (layoutData == NULL || layoutData->keyShortcut == NULL)
		return;

	if (layoutManager != NULL && layoutManager->guiMain != NULL)
	{
		layoutManager->guiMain->RemoveKeyboardShortcut(layoutData->keyShortcut);
	}
	delete layoutData->keyShortcut;
	layoutData->keyShortcut = NULL;
}

static std::vector<SDefaultWorkspaceLayoutSpec> BuildAllDefaultWorkspaceSpecs()
{
	std::vector<SDefaultWorkspaceLayoutSpec> allSpecs;
	allSpecs.reserve(kC64DefaultWorkspaceSpecs.size() + kAtariDefaultWorkspaceSpecs.size() + kNesDefaultWorkspaceSpecs.size());
	allSpecs.insert(allSpecs.end(), kC64DefaultWorkspaceSpecs.begin(), kC64DefaultWorkspaceSpecs.end());
	allSpecs.insert(allSpecs.end(), kAtariDefaultWorkspaceSpecs.begin(), kAtariDefaultWorkspaceSpecs.end());
	allSpecs.insert(allSpecs.end(), kNesDefaultWorkspaceSpecs.begin(), kNesDefaultWorkspaceSpecs.end());
	return allSpecs;
}

const std::vector<SDefaultWorkspaceLayoutSpec> &GetDefaultWorkspaceLayoutSpecs(EDefaultWorkspacePlatform platform)
{
	switch (platform)
	{
		case DefaultWorkspacePlatform_C64:
			return kC64DefaultWorkspaceSpecs;
		case DefaultWorkspacePlatform_Atari800:
			return kAtariDefaultWorkspaceSpecs;
		case DefaultWorkspacePlatform_NES:
			return kNesDefaultWorkspaceSpecs;
	}

	return kEmptyDefaultWorkspaceSpecs;
}

const std::vector<SDefaultWorkspaceLayoutSpec> &GetAllDefaultWorkspaceLayoutSpecs()
{
	static const std::vector<SDefaultWorkspaceLayoutSpec> allSpecs = BuildAllDefaultWorkspaceSpecs();
	return allSpecs;
}

const SDefaultWorkspaceLayoutSpec *FindDefaultWorkspaceLayoutSpec(EDefaultWorkspacePlatform platform, EDefaultWorkspaceSlot slot)
{
	const std::vector<SDefaultWorkspaceLayoutSpec> &specs = GetDefaultWorkspaceLayoutSpecs(platform);
	for (std::vector<SDefaultWorkspaceLayoutSpec>::const_iterator it = specs.begin(); it != specs.end(); ++it)
	{
		if (it->slot == slot)
			return &(*it);
	}

	return NULL;
}

const std::vector<SDefaultWorkspaceShortcutSlotSpec> &GetDefaultWorkspaceShortcutSlotSpecs()
{
	return kDefaultWorkspaceShortcutSlotSpecs;
}

const SDefaultWorkspaceShortcutSlotSpec *FindDefaultWorkspaceShortcutSlotSpec(EDefaultWorkspaceSlot slot)
{
	for (std::vector<SDefaultWorkspaceShortcutSlotSpec>::const_iterator it = kDefaultWorkspaceShortcutSlotSpecs.begin(); it != kDefaultWorkspaceShortcutSlotSpecs.end(); ++it)
	{
		if (it->slot == slot)
			return &(*it);
	}

	return NULL;
}

static float GetDefaultWorkspaceScaleX(float viewportWidth)
{
	return viewportWidth / kDefaultWorkspaceReferenceWidth;
}

static float GetDefaultWorkspaceScaleY(float viewportHeight)
{
	return viewportHeight / kDefaultWorkspaceReferenceHeight;
}

static float GetDefaultWorkspaceUniformScale(float viewportWidth, float viewportHeight)
{
	float scaleX = GetDefaultWorkspaceScaleX(viewportWidth);
	float scaleY = GetDefaultWorkspaceScaleY(viewportHeight);
	return scaleX < scaleY ? scaleX : scaleY;
}

SDefaultWorkspaceViewPlacementSpec ScaleDefaultWorkspacePlacement(const SDefaultWorkspaceViewPlacementSpec &placement, float viewportWidth, float viewportHeight)
{
	float scaleX = GetDefaultWorkspaceScaleX(viewportWidth);
	float scaleY = GetDefaultWorkspaceScaleY(viewportHeight);
	float uniformScale = scaleX < scaleY ? scaleX : scaleY;
	SDefaultWorkspaceViewPlacementSpec scaledPlacement = placement;
	scaledPlacement.x = placement.x * scaleX;
	scaledPlacement.y = placement.y * scaleY;
	scaledPlacement.width = placement.width * scaleX;
	scaledPlacement.height = placement.height * scaleY;
	scaledPlacement.fontSize = placement.fontSize * uniformScale;
	if (scaledPlacement.width < kDefaultWorkspaceMinimumWindowSize)
		scaledPlacement.width = kDefaultWorkspaceMinimumWindowSize;
	if (scaledPlacement.height < kDefaultWorkspaceMinimumWindowSize)
		scaledPlacement.height = kDefaultWorkspaceMinimumWindowSize;
	return scaledPlacement;
}

static float GetDefaultWorkspaceLayoutParameterUnits(CViewDisassembly *view)
{
	if (view == NULL)
		return 0.0f;

	float fontSize = view->fontSize > 0.01f ? view->fontSize : 1.0f;
	float units = 0.0f;
	if (view->showLabels)
		units += (float)view->labelNumCharacters;

	// Address plus one-column gap. Current address adapters usually render 4 hex digits.
	units += 5.0f;
	float prefixUnits = units;
	if (view->showHexCodes)
	{
		// Three opcode bytes occupy 9 columns; keep one column for the illegal opcode marker/gap.
		prefixUnits += 10.0f;
		units = prefixUnits;
	}

	float maxUnits = units;
	if (view->showCodeCycles)
	{
		float codeCycleUnits = prefixUnits + view->codeCyclesOffsetX / fontSize + 2.0f;
		if (codeCycleUnits > maxUnits)
			maxUnits = codeCycleUnits;
	}

	float mnemonicUnits = prefixUnits + view->mnemonicsOffsetX / fontSize + 10.0f;
	if (mnemonicUnits > maxUnits)
		maxUnits = mnemonicUnits;

	return maxUnits > 1.0f ? maxUnits : 1.0f;
}

static float GetDefaultWorkspaceDisassemblyFontSize(CViewDisassembly *view, const SDefaultWorkspaceViewPlacementSpec &scaledPlacement, float uniformScale)
{
	float fontSize = scaledPlacement.fontSize;
	if (view == NULL || fontSize <= 0.0f)
		return fontSize;

	view->fontSize = fontSize;
	view->LayoutParameterChanged(NULL);
	float units = GetDefaultWorkspaceLayoutParameterUnits(view);
	float fittedFontSize = scaledPlacement.width / units;
	if (scaledPlacement.disassemblyFontFitScale > 0.0f)
		fittedFontSize *= scaledPlacement.disassemblyFontFitScale;
	float minimumFontSize = kDefaultWorkspaceMinimumReadableFontSize * uniformScale;
	if (fittedFontSize < minimumFontSize)
		fittedFontSize = minimumFontSize;
	return fittedFontSize;
}

static float GetDefaultWorkspaceDataDumpWidth(CViewDataDump *view, int bytesPerLine, float fontSize)
{
	int addressDigits = 4;
	if (view != NULL && view->dataAddressEditBox != NULL)
		addressDigits = view->dataAddressEditBox->GetNumDigits();

	float fontBytesSize = view != NULL ? view->fontBytesSize : fontSize;
	float gapAddress = view != NULL ? view->gapAddress : fontSize;
	float gapHexData = view != NULL ? view->gapHexData : 0.5f * fontSize;
	float gapDataCharacters = view != NULL ? view->gapDataCharacters : 0.5f * fontSize;
	float fontCharactersWidth = view != NULL ? view->fontCharactersWidth : 0.84f * fontSize;
	float width = (float)addressDigits * fontBytesSize;
	width += gapAddress;
	width += (float)bytesPerLine * (2.0f * fontBytesSize + gapHexData);

	bool showDataCharacters = view == NULL || view->showDataCharacters;
	bool showCharacters = view != NULL && view->showCharacters;
	bool showSprites = view != NULL && view->showSprites;
	if (showDataCharacters)
	{
		width += gapDataCharacters;
		width += (float)bytesPerLine * fontCharactersWidth;
	}
	if (showCharacters)
	{
		const float characterPreviewGap = 5.0f;
		const float characterPreviewSize = 32.0f;
		const float characterPreviewAdvance = 40.0f;
		width += characterPreviewGap;
		width += showSprites ? characterPreviewAdvance : characterPreviewSize;
	}
	else if (showSprites)
	{
		width += 5.0f;
	}
	if (showSprites)
	{
		const float spritePreviewWidth = 24.0f * 1.9f;
		width += spritePreviewWidth;
	}
	return width;
}

static float GetDefaultWorkspaceDataDumpFitWidth(const SDefaultWorkspaceViewPlacementSpec &scaledPlacement, float viewportWidth)
{
	float fitWidth = scaledPlacement.width;
	if (scaledPlacement.x + fitWidth > viewportWidth)
		fitWidth = viewportWidth - scaledPlacement.x;
	return fitWidth > 0.0f ? fitWidth : 0.0f;
}

static int GetDefaultWorkspaceDataDumpBytesPerLine(CViewDataDump *view, const SDefaultWorkspaceViewPlacementSpec &scaledPlacement, float uniformScale, float fitWidth)
{
	int bytesPerLine = 8;
	while (bytesPerLine > 1 && GetDefaultWorkspaceDataDumpWidth(view, bytesPerLine, scaledPlacement.fontSize) > fitWidth)
	{
		bytesPerLine /= 2;
	}

	float minimumFontSize = kDefaultWorkspaceMinimumReadableFontSize * uniformScale;
	if (scaledPlacement.fontSize < minimumFontSize)
		return bytesPerLine;

	while (bytesPerLine < 256)
	{
		int nextBytesPerLine = bytesPerLine * 2;
		float requiredWidth = GetDefaultWorkspaceDataDumpWidth(view, nextBytesPerLine, scaledPlacement.fontSize);
		if (requiredWidth > fitWidth)
			break;
		bytesPerLine = nextBytesPerLine;
	}
	return bytesPerLine;
}

static CLayoutParameterFloat *FindDefaultWorkspaceFloatLayoutParameter(CGuiView *view, const char *name)
{
	if (view == NULL || name == NULL)
		return NULL;

	for (std::list<CLayoutParameter *>::iterator it = view->layoutParameters.begin(); it != view->layoutParameters.end(); ++it)
	{
		if ((*it) != NULL && strcmp((*it)->name, name) == 0)
			return dynamic_cast<CLayoutParameterFloat *>(*it);
	}
	return NULL;
}

static void ApplyDefaultWorkspaceViewLayoutParameters(CGuiView *view, const SDefaultWorkspaceViewPlacementSpec &scaledPlacement, float uniformScale, float viewportWidth)
{
	if (view == NULL || scaledPlacement.fontSize <= 0.0f)
		return;

	CViewDisassembly *disassemblyView = dynamic_cast<CViewDisassembly *>(view);
	if (disassemblyView != NULL)
	{
		if (scaledPlacement.hasDisassemblyParameters)
		{
			disassemblyView->showHexCodes = scaledPlacement.disassemblyShowHexCodes;
			disassemblyView->showCodeCycles = scaledPlacement.disassemblyShowCodeCycles;
			disassemblyView->showLabels = scaledPlacement.disassemblyShowLabels;
			disassemblyView->LayoutParameterChanged(NULL);
		}
		disassemblyView->fontSize = GetDefaultWorkspaceDisassemblyFontSize(disassemblyView, scaledPlacement, uniformScale);
		disassemblyView->LayoutParameterChanged(NULL);
		return;
	}

	CViewDataDump *dataDumpView = dynamic_cast<CViewDataDump *>(view);
	if (dataDumpView != NULL)
	{
		dataDumpView->fontSize = scaledPlacement.fontSize;
		dataDumpView->LayoutParameterChanged(NULL);
		float fitWidth = GetDefaultWorkspaceDataDumpFitWidth(scaledPlacement, viewportWidth);
		dataDumpView->numberOfBytesPerLine = GetDefaultWorkspaceDataDumpBytesPerLine(dataDumpView, scaledPlacement, uniformScale, fitWidth);
		return;
	}

	CLayoutParameterFloat *fontSizeParameter = FindDefaultWorkspaceFloatLayoutParameter(view, "Font Size");
	if (fontSizeParameter != NULL && fontSizeParameter->value != NULL)
	{
		*fontSizeParameter->value = scaledPlacement.fontSize;
		view->LayoutParameterChanged(fontSizeParameter);
	}
}

// CGuiView stores ImGui InnerRect size minus one pixel, so queue one extra
// pixel to preserve placement dimensions as the view body size.
static const float kDefaultWorkspaceImGuiInnerRectSizeCompensation = 1.0f;

static float GetDefaultWorkspaceFloatingWindowHeightForInterior(float interiorHeight)
{
	return interiorHeight + ImGui::GetFrameHeight() + kDefaultWorkspaceImGuiInnerRectSizeCompensation;
}

static float GetDefaultWorkspaceFloatingWindowWidthForInterior(float interiorWidth)
{
	return interiorWidth + kDefaultWorkspaceImGuiInnerRectSizeCompensation;
}

CDefaultWorkspaceLayouts::CDefaultWorkspaceLayouts(CViewC64 *viewC64, CLayoutManager *layoutManager)
{
	this->viewC64 = viewC64;
	this->layoutManager = layoutManager;
	this->registeredKeyboardShortcuts = NULL;
	this->generationStep = DefaultWorkspaceGenerationStep_Idle;
	this->generationLockedDefaultLayouts = true;
	this->generationNoTabBarsInGeneratedLayouts = true;
	this->generationSpecIndex = 0;
	this->generationWaitFrames = 0;
	this->generationOriginalLayout = NULL;
	this->generationCurrentLayoutData = NULL;
	this->generationOriginalC64Running = false;
	this->generationOriginalAtariRunning = false;
	this->generationOriginalNesRunning = false;

	const std::vector<SDefaultWorkspaceShortcutSlotSpec> &shortcutSpecs = GetDefaultWorkspaceShortcutSlotSpecs();
	shortcutSlotStates.reserve(shortcutSpecs.size());
	for (std::vector<SDefaultWorkspaceShortcutSlotSpec>::const_iterator it = shortcutSpecs.begin(); it != shortcutSpecs.end(); ++it)
	{
		SDefaultWorkspaceShortcutSlotState state;
		state.slot = it->slot;
		shortcutSlotStates.push_back(state);
	}
}

CDefaultWorkspaceLayouts::~CDefaultWorkspaceLayouts()
{
	ClearAllShortcutSlots(registeredKeyboardShortcuts);
}

SDefaultWorkspaceShortcutSlotState *CDefaultWorkspaceLayouts::GetMutableShortcutSlotState(EDefaultWorkspaceSlot slot)
{
	for (std::vector<SDefaultWorkspaceShortcutSlotState>::iterator it = shortcutSlotStates.begin(); it != shortcutSlotStates.end(); ++it)
	{
		if (it->slot == slot)
			return &(*it);
	}

	return NULL;
}

const SDefaultWorkspaceShortcutSlotState *CDefaultWorkspaceLayouts::GetShortcutSlotState(EDefaultWorkspaceSlot slot) const
{
	for (std::vector<SDefaultWorkspaceShortcutSlotState>::const_iterator it = shortcutSlotStates.begin(); it != shortcutSlotStates.end(); ++it)
	{
		if (it->slot == slot)
			return &(*it);
	}

	return NULL;
}

void CDefaultWorkspaceLayouts::RegisterDefaultShortcutSlots(CSlrKeyboardShortcuts *keyboardShortcuts)
{
	if (keyboardShortcuts == NULL)
		return;

	const std::vector<SDefaultWorkspaceShortcutSlotSpec> &shortcutSpecs = GetDefaultWorkspaceShortcutSlotSpecs();
	for (std::vector<SDefaultWorkspaceShortcutSlotSpec>::const_iterator it = shortcutSpecs.begin(); it != shortcutSpecs.end(); ++it)
	{
		SDefaultWorkspaceShortcutSlotState *state = GetMutableShortcutSlotState(it->slot);
		if (state != NULL && state->userModified)
			continue;

		AssignShortcutSlotInternal(keyboardShortcuts,
								   it->slot,
								   it->keyCode,
								   it->isShift,
								   it->isAlt,
								   it->isControl,
								   it->isSuper,
								   it->defaultShortcutLabel,
								   false,
								   false);
	}
}

void CDefaultWorkspaceLayouts::ResetShortcutSlotsToDefaults(CSlrKeyboardShortcuts *keyboardShortcuts)
{
	ClearAllShortcutSlots(keyboardShortcuts);
	RegisterDefaultShortcutSlots(keyboardShortcuts);
}

void CDefaultWorkspaceLayouts::SaveShortcutSlotStates(std::vector<SDefaultWorkspaceShortcutSlotSnapshot> *snapshots) const
{
	if (snapshots == NULL)
		return;

	snapshots->clear();
	for (std::vector<SDefaultWorkspaceShortcutSlotState>::const_iterator it = shortcutSlotStates.begin(); it != shortcutSlotStates.end(); ++it)
	{
		SDefaultWorkspaceShortcutSlotSnapshot snapshot;
		snapshot.slot = it->slot;
		snapshot.hasConflict = it->hasConflict;
		snapshot.userModified = it->userModified;
		snapshot.suppressNextActivation = it->suppressNextActivation;
		snapshot.conflictShortcutName = it->conflictShortcutName;
		snapshot.assignedShortcutLabel = it->assignedShortcutLabel;
		snapshot.statusText = it->statusText;
		if (it->shortcut != NULL)
		{
			snapshot.hasShortcut = true;
			snapshot.keyCode = it->shortcut->keyCode;
			snapshot.isShift = it->shortcut->isShift;
			snapshot.isAlt = it->shortcut->isAlt;
			snapshot.isControl = it->shortcut->isControl;
			snapshot.isSuper = it->shortcut->isSuper;
		}
		snapshots->push_back(snapshot);
	}
}

void CDefaultWorkspaceLayouts::RestoreShortcutSlotStates(CSlrKeyboardShortcuts *keyboardShortcuts, const std::vector<SDefaultWorkspaceShortcutSlotSnapshot> &snapshots)
{
	ClearAllShortcutSlots(keyboardShortcuts);
	if (keyboardShortcuts != NULL)
	{
		registeredKeyboardShortcuts = keyboardShortcuts;
	}

	for (std::vector<SDefaultWorkspaceShortcutSlotSnapshot>::const_iterator it = snapshots.begin(); it != snapshots.end(); ++it)
	{
		SDefaultWorkspaceShortcutSlotState *state = GetMutableShortcutSlotState(it->slot);
		const SDefaultWorkspaceShortcutSlotSpec *shortcutSpec = FindDefaultWorkspaceShortcutSlotSpec(it->slot);
		if (state == NULL || shortcutSpec == NULL)
			continue;

		state->hasConflict = it->hasConflict;
		state->userModified = it->userModified;
		state->suppressNextActivation = it->suppressNextActivation;
		state->conflictShortcutName = it->conflictShortcutName;
		state->assignedShortcutLabel = it->assignedShortcutLabel;
		state->statusText = it->statusText;

		if (it->hasShortcut && keyboardShortcuts != NULL)
		{
			char shortcutName[128];
			snprintf(shortcutName, sizeof(shortcutName), "Default Workspace: %s", shortcutSpec->roleName);
			state->shortcut = new CSlrKeyboardShortcut(KBZONE_GLOBAL,
													 shortcutName,
													 it->keyCode,
													 it->isShift,
													 it->isAlt,
													 it->isControl,
													 it->isSuper,
													 this);
			state->shortcut->userData = state;
			keyboardShortcuts->AddShortcut(state->shortcut);
		}
	}
}

void CDefaultWorkspaceLayouts::SaveShortcutSlotSettings(CConfigStorageHjson *config, bool contextShortcutsEnabled) const
{
	if (config == NULL)
		return;

	config->hjsonRoot[kDefaultWorkspaceShortcutSlotsVersionConfigKey] = 1;
	config->hjsonRoot[kDefaultWorkspaceShortcutSlotsEnabledConfigKey] = contextShortcutsEnabled;

	Hjson::Value hjsonSlots;
	for (std::vector<SDefaultWorkspaceShortcutSlotState>::const_iterator it = shortcutSlotStates.begin(); it != shortcutSlotStates.end(); ++it)
	{
		const SDefaultWorkspaceShortcutSlotSpec *shortcutSpec = FindDefaultWorkspaceShortcutSlotSpec(it->slot);
		bool saveDefaultShortcut = contextShortcutsEnabled && it->shortcut == NULL && !it->userModified && !it->hasConflict && shortcutSpec != NULL;
		Hjson::Value hjsonSlot;
		hjsonSlot["Slot"] = (int)it->slot;
		hjsonSlot["UserModified"] = it->userModified;
		hjsonSlot["HasShortcut"] = it->shortcut != NULL || saveDefaultShortcut;
		if (it->shortcut != NULL)
		{
			hjsonSlot["KeyCode"] = it->shortcut->keyCode;
			hjsonSlot["Shift"] = it->shortcut->isShift;
			hjsonSlot["Alt"] = it->shortcut->isAlt;
			hjsonSlot["Control"] = it->shortcut->isControl;
			hjsonSlot["Super"] = it->shortcut->isSuper;
		}
		else if (saveDefaultShortcut)
		{
			hjsonSlot["KeyCode"] = shortcutSpec->keyCode;
			hjsonSlot["Shift"] = shortcutSpec->isShift;
			hjsonSlot["Alt"] = shortcutSpec->isAlt;
			hjsonSlot["Control"] = shortcutSpec->isControl;
			hjsonSlot["Super"] = shortcutSpec->isSuper;
		}
		hjsonSlots.push_back(hjsonSlot);
	}
	config->hjsonRoot[kDefaultWorkspaceShortcutSlotsConfigKey] = hjsonSlots;
	SaveDefaultWorkspaceShortcutConfig(config);
}

bool CDefaultWorkspaceLayouts::RegisterStoredShortcutSlotsForGeneratedLayouts(CSlrKeyboardShortcuts *keyboardShortcuts, CConfigStorageHjson *config)
{
	if (keyboardShortcuts == NULL || !HasGeneratedDefaultWorkspaceLayouts())
		return false;

	bool contextShortcutsEnabled = true;
	if (config != NULL)
	{
		config->GetBool(kDefaultWorkspaceShortcutSlotsEnabledConfigKey, &contextShortcutsEnabled, true);
	}

	if (!contextShortcutsEnabled)
	{
		ClearAllShortcutSlots(keyboardShortcuts);
		return false;
	}

	if (config == NULL || !config->E_x_i_s_t_s(kDefaultWorkspaceShortcutSlotsConfigKey))
	{
		RegisterDefaultShortcutSlots(keyboardShortcuts);
		return true;
	}

	ClearAllShortcutSlots(keyboardShortcuts);
	Hjson::Value hjsonSlots = config->hjsonRoot[kDefaultWorkspaceShortcutSlotsConfigKey];
	for (int i = 0; i < hjsonSlots.size(); i++)
	{
		Hjson::Value hjsonSlot = hjsonSlots[i];
		EDefaultWorkspaceSlot slot = (EDefaultWorkspaceSlot)GetHjsonInt(hjsonSlot, "Slot", 0);
		if (FindDefaultWorkspaceShortcutSlotSpec(slot) == NULL)
			continue;

		bool userModified = GetHjsonBool(hjsonSlot, "UserModified", false);
		bool hasShortcut = GetHjsonBool(hjsonSlot, "HasShortcut", false);
		if (!hasShortcut)
		{
			ClearShortcutSlotInternal(keyboardShortcuts, slot, userModified);
			continue;
		}

		int keyCode = GetHjsonInt(hjsonSlot, "KeyCode", 0);
		bool isShift = GetHjsonBool(hjsonSlot, "Shift", false);
		bool isAlt = GetHjsonBool(hjsonSlot, "Alt", false);
		bool isControl = GetHjsonBool(hjsonSlot, "Control", false);
		bool isSuper = GetHjsonBool(hjsonSlot, "Super", false);
		AssignShortcutSlotInternal(keyboardShortcuts, slot, keyCode, isShift, isAlt, isControl, isSuper, NULL, userModified, false);
	}

	return true;
}

bool CDefaultWorkspaceLayouts::AssignShortcutSlot(CSlrKeyboardShortcuts *keyboardShortcuts, EDefaultWorkspaceSlot slot, int keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return AssignShortcutSlotInternal(keyboardShortcuts, slot, keyCode, isShift, isAlt, isControl, isSuper, NULL, true, true);
}

bool CDefaultWorkspaceLayouts::AssignShortcutSlotInternal(CSlrKeyboardShortcuts *keyboardShortcuts, EDefaultWorkspaceSlot slot, int keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper, const char *assignedShortcutLabel, bool userModified, bool suppressNextActivation)
{
	if (keyboardShortcuts == NULL)
		return false;

	if (registeredKeyboardShortcuts != NULL && registeredKeyboardShortcuts != keyboardShortcuts)
	{
		ClearAllShortcutSlots(registeredKeyboardShortcuts);
	}
	registeredKeyboardShortcuts = keyboardShortcuts;

	SDefaultWorkspaceShortcutSlotState *state = GetMutableShortcutSlotState(slot);
	const SDefaultWorkspaceShortcutSlotSpec *shortcutSpec = FindDefaultWorkspaceShortcutSlotSpec(slot);
	if (state == NULL || shortcutSpec == NULL)
		return false;

	CSlrKeyboardShortcut *conflictingShortcut = keyboardShortcuts->FindShortcut(KBZONE_GLOBAL,
													keyCode,
													isShift,
													isAlt,
													isControl,
													isSuper);
	if (conflictingShortcut != NULL && conflictingShortcut != state->shortcut)
	{
		state->hasConflict = true;
		state->conflictShortcutName = conflictingShortcut->name ? conflictingShortcut->name : "Unknown";
		state->statusText = "Conflict: ";
		state->statusText += state->conflictShortcutName;
		state->userModified = userModified;
		return false;
	}

	state->hasConflict = false;
	state->conflictShortcutName.clear();
	state->userModified = userModified;

	if (state->shortcut != NULL)
	{
		if (conflictingShortcut == state->shortcut)
		{
			state->assignedShortcutLabel = assignedShortcutLabel ? assignedShortcutLabel : (state->shortcut->cstr ? state->shortcut->cstr : "");
			state->statusText = "Ready";
			state->suppressNextActivation = suppressNextActivation;
			return true;
		}

		keyboardShortcuts->RemoveShortcut(state->shortcut);
		delete state->shortcut;
		state->shortcut = NULL;
	}

	state->assignedShortcutLabel.clear();
	state->statusText = "Unassigned";

	char shortcutName[128];
	snprintf(shortcutName, sizeof(shortcutName), "Default Workspace: %s", shortcutSpec->roleName);
	state->shortcut = new CSlrKeyboardShortcut(KBZONE_GLOBAL,
											 shortcutName,
											 keyCode,
											 isShift,
											 isAlt,
											 isControl,
											 isSuper,
											 this);
	state->shortcut->userData = state;
	keyboardShortcuts->AddShortcut(state->shortcut);
	state->assignedShortcutLabel = assignedShortcutLabel ? assignedShortcutLabel : (state->shortcut->cstr ? state->shortcut->cstr : "");
	state->statusText = "Ready";
	state->suppressNextActivation = suppressNextActivation;
	return true;
}

void CDefaultWorkspaceLayouts::ClearShortcutSlot(CSlrKeyboardShortcuts *keyboardShortcuts, EDefaultWorkspaceSlot slot)
{
	ClearShortcutSlotInternal(keyboardShortcuts, slot, true);
}

void CDefaultWorkspaceLayouts::ClearShortcutSlotInternal(CSlrKeyboardShortcuts *keyboardShortcuts, EDefaultWorkspaceSlot slot, bool userModified)
{
	SDefaultWorkspaceShortcutSlotState *state = GetMutableShortcutSlotState(slot);
	if (state == NULL)
		return;

	if (state->shortcut != NULL)
	{
		if (keyboardShortcuts != NULL)
		{
			keyboardShortcuts->RemoveShortcut(state->shortcut);
		}
		delete state->shortcut;
		state->shortcut = NULL;
	}

	state->hasConflict = false;
	state->suppressNextActivation = false;
	state->conflictShortcutName.clear();
	state->assignedShortcutLabel.clear();
	state->statusText = "Unassigned";
	state->userModified = userModified;
}

void CDefaultWorkspaceLayouts::ClearAllShortcutSlots(CSlrKeyboardShortcuts *keyboardShortcuts)
{
	for (std::vector<SDefaultWorkspaceShortcutSlotState>::iterator it = shortcutSlotStates.begin(); it != shortcutSlotStates.end(); ++it)
	{
		ClearShortcutSlotInternal(keyboardShortcuts, it->slot, false);
	}

	if (keyboardShortcuts == registeredKeyboardShortcuts)
	{
		registeredKeyboardShortcuts = NULL;
	}
}

bool CDefaultWorkspaceLayouts::HasGeneratedDefaultWorkspaceLayouts() const
{
	if (layoutManager == NULL)
		return false;

	const std::vector<SDefaultWorkspaceLayoutSpec> &allSpecs = GetAllDefaultWorkspaceLayoutSpecs();
	for (std::vector<SDefaultWorkspaceLayoutSpec>::const_iterator it = allSpecs.begin(); it != allSpecs.end(); ++it)
	{
		if (layoutManager->GetPredefinedLayoutById(it->predefinedId) != NULL)
			return true;
	}

	return false;
}

const char *CDefaultWorkspaceLayouts::GetShortcutLabel(EDefaultWorkspaceSlot slot) const
{
	const SDefaultWorkspaceShortcutSlotState *state = GetShortcutSlotState(slot);
	if (state == NULL)
		return "";
	return state->assignedShortcutLabel.c_str();
}

const char *CDefaultWorkspaceLayouts::GetShortcutStatus(EDefaultWorkspaceSlot slot) const
{
	const SDefaultWorkspaceShortcutSlotState *state = GetShortcutSlotState(slot);
	if (state == NULL)
		return "Unassigned";
	return state->statusText.c_str();
}

CLayoutData *CDefaultWorkspaceLayouts::CreateOrUpdateDefaultWorkspaceLayoutData(const SDefaultWorkspaceLayoutSpec *layoutSpec, bool lockedDefaultLayout)
{
	if (layoutManager == NULL || layoutSpec == NULL || layoutSpec->predefinedId == NULL || layoutSpec->displayName == NULL)
		return NULL;

	CLayoutData *layoutData = layoutManager->AddPredefinedLayout(layoutSpec->predefinedId, layoutSpec->displayName);
	if (layoutData == NULL)
		return NULL;

	SetLayoutName(layoutManager, layoutData, layoutSpec->displayName);
	SetLayoutString(&layoutData->predefinedId, layoutSpec->predefinedId);
	layoutData->doNotUpdateViewsPositions = lockedDefaultLayout;
	ClearLayoutShortcut(layoutManager, layoutData);
	return layoutData;
}

int CDefaultWorkspaceLayouts::CreateOrUpdateAllDefaultWorkspaceLayoutData(bool lockedDefaultLayouts)
{
	int numCreatedOrUpdated = 0;
	const std::vector<SDefaultWorkspaceLayoutSpec> &allSpecs = GetAllDefaultWorkspaceLayoutSpecs();
	for (std::vector<SDefaultWorkspaceLayoutSpec>::const_iterator it = allSpecs.begin(); it != allSpecs.end(); ++it)
	{
		if (CreateOrUpdateDefaultWorkspaceLayoutData(&(*it), lockedDefaultLayouts) != NULL)
		{
			numCreatedOrUpdated++;
		}
	}
	return numCreatedOrUpdated;
}

void CDefaultWorkspaceLayouts::BeginDefaultWorkspaceGeneration(bool lockedDefaultLayouts, bool noTabBarsInGeneratedLayouts)
{
	lastGenerationSummary = SDefaultWorkspaceGenerationSummary();
	for (std::vector<SDefaultWorkspaceShortcutSlotState>::const_iterator it = shortcutSlotStates.begin(); it != shortcutSlotStates.end(); ++it)
	{
		if (it->shortcut != NULL)
			lastGenerationSummary.numAssignedShortcutSlots++;
		if (it->hasConflict)
			lastGenerationSummary.numShortcutConflicts++;
	}

	generationLockedDefaultLayouts = lockedDefaultLayouts;
	generationNoTabBarsInGeneratedLayouts = noTabBarsInGeneratedLayouts;
	generationSpecIndex = 0;
	generationWaitFrames = 0;
	generationCurrentLayoutData = NULL;
	generationOriginalLayout = layoutManager != NULL ? layoutManager->currentLayout : NULL;
	generationOriginalC64Running = viewC64 != NULL && viewC64->debugInterfaceC64 != NULL && viewC64->debugInterfaceC64->isRunning;
	generationOriginalAtariRunning = viewC64 != NULL && viewC64->debugInterfaceAtari != NULL && viewC64->debugInterfaceAtari->isRunning;
	generationOriginalNesRunning = viewC64 != NULL && viewC64->debugInterfaceNes != NULL && viewC64->debugInterfaceNes->isRunning;
	generationStep = (layoutManager != NULL && viewC64 != NULL && guiMain != NULL) ? DefaultWorkspaceGenerationStep_ApplyFloating : DefaultWorkspaceGenerationStep_Idle;
	if (generationStep == DefaultWorkspaceGenerationStep_Idle)
	{
		lastGenerationSummary.completed = true;
		lastGenerationSummary.numFailed = (int)GetAllDefaultWorkspaceLayoutSpecs().size();
	}
}

bool CDefaultWorkspaceLayouts::IsGeneratingDefaultWorkspaces() const
{
	return generationStep != DefaultWorkspaceGenerationStep_Idle;
}

const SDefaultWorkspaceGenerationSummary &CDefaultWorkspaceLayouts::GetLastGenerationSummary() const
{
	return lastGenerationSummary;
}

CDebugInterface *CDefaultWorkspaceLayouts::GetDebugInterfaceForPlatform(EDefaultWorkspacePlatform platform) const
{
	if (viewC64 == NULL)
		return NULL;

	switch (platform)
	{
		case DefaultWorkspacePlatform_C64:
			return (CDebugInterface *)viewC64->debugInterfaceC64;
		case DefaultWorkspacePlatform_Atari800:
			return (CDebugInterface *)viewC64->debugInterfaceAtari;
		case DefaultWorkspacePlatform_NES:
			return (CDebugInterface *)viewC64->debugInterfaceNes;
	}

	return NULL;
}

bool CDefaultWorkspaceLayouts::SetDebugInterfaceRunning(CDebugInterface *debugInterface, bool shouldBeRunning)
{
	if (viewC64 == NULL || debugInterface == NULL || debugInterface->isRunning == shouldBeRunning)
		return false;

	if (shouldBeRunning)
		viewC64->StartEmulationThread(debugInterface);
	else
		viewC64->StopEmulationThread(debugInterface);
	return true;
}

bool CDefaultWorkspaceLayouts::EnsureOnlyGenerationPlatformRunning(EDefaultWorkspacePlatform platform)
{
	bool changed = false;
	changed |= SetDebugInterfaceRunning(GetDebugInterfaceForPlatform(DefaultWorkspacePlatform_C64), platform == DefaultWorkspacePlatform_C64);
	changed |= SetDebugInterfaceRunning(GetDebugInterfaceForPlatform(DefaultWorkspacePlatform_Atari800), platform == DefaultWorkspacePlatform_Atari800);
	changed |= SetDebugInterfaceRunning(GetDebugInterfaceForPlatform(DefaultWorkspacePlatform_NES), platform == DefaultWorkspacePlatform_NES);
	return changed;
}

bool CDefaultWorkspaceLayouts::IsOnlyGenerationPlatformRunning(EDefaultWorkspacePlatform platform) const
{
	CDebugInterface *c64 = GetDebugInterfaceForPlatform(DefaultWorkspacePlatform_C64);
	CDebugInterface *atari = GetDebugInterfaceForPlatform(DefaultWorkspacePlatform_Atari800);
	CDebugInterface *nes = GetDebugInterfaceForPlatform(DefaultWorkspacePlatform_NES);

	bool c64Running = c64 != NULL && c64->isRunning;
	bool atariRunning = atari != NULL && atari->isRunning;
	bool nesRunning = nes != NULL && nes->isRunning;

	return c64Running == (platform == DefaultWorkspacePlatform_C64)
		&& atariRunning == (platform == DefaultWorkspacePlatform_Atari800)
		&& nesRunning == (platform == DefaultWorkspacePlatform_NES);
}

bool CDefaultWorkspaceLayouts::RestoreGenerationOriginalPlatformState()
{
	bool changed = false;
	changed |= SetDebugInterfaceRunning(GetDebugInterfaceForPlatform(DefaultWorkspacePlatform_C64), generationOriginalC64Running);
	changed |= SetDebugInterfaceRunning(GetDebugInterfaceForPlatform(DefaultWorkspacePlatform_Atari800), generationOriginalAtariRunning);
	changed |= SetDebugInterfaceRunning(GetDebugInterfaceForPlatform(DefaultWorkspacePlatform_NES), generationOriginalNesRunning);
	return changed;
}

bool CDefaultWorkspaceLayouts::IsGenerationOriginalPlatformStateRestored() const
{
	CDebugInterface *c64 = GetDebugInterfaceForPlatform(DefaultWorkspacePlatform_C64);
	CDebugInterface *atari = GetDebugInterfaceForPlatform(DefaultWorkspacePlatform_Atari800);
	CDebugInterface *nes = GetDebugInterfaceForPlatform(DefaultWorkspacePlatform_NES);

	bool c64Running = c64 != NULL && c64->isRunning;
	bool atariRunning = atari != NULL && atari->isRunning;
	bool nesRunning = nes != NULL && nes->isRunning;

	return c64Running == generationOriginalC64Running
		&& atariRunning == generationOriginalAtariRunning
		&& nesRunning == generationOriginalNesRunning;
}

void CDefaultWorkspaceLayouts::RestoreOriginalLayoutOrFinish()
{
	if (generationOriginalLayout != NULL && layoutManager != NULL)
	{
		layoutManager->SetLayoutAsync(generationOriginalLayout, false);
		generationWaitFrames = 1;
		generationStep = DefaultWorkspaceGenerationStep_WaitRestoreFrame;
	}
	else
	{
		FinishDefaultWorkspaceGeneration();
	}
}

void CDefaultWorkspaceLayouts::FinishDefaultWorkspaceGeneration()
{
	if (layoutManager != NULL)
	{
		layoutManager->StoreLayouts();
	}

	lastGenerationSummary.completed = true;
	generationCurrentLayoutData = NULL;
	generationOriginalLayout = NULL;
	generationWaitFrames = 0;
	generationSpecIndex = 0;
	generationStep = DefaultWorkspaceGenerationStep_Idle;

	if (guiMain != NULL)
	{
		char message[256];
		snprintf(message,
				 sizeof(message),
				 "Generated %d default workspaces (%d failed), %d shortcut slots assigned, %d conflicts.",
				 lastGenerationSummary.numGenerated,
				 lastGenerationSummary.numFailed,
				 lastGenerationSummary.numAssignedShortcutSlots,
				 lastGenerationSummary.numShortcutConflicts);
		guiMain->ShowNotification("Information", message);
	}
}

void CDefaultWorkspaceLayouts::UpdateDefaultWorkspaceGenerationFrame()
{
	if (generationStep == DefaultWorkspaceGenerationStep_Idle)
		return;

	const std::vector<SDefaultWorkspaceLayoutSpec> &allSpecs = GetAllDefaultWorkspaceLayoutSpecs();
	switch (generationStep)
	{
		case DefaultWorkspaceGenerationStep_ApplyFloating:
		{
			if (generationSpecIndex >= (int)allSpecs.size())
			{
				if (!IsGenerationOriginalPlatformStateRestored())
				{
					RestoreGenerationOriginalPlatformState();
					generationWaitFrames = 10;
					generationStep = DefaultWorkspaceGenerationStep_WaitRestoreEmulators;
				}
				else
				{
					RestoreOriginalLayoutOrFinish();
				}
				return;
			}

			const SDefaultWorkspaceLayoutSpec *layoutSpec = &allSpecs[generationSpecIndex];
			if (!IsOnlyGenerationPlatformRunning(layoutSpec->platform))
			{
				if (EnsureOnlyGenerationPlatformRunning(layoutSpec->platform))
				{
					generationWaitFrames = 10;
					generationStep = DefaultWorkspaceGenerationStep_WaitPlatformFrame;
					return;
				}
			}

			generationCurrentLayoutData = CreateOrUpdateDefaultWorkspaceLayoutData(layoutSpec, generationLockedDefaultLayouts);
			if (generationCurrentLayoutData == NULL)
			{
				lastGenerationSummary.numFailed++;
				generationSpecIndex++;
				return;
			}
			lastGenerationSummary.numCreatedOrUpdated++;

			ImGuiViewport *viewport = ImGui::GetMainViewport();
			float viewportWidth = viewport != NULL ? viewport->WorkSize.x : kDefaultWorkspaceReferenceWidth;
			float viewportHeight = viewport != NULL ? viewport->WorkSize.y : kDefaultWorkspaceReferenceHeight;
			if (!ApplyLayoutSpecFloating(layoutSpec, viewportWidth, viewportHeight, generationNoTabBarsInGeneratedLayouts))
			{
				lastGenerationSummary.numFailed++;
				generationCurrentLayoutData = NULL;
				generationSpecIndex++;
				return;
			}

			generationWaitFrames = 1;
			generationStep = DefaultWorkspaceGenerationStep_WaitFloatingFrame;
			return;
		}

		case DefaultWorkspaceGenerationStep_WaitPlatformFrame:
		{
			if (generationSpecIndex < (int)allSpecs.size()
				&& !IsOnlyGenerationPlatformRunning(allSpecs[generationSpecIndex].platform)
				&& generationWaitFrames > 0)
			{
				generationWaitFrames--;
				return;
			}

			generationStep = DefaultWorkspaceGenerationStep_ApplyFloating;
			return;
		}

		case DefaultWorkspaceGenerationStep_WaitFloatingFrame:
		{
			if (generationWaitFrames > 0)
			{
				generationWaitFrames--;
				return;
			}

			if (guiMain != NULL)
			{
				UndockHiddenDefaultWorkspacePlatformViews(viewC64);
				guiMain->RequestAutoLayoutVisibleViewsDockedPreserveScan(generationNoTabBarsInGeneratedLayouts
					? AutoLayoutDockedPreserveScanTabBarMode_NoTabBar
					: AutoLayoutDockedPreserveScanTabBarMode_TabBar);
			}
			generationWaitFrames = 1;
			generationStep = DefaultWorkspaceGenerationStep_WaitDockedFrame;
			return;
		}

		case DefaultWorkspaceGenerationStep_WaitDockedFrame:
		{
			if (generationWaitFrames > 0)
			{
				generationWaitFrames--;
				return;
			}

			if (generationCurrentLayoutData != NULL && guiMain != NULL)
			{
				guiMain->SerializeLayout(generationCurrentLayoutData);
				lastGenerationSummary.numGenerated++;
			}
			else
			{
				lastGenerationSummary.numFailed++;
			}

			generationCurrentLayoutData = NULL;
			generationSpecIndex++;
			generationStep = DefaultWorkspaceGenerationStep_ApplyFloating;
			return;
		}

		case DefaultWorkspaceGenerationStep_WaitRestoreFrame:
		{
			if (generationWaitFrames > 0)
			{
				generationWaitFrames--;
				return;
			}
			FinishDefaultWorkspaceGeneration();
			return;
		}

		case DefaultWorkspaceGenerationStep_RestoreOriginal:
		case DefaultWorkspaceGenerationStep_WaitRestoreEmulators:
		{
			if (!IsGenerationOriginalPlatformStateRestored() && generationWaitFrames > 0)
			{
				generationWaitFrames--;
				return;
			}

			RestoreOriginalLayoutOrFinish();
			return;
		}

		case DefaultWorkspaceGenerationStep_Idle:
			break;
	}
}

CGuiView *CDefaultWorkspaceLayouts::FindViewForPlacement(const char *viewId) const
{
	if (viewC64 == NULL || viewId == NULL)
		return NULL;

	if (strcmp(viewId, "c64.screen") == 0)
		return viewC64->viewC64Screen;
	if (strcmp(viewId, "c64.cpu_state") == 0)
		return viewC64->viewC64StateCPU;
	if (strcmp(viewId, "c64.disassembly") == 0)
		return viewC64->viewC64Disassembly;
	if (strcmp(viewId, "c64.disassembly2") == 0)
		return viewC64->viewC64Disassembly2;
	if (strcmp(viewId, "c64.memory_map") == 0)
		return viewC64->viewC64MemoryMap;
	if (strcmp(viewId, "c64.data_dump") == 0)
		return viewC64->viewC64MemoryDataDump;
	if (strcmp(viewId, "c64.data_dump2") == 0)
		return viewC64->viewC64MemoryDataDump2;
	if (strcmp(viewId, "c64.data_dump3") == 0)
		return viewC64->viewC64MemoryDataDump3;
	if (strcmp(viewId, "c64.source_code") == 0)
		return viewC64->viewC64SourceCode;
	if (strcmp(viewId, "c64.monitor_console") == 0)
		return viewC64->viewC64MonitorConsole;
	if (strcmp(viewId, "c64.cia_state") == 0)
		return viewC64->viewC64StateCIA;
	if (strcmp(viewId, "c64.vic_state") == 0)
		return viewC64->viewC64StateVIC;
	if (strcmp(viewId, "c64.sid_state") == 0)
		return viewC64->viewC64StateSID;
	if (strcmp(viewId, "c64.sid_tracker_history") == 0)
		return viewC64->viewC64SidTrackerHistory;
	if (strcmp(viewId, "c64.sid_piano_keyboard") == 0)
		return viewC64->viewC64SidPianoKeyboard;
	if (strcmp(viewId, "c64.reu_state") == 0)
		return viewC64->viewC64StateREU;
	if (strcmp(viewId, "c64.emulation_counters") == 0)
		return viewC64->viewC64EmulationCounters;
	if (strcmp(viewId, "c64.emulation_state") == 0)
		return viewC64->viewEmulationState;
	if (strcmp(viewId, "c64.vic_display") == 0)
		return viewC64->viewC64VicDisplay;
	if (strcmp(viewId, "c64.vic_control") == 0)
		return viewC64->viewC64VicControl;
	if (strcmp(viewId, "c64.all_graphics_bitmaps") == 0)
		return viewC64->viewC64AllGraphicsBitmaps;
	if (strcmp(viewId, "c64.all_graphics_screens") == 0)
		return viewC64->viewC64AllGraphicsScreens;
	if (strcmp(viewId, "c64.all_graphics_charsets") == 0)
		return viewC64->viewC64AllGraphicsCharsets;
	if (strcmp(viewId, "c64.all_graphics_sprites") == 0)
		return viewC64->viewC64AllGraphicsSprites;
	if (strcmp(viewId, "drive1541.cpu_state") == 0)
		return viewC64->viewDrive1541StateCPU;
	if (strcmp(viewId, "drive1541.disassembly") == 0)
		return viewC64->viewDrive1541Disassembly;
	if (strcmp(viewId, "drive1541.memory_map") == 0)
		return viewC64->viewDrive1541MemoryMap;
	if (strcmp(viewId, "drive1541.via_state") == 0)
		return viewC64->viewDrive1541StateVIA;
	if (strcmp(viewId, "atari.screen") == 0)
		return viewC64->viewAtariScreen;
	if (strcmp(viewId, "atari.cpu_state") == 0)
		return viewC64->viewAtariStateCPU;
	if (strcmp(viewId, "atari.disassembly") == 0)
		return viewC64->viewAtariDisassembly;
	if (strcmp(viewId, "atari.memory_map") == 0)
		return viewC64->viewAtariMemoryMap;
	if (strcmp(viewId, "atari.data_dump") == 0)
		return viewC64->viewAtariMemoryDataDump;
	if (strcmp(viewId, "atari.source_code") == 0)
		return viewC64->viewAtariSourceCode;
	if (strcmp(viewId, "atari.monitor_console") == 0)
		return viewC64->viewAtariMonitorConsole;
	if (strcmp(viewId, "atari.antic_state") == 0)
		return viewC64->viewAtariStateANTIC;
	if (strcmp(viewId, "atari.gtia_state") == 0)
		return viewC64->viewAtariStateGTIA;
	if (strcmp(viewId, "atari.pia_state") == 0)
		return viewC64->viewAtariStatePIA;
	if (strcmp(viewId, "atari.pokey_state") == 0)
		return viewC64->viewAtariStatePOKEY;
	if (strcmp(viewId, "atari.emulation_counters") == 0)
		return viewC64->viewAtariEmulationCounters;
	if (strcmp(viewId, "atari.emulation_state") == 0)
		return viewC64->viewEmulationState;
	if (strcmp(viewId, "nes.screen") == 0)
		return viewC64->viewNesScreen;
	if (strcmp(viewId, "nes.cpu_state") == 0)
		return viewC64->viewNesStateCPU;
	if (strcmp(viewId, "nes.disassembly") == 0)
		return viewC64->viewNesDisassembly;
	if (strcmp(viewId, "nes.memory_map") == 0)
		return viewC64->viewNesMemoryMap;
	if (strcmp(viewId, "nes.data_dump") == 0)
		return viewC64->viewNesMemoryDataDump;
	if (strcmp(viewId, "nes.monitor_console") == 0)
		return viewC64->viewNesMonitorConsole;
	if (strcmp(viewId, "nes.apu_state") == 0)
		return viewC64->viewNesStateAPU;
	if (strcmp(viewId, "nes.piano_keyboard") == 0)
		return viewC64->viewNesPianoKeyboard;
	if (strcmp(viewId, "nes.ppu_state") == 0)
		return viewC64->viewNesStatePPU;
	if (strcmp(viewId, "nes.ppu_nametables") == 0)
		return viewC64->viewNesPpuNametables;
	if (strcmp(viewId, "nes.ppu_nametable_memory_map") == 0)
		return viewC64->viewNesPpuNametableMemoryMap;
	if (strcmp(viewId, "nes.ppu_nametable_data_dump") == 0)
		return viewC64->viewNesPpuNametableMemoryDataDump;
	if (strcmp(viewId, "nes.ppu_palette") == 0)
		return viewC64->viewNesPpuPalette;
	if (strcmp(viewId, "nes.emulation_counters") == 0)
		return viewC64->viewNesEmulationCounters;

	return NULL;
}

bool CDefaultWorkspaceLayouts::ApplyLayoutSpecFloating(const SDefaultWorkspaceLayoutSpec *layoutSpec, float viewportWidth, float viewportHeight, bool positionForNoTabBarPreserveScan)
{
	if (layoutSpec == NULL || layoutSpec->placements == NULL || layoutSpec->numPlacements <= 0)
		return false;

	std::vector<CGuiView *> placementViews;
	placementViews.reserve(layoutSpec->numPlacements);
	for (int i = 0; i < layoutSpec->numPlacements; i++)
	{
		CGuiView *view = FindViewForPlacement(layoutSpec->placements[i].viewId);
		if (view == NULL)
			return false;
		placementViews.push_back(view);
	}

	HideDefaultWorkspacePlatformViews(viewC64);

	float uniformScale = GetDefaultWorkspaceUniformScale(viewportWidth, viewportHeight);
	for (int i = 0; i < layoutSpec->numPlacements; i++)
	{
		CGuiView *view = placementViews[i];
		SDefaultWorkspaceViewPlacementSpec scaledPlacement = ScaleDefaultWorkspacePlacement(layoutSpec->placements[i], viewportWidth, viewportHeight);
		view->SetVisible(true);
		view->SetPosition(scaledPlacement.x, scaledPlacement.y, view->posZ, scaledPlacement.width, scaledPlacement.height);
		ApplyDefaultWorkspaceViewLayoutParameters(view, scaledPlacement, uniformScale, viewportWidth);
		// NoTabBar preserve-scan reads ImGui InnerRect; lift the temporary titlebar so the body top-left stays on old-layout coordinates.
		float queuedWindowY = scaledPlacement.y;
		if (positionForNoTabBarPreserveScan)
			queuedWindowY -= ImGui::GetFrameHeight();
		view->SetNewImGuiWindowPosition(scaledPlacement.x, queuedWindowY);
		view->SetNewImGuiWindowSize(GetDefaultWorkspaceFloatingWindowWidthForInterior(scaledPlacement.width), GetDefaultWorkspaceFloatingWindowHeightForInterior(scaledPlacement.height));
	}

	return true;
}

SDefaultWorkspaceActivePlatformState CDefaultWorkspaceLayouts::GetActivePlatformState() const
{
	SDefaultWorkspaceActivePlatformState activePlatforms;
	if (viewC64 == NULL)
		return activePlatforms;

	activePlatforms.c64 = viewC64->debugInterfaceC64 != NULL && viewC64->debugInterfaceC64->isRunning;
	activePlatforms.atari800 = viewC64->debugInterfaceAtari != NULL && viewC64->debugInterfaceAtari->isRunning;
	activePlatforms.nes = viewC64->debugInterfaceNes != NULL && viewC64->debugInterfaceNes->isRunning;
	return activePlatforms;
}

const SDefaultWorkspaceLayoutSpec *CDefaultWorkspaceLayouts::ResolveShortcutLayout(const SDefaultWorkspaceActivePlatformState &activePlatforms, EDefaultWorkspaceSlot slot) const
{
	int activeCount = 0;
	EDefaultWorkspacePlatform activePlatform = DefaultWorkspacePlatform_C64;
	if (activePlatforms.c64)
	{
		activePlatform = DefaultWorkspacePlatform_C64;
		activeCount++;
	}
	if (activePlatforms.atari800)
	{
		activePlatform = DefaultWorkspacePlatform_Atari800;
		activeCount++;
	}
	if (activePlatforms.nes)
	{
		activePlatform = DefaultWorkspacePlatform_NES;
		activeCount++;
	}

	if (activeCount != 1)
		return NULL;

	return FindDefaultWorkspaceLayoutSpec(activePlatform, slot);
}

const SDefaultWorkspaceShortcutSlotState *CDefaultWorkspaceLayouts::FindShortcutSlotStateForShortcut(CSlrKeyboardShortcut *keyboardShortcut) const
{
	for (std::vector<SDefaultWorkspaceShortcutSlotState>::const_iterator it = shortcutSlotStates.begin(); it != shortcutSlotStates.end(); ++it)
	{
		if (it->shortcut == keyboardShortcut)
			return &(*it);
	}

	return NULL;
}

bool CDefaultWorkspaceLayouts::SwitchToDefaultWorkspace(const SDefaultWorkspaceLayoutSpec *layoutSpec)
{
	if (layoutSpec == NULL || layoutManager == NULL)
		return false;

	CLayoutData *layoutData = layoutManager->GetPredefinedLayoutById(layoutSpec->predefinedId);
	if (layoutData == NULL)
		return false;

	layoutManager->SetLayoutAsync(layoutData, true);
	return true;
}

bool CDefaultWorkspaceLayouts::ProcessKeyboardShortcut(u32 zone, u8 actionType, CSlrKeyboardShortcut *keyboardShortcut)
{
	SDefaultWorkspaceShortcutSlotState *state = NULL;
	for (std::vector<SDefaultWorkspaceShortcutSlotState>::iterator it = shortcutSlotStates.begin(); it != shortcutSlotStates.end(); ++it)
	{
		if (it->shortcut == keyboardShortcut)
		{
			state = &(*it);
			break;
		}
	}

	if (zone != KBZONE_GLOBAL || state == NULL)
		return false;

	if (state->suppressNextActivation)
	{
		state->suppressNextActivation = false;
		return false;
	}

	const SDefaultWorkspaceLayoutSpec *layoutSpec = ResolveShortcutLayout(GetActivePlatformState(), state->slot);
	return SwitchToDefaultWorkspace(layoutSpec);
}
