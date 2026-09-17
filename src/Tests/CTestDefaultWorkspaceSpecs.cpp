#include "DBG_Log.h"
#include "CTestDefaultWorkspaceSpecs.h"
#include "CDebugInterface.h"
#include "CDebugInterfaceAtari.h"
#include "CDebugInterfaceC64.h"
#include "CDebugInterfaceNes.h"
#include "CDefaultWorkspaceLayouts.h"
#include "CViewC64.h"
#include "CViewAtariScreen.h"
#include "CViewBaseStateCPU.h"
#include "CGuiViewDebugLog.h"
#include "CGuiView.h"
#include "CViewDataDump.h"
#include "CViewDisassembly.h"
#include "CViewC64SidPianoKeyboard.h"
#include "CViewC64SidTrackerHistory.h"
#include "CViewC64StateSID.h"
#include "CViewNesScreen.h"
#include "CGuiMain.h"
#include "CLayoutManager.h"
#include "CByteBuffer.h"
#include "CConfigStorageHjson.h"
#include "CSlrKeyboardShortcuts.h"
#include "SYS_KeyCodes.h"
#include "imgui.h"
#include <set>
#include <list>
#include <string>
#include <vector>
#include <cstdio>
#include <cstring>

static char failureMsg[1024];

static bool ContainsText(const char *text, const char *needle)
{
	return text != NULL && strstr(text, needle) != NULL;
}

static bool NearlyEqual(float a, float b)
{
	float diff = a - b;
	if (diff < 0.0f)
		diff = -diff;
	return diff < 0.001f;
}

static float GetExpectedFloatingWindowHeightForInterior(float interiorHeight)
{
	return interiorHeight + ImGui::GetFrameHeight() + 1.0f;
}

static float GetExpectedFloatingWindowWidthForInterior(float interiorWidth)
{
	return interiorWidth + 1.0f;
}

static bool VerifyPreserveScanTabBarModeFlagPolicy()
{
	const ImGuiDockNodeFlags staleTabFlags = ImGuiDockNodeFlags_NoTabBar | ImGuiDockNodeFlags_HiddenTabBar;
	ImGuiDockNodeFlags defaultFlags = GetAutoLayoutDockedPreserveScanLeafFlags(staleTabFlags, AutoLayoutDockedPreserveScanTabBarMode_Default);
	if (defaultFlags != staleTabFlags)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Default preserve-scan mode should leave existing dock leaf tab-bar flags unchanged");
		return false;
	}

	ImGuiDockNodeFlags rootOnlyFlags = GetAutoLayoutDockedPreserveScanLeafFlags(ImGuiDockNodeFlags_DockSpace | ImGuiDockNodeFlags_NoTabBar,
		AutoLayoutDockedPreserveScanTabBarMode_Default);
	if ((rootOnlyFlags & ImGuiDockNodeFlags_DockSpace) != 0)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Preserve-scan leaf flags should strip root-only DockSpace flag");
		return false;
	}

	ImGuiDockNodeFlags noTabBarFlags = GetAutoLayoutDockedPreserveScanLeafFlags(ImGuiDockNodeFlags_HiddenTabBar, AutoLayoutDockedPreserveScanTabBarMode_NoTabBar);
	if ((noTabBarFlags & ImGuiDockNodeFlags_NoTabBar) == 0 || (noTabBarFlags & ImGuiDockNodeFlags_HiddenTabBar) != 0)
	{
		snprintf(failureMsg, sizeof(failureMsg), "NoTabBar preserve-scan mode should set NoTabBar and clear HiddenTabBar");
		return false;
	}

	ImGuiDockNodeFlags tabBarFlags = GetAutoLayoutDockedPreserveScanLeafFlags(staleTabFlags, AutoLayoutDockedPreserveScanTabBarMode_TabBar);
	if ((tabBarFlags & (ImGuiDockNodeFlags_NoTabBar | ImGuiDockNodeFlags_HiddenTabBar)) != 0)
	{
		snprintf(failureMsg, sizeof(failureMsg), "TabBar preserve-scan mode should clear stale NoTabBar and HiddenTabBar leaf flags");
		return false;
	}

	return true;
}

static bool VerifyDebugLogDefaultVisibilityPolicy()
{
	CGuiViewDebugLog debugLog("Debug Log Unit Test", 50.0f, 50.0f, -1.0f, 200.0f, 200.0f);
	if (debugLog.visible)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Debug Log view should default to hidden so DBG_Log builds do not force it into layouts");
		return false;
	}

	CByteBuffer layoutBuffer;
	layoutBuffer.PutBool(true);
	layoutBuffer.PutFloat(50.0f);
	layoutBuffer.PutFloat(50.0f);
	layoutBuffer.PutFloat(200.0f);
	layoutBuffer.PutFloat(200.0f);
	layoutBuffer.PutU32(0);
	layoutBuffer.Rewind();
	if (!debugLog.DeserializeLayout(&layoutBuffer, 2) || !debugLog.visible)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Debug Log visible=true should restore from normal view layout data");
		return false;
	}

	layoutBuffer.Clear();
	layoutBuffer.PutBool(false);
	layoutBuffer.PutFloat(50.0f);
	layoutBuffer.PutFloat(50.0f);
	layoutBuffer.PutFloat(200.0f);
	layoutBuffer.PutFloat(200.0f);
	layoutBuffer.PutU32(0);
	layoutBuffer.Rewind();
	if (!debugLog.DeserializeLayout(&layoutBuffer, 2) || debugLog.visible)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Debug Log visible=false should restore from normal view layout data");
		return false;
	}

	return true;
}

static float GetExpectedDataDumpRenderedWidth(CViewDataDump *view)
{
	int addressDigits = 4;
	if (view != NULL && view->dataAddressEditBox != NULL)
		addressDigits = view->dataAddressEditBox->GetNumDigits();

	float width = (float)addressDigits * view->fontBytesSize;
	width += view->gapAddress;
	width += (float)view->numberOfBytesPerLine * (2.0f * view->fontBytesSize + view->gapHexData);
	if (view->showDataCharacters)
	{
		width += view->gapDataCharacters;
		width += (float)view->numberOfBytesPerLine * view->fontCharactersWidth;
	}
	if (view->showCharacters)
	{
		width += 5.0f;
		width += view->showSprites ? 40.0f : 32.0f;
	}
	else if (view->showSprites)
	{
		width += 5.0f;
	}
	if (view->showSprites)
	{
		width += 24.0f * 1.9f;
	}
	return width;
}

static float GetVisibleWidthWithinViewport(CGuiView *view, float viewportWidth)
{
	float visibleWidth = view->sizeX;
	if (view->posX + visibleWidth > viewportWidth)
		visibleWidth = viewportWidth - view->posX;
	return visibleWidth > 0.0f ? visibleWidth : 0.0f;
}

struct SDefaultWorkspaceTestViewState
{
	CGuiView *view;
	bool visible;
	float posX;
	float posY;
	float posZ;
	float sizeX;
	float sizeY;
	bool forceNewPosition;
	bool forceNewSize;
	float newPosX;
	float newPosY;
	float newSizeX;
	float newSizeY;
	CViewDisassembly *disassemblyView;
	float disassemblyFontSize;
	bool disassemblyShowHexCodes;
	bool disassemblyShowCodeCycles;
	bool disassemblyShowLabels;
	int disassemblyLabelNumCharacters;
	float disassemblyMnemonicsDisplayOffsetX;
	float disassemblyCodeCyclesDisplayOffsetX;
	CViewDataDump *dataDumpView;
	float dataDumpFontSize;
	int dataDumpNumberOfBytesPerLine;
	bool dataDumpShowDataCharacters;
	bool dataDumpShowCharacters;
	bool dataDumpShowSprites;
	CViewBaseStateCPU *baseStateCpuView;
	float baseStateCpuFontSize;
	bool baseStateCpuHasManualFontSize;
};

class CScopedDefaultWorkspaceTestViewState
{
public:
	void Add(CGuiView *view)
	{
		if (view == NULL)
			return;

		SDefaultWorkspaceTestViewState state;
		state.view = view;
		state.visible = view->visible;
		state.posX = view->posX;
		state.posY = view->posY;
		state.posZ = view->posZ;
		state.sizeX = view->sizeX;
		state.sizeY = view->sizeY;
		state.forceNewPosition = view->imGuiForceThisFrameNewPosition;
		state.forceNewSize = view->imGuiForceThisFrameNewSize;
		state.newPosX = view->thisFrameNewPosX;
		state.newPosY = view->thisFrameNewPosY;
		state.newSizeX = view->thisFrameNewSizeX;
		state.newSizeY = view->thisFrameNewSizeY;
		state.disassemblyView = dynamic_cast<CViewDisassembly *>(view);
		if (state.disassemblyView != NULL)
		{
			state.disassemblyFontSize = state.disassemblyView->fontSize;
			state.disassemblyShowHexCodes = state.disassemblyView->showHexCodes;
			state.disassemblyShowCodeCycles = state.disassemblyView->showCodeCycles;
			state.disassemblyShowLabels = state.disassemblyView->showLabels;
			state.disassemblyLabelNumCharacters = state.disassemblyView->labelNumCharacters;
			state.disassemblyMnemonicsDisplayOffsetX = state.disassemblyView->mnemonicsDisplayOffsetX;
			state.disassemblyCodeCyclesDisplayOffsetX = state.disassemblyView->codeCyclesDisplayOffsetX;
		}
		state.dataDumpView = dynamic_cast<CViewDataDump *>(view);
		if (state.dataDumpView != NULL)
		{
			state.dataDumpFontSize = state.dataDumpView->fontSize;
			state.dataDumpNumberOfBytesPerLine = state.dataDumpView->numberOfBytesPerLine;
			state.dataDumpShowDataCharacters = state.dataDumpView->showDataCharacters;
			state.dataDumpShowCharacters = state.dataDumpView->showCharacters;
			state.dataDumpShowSprites = state.dataDumpView->showSprites;
		}
		state.baseStateCpuView = dynamic_cast<CViewBaseStateCPU *>(view);
		if (state.baseStateCpuView != NULL)
		{
			state.baseStateCpuFontSize = state.baseStateCpuView->fontSize;
			state.baseStateCpuHasManualFontSize = state.baseStateCpuView->hasManualFontSize;
		}
		viewStates.push_back(state);
	}

	void AddDebugInterface(CDebugInterface *debugInterface)
	{
		if (debugInterface == NULL)
			return;

		for (std::list<CGuiView *>::iterator it = debugInterface->views.begin(); it != debugInterface->views.end(); ++it)
		{
			Add(*it);
		}
	}

	~CScopedDefaultWorkspaceTestViewState()
	{
		for (std::vector<SDefaultWorkspaceTestViewState>::iterator it = viewStates.begin(); it != viewStates.end(); ++it)
		{
			it->view->SetVisible(it->visible);
			it->view->SetPosition(it->posX, it->posY, it->posZ, it->sizeX, it->sizeY);
			it->view->imGuiForceThisFrameNewPosition = it->forceNewPosition;
			it->view->imGuiForceThisFrameNewSize = it->forceNewSize;
			it->view->thisFrameNewPosX = it->newPosX;
			it->view->thisFrameNewPosY = it->newPosY;
			it->view->thisFrameNewSizeX = it->newSizeX;
			it->view->thisFrameNewSizeY = it->newSizeY;
			if (it->disassemblyView != NULL)
			{
				it->disassemblyView->fontSize = it->disassemblyFontSize;
				it->disassemblyView->showHexCodes = it->disassemblyShowHexCodes;
				it->disassemblyView->showCodeCycles = it->disassemblyShowCodeCycles;
				it->disassemblyView->showLabels = it->disassemblyShowLabels;
				it->disassemblyView->labelNumCharacters = it->disassemblyLabelNumCharacters;
				it->disassemblyView->mnemonicsDisplayOffsetX = it->disassemblyMnemonicsDisplayOffsetX;
				it->disassemblyView->codeCyclesDisplayOffsetX = it->disassemblyCodeCyclesDisplayOffsetX;
				it->disassemblyView->LayoutParameterChanged(NULL);
			}
			if (it->dataDumpView != NULL)
			{
				it->dataDumpView->fontSize = it->dataDumpFontSize;
				it->dataDumpView->numberOfBytesPerLine = it->dataDumpNumberOfBytesPerLine;
				it->dataDumpView->showDataCharacters = it->dataDumpShowDataCharacters;
				it->dataDumpView->showCharacters = it->dataDumpShowCharacters;
				it->dataDumpView->showSprites = it->dataDumpShowSprites;
				it->dataDumpView->LayoutParameterChanged(NULL);
			}
			if (it->baseStateCpuView != NULL)
			{
				it->baseStateCpuView->fontSize = it->baseStateCpuFontSize;
				it->baseStateCpuView->hasManualFontSize = it->baseStateCpuHasManualFontSize;
			}
		}
	}

private:
	std::vector<SDefaultWorkspaceTestViewState> viewStates;
};

struct SExpectedDefaultWorkspaceLayoutSpec
{
	EDefaultWorkspaceSlot slot;
	const char *displayName;
	const char *predefinedId;
};

static bool VerifyLayoutSpecs(const std::vector<SDefaultWorkspaceLayoutSpec> &actualSpecs,
							  const SExpectedDefaultWorkspaceLayoutSpec *expectedSpecs,
							  int numExpectedSpecs,
							  const char *platformName)
{
	if ((int)actualSpecs.size() != numExpectedSpecs)
	{
		snprintf(failureMsg, sizeof(failureMsg), "%s expected %d specs, got %zu", platformName, numExpectedSpecs, actualSpecs.size());
		return false;
	}

	for (int i = 0; i < numExpectedSpecs; i++)
	{
		const SDefaultWorkspaceLayoutSpec &actual = actualSpecs[i];
		const SExpectedDefaultWorkspaceLayoutSpec &expected = expectedSpecs[i];
		if (actual.slot != expected.slot
			|| strcmp(actual.displayName, expected.displayName) != 0
			|| strcmp(actual.predefinedId, expected.predefinedId) != 0)
		{
			snprintf(failureMsg, sizeof(failureMsg), "%s spec %d mismatch: got slot=%d name='%s' id='%s'",
					 platformName, i, (int)actual.slot, actual.displayName, actual.predefinedId);
			return false;
		}
	}

	return true;
}

static const SDefaultWorkspaceViewPlacementSpec *FindPlacementSpec(const SDefaultWorkspaceLayoutSpec *layoutSpec, const char *viewId)
{
	if (layoutSpec == NULL || viewId == NULL)
		return NULL;

	for (int i = 0; i < layoutSpec->numPlacements; i++)
	{
		if (strcmp(layoutSpec->placements[i].viewId, viewId) == 0)
			return &layoutSpec->placements[i];
	}

	return NULL;
}

static bool VerifyPlacementSpec(const SDefaultWorkspaceLayoutSpec *layoutSpec, const char *viewId, float x, float y, float width, float height, float fontSize = -1.0f)
{
	const SDefaultWorkspaceViewPlacementSpec *placement = FindPlacementSpec(layoutSpec, viewId);
	if (placement == NULL)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Missing placement '%s'", viewId);
		return false;
	}

	if (!NearlyEqual(placement->x, x)
		|| !NearlyEqual(placement->y, y)
		|| !NearlyEqual(placement->width, width)
		|| !NearlyEqual(placement->height, height)
		|| (fontSize >= 0.0f && !NearlyEqual(placement->fontSize, fontSize)))
	{
		snprintf(failureMsg, sizeof(failureMsg), "Placement '%s' mismatch: got %.3f %.3f %.3f %.3f font %.3f", viewId, placement->x, placement->y, placement->width, placement->height, placement->fontSize);
		return false;
	}

	return true;
}

static bool VerifyQueuedPlacement(CGuiView *view, const char *viewId, float x, float y, float width, float height)
{
	if (view == NULL)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Missing queued placement view '%s'", viewId);
		return false;
	}

	if (!view->imGuiForceThisFrameNewPosition
		|| !view->imGuiForceThisFrameNewSize
		|| !NearlyEqual(view->thisFrameNewPosX, x)
		|| !NearlyEqual(view->thisFrameNewPosY, y)
		|| !NearlyEqual(view->thisFrameNewSizeX, GetExpectedFloatingWindowWidthForInterior(width))
		|| !NearlyEqual(view->thisFrameNewSizeY, GetExpectedFloatingWindowHeightForInterior(height)))
	{
		snprintf(failureMsg, sizeof(failureMsg), "Queued placement '%s' mismatch: force=%d/%d got %.3f %.3f %.3f %.3f, expected %.3f %.3f %.3f %.3f",
				 viewId,
				 view->imGuiForceThisFrameNewPosition ? 1 : 0,
				 view->imGuiForceThisFrameNewSize ? 1 : 0,
				 view->thisFrameNewPosX,
				 view->thisFrameNewPosY,
				 view->thisFrameNewSizeX,
				 view->thisFrameNewSizeY,
				 x,
				 y,
				 GetExpectedFloatingWindowWidthForInterior(width),
				 GetExpectedFloatingWindowHeightForInterior(height));
		return false;
	}

	return true;
}

static bool VerifyDefaultWorkspaceShortcutRegistration()
{
	CSlrKeyboardShortcuts keyboardShortcuts;
	CDefaultWorkspaceLayouts defaultLayouts(NULL, NULL);
	defaultLayouts.RegisterDefaultShortcutSlots(&keyboardShortcuts);

	const SDefaultWorkspaceShortcutSlotSpec *dataDumpSpec = FindDefaultWorkspaceShortcutSlotSpec(DefaultWorkspaceSlot_DataDump);
	const SDefaultWorkspaceShortcutSlotState *dataDumpState = defaultLayouts.GetShortcutSlotState(DefaultWorkspaceSlot_DataDump);
	CSlrKeyboardShortcut *dataDumpShortcut = keyboardShortcuts.FindShortcut(KBZONE_GLOBAL,
																	dataDumpSpec->keyCode,
																	dataDumpSpec->isShift,
																	dataDumpSpec->isAlt,
																	dataDumpSpec->isControl,
																	dataDumpSpec->isSuper);
	if (dataDumpState == NULL || dataDumpState->shortcut == NULL || dataDumpShortcut != dataDumpState->shortcut)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Data Dump default workspace shortcut should be assigned when no conflict exists");
		return false;
	}

	if (strcmp(dataDumpShortcut->name, "Default Workspace: Data Dump") != 0
		|| defaultLayouts.GetShortcutLabel(DefaultWorkspaceSlot_DataDump) == NULL
		|| strcmp(defaultLayouts.GetShortcutLabel(DefaultWorkspaceSlot_DataDump), "Ctrl+F2") != 0
		|| strcmp(defaultLayouts.GetShortcutStatus(DefaultWorkspaceSlot_DataDump), "Ready") != 0)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Assigned shortcut should use deterministic name, label, and Ready status");
		return false;
	}

	defaultLayouts.ClearShortcutSlot(&keyboardShortcuts, DefaultWorkspaceSlot_DataDump);
	dataDumpShortcut = keyboardShortcuts.FindShortcut(KBZONE_GLOBAL,
														 dataDumpSpec->keyCode,
														 dataDumpSpec->isShift,
														 dataDumpSpec->isAlt,
														 dataDumpSpec->isControl,
														 dataDumpSpec->isSuper);
	if (dataDumpShortcut != NULL
		|| strcmp(defaultLayouts.GetShortcutLabel(DefaultWorkspaceSlot_DataDump), "") != 0
		|| strcmp(defaultLayouts.GetShortcutStatus(DefaultWorkspaceSlot_DataDump), "Unassigned") != 0)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Clearing a slot should remove its shortcut and leave it unassigned");
		return false;
	}

	return true;
}

static bool VerifyDefaultWorkspaceShortcutConflicts()
{
	CSlrKeyboardShortcuts keyboardShortcuts;
	const SDefaultWorkspaceShortcutSlotSpec *dataDumpSpec = FindDefaultWorkspaceShortcutSlotSpec(DefaultWorkspaceSlot_DataDump);
	CSlrKeyboardShortcut existingShortcut(KBZONE_GLOBAL,
											 "Existing Ctrl+F2",
											 dataDumpSpec->keyCode,
											 dataDumpSpec->isShift,
											 dataDumpSpec->isAlt,
											 dataDumpSpec->isControl,
											 dataDumpSpec->isSuper,
											 NULL);
	keyboardShortcuts.AddShortcut(&existingShortcut);

	CDefaultWorkspaceLayouts defaultLayouts(NULL, NULL);
	defaultLayouts.RegisterDefaultShortcutSlots(&keyboardShortcuts);

	const SDefaultWorkspaceShortcutSlotState *dataDumpState = defaultLayouts.GetShortcutSlotState(DefaultWorkspaceSlot_DataDump);
	CSlrKeyboardShortcut *foundShortcut = keyboardShortcuts.FindShortcut(KBZONE_GLOBAL,
															 dataDumpSpec->keyCode,
															 dataDumpSpec->isShift,
															 dataDumpSpec->isAlt,
															 dataDumpSpec->isControl,
															 dataDumpSpec->isSuper);
	if (dataDumpState == NULL
		|| dataDumpState->shortcut != NULL
		|| !dataDumpState->hasConflict
		|| strcmp(dataDumpState->conflictShortcutName.c_str(), "Existing Ctrl+F2") != 0
		|| strcmp(defaultLayouts.GetShortcutLabel(DefaultWorkspaceSlot_DataDump), "") != 0
		|| strcmp(defaultLayouts.GetShortcutStatus(DefaultWorkspaceSlot_DataDump), "Conflict: Existing Ctrl+F2") != 0
		|| foundShortcut != &existingShortcut)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Conflicting default shortcut should leave the existing shortcut unchanged and mark the slot unassigned");
		return false;
	}

	return true;
}

static bool VerifyDefaultWorkspaceShortcutAssignment()
{
	CSlrKeyboardShortcuts keyboardShortcuts;
	CDefaultWorkspaceLayouts defaultLayouts(NULL, NULL);

	bool didAssign = defaultLayouts.AssignShortcutSlot(&keyboardShortcuts, DefaultWorkspaceSlot_DataDump, 'z', true, false, true, false);
	const SDefaultWorkspaceShortcutSlotState *dataDumpState = defaultLayouts.GetShortcutSlotState(DefaultWorkspaceSlot_DataDump);
	CSlrKeyboardShortcut *assignedShortcut = keyboardShortcuts.FindShortcut(KBZONE_GLOBAL, 'z', true, false, true, false);
	CSlrKeyboardShortcut *originalAssignedShortcut = assignedShortcut;
	std::string originalAssignedLabel;
	if (!didAssign
		|| dataDumpState == NULL
		|| dataDumpState->shortcut == NULL
		|| dataDumpState->shortcut != assignedShortcut
		|| strcmp(dataDumpState->shortcut->name, "Default Workspace: Data Dump") != 0
		|| !dataDumpState->suppressNextActivation
		|| strcmp(defaultLayouts.GetShortcutStatus(DefaultWorkspaceSlot_DataDump), "Ready") != 0
		|| strlen(defaultLayouts.GetShortcutLabel(DefaultWorkspaceSlot_DataDump)) == 0)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Set shortcut assignment should create a named slot shortcut with Ready status");
		return false;
	}

	if (defaultLayouts.ProcessKeyboardShortcut(KBZONE_GLOBAL, MT_ACTION_TYPE_KEYBOARD_SHORTCUT, dataDumpState->shortcut)
		|| dataDumpState->suppressNextActivation)
	{
		snprintf(failureMsg, sizeof(failureMsg), "A captured shortcut assignment should suppress activation for the same key event once");
		return false;
	}
	originalAssignedLabel = defaultLayouts.GetShortcutLabel(DefaultWorkspaceSlot_DataDump);

	CSlrKeyboardShortcut existingShortcut(KBZONE_GLOBAL, "Existing Ctrl+Y", 'y', false, false, true, false, NULL);
	keyboardShortcuts.AddShortcut(&existingShortcut);
	didAssign = defaultLayouts.AssignShortcutSlot(&keyboardShortcuts, DefaultWorkspaceSlot_DataDump, 'y', false, false, true, false);
	assignedShortcut = keyboardShortcuts.FindShortcut(KBZONE_GLOBAL, 'y', false, false, true, false);
	CSlrKeyboardShortcut *preservedShortcut = keyboardShortcuts.FindShortcut(KBZONE_GLOBAL, 'z', true, false, true, false);
	dataDumpState = defaultLayouts.GetShortcutSlotState(DefaultWorkspaceSlot_DataDump);
	if (didAssign
		|| assignedShortcut != &existingShortcut
		|| dataDumpState == NULL
		|| dataDumpState->shortcut != originalAssignedShortcut
		|| preservedShortcut != originalAssignedShortcut
		|| !dataDumpState->hasConflict
		|| strcmp(dataDumpState->conflictShortcutName.c_str(), "Existing Ctrl+Y") != 0
		|| strcmp(defaultLayouts.GetShortcutLabel(DefaultWorkspaceSlot_DataDump), originalAssignedLabel.c_str()) != 0
		|| strcmp(defaultLayouts.GetShortcutStatus(DefaultWorkspaceSlot_DataDump), "Conflict: Existing Ctrl+Y") != 0)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Set shortcut assignment should reject conflicts without losing the previous slot shortcut");
		return false;
	}

	return true;
}

static bool VerifyDefaultWorkspaceShortcutRestoreAndReset()
{
	CSlrKeyboardShortcuts keyboardShortcuts;
	CDefaultWorkspaceLayouts defaultLayouts(NULL, NULL);
	defaultLayouts.RegisterDefaultShortcutSlots(&keyboardShortcuts);

	const SDefaultWorkspaceShortcutSlotSpec *dataDumpSpec = FindDefaultWorkspaceShortcutSlotSpec(DefaultWorkspaceSlot_DataDump);
	const SDefaultWorkspaceShortcutSlotSpec *debuggerSpec = FindDefaultWorkspaceShortcutSlotSpec(DefaultWorkspaceSlot_Debugger);
	std::vector<SDefaultWorkspaceShortcutSlotSnapshot> snapshot;
	defaultLayouts.SaveShortcutSlotStates(&snapshot);

	defaultLayouts.AssignShortcutSlot(&keyboardShortcuts, DefaultWorkspaceSlot_DataDump, 'z', true, false, true, false);
	defaultLayouts.ClearShortcutSlot(&keyboardShortcuts, DefaultWorkspaceSlot_Debugger);
	defaultLayouts.RestoreShortcutSlotStates(&keyboardShortcuts, snapshot);

	CSlrKeyboardShortcut *customDataDumpShortcut = keyboardShortcuts.FindShortcut(KBZONE_GLOBAL, 'z', true, false, true, false);
	CSlrKeyboardShortcut *defaultDataDumpShortcut = keyboardShortcuts.FindShortcut(KBZONE_GLOBAL,
														dataDumpSpec->keyCode,
														dataDumpSpec->isShift,
														dataDumpSpec->isAlt,
														dataDumpSpec->isControl,
														dataDumpSpec->isSuper);
	CSlrKeyboardShortcut *defaultDebuggerShortcut = keyboardShortcuts.FindShortcut(KBZONE_GLOBAL,
													debuggerSpec->keyCode,
													debuggerSpec->isShift,
													debuggerSpec->isAlt,
													debuggerSpec->isControl,
													debuggerSpec->isSuper);
	if (customDataDumpShortcut != NULL
		|| defaultDataDumpShortcut == NULL
		|| defaultDebuggerShortcut == NULL
		|| strcmp(defaultLayouts.GetShortcutStatus(DefaultWorkspaceSlot_DataDump), "Ready") != 0
		|| strcmp(defaultLayouts.GetShortcutStatus(DefaultWorkspaceSlot_Debugger), "Ready") != 0)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Restoring shortcut slot states should undo popup Set/Clear edits");
		return false;
	}

	defaultLayouts.AssignShortcutSlot(&keyboardShortcuts, DefaultWorkspaceSlot_DataDump, 'z', true, false, true, false);
	defaultLayouts.ClearShortcutSlot(&keyboardShortcuts, DefaultWorkspaceSlot_Debugger);
	defaultLayouts.ResetShortcutSlotsToDefaults(&keyboardShortcuts);

	customDataDumpShortcut = keyboardShortcuts.FindShortcut(KBZONE_GLOBAL, 'z', true, false, true, false);
	defaultDataDumpShortcut = keyboardShortcuts.FindShortcut(KBZONE_GLOBAL,
													 dataDumpSpec->keyCode,
													 dataDumpSpec->isShift,
													 dataDumpSpec->isAlt,
													 dataDumpSpec->isControl,
													 dataDumpSpec->isSuper);
	defaultDebuggerShortcut = keyboardShortcuts.FindShortcut(KBZONE_GLOBAL,
													debuggerSpec->keyCode,
													debuggerSpec->isShift,
													debuggerSpec->isAlt,
													debuggerSpec->isControl,
													debuggerSpec->isSuper);
	if (customDataDumpShortcut != NULL
		|| defaultDataDumpShortcut == NULL
		|| defaultDebuggerShortcut == NULL
		|| strcmp(defaultLayouts.GetShortcutLabel(DefaultWorkspaceSlot_DataDump), dataDumpSpec->defaultShortcutLabel) != 0
		|| strcmp(defaultLayouts.GetShortcutStatus(DefaultWorkspaceSlot_Debugger), "Ready") != 0)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Reset to defaults should restore default slot shortcuts after popup edits");
		return false;
	}

	defaultLayouts.ClearAllShortcutSlots(&keyboardShortcuts);
	defaultDataDumpShortcut = keyboardShortcuts.FindShortcut(KBZONE_GLOBAL,
													 dataDumpSpec->keyCode,
													 dataDumpSpec->isShift,
													 dataDumpSpec->isAlt,
													 dataDumpSpec->isControl,
													 dataDumpSpec->isSuper);
	if (defaultDataDumpShortcut != NULL
		|| strcmp(defaultLayouts.GetShortcutLabel(DefaultWorkspaceSlot_DataDump), "") != 0
		|| strcmp(defaultLayouts.GetShortcutStatus(DefaultWorkspaceSlot_DataDump), "Unassigned") != 0)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Disabling context shortcuts should remove registered default workspace shortcuts");
		return false;
	}

	return true;
}

static bool VerifyDefaultWorkspacePopupCaptureState()
{
	SDefaultWorkspacePopupState popupState;
	if (popupState.ConsumeKeyboardCallbackRemovalRequest())
	{
		snprintf(failureMsg, sizeof(failureMsg), "Popup state should not request callback removal by default");
		return false;
	}

	popupState.StartShortcutCapture(DefaultWorkspaceSlot_DataDump);
	if (popupState.capturingShortcutSlot != DefaultWorkspaceSlot_DataDump
		|| popupState.ConsumeKeyboardCallbackRemovalRequest())
	{
		snprintf(failureMsg, sizeof(failureMsg), "Starting shortcut capture should not immediately request callback removal");
		return false;
	}

	popupState.StopShortcutCapture(true);
	if (popupState.capturingShortcutSlot != (EDefaultWorkspaceSlot)0
		|| !popupState.ConsumeKeyboardCallbackRemovalRequest()
		|| popupState.ConsumeKeyboardCallbackRemovalRequest())
	{
		snprintf(failureMsg, sizeof(failureMsg), "Stopping shortcut capture should request exactly one deferred callback removal");
		return false;
	}

	return true;
}

static bool VerifyDefaultWorkspaceApplyC64DataDumpLayout()
{
	if (viewC64 == NULL)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Global viewC64 should exist before applying default workspace layouts");
		return false;
	}

	CDefaultWorkspaceLayouts defaultLayouts(viewC64, NULL);
	const SDefaultWorkspaceLayoutSpec *layoutSpec = FindDefaultWorkspaceLayoutSpec(DefaultWorkspacePlatform_C64, DefaultWorkspaceSlot_DataDump);
	CGuiView *screenView = defaultLayouts.FindViewForPlacement("c64.screen");
	CGuiView *cpuStateView = defaultLayouts.FindViewForPlacement("c64.cpu_state");
	CGuiView *disassemblyView = defaultLayouts.FindViewForPlacement("c64.disassembly");
	CGuiView *memoryMapView = defaultLayouts.FindViewForPlacement("c64.memory_map");
	CGuiView *dataDumpView = defaultLayouts.FindViewForPlacement("c64.data_dump");
	CGuiView *unrelatedC64View = viewC64->viewC64StateSID;
	CGuiView *unrelatedAtariView = viewC64->viewAtariScreen;
	CGuiView *unrelatedNesView = viewC64->viewNesScreen;

	if (layoutSpec == NULL || screenView == NULL || cpuStateView == NULL || disassemblyView == NULL || memoryMapView == NULL || dataDumpView == NULL
		|| unrelatedC64View == NULL || unrelatedAtariView == NULL || unrelatedNesView == NULL)
	{
		snprintf(failureMsg, sizeof(failureMsg), "C64 Data Dump placement IDs should resolve to current RetroDebugger views");
		return false;
	}

	CScopedDefaultWorkspaceTestViewState restoreViews;
	restoreViews.AddDebugInterface(viewC64->debugInterfaceC64);
	restoreViews.AddDebugInterface(viewC64->debugInterfaceAtari);
	restoreViews.AddDebugInterface(viewC64->debugInterfaceNes);

	screenView->SetVisible(false);
	cpuStateView->SetVisible(false);
	disassemblyView->SetVisible(false);
	memoryMapView->SetVisible(false);
	dataDumpView->SetVisible(false);
	unrelatedC64View->SetVisible(true);
	unrelatedAtariView->SetVisible(true);
	unrelatedNesView->SetVisible(true);

	static const SDefaultWorkspaceViewPlacementSpec invalidPlacements[] = {
		{ "c64.screen", 10.0f, 10.0f, 50.0f, 50.0f },
		{ "c64.missing_view", 20.0f, 20.0f, 50.0f, 50.0f }
	};
	SDefaultWorkspaceLayoutSpec invalidLayoutSpec = { DefaultWorkspacePlatform_C64,
		DefaultWorkspaceSlot_DataDump,
		"Invalid C64 Layout",
		"retro.default.invalid",
		invalidPlacements,
		sizeof(invalidPlacements) / sizeof(invalidPlacements[0]) };
	if (defaultLayouts.ApplyLayoutSpecFloating(&invalidLayoutSpec, 580.0f, 360.0f)
		|| !unrelatedC64View->visible
		|| !unrelatedAtariView->visible
		|| !unrelatedNesView->visible)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Invalid placement specs should fail without hiding existing platform views");
		return false;
	}

	screenView->SetVisible(false);
	cpuStateView->SetVisible(false);
	disassemblyView->SetVisible(false);
	memoryMapView->SetVisible(false);
	dataDumpView->SetVisible(false);
	unrelatedC64View->SetVisible(true);
	unrelatedAtariView->SetVisible(true);
	unrelatedNesView->SetVisible(true);

	if (!defaultLayouts.ApplyLayoutSpecFloating(layoutSpec, 580.0f, 360.0f))
	{
		snprintf(failureMsg, sizeof(failureMsg), "Applying C64 Data Dump floating placement should succeed");
		return false;
	}

	if (!screenView->visible
		|| !cpuStateView->visible
		|| !disassemblyView->visible
		|| !memoryMapView->visible
		|| !dataDumpView->visible
		|| !NearlyEqual(screenView->posX, 317.416f)
		|| !NearlyEqual(screenView->posY, 10.5f)
		|| !NearlyEqual(screenView->sizeX, 259.584f)
		|| !NearlyEqual(screenView->sizeY, 183.872f)
		|| !screenView->imGuiForceThisFrameNewPosition
		|| !screenView->imGuiForceThisFrameNewSize
		|| !NearlyEqual(screenView->thisFrameNewPosX, 317.416f)
		|| !NearlyEqual(screenView->thisFrameNewPosY, 10.5f)
		|| !NearlyEqual(screenView->thisFrameNewSizeX, GetExpectedFloatingWindowWidthForInterior(259.584f))
		|| !NearlyEqual(screenView->thisFrameNewSizeY, GetExpectedFloatingWindowHeightForInterior(183.872f)))
	{
		snprintf(failureMsg, sizeof(failureMsg), "Applying C64 Data Dump should show and position the screen view using scaled geometry");
		return false;
	}

	if (!NearlyEqual(cpuStateView->sizeX, 24.0f)
		|| !NearlyEqual(cpuStateView->sizeY, 24.0f)
		|| !cpuStateView->imGuiForceThisFrameNewSize
		|| !NearlyEqual(cpuStateView->thisFrameNewSizeX, GetExpectedFloatingWindowWidthForInterior(24.0f))
		|| !NearlyEqual(cpuStateView->thisFrameNewSizeY, GetExpectedFloatingWindowHeightForInterior(24.0f)))
	{
		snprintf(failureMsg, sizeof(failureMsg), "Applying C64 Data Dump should queue CPU State outer size for 24px interior, got body %.3f %.3f queued %.3f %.3f",
				 cpuStateView->sizeX,
				 cpuStateView->sizeY,
				 cpuStateView->thisFrameNewSizeX,
				 cpuStateView->thisFrameNewSizeY);
		return false;
	}

	if (unrelatedC64View->visible
		|| unrelatedAtariView->visible
		|| unrelatedNesView->visible)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Applying C64 Data Dump should hide unrelated C64/Atari/NES workspace views");
		return false;
	}

	if (!NearlyEqual(disassemblyView->posX, 1.0f)
		|| !NearlyEqual(disassemblyView->posY, 1.0f)
		|| !NearlyEqual(disassemblyView->sizeX, 105.0f)
		|| !NearlyEqual(disassemblyView->sizeY, 356.0f))
	{
		snprintf(failureMsg, sizeof(failureMsg), "Applying C64 Data Dump should position the disassembly view using old geometry");
		return false;
	}

	if (!defaultLayouts.ApplyLayoutSpecFloating(layoutSpec, 580.0f, 360.0f, true))
	{
		snprintf(failureMsg, sizeof(failureMsg), "Applying C64 Data Dump NoTabBar floating placement should succeed");
		return false;
	}

	float expectedNoTabBarPosY = 10.5f - ImGui::GetFrameHeight();
	if (!screenView->imGuiForceThisFrameNewPosition
		|| !NearlyEqual(screenView->thisFrameNewPosX, 317.416f)
		|| !NearlyEqual(screenView->thisFrameNewPosY, expectedNoTabBarPosY)
		|| !NearlyEqual(screenView->posY, 10.5f))
	{
		snprintf(failureMsg, sizeof(failureMsg), "NoTabBar preserve-scan floating placement should queue outer Y %.3f so the body scans at %.3f, got queued %.3f body %.3f",
				 expectedNoTabBarPosY,
				 10.5f,
				 screenView->thisFrameNewPosY,
				 screenView->posY);
		return false;
	}

	return true;
}

static bool VerifyDefaultWorkspaceHidesGlobalDebugLog()
{
	if (viewC64 == NULL)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Global viewC64 should exist before checking Debug Log default workspace behavior");
		return false;
	}
	if (guiViewDebugLog == NULL)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Global Debug Log view should exist in DBG_Log builds");
		return false;
	}

	CDefaultWorkspaceLayouts defaultLayouts(viewC64, NULL);
	const SDefaultWorkspaceLayoutSpec *layoutSpec = FindDefaultWorkspaceLayoutSpec(DefaultWorkspacePlatform_C64, DefaultWorkspaceSlot_DataDump);
	if (layoutSpec == NULL)
	{
		snprintf(failureMsg, sizeof(failureMsg), "C64 Data Dump default workspace spec should exist before checking Debug Log visibility");
		return false;
	}

	CScopedDefaultWorkspaceTestViewState restoreViews;
	restoreViews.AddDebugInterface(viewC64->debugInterfaceC64);
	restoreViews.AddDebugInterface(viewC64->debugInterfaceAtari);
	restoreViews.AddDebugInterface(viewC64->debugInterfaceNes);
	restoreViews.Add(guiViewDebugLog);

	guiViewDebugLog->SetVisible(true);
	if (!defaultLayouts.ApplyLayoutSpecFloating(layoutSpec, 580.0f, 360.0f))
	{
		snprintf(failureMsg, sizeof(failureMsg), "Applying C64 Data Dump floating placement should succeed before checking Debug Log visibility");
		return false;
	}

	if (guiViewDebugLog->visible)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Default workspace generation should hide global Debug Log so it is not saved into generated layouts");
		return false;
	}

	return true;
}

static bool VerifyDefaultWorkspaceApplyC64DebuggerLayout()
{
	if (viewC64 == NULL)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Global viewC64 should exist before applying C64 Debugger default workspace layouts");
		return false;
	}

	CDefaultWorkspaceLayouts defaultLayouts(viewC64, NULL);
	const SDefaultWorkspaceLayoutSpec *layoutSpec = FindDefaultWorkspaceLayoutSpec(DefaultWorkspacePlatform_C64, DefaultWorkspaceSlot_Debugger);
	CGuiView *screenView = defaultLayouts.FindViewForPlacement("c64.screen");
	CGuiView *cpuStateView = defaultLayouts.FindViewForPlacement("c64.cpu_state");
	CGuiView *disassemblyView = defaultLayouts.FindViewForPlacement("c64.disassembly");
	CGuiView *dataDumpView = defaultLayouts.FindViewForPlacement("c64.data_dump");
	CGuiView *vicStateView = defaultLayouts.FindViewForPlacement("c64.vic_state");

	if (layoutSpec == NULL || screenView == NULL || cpuStateView == NULL || disassemblyView == NULL || dataDumpView == NULL || vicStateView == NULL)
	{
		snprintf(failureMsg, sizeof(failureMsg), "C64 Debugger placement IDs should resolve to current RetroDebugger views");
		return false;
	}

	CScopedDefaultWorkspaceTestViewState restoreViews;
	restoreViews.AddDebugInterface(viewC64->debugInterfaceC64);
	restoreViews.AddDebugInterface(viewC64->debugInterfaceAtari);
	restoreViews.AddDebugInterface(viewC64->debugInterfaceNes);

	screenView->SetVisible(false);
	cpuStateView->SetVisible(false);
	disassemblyView->SetVisible(false);
	dataDumpView->SetVisible(false);
	vicStateView->SetVisible(false);

	if (!defaultLayouts.ApplyLayoutSpecFloating(layoutSpec, 580.0f, 360.0f))
	{
		snprintf(failureMsg, sizeof(failureMsg), "Applying C64 Debugger floating placement should succeed");
		return false;
	}

	if (!screenView->visible
		|| !cpuStateView->visible
		|| !disassemblyView->visible
		|| !dataDumpView->visible
		|| !vicStateView->visible)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Applying C64 Debugger should show all old debugger views");
		return false;
	}

	if (!NearlyEqual(vicStateView->posX, 440.0f)
		|| !NearlyEqual(vicStateView->posY, 0.0f)
		|| !NearlyEqual(vicStateView->sizeX, 136.0f)
		|| !NearlyEqual(vicStateView->sizeY, 192.0f))
	{
		snprintf(failureMsg, sizeof(failureMsg), "Applying C64 Debugger should position the VIC state view using old geometry");
		return false;
	}

	return true;
}

static bool VerifyDefaultWorkspaceApplyC64MemoryMapLayout()
{
	if (viewC64 == NULL)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Global viewC64 should exist before applying C64 Memory Map default workspace layouts");
		return false;
	}

	CDefaultWorkspaceLayouts defaultLayouts(viewC64, NULL);
	const SDefaultWorkspaceLayoutSpec *layoutSpec = FindDefaultWorkspaceLayoutSpec(DefaultWorkspacePlatform_C64, DefaultWorkspaceSlot_MemoryMap);
	CGuiView *screenView = defaultLayouts.FindViewForPlacement("c64.screen");
	CGuiView *cpuStateView = defaultLayouts.FindViewForPlacement("c64.cpu_state");
	CGuiView *disassemblyView = defaultLayouts.FindViewForPlacement("c64.disassembly");
	CGuiView *memoryMapView = defaultLayouts.FindViewForPlacement("c64.memory_map");
	CViewDataDump *dataDumpView = dynamic_cast<CViewDataDump *>(defaultLayouts.FindViewForPlacement("c64.data_dump"));

	if (layoutSpec == NULL || screenView == NULL || cpuStateView == NULL || disassemblyView == NULL || memoryMapView == NULL || dataDumpView == NULL)
	{
		snprintf(failureMsg, sizeof(failureMsg), "C64 Memory Map placement IDs should resolve to current RetroDebugger views");
		return false;
	}

	CScopedDefaultWorkspaceTestViewState restoreViews;
	restoreViews.AddDebugInterface(viewC64->debugInterfaceC64);
	restoreViews.AddDebugInterface(viewC64->debugInterfaceAtari);
	restoreViews.AddDebugInterface(viewC64->debugInterfaceNes);

	dataDumpView->showDataCharacters = true;
	dataDumpView->showCharacters = false;
	dataDumpView->showSprites = false;
	dataDumpView->LayoutParameterChanged(NULL);

	if (!defaultLayouts.ApplyLayoutSpecFloating(layoutSpec, 1160.0f, 720.0f))
	{
		snprintf(failureMsg, sizeof(failureMsg), "Applying C64 Memory Map floating placement should succeed");
		return false;
	}

	float renderedWidth = GetExpectedDataDumpRenderedWidth(dataDumpView);
	float visibleWidth = GetVisibleWidthWithinViewport(dataDumpView, 1160.0f);
	if (renderedWidth > visibleWidth + 0.001f)
	{
		snprintf(failureMsg, sizeof(failureMsg), "C64 Memory Map data dump should fit within visible right-column width, got rendered %.3f visible %.3f bytes %d",
				 renderedWidth,
				 visibleWidth,
				 dataDumpView->numberOfBytesPerLine);
		return false;
	}

	return true;
}

static bool VerifyDefaultWorkspaceApplyC64DriveDebuggerLayout()
{
	if (viewC64 == NULL)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Global viewC64 should exist before applying C64 1541 Debugger default workspace layouts");
		return false;
	}

	CDefaultWorkspaceLayouts defaultLayouts(viewC64, NULL);
	const SDefaultWorkspaceLayoutSpec *layoutSpec = FindDefaultWorkspaceLayoutSpec(DefaultWorkspacePlatform_C64, DefaultWorkspaceSlot_1541Debugger);
	CGuiView *screenView = defaultLayouts.FindViewForPlacement("c64.screen");
	CGuiView *c64CpuStateView = defaultLayouts.FindViewForPlacement("c64.cpu_state");
	CGuiView *c64DisassemblyView = defaultLayouts.FindViewForPlacement("c64.disassembly");
	CGuiView *driveCpuStateView = defaultLayouts.FindViewForPlacement("drive1541.cpu_state");
	CGuiView *driveDisassemblyView = defaultLayouts.FindViewForPlacement("drive1541.disassembly");
	CGuiView *driveViaStateView = defaultLayouts.FindViewForPlacement("drive1541.via_state");
	CGuiView *c64MemoryMapView = defaultLayouts.FindViewForPlacement("c64.memory_map");
	CGuiView *driveMemoryMapView = defaultLayouts.FindViewForPlacement("drive1541.memory_map");
	CGuiView *c64CiaStateView = defaultLayouts.FindViewForPlacement("c64.cia_state");

	if (layoutSpec == NULL || screenView == NULL || c64CpuStateView == NULL || c64DisassemblyView == NULL
		|| driveCpuStateView == NULL || driveDisassemblyView == NULL || driveViaStateView == NULL
		|| c64MemoryMapView == NULL || driveMemoryMapView == NULL || c64CiaStateView == NULL)
	{
		snprintf(failureMsg, sizeof(failureMsg), "C64 1541 Debugger placement IDs should resolve to current RetroDebugger views");
		return false;
	}

	CScopedDefaultWorkspaceTestViewState restoreViews;
	restoreViews.AddDebugInterface(viewC64->debugInterfaceC64);
	restoreViews.AddDebugInterface(viewC64->debugInterfaceAtari);
	restoreViews.AddDebugInterface(viewC64->debugInterfaceNes);

	screenView->SetVisible(false);
	c64CpuStateView->SetVisible(false);
	c64DisassemblyView->SetVisible(false);
	driveCpuStateView->SetVisible(false);
	driveDisassemblyView->SetVisible(false);
	driveViaStateView->SetVisible(false);
	c64MemoryMapView->SetVisible(false);
	driveMemoryMapView->SetVisible(false);
	c64CiaStateView->SetVisible(false);

	if (!defaultLayouts.ApplyLayoutSpecFloating(layoutSpec, 580.0f, 360.0f))
	{
		snprintf(failureMsg, sizeof(failureMsg), "Applying C64 1541 Debugger floating placement should succeed");
		return false;
	}

	if (!screenView->visible
		|| !c64CpuStateView->visible
		|| !c64DisassemblyView->visible
		|| !driveCpuStateView->visible
		|| !driveDisassemblyView->visible
		|| !driveViaStateView->visible
		|| !c64MemoryMapView->visible
		|| !driveMemoryMapView->visible
		|| !c64CiaStateView->visible)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Applying C64 1541 Debugger should show all old drive debugger views");
		return false;
	}

	if (!NearlyEqual(driveViaStateView->posX, 342.0f)
		|| !NearlyEqual(driveViaStateView->posY, 310.0f)
		|| !NearlyEqual(driveViaStateView->sizeX, 240.0f)
		|| !NearlyEqual(driveViaStateView->sizeY, 50.0f))
	{
		snprintf(failureMsg, sizeof(failureMsg), "Applying C64 1541 Debugger should position the drive VIA state view using old geometry");
		return false;
	}

	return true;
}

static bool VerifyDefaultWorkspaceApplyC64DriveMemoryMapLayout()
{
	if (viewC64 == NULL)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Global viewC64 should exist before applying C64 1541 Memory Map default workspace layouts");
		return false;
	}

	CDefaultWorkspaceLayouts defaultLayouts(viewC64, NULL);
	const SDefaultWorkspaceLayoutSpec *layoutSpec = FindDefaultWorkspaceLayoutSpec(DefaultWorkspacePlatform_C64, DefaultWorkspaceSlot_1541MemoryMap);
	CGuiView *screenView = defaultLayouts.FindViewForPlacement("c64.screen");
	CGuiView *c64CpuStateView = defaultLayouts.FindViewForPlacement("c64.cpu_state");
	CGuiView *c64DisassemblyView = defaultLayouts.FindViewForPlacement("c64.disassembly");
	CGuiView *driveCpuStateView = defaultLayouts.FindViewForPlacement("drive1541.cpu_state");
	CGuiView *driveDisassemblyView = defaultLayouts.FindViewForPlacement("drive1541.disassembly");
	CGuiView *c64MemoryMapView = defaultLayouts.FindViewForPlacement("c64.memory_map");
	CGuiView *driveMemoryMapView = defaultLayouts.FindViewForPlacement("drive1541.memory_map");
	CViewBaseStateCPU *c64CpuState = dynamic_cast<CViewBaseStateCPU *>(c64CpuStateView);
	CViewBaseStateCPU *driveCpuState = dynamic_cast<CViewBaseStateCPU *>(driveCpuStateView);
	CViewDisassembly *c64Disassembly = dynamic_cast<CViewDisassembly *>(c64DisassemblyView);
	CViewDisassembly *driveDisassembly = dynamic_cast<CViewDisassembly *>(driveDisassemblyView);

	if (layoutSpec == NULL || screenView == NULL || c64CpuStateView == NULL || c64DisassemblyView == NULL
		|| driveCpuStateView == NULL || driveDisassemblyView == NULL || c64MemoryMapView == NULL || driveMemoryMapView == NULL
		|| c64CpuState == NULL || driveCpuState == NULL || c64Disassembly == NULL || driveDisassembly == NULL)
	{
		snprintf(failureMsg, sizeof(failureMsg), "C64 1541 Memory Map placement IDs should resolve to current RetroDebugger views");
		return false;
	}

	CScopedDefaultWorkspaceTestViewState restoreViews;
	restoreViews.AddDebugInterface(viewC64->debugInterfaceC64);
	restoreViews.AddDebugInterface(viewC64->debugInterfaceAtari);
	restoreViews.AddDebugInterface(viewC64->debugInterfaceNes);

	screenView->SetVisible(false);
	c64CpuStateView->SetVisible(false);
	c64DisassemblyView->SetVisible(false);
	driveCpuStateView->SetVisible(false);
	driveDisassemblyView->SetVisible(false);
	c64MemoryMapView->SetVisible(false);
	driveMemoryMapView->SetVisible(false);
	c64Disassembly->showHexCodes = true;
	c64Disassembly->showCodeCycles = true;
	c64Disassembly->showLabels = true;
	driveDisassembly->showHexCodes = true;
	driveDisassembly->showCodeCycles = true;
	driveDisassembly->showLabels = true;

	if (!defaultLayouts.ApplyLayoutSpecFloating(layoutSpec, 580.0f, 360.0f))
	{
		snprintf(failureMsg, sizeof(failureMsg), "Applying C64 1541 Memory Map floating placement should succeed");
		return false;
	}

	if (!screenView->visible
		|| !c64CpuStateView->visible
		|| !c64DisassemblyView->visible
		|| !driveCpuStateView->visible
		|| !driveDisassemblyView->visible
		|| !c64MemoryMapView->visible
		|| !driveMemoryMapView->visible)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Applying C64 1541 Memory Map should show all old dual-memory-map views");
		return false;
	}

	if (!NearlyEqual(c64DisassemblyView->posX, 0.5f)
		|| !NearlyEqual(c64DisassemblyView->posY, 17.0f)
		|| !NearlyEqual(c64DisassemblyView->sizeX, 178.5f)
		|| !NearlyEqual(c64DisassemblyView->sizeY, 338.0f)
		|| !NearlyEqual(driveDisassemblyView->posX, 429.0f)
		|| !NearlyEqual(driveDisassemblyView->posY, 17.0f)
		|| !NearlyEqual(driveDisassemblyView->sizeX, 150.0f)
		|| !NearlyEqual(driveDisassemblyView->sizeY, 338.0f)
		|| !NearlyEqual(c64MemoryMapView->posX, 190.0f)
		|| !NearlyEqual(c64MemoryMapView->posY, 155.0f)
		|| !NearlyEqual(c64MemoryMapView->sizeX, 115.0f)
		|| !NearlyEqual(c64MemoryMapView->sizeY, 200.0f)
		|| !NearlyEqual(driveMemoryMapView->posX, 306.0f)
		|| !NearlyEqual(driveMemoryMapView->posY, 155.0f)
		|| !NearlyEqual(driveMemoryMapView->sizeX, 115.0f)
		|| !NearlyEqual(driveMemoryMapView->sizeY, 200.0f))
	{
		snprintf(failureMsg, sizeof(failureMsg), "Applying C64 1541 Memory Map should position views using old geometry");
		return false;
	}

	if (!NearlyEqual(c64CpuStateView->posX, c64DisassemblyView->posX)
		|| c64CpuStateView->posX + c64CpuStateView->sizeX > screenView->posX + 0.001f
		|| screenView->posX + screenView->sizeX > driveCpuStateView->posX + 0.001f
		|| driveDisassemblyView->posX < driveCpuStateView->posX
		|| driveDisassemblyView->posX + driveDisassemblyView->sizeX > driveCpuStateView->posX + driveCpuStateView->sizeX + 0.001f
		|| !c64CpuState->hasManualFontSize
		|| !driveCpuState->hasManualFontSize
		|| !NearlyEqual(c64CpuState->fontSize, 3.5f)
		|| !NearlyEqual(driveCpuState->fontSize, 5.0f))
	{
		snprintf(failureMsg, sizeof(failureMsg), "C64 1541 Memory Map should place readable C64/1541 CPU status views in left/right columns, got C64 CPU %.3f %.3f %.3f font %.3f manual %d, screen %.3f %.3f, 1541 CPU %.3f %.3f %.3f font %.3f manual %d, 1541 disasm %.3f %.3f",
				 c64CpuStateView->posX,
				 c64CpuStateView->posY,
				 c64CpuStateView->sizeX,
				 c64CpuState->fontSize,
				 c64CpuState->hasManualFontSize ? 1 : 0,
				 screenView->posX,
				 screenView->posX + screenView->sizeX,
				 driveCpuStateView->posX,
				 driveCpuStateView->posY,
				 driveCpuStateView->sizeX,
				 driveCpuState->fontSize,
				 driveCpuState->hasManualFontSize ? 1 : 0,
				 driveDisassemblyView->posX,
				 driveDisassemblyView->posX + driveDisassemblyView->sizeX);
		return false;
	}

	if (!NearlyEqual(c64Disassembly->fontSize, 7.14f)
		|| !NearlyEqual(driveDisassembly->fontSize, 6.0f))
	{
		snprintf(failureMsg, sizeof(failureMsg), "C64 1541 Memory Map should use 60%% fitted disassembly fonts, got C64 %.3f and 1541 %.3f",
				 c64Disassembly->fontSize,
				 driveDisassembly->fontSize);
		return false;
	}

	if (c64Disassembly->showHexCodes
		|| c64Disassembly->showCodeCycles
		|| c64Disassembly->showLabels
		|| driveDisassembly->showHexCodes
		|| driveDisassembly->showCodeCycles
		|| driveDisassembly->showLabels)
	{
		snprintf(failureMsg, sizeof(failureMsg), "C64 1541 Memory Map should restore old compact disassembly columns");
		return false;
	}

	return true;
}

static bool VerifyDefaultWorkspaceC64DisassemblyParametersDoNotLeakBetweenLayouts()
{
	if (viewC64 == NULL)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Global viewC64 should exist before checking C64 disassembly layout parameters");
		return false;
	}

	CDefaultWorkspaceLayouts defaultLayouts(viewC64, NULL);
	const SDefaultWorkspaceLayoutSpec *driveMemoryMapSpec = FindDefaultWorkspaceLayoutSpec(DefaultWorkspacePlatform_C64, DefaultWorkspaceSlot_1541MemoryMap);
	const SDefaultWorkspaceLayoutSpec *monitorConsoleSpec = FindDefaultWorkspaceLayoutSpec(DefaultWorkspacePlatform_C64, DefaultWorkspaceSlot_MonitorConsole);
	const SDefaultWorkspaceLayoutSpec *cyclerSpec = FindDefaultWorkspaceLayoutSpec(DefaultWorkspacePlatform_C64, DefaultWorkspaceSlot_Cycler);
	const SDefaultWorkspaceLayoutSpec *vicDisplaySpec = FindDefaultWorkspaceLayoutSpec(DefaultWorkspacePlatform_C64, DefaultWorkspaceSlot_VicDisplay);
	const SDefaultWorkspaceLayoutSpec *sourceCodeSpec = FindDefaultWorkspaceLayoutSpec(DefaultWorkspacePlatform_C64, DefaultWorkspaceSlot_SourceCode);
	CViewDisassembly *c64Disassembly = dynamic_cast<CViewDisassembly *>(defaultLayouts.FindViewForPlacement("c64.disassembly"));

	if (driveMemoryMapSpec == NULL || monitorConsoleSpec == NULL || cyclerSpec == NULL || vicDisplaySpec == NULL || sourceCodeSpec == NULL || c64Disassembly == NULL)
	{
		snprintf(failureMsg, sizeof(failureMsg), "C64 disassembly leakage test should resolve all layout specs and the C64 disassembly view");
		return false;
	}

	CScopedDefaultWorkspaceTestViewState restoreViews;
	restoreViews.AddDebugInterface(viewC64->debugInterfaceC64);
	restoreViews.AddDebugInterface(viewC64->debugInterfaceAtari);
	restoreViews.AddDebugInterface(viewC64->debugInterfaceNes);

	c64Disassembly->showHexCodes = true;
	c64Disassembly->showCodeCycles = true;
	c64Disassembly->showLabels = true;
	c64Disassembly->LayoutParameterChanged(NULL);

	if (!defaultLayouts.ApplyLayoutSpecFloating(driveMemoryMapSpec, 580.0f, 360.0f))
	{
		snprintf(failureMsg, sizeof(failureMsg), "Applying C64 1541 Memory Map before leakage test should succeed");
		return false;
	}

	if (!defaultLayouts.ApplyLayoutSpecFloating(monitorConsoleSpec, 580.0f, 360.0f))
	{
		snprintf(failureMsg, sizeof(failureMsg), "Applying C64 Monitor Console after C64 1541 Memory Map should succeed");
		return false;
	}

	if (!c64Disassembly->showHexCodes || !c64Disassembly->showCodeCycles || c64Disassembly->showLabels)
	{
		snprintf(failureMsg, sizeof(failureMsg), "C64 Monitor Console should restore old disassembly columns instead of inheriting compact 1541 Memory Map columns");
		return false;
	}

	if (!defaultLayouts.ApplyLayoutSpecFloating(cyclerSpec, 580.0f, 360.0f))
	{
		snprintf(failureMsg, sizeof(failureMsg), "Applying C64 Cycler after C64 Monitor Console should succeed");
		return false;
	}

	if (!c64Disassembly->showHexCodes || !c64Disassembly->showCodeCycles || !c64Disassembly->showLabels)
	{
		snprintf(failureMsg, sizeof(failureMsg), "C64 Cycler should restore old labeled disassembly columns instead of inheriting prior layout columns");
		return false;
	}

	if (!defaultLayouts.ApplyLayoutSpecFloating(vicDisplaySpec, 580.0f, 360.0f))
	{
		snprintf(failureMsg, sizeof(failureMsg), "Applying C64 VIC Display after C64 Cycler should succeed");
		return false;
	}

	if (c64Disassembly->showHexCodes || !c64Disassembly->showCodeCycles || !c64Disassembly->showLabels)
	{
		snprintf(failureMsg, sizeof(failureMsg), "C64 VIC Display should restore old label/cycle disassembly columns instead of inheriting prior layout columns");
		return false;
	}

	if (!defaultLayouts.ApplyLayoutSpecFloating(sourceCodeSpec, 580.0f, 360.0f))
	{
		snprintf(failureMsg, sizeof(failureMsg), "Applying C64 Source Code after C64 VIC Display should succeed");
		return false;
	}

	if (c64Disassembly->showHexCodes || c64Disassembly->showCodeCycles || c64Disassembly->showLabels)
	{
		snprintf(failureMsg, sizeof(failureMsg), "C64 Source Code should restore old compact disassembly columns instead of inheriting prior layout columns");
		return false;
	}

	return true;
}

static bool VerifyDefaultWorkspaceApplyC64ShowStatesLayout()
{
	if (viewC64 == NULL)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Global viewC64 should exist before applying C64 Show States default workspace layouts");
		return false;
	}

	CDefaultWorkspaceLayouts defaultLayouts(viewC64, NULL);
	const SDefaultWorkspaceLayoutSpec *layoutSpec = FindDefaultWorkspaceLayoutSpec(DefaultWorkspacePlatform_C64, DefaultWorkspaceSlot_ShowStates);
	CGuiView *screenView = defaultLayouts.FindViewForPlacement("c64.screen");
	CGuiView *cpuStateView = defaultLayouts.FindViewForPlacement("c64.cpu_state");
	CGuiView *vicStateView = defaultLayouts.FindViewForPlacement("c64.vic_state");
	CGuiView *sidStateView = defaultLayouts.FindViewForPlacement("c64.sid_state");
	CGuiView *ciaStateView = defaultLayouts.FindViewForPlacement("c64.cia_state");
	CGuiView *reuStateView = defaultLayouts.FindViewForPlacement("c64.reu_state");
	CGuiView *emulationCountersView = defaultLayouts.FindViewForPlacement("c64.emulation_counters");
	CGuiView *driveViaStateView = defaultLayouts.FindViewForPlacement("drive1541.via_state");
	CGuiView *emulationStateView = defaultLayouts.FindViewForPlacement("c64.emulation_state");

	if (layoutSpec == NULL || screenView == NULL || cpuStateView == NULL || vicStateView == NULL
		|| sidStateView == NULL || ciaStateView == NULL || reuStateView == NULL || emulationCountersView == NULL
		|| driveViaStateView == NULL || emulationStateView == NULL)
	{
		snprintf(failureMsg, sizeof(failureMsg), "C64 Show States placement IDs should resolve to current RetroDebugger views");
		return false;
	}

	CScopedDefaultWorkspaceTestViewState restoreViews;
	restoreViews.AddDebugInterface(viewC64->debugInterfaceC64);
	restoreViews.AddDebugInterface(viewC64->debugInterfaceAtari);
	restoreViews.AddDebugInterface(viewC64->debugInterfaceNes);
	restoreViews.Add(emulationStateView);

	screenView->SetVisible(false);
	cpuStateView->SetVisible(false);
	vicStateView->SetVisible(false);
	sidStateView->SetVisible(false);
	ciaStateView->SetVisible(false);
	reuStateView->SetVisible(false);
	emulationCountersView->SetVisible(false);
	driveViaStateView->SetVisible(false);
	emulationStateView->SetVisible(false);

	if (!defaultLayouts.ApplyLayoutSpecFloating(layoutSpec, 580.0f, 360.0f))
	{
		snprintf(failureMsg, sizeof(failureMsg), "Applying C64 Show States floating placement should succeed");
		return false;
	}

	if (!screenView->visible
		|| !cpuStateView->visible
		|| !vicStateView->visible
		|| !sidStateView->visible
		|| !ciaStateView->visible
		|| !reuStateView->visible
		|| !emulationCountersView->visible
		|| !driveViaStateView->visible
		|| !emulationStateView->visible)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Applying C64 Show States should show all old state views");
		return false;
	}

	if (!NearlyEqual(vicStateView->posX, 13.0f)
		|| !NearlyEqual(vicStateView->posY, 13.0f)
		|| !NearlyEqual(vicStateView->sizeX, 290.0f)
		|| !NearlyEqual(vicStateView->sizeY, 160.0f)
		|| !NearlyEqual(sidStateView->posX, 0.0f)
		|| !NearlyEqual(sidStateView->posY, 190.0f)
		|| !NearlyEqual(reuStateView->posX, 315.0f)
		|| !NearlyEqual(reuStateView->posY, 315.0f)
		|| !NearlyEqual(emulationCountersView->posX, 496.0f)
		|| !NearlyEqual(emulationCountersView->posY, 335.0f)
		|| !NearlyEqual(driveViaStateView->posX, 190.0f)
		|| !NearlyEqual(driveViaStateView->posY, 265.0f)
		|| !NearlyEqual(driveViaStateView->sizeX, 240.0f)
		|| !NearlyEqual(driveViaStateView->sizeY, 50.0f)
		|| !NearlyEqual(emulationStateView->posX, 371.0f)
		|| !NearlyEqual(emulationStateView->posY, 350.0f)
		|| !NearlyEqual(emulationStateView->sizeX, 100.0f)
		|| !NearlyEqual(emulationStateView->sizeY, 100.0f))
	{
		snprintf(failureMsg, sizeof(failureMsg), "Applying C64 Show States should position state views using old geometry");
		return false;
	}

	return true;
}

static bool VerifyDefaultWorkspaceApplyC64MonitorConsoleLayout()
{
	if (viewC64 == NULL)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Global viewC64 should exist before applying C64 Monitor Console default workspace layouts");
		return false;
	}

	CDefaultWorkspaceLayouts defaultLayouts(viewC64, NULL);
	const SDefaultWorkspaceLayoutSpec *layoutSpec = FindDefaultWorkspaceLayoutSpec(DefaultWorkspacePlatform_C64, DefaultWorkspaceSlot_MonitorConsole);
	CGuiView *screenView = defaultLayouts.FindViewForPlacement("c64.screen");
	CGuiView *cpuStateView = defaultLayouts.FindViewForPlacement("c64.cpu_state");
	CGuiView *monitorConsoleView = defaultLayouts.FindViewForPlacement("c64.monitor_console");
	CGuiView *disassemblyView = defaultLayouts.FindViewForPlacement("c64.disassembly");
	CGuiView *dataDumpView = defaultLayouts.FindViewForPlacement("c64.data_dump");
	CGuiView *memoryMapView = defaultLayouts.FindViewForPlacement("c64.memory_map");

	if (layoutSpec == NULL || screenView == NULL || cpuStateView == NULL || monitorConsoleView == NULL
		|| disassemblyView == NULL || dataDumpView == NULL || memoryMapView == NULL)
	{
		snprintf(failureMsg, sizeof(failureMsg), "C64 Monitor Console placement IDs should resolve to current RetroDebugger views");
		return false;
	}

	CScopedDefaultWorkspaceTestViewState restoreViews;
	restoreViews.AddDebugInterface(viewC64->debugInterfaceC64);
	restoreViews.AddDebugInterface(viewC64->debugInterfaceAtari);
	restoreViews.AddDebugInterface(viewC64->debugInterfaceNes);

	screenView->SetVisible(false);
	cpuStateView->SetVisible(false);
	monitorConsoleView->SetVisible(false);
	disassemblyView->SetVisible(false);
	dataDumpView->SetVisible(false);
	memoryMapView->SetVisible(false);

	if (!defaultLayouts.ApplyLayoutSpecFloating(layoutSpec, 580.0f, 360.0f))
	{
		snprintf(failureMsg, sizeof(failureMsg), "Applying C64 Monitor Console floating placement should succeed");
		return false;
	}

	if (!screenView->visible
		|| !cpuStateView->visible
		|| !monitorConsoleView->visible
		|| !disassemblyView->visible
		|| !dataDumpView->visible
		|| !memoryMapView->visible)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Applying C64 Monitor Console should show all old monitor console views with current equivalents");
		return false;
	}

	if (!NearlyEqual(monitorConsoleView->posX, 1.0f)
		|| !NearlyEqual(monitorConsoleView->posY, 1.0f)
		|| !NearlyEqual(monitorConsoleView->sizeX, 310.0f)
		|| !NearlyEqual(monitorConsoleView->sizeY, 194.372f)
		|| !NearlyEqual(disassemblyView->posX, 1.0f)
		|| !NearlyEqual(disassemblyView->posY, 195.5f)
		|| !NearlyEqual(disassemblyView->sizeX, 125.0f)
		|| !NearlyEqual(disassemblyView->sizeY, 159.5f)
		|| !NearlyEqual(dataDumpView->posX, 128.0f)
		|| !NearlyEqual(dataDumpView->posY, 195.5f)
		|| !NearlyEqual(dataDumpView->sizeX, 252.0f)
		|| !NearlyEqual(dataDumpView->sizeY, 165.0f)
		|| !NearlyEqual(memoryMapView->posX, 381.0f)
		|| !NearlyEqual(memoryMapView->posY, 195.5f)
		|| !NearlyEqual(memoryMapView->sizeX, 199.0f)
		|| !NearlyEqual(memoryMapView->sizeY, 164.0f))
	{
		snprintf(failureMsg, sizeof(failureMsg), "Applying C64 Monitor Console should position monitor, disassembly, data dump, and memory map using old geometry");
		return false;
	}

	return true;
}

static bool VerifyDefaultWorkspaceApplyC64FullScreenZoomLayout()
{
	if (viewC64 == NULL)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Global viewC64 should exist before applying C64 Full Screen Zoom default workspace layouts");
		return false;
	}

	CDefaultWorkspaceLayouts defaultLayouts(viewC64, NULL);
	const SDefaultWorkspaceLayoutSpec *layoutSpec = FindDefaultWorkspaceLayoutSpec(DefaultWorkspacePlatform_C64, DefaultWorkspaceSlot_FullScreenZoom);
	CGuiView *screenView = defaultLayouts.FindViewForPlacement("c64.screen");
	CGuiView *cpuStateView = defaultLayouts.FindViewForPlacement("c64.cpu_state");
	CGuiView *disassemblyView = defaultLayouts.FindViewForPlacement("c64.disassembly");
	CGuiView *memoryMapView = defaultLayouts.FindViewForPlacement("c64.memory_map");
	CGuiView *dataDumpView = defaultLayouts.FindViewForPlacement("c64.data_dump");
	CGuiView *unrelatedC64View = viewC64->viewC64StateSID;
	CGuiView *unrelatedAtariView = viewC64->viewAtariScreen;
	CGuiView *unrelatedNesView = viewC64->viewNesScreen;

	if (layoutSpec == NULL || screenView == NULL || cpuStateView == NULL || disassemblyView == NULL || memoryMapView == NULL || dataDumpView == NULL
		|| unrelatedC64View == NULL || unrelatedAtariView == NULL || unrelatedNesView == NULL)
	{
		snprintf(failureMsg, sizeof(failureMsg), "C64 Full Screen Zoom placement IDs should resolve to current RetroDebugger views");
		return false;
	}

	CScopedDefaultWorkspaceTestViewState restoreViews;
	restoreViews.AddDebugInterface(viewC64->debugInterfaceC64);
	restoreViews.AddDebugInterface(viewC64->debugInterfaceAtari);
	restoreViews.AddDebugInterface(viewC64->debugInterfaceNes);

	screenView->SetVisible(false);
	cpuStateView->SetVisible(true);
	disassemblyView->SetVisible(true);
	memoryMapView->SetVisible(true);
	dataDumpView->SetVisible(true);
	unrelatedC64View->SetVisible(true);
	unrelatedAtariView->SetVisible(true);
	unrelatedNesView->SetVisible(true);

	if (!defaultLayouts.ApplyLayoutSpecFloating(layoutSpec, 580.0f, 360.0f))
	{
		snprintf(failureMsg, sizeof(failureMsg), "Applying C64 Full Screen Zoom floating placement should succeed");
		return false;
	}

	if (!screenView->visible
		|| cpuStateView->visible
		|| disassemblyView->visible
		|| memoryMapView->visible
		|| dataDumpView->visible
		|| unrelatedC64View->visible
		|| unrelatedAtariView->visible
		|| unrelatedNesView->visible)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Applying C64 Full Screen Zoom should show only the approximated screen placement and hide other platform workspace views");
		return false;
	}

	if (!NearlyEqual(screenView->posX, 35.882f)
		|| !NearlyEqual(screenView->posY, 0.0f)
		|| !NearlyEqual(screenView->sizeX, 508.235f)
		|| !NearlyEqual(screenView->sizeY, 360.0f))
	{
		snprintf(failureMsg, sizeof(failureMsg), "Applying C64 Full Screen Zoom should position the screen-only approximation using old zoomed-screen geometry");
		return false;
	}

	if (!VerifyQueuedPlacement(screenView, "c64.screen", 35.882f, 0.0f, 508.235f, 360.0f))
	{
		return false;
	}

	return true;
}

static bool VerifyDefaultWorkspaceApplyC64CyclerLayout()
{
	if (viewC64 == NULL)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Global viewC64 should exist before applying C64 Cycler default workspace layouts");
		return false;
	}

	CDefaultWorkspaceLayouts defaultLayouts(viewC64, NULL);
	const SDefaultWorkspaceLayoutSpec *layoutSpec = FindDefaultWorkspaceLayoutSpec(DefaultWorkspacePlatform_C64, DefaultWorkspaceSlot_Cycler);
	CGuiView *screenView = defaultLayouts.FindViewForPlacement("c64.screen");
	CGuiView *cpuStateView = defaultLayouts.FindViewForPlacement("c64.cpu_state");
	CGuiView *disassemblyView = defaultLayouts.FindViewForPlacement("c64.disassembly");
	CGuiView *dataDumpView = defaultLayouts.FindViewForPlacement("c64.data_dump");
	CGuiView *vicStateView = defaultLayouts.FindViewForPlacement("c64.vic_state");

	if (layoutSpec == NULL || screenView == NULL || cpuStateView == NULL || disassemblyView == NULL
		|| dataDumpView == NULL || vicStateView == NULL)
	{
		snprintf(failureMsg, sizeof(failureMsg), "C64 Cycler placement IDs should resolve to current RetroDebugger views");
		return false;
	}

	CScopedDefaultWorkspaceTestViewState restoreViews;
	restoreViews.AddDebugInterface(viewC64->debugInterfaceC64);
	restoreViews.AddDebugInterface(viewC64->debugInterfaceAtari);
	restoreViews.AddDebugInterface(viewC64->debugInterfaceNes);

	screenView->SetVisible(false);
	cpuStateView->SetVisible(false);
	disassemblyView->SetVisible(false);
	dataDumpView->SetVisible(false);
	vicStateView->SetVisible(false);

	if (!defaultLayouts.ApplyLayoutSpecFloating(layoutSpec, 580.0f, 360.0f))
	{
		snprintf(failureMsg, sizeof(failureMsg), "Applying C64 Cycler floating placement should succeed");
		return false;
	}

	if (!screenView->visible
		|| !cpuStateView->visible
		|| !disassemblyView->visible
		|| !dataDumpView->visible
		|| !vicStateView->visible)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Applying C64 Cycler should show all old cycler views with current equivalents");
		return false;
	}

	if (!NearlyEqual(screenView->posX, 317.416f)
		|| !NearlyEqual(screenView->posY, 10.5f)
		|| !NearlyEqual(screenView->sizeX, 259.584f)
		|| !NearlyEqual(screenView->sizeY, 183.872f)
		|| !NearlyEqual(cpuStateView->posX, 317.416f)
		|| !NearlyEqual(cpuStateView->posY, 0.0f)
		|| !NearlyEqual(cpuStateView->sizeX, 24.0f)
		|| !NearlyEqual(cpuStateView->sizeY, 24.0f)
		|| !NearlyEqual(disassemblyView->posX, 1.0f)
		|| !NearlyEqual(disassemblyView->posY, 1.0f)
		|| !NearlyEqual(disassemblyView->sizeX, 315.0f)
		|| !NearlyEqual(disassemblyView->sizeY, 208.0f)
		|| !NearlyEqual(dataDumpView->posX, 0.0f)
		|| !NearlyEqual(dataDumpView->posY, 217.0f)
		|| !NearlyEqual(dataDumpView->sizeX, 313.0f)
		|| !NearlyEqual(dataDumpView->sizeY, 143.0f)
		|| !NearlyEqual(vicStateView->posX, 320.0f)
		|| !NearlyEqual(vicStateView->posY, 330.0f)
		|| !NearlyEqual(vicStateView->sizeX, 256.0f)
		|| !NearlyEqual(vicStateView->sizeY, 28.0f))
	{
		snprintf(failureMsg, sizeof(failureMsg), "Applying C64 Cycler should position screen, CPU state, disassembly, data dump, and VIC state using old geometry");
		return false;
	}

	return true;
}

static bool VerifyDefaultWorkspaceCreatePreservesPopupAssignments()
{
	CSlrKeyboardShortcuts keyboardShortcuts;
	CDefaultWorkspaceLayouts defaultLayouts(NULL, NULL);
	const SDefaultWorkspaceShortcutSlotSpec *dataDumpSpec = FindDefaultWorkspaceShortcutSlotSpec(DefaultWorkspaceSlot_DataDump);
	const SDefaultWorkspaceShortcutSlotSpec *debuggerSpec = FindDefaultWorkspaceShortcutSlotSpec(DefaultWorkspaceSlot_Debugger);
	const SDefaultWorkspaceShortcutSlotSpec *onlySpec = FindDefaultWorkspaceShortcutSlotSpec(DefaultWorkspaceSlot_Only);

	defaultLayouts.AssignShortcutSlot(&keyboardShortcuts, DefaultWorkspaceSlot_DataDump, 'z', true, false, true, false);
	defaultLayouts.ClearShortcutSlot(&keyboardShortcuts, DefaultWorkspaceSlot_Debugger);
	defaultLayouts.RegisterDefaultShortcutSlots(&keyboardShortcuts);

	const SDefaultWorkspaceShortcutSlotState *dataDumpState = defaultLayouts.GetShortcutSlotState(DefaultWorkspaceSlot_DataDump);
	const SDefaultWorkspaceShortcutSlotState *debuggerState = defaultLayouts.GetShortcutSlotState(DefaultWorkspaceSlot_Debugger);
	const SDefaultWorkspaceShortcutSlotState *onlyState = defaultLayouts.GetShortcutSlotState(DefaultWorkspaceSlot_Only);
	CSlrKeyboardShortcut *customDataDumpShortcut = keyboardShortcuts.FindShortcut(KBZONE_GLOBAL, 'z', true, false, true, false);
	CSlrKeyboardShortcut *defaultDataDumpShortcut = keyboardShortcuts.FindShortcut(KBZONE_GLOBAL,
																	dataDumpSpec->keyCode,
																	dataDumpSpec->isShift,
																	dataDumpSpec->isAlt,
																	dataDumpSpec->isControl,
																	dataDumpSpec->isSuper);
	CSlrKeyboardShortcut *defaultDebuggerShortcut = keyboardShortcuts.FindShortcut(KBZONE_GLOBAL,
																	 debuggerSpec->keyCode,
																	 debuggerSpec->isShift,
																	 debuggerSpec->isAlt,
																	 debuggerSpec->isControl,
																	 debuggerSpec->isSuper);
	CSlrKeyboardShortcut *defaultOnlyShortcut = keyboardShortcuts.FindShortcut(KBZONE_GLOBAL,
															 onlySpec->keyCode,
															 onlySpec->isShift,
															 onlySpec->isAlt,
															 onlySpec->isControl,
															 onlySpec->isSuper);

	if (dataDumpState == NULL
		|| dataDumpState->shortcut != customDataDumpShortcut
		|| defaultDataDumpShortcut != NULL
		|| debuggerState == NULL
		|| debuggerState->shortcut != NULL
		|| defaultDebuggerShortcut != NULL
		|| onlyState == NULL
		|| onlyState->shortcut != defaultOnlyShortcut
		|| defaultOnlyShortcut == NULL)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Create/default registration should preserve popup Set/Clear choices and fill untouched slots");
		return false;
	}

	return true;
}

static bool VerifyDefaultWorkspaceShortcutPersistenceAcrossStartup()
{
	const char *configPath = "/tmp/retrodebugger-default-workspace-shortcuts-test.hjson";
	remove(configPath);

	CLayoutManager layoutManager(NULL);
	CDefaultWorkspaceLayouts sourceLayouts(NULL, &layoutManager);
	CSlrKeyboardShortcuts sourceKeyboardShortcuts;
	const SDefaultWorkspaceLayoutSpec *dataDumpLayoutSpec = FindDefaultWorkspaceLayoutSpec(DefaultWorkspacePlatform_C64, DefaultWorkspaceSlot_DataDump);
	const SDefaultWorkspaceShortcutSlotSpec *dataDumpShortcutSpec = FindDefaultWorkspaceShortcutSlotSpec(DefaultWorkspaceSlot_DataDump);
	const SDefaultWorkspaceShortcutSlotSpec *debuggerShortcutSpec = FindDefaultWorkspaceShortcutSlotSpec(DefaultWorkspaceSlot_Debugger);
	const SDefaultWorkspaceShortcutSlotSpec *onlyShortcutSpec = FindDefaultWorkspaceShortcutSlotSpec(DefaultWorkspaceSlot_Only);
	if (dataDumpLayoutSpec == NULL || dataDumpShortcutSpec == NULL || debuggerShortcutSpec == NULL || onlyShortcutSpec == NULL)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Default workspace specs should exist before persistence test");
		return false;
	}

	if (sourceLayouts.CreateOrUpdateDefaultWorkspaceLayoutData(dataDumpLayoutSpec, true) == NULL)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Persistence test should create at least one generated default workspace layout");
		return false;
	}

	sourceLayouts.AssignShortcutSlot(&sourceKeyboardShortcuts, DefaultWorkspaceSlot_DataDump, '2', false, false, true, false);
	sourceLayouts.ClearShortcutSlot(&sourceKeyboardShortcuts, DefaultWorkspaceSlot_Debugger);
	CConfigStorageHjson saveConfig(configPath, false);
	sourceLayouts.SaveShortcutSlotSettings(&saveConfig, true);

	CConfigStorageHjson loadConfig(configPath, false);
	CLayoutManager emptyLayoutManager(NULL);
	CDefaultWorkspaceLayouts noGeneratedLayouts(NULL, &emptyLayoutManager);
	CSlrKeyboardShortcuts noGeneratedKeyboardShortcuts;
	if (noGeneratedLayouts.RegisterStoredShortcutSlotsForGeneratedLayouts(&noGeneratedKeyboardShortcuts, &loadConfig)
		|| noGeneratedKeyboardShortcuts.FindShortcut(KBZONE_GLOBAL, '2', false, false, true, false) != NULL)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Startup should not restore shortcut slots before generated default workspaces exist");
		return false;
	}

	CDefaultWorkspaceLayouts restoredLayouts(NULL, &layoutManager);
	CSlrKeyboardShortcuts restoredKeyboardShortcuts;
	if (!restoredLayouts.RegisterStoredShortcutSlotsForGeneratedLayouts(&restoredKeyboardShortcuts, &loadConfig))
	{
		snprintf(failureMsg, sizeof(failureMsg), "Startup should restore shortcut slots when generated default workspaces exist");
		return false;
	}

	CSlrKeyboardShortcut *customDataDumpShortcut = restoredKeyboardShortcuts.FindShortcut(KBZONE_GLOBAL, '2', false, false, true, false);
	CSlrKeyboardShortcut *defaultDataDumpShortcut = restoredKeyboardShortcuts.FindShortcut(KBZONE_GLOBAL,
																 dataDumpShortcutSpec->keyCode,
																 dataDumpShortcutSpec->isShift,
																 dataDumpShortcutSpec->isAlt,
																 dataDumpShortcutSpec->isControl,
																 dataDumpShortcutSpec->isSuper);
	CSlrKeyboardShortcut *defaultDebuggerShortcut = restoredKeyboardShortcuts.FindShortcut(KBZONE_GLOBAL,
																  debuggerShortcutSpec->keyCode,
																  debuggerShortcutSpec->isShift,
																  debuggerShortcutSpec->isAlt,
																  debuggerShortcutSpec->isControl,
																  debuggerShortcutSpec->isSuper);
	CSlrKeyboardShortcut *defaultOnlyShortcut = restoredKeyboardShortcuts.FindShortcut(KBZONE_GLOBAL,
															 onlyShortcutSpec->keyCode,
															 onlyShortcutSpec->isShift,
															 onlyShortcutSpec->isAlt,
															 onlyShortcutSpec->isControl,
															 onlyShortcutSpec->isSuper);
	if (customDataDumpShortcut == NULL
		|| defaultDataDumpShortcut != NULL
		|| defaultDebuggerShortcut != NULL
		|| defaultOnlyShortcut == NULL
		|| strcmp(restoredLayouts.GetShortcutStatus(DefaultWorkspaceSlot_DataDump), "Ready") != 0
		|| strcmp(restoredLayouts.GetShortcutStatus(DefaultWorkspaceSlot_Debugger), "Unassigned") != 0
		|| strcmp(restoredLayouts.GetShortcutStatus(DefaultWorkspaceSlot_Only), "Ready") != 0)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Startup restore should keep custom, cleared, and default shortcut slot states");
		return false;
	}

	CConfigStorageHjson disabledConfig(configPath, false);
	restoredLayouts.SaveShortcutSlotSettings(&disabledConfig, false);
	CConfigStorageHjson loadDisabledConfig(configPath, false);
	CDefaultWorkspaceLayouts disabledLayouts(NULL, &layoutManager);
	CSlrKeyboardShortcuts disabledKeyboardShortcuts;
	if (disabledLayouts.RegisterStoredShortcutSlotsForGeneratedLayouts(&disabledKeyboardShortcuts, &loadDisabledConfig)
		|| disabledKeyboardShortcuts.FindShortcut(KBZONE_GLOBAL, '2', false, false, true, false) != NULL)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Disabled context shortcuts should remain disabled across startup restore");
		return false;
	}

	remove(configPath);
	return true;
}

static bool VerifyDefaultWorkspaceLayoutDataCreation()
{
	CLayoutManager layoutManager(NULL);
	CDefaultWorkspaceLayouts defaultLayouts(NULL, &layoutManager);
	const SDefaultWorkspaceLayoutSpec *dataDumpSpec = FindDefaultWorkspaceLayoutSpec(DefaultWorkspacePlatform_C64, DefaultWorkspaceSlot_DataDump);
	if (dataDumpSpec == NULL)
	{
		snprintf(failureMsg, sizeof(failureMsg), "C64 Data Dump spec should exist before creating default layout data");
		return false;
	}

	CLayoutData *layoutData = defaultLayouts.CreateOrUpdateDefaultWorkspaceLayoutData(dataDumpSpec, true);
	if (layoutData == NULL
		|| layoutManager.GetPredefinedLayoutById("retro.default.c64.data_dump") != layoutData
		|| layoutData->layoutName == NULL
		|| strcmp(layoutData->layoutName, "C64 Data Dump") != 0
		|| layoutData->predefinedId == NULL
		|| strcmp(layoutData->predefinedId, "retro.default.c64.data_dump") != 0
		|| !layoutData->doNotUpdateViewsPositions
		|| layoutData->keyShortcut != NULL)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Created default layout data should use stable predefined ID, display name, locked flag, and no per-layout shortcut");
		layoutManager.DeleteAllLayouts();
		return false;
	}

	if (layoutManager.layouts.size() != 1)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Creating one default layout should add one layout, got %zu", layoutManager.layouts.size());
		layoutManager.DeleteAllLayouts();
		return false;
	}

	CLayoutData *updatedLayoutData = defaultLayouts.CreateOrUpdateDefaultWorkspaceLayoutData(dataDumpSpec, false);
	if (updatedLayoutData != layoutData
		|| layoutManager.layouts.size() != 1
		|| updatedLayoutData->doNotUpdateViewsPositions)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Updating existing default layout data should reuse the predefined layout and update the locked flag without duplicates");
		layoutManager.DeleteAllLayouts();
		return false;
	}

	int numCreatedOrUpdated = defaultLayouts.CreateOrUpdateAllDefaultWorkspaceLayoutData(true);
	if (numCreatedOrUpdated != 32 || layoutManager.layouts.size() != 32)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Creating all default layout data should cover all 32 specs, got %d and %zu layouts", numCreatedOrUpdated, layoutManager.layouts.size());
		layoutManager.DeleteAllLayouts();
		return false;
	}

	const std::vector<SDefaultWorkspaceLayoutSpec> &allSpecs = GetAllDefaultWorkspaceLayoutSpecs();
	for (std::vector<SDefaultWorkspaceLayoutSpec>::const_iterator it = allSpecs.begin(); it != allSpecs.end(); ++it)
	{
		CLayoutData *specLayoutData = layoutManager.GetPredefinedLayoutById(it->predefinedId);
		if (specLayoutData == NULL
			|| specLayoutData->layoutName == NULL
			|| strcmp(specLayoutData->layoutName, it->displayName) != 0
			|| !specLayoutData->doNotUpdateViewsPositions
			|| specLayoutData->keyShortcut != NULL)
		{
			snprintf(failureMsg, sizeof(failureMsg), "Default layout data for '%s' should be registered, locked, display-named, and shortcut-free", it->displayName);
			layoutManager.DeleteAllLayouts();
			return false;
		}
	}

	layoutManager.DeleteAllLayouts();
	return true;
}

static bool VerifyAllDefaultWorkspaceSpecsHaveResolvablePlacements()
{
	if (viewC64 == NULL)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Global viewC64 should exist before validating default workspace placements");
		return false;
	}

	CDefaultWorkspaceLayouts defaultLayouts(viewC64, NULL);
	const std::vector<SDefaultWorkspaceLayoutSpec> &allSpecs = GetAllDefaultWorkspaceLayoutSpecs();
	for (std::vector<SDefaultWorkspaceLayoutSpec>::const_iterator it = allSpecs.begin(); it != allSpecs.end(); ++it)
	{
		if (it->placements == NULL || it->numPlacements <= 0)
		{
			snprintf(failureMsg, sizeof(failureMsg), "Default workspace '%s' should have placements before generation", it->displayName);
			return false;
		}

		for (int i = 0; i < it->numPlacements; i++)
		{
			if (defaultLayouts.FindViewForPlacement(it->placements[i].viewId) == NULL)
			{
				snprintf(failureMsg, sizeof(failureMsg), "Default workspace '%s' placement '%s' should resolve to a current view", it->displayName, it->placements[i].viewId);
				return false;
			}
		}
	}

	return true;
}

static bool VerifyDefaultWorkspaceApplyC64VicDisplayLiteLayout()
{
	if (viewC64 == NULL)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Global viewC64 should exist before applying C64 VIC Display Lite default workspace layouts");
		return false;
	}

	CDefaultWorkspaceLayouts defaultLayouts(viewC64, NULL);
	const SDefaultWorkspaceLayoutSpec *layoutSpec = FindDefaultWorkspaceLayoutSpec(DefaultWorkspacePlatform_C64, DefaultWorkspaceSlot_VicDisplayLite);
	CGuiView *screenView = defaultLayouts.FindViewForPlacement("c64.screen");
	CGuiView *cpuStateView = defaultLayouts.FindViewForPlacement("c64.cpu_state");
	CGuiView *disassemblyView = defaultLayouts.FindViewForPlacement("c64.disassembly");
	CGuiView *dataDumpView = defaultLayouts.FindViewForPlacement("c64.data_dump");
	CGuiView *memoryMapView = defaultLayouts.FindViewForPlacement("c64.memory_map");

	if (layoutSpec == NULL || screenView == NULL || cpuStateView == NULL || disassemblyView == NULL || dataDumpView == NULL || memoryMapView == NULL)
	{
		snprintf(failureMsg, sizeof(failureMsg), "C64 VIC Display Lite placement IDs should resolve to current RetroDebugger views");
		return false;
	}

	CScopedDefaultWorkspaceTestViewState restoreViews;
	restoreViews.AddDebugInterface(viewC64->debugInterfaceC64);
	restoreViews.AddDebugInterface(viewC64->debugInterfaceAtari);
	restoreViews.AddDebugInterface(viewC64->debugInterfaceNes);

	screenView->SetVisible(false);
	cpuStateView->SetVisible(false);
	disassemblyView->SetVisible(false);
	dataDumpView->SetVisible(false);
	memoryMapView->SetVisible(false);

	if (!defaultLayouts.ApplyLayoutSpecFloating(layoutSpec, 580.0f, 360.0f))
	{
		snprintf(failureMsg, sizeof(failureMsg), "Applying C64 VIC Display Lite floating placement should succeed");
		return false;
	}

	if (!screenView->visible
		|| !cpuStateView->visible
		|| !disassemblyView->visible
		|| !dataDumpView->visible
		|| !memoryMapView->visible)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Applying C64 VIC Display Lite should show all old visible current-equivalent views");
		return false;
	}

	if (!NearlyEqual(screenView->posX, 185.5f)
		|| !NearlyEqual(screenView->posY, 13.0f)
		|| !NearlyEqual(screenView->sizeX, 345.6f)
		|| !NearlyEqual(screenView->sizeY, 244.8f)
		|| !NearlyEqual(cpuStateView->posX, 185.5f)
		|| !NearlyEqual(cpuStateView->posY, 2.5f)
		|| !NearlyEqual(cpuStateView->sizeX, 24.0f)
		|| !NearlyEqual(cpuStateView->sizeY, 24.0f)
		|| !NearlyEqual(disassemblyView->posX, 1.0f)
		|| !NearlyEqual(disassemblyView->posY, 1.0f)
		|| !NearlyEqual(disassemblyView->sizeX, 180.6f)
		|| !NearlyEqual(disassemblyView->sizeY, 356.0f)
		|| !NearlyEqual(dataDumpView->posX, 185.5f)
		|| !NearlyEqual(dataDumpView->posY, 260.0f)
		|| !NearlyEqual(dataDumpView->sizeX, 277.0f)
		|| !NearlyEqual(dataDumpView->sizeY, 97.0f)
		|| !NearlyEqual(memoryMapView->posX, 467.5f)
		|| !NearlyEqual(memoryMapView->posY, 260.0f)
		|| !NearlyEqual(memoryMapView->sizeX, 110.0f)
		|| !NearlyEqual(memoryMapView->sizeY, 99.0f))
	{
		snprintf(failureMsg, sizeof(failureMsg), "Applying C64 VIC Display Lite should position views using old geometry");
		return false;
	}

	return true;
}

static bool VerifyDefaultWorkspaceApplyC64VicDisplayLayout()
{
	if (viewC64 == NULL)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Global viewC64 should exist before applying C64 VIC Display default workspace layouts");
		return false;
	}

	CDefaultWorkspaceLayouts defaultLayouts(viewC64, NULL);
	const SDefaultWorkspaceLayoutSpec *layoutSpec = FindDefaultWorkspaceLayoutSpec(DefaultWorkspacePlatform_C64, DefaultWorkspaceSlot_VicDisplay);
	CGuiView *screenView = defaultLayouts.FindViewForPlacement("c64.screen");
	CGuiView *cpuStateView = defaultLayouts.FindViewForPlacement("c64.cpu_state");
	CGuiView *disassemblyView = defaultLayouts.FindViewForPlacement("c64.disassembly");
	CGuiView *vicStateView = defaultLayouts.FindViewForPlacement("c64.vic_state");
	CGuiView *dataDumpView = defaultLayouts.FindViewForPlacement("c64.data_dump");
	CGuiView *vicDisplayView = defaultLayouts.FindViewForPlacement("c64.vic_display");
	CGuiView *vicControlView = defaultLayouts.FindViewForPlacement("c64.vic_control");

	if (layoutSpec == NULL || screenView == NULL || cpuStateView == NULL || disassemblyView == NULL
		|| vicStateView == NULL || dataDumpView == NULL || vicDisplayView == NULL || vicControlView == NULL)
	{
		snprintf(failureMsg, sizeof(failureMsg), "C64 VIC Display placement IDs should resolve to current RetroDebugger views");
		return false;
	}

	CScopedDefaultWorkspaceTestViewState restoreViews;
	restoreViews.AddDebugInterface(viewC64->debugInterfaceC64);
	restoreViews.AddDebugInterface(viewC64->debugInterfaceAtari);
	restoreViews.AddDebugInterface(viewC64->debugInterfaceNes);

	screenView->SetVisible(false);
	cpuStateView->SetVisible(false);
	disassemblyView->SetVisible(false);
	vicStateView->SetVisible(false);
	dataDumpView->SetVisible(false);
	vicDisplayView->SetVisible(false);
	vicControlView->SetVisible(false);

	if (!defaultLayouts.ApplyLayoutSpecFloating(layoutSpec, 580.0f, 360.0f))
	{
		snprintf(failureMsg, sizeof(failureMsg), "Applying C64 VIC Display floating placement should succeed");
		return false;
	}

	if (!screenView->visible
		|| !cpuStateView->visible
		|| !disassemblyView->visible
		|| !vicStateView->visible
		|| !dataDumpView->visible
		|| !vicDisplayView->visible
		|| !vicControlView->visible)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Applying C64 VIC Display should show all old visible current-equivalent views");
		return false;
	}

	if (!NearlyEqual(screenView->posX, 444.6f)
		|| !NearlyEqual(screenView->posY, 15.0f)
		|| !NearlyEqual(screenView->sizeX, 134.4f)
		|| !NearlyEqual(screenView->sizeY, 95.2f)
		|| !NearlyEqual(cpuStateView->posX, 354.6f)
		|| !NearlyEqual(cpuStateView->posY, 2.5f)
		|| !NearlyEqual(cpuStateView->sizeX, 24.0f)
		|| !NearlyEqual(cpuStateView->sizeY, 24.0f)
		|| !NearlyEqual(disassemblyView->posX, 1.0f)
		|| !NearlyEqual(disassemblyView->posY, 1.0f)
		|| !NearlyEqual(disassemblyView->sizeX, 180.6f)
		|| !NearlyEqual(disassemblyView->sizeY, 356.0f)
		|| !NearlyEqual(vicStateView->posX, 186.5f)
		|| !NearlyEqual(vicStateView->posY, 1.0f)
		|| !NearlyEqual(vicStateView->sizeX, 256.5f)
		|| !NearlyEqual(vicStateView->sizeY, 144.0f)
		|| !NearlyEqual(dataDumpView->posX, 449.5f)
		|| !NearlyEqual(dataDumpView->posY, 113.0f)
		|| !NearlyEqual(dataDumpView->sizeX, 125.0f)
		|| !NearlyEqual(dataDumpView->sizeY, 34.0f)
		|| !NearlyEqual(vicDisplayView->posX, 185.5f)
		|| !NearlyEqual(vicDisplayView->posY, 150.0f)
		|| !NearlyEqual(vicDisplayView->sizeX, 329.989f)
		|| !NearlyEqual(vicDisplayView->sizeY, 204.280f)
		|| !NearlyEqual(vicControlView->posX, 521.5f)
		|| !NearlyEqual(vicControlView->posY, 150.0f)
		|| !NearlyEqual(vicControlView->sizeX, 200.0f)
		|| !NearlyEqual(vicControlView->sizeY, 200.0f))
	{
		snprintf(failureMsg, sizeof(failureMsg), "Applying C64 VIC Display should position views using old geometry");
		return false;
	}

	if (!VerifyQueuedPlacement(screenView, "c64.screen", 444.6f, 15.0f, 134.4f, 95.2f)
		|| !VerifyQueuedPlacement(cpuStateView, "c64.cpu_state", 354.6f, 2.5f, 24.0f, 24.0f)
		|| !VerifyQueuedPlacement(disassemblyView, "c64.disassembly", 1.0f, 1.0f, 180.6f, 356.0f)
		|| !VerifyQueuedPlacement(vicStateView, "c64.vic_state", 186.5f, 1.0f, 256.5f, 144.0f)
		|| !VerifyQueuedPlacement(dataDumpView, "c64.data_dump", 449.5f, 113.0f, 125.0f, 34.0f)
		|| !VerifyQueuedPlacement(vicDisplayView, "c64.vic_display", 185.5f, 150.0f, 329.989f, 204.280f)
		|| !VerifyQueuedPlacement(vicControlView, "c64.vic_control", 521.5f, 150.0f, 200.0f, 200.0f))
	{
		return false;
	}

	return true;
}

static bool VerifyDefaultWorkspaceApplyC64SourceCodeLayout()
{
	if (viewC64 == NULL)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Global viewC64 should exist before applying C64 Source Code default workspace layouts");
		return false;
	}

	CDefaultWorkspaceLayouts defaultLayouts(viewC64, NULL);
	const SDefaultWorkspaceLayoutSpec *layoutSpec = FindDefaultWorkspaceLayoutSpec(DefaultWorkspacePlatform_C64, DefaultWorkspaceSlot_SourceCode);
	CGuiView *screenView = defaultLayouts.FindViewForPlacement("c64.screen");
	CGuiView *cpuStateView = defaultLayouts.FindViewForPlacement("c64.cpu_state");
	CGuiView *disassemblyView = defaultLayouts.FindViewForPlacement("c64.disassembly");
	CGuiView *sourceCodeView = defaultLayouts.FindViewForPlacement("c64.source_code");

	if (layoutSpec == NULL || screenView == NULL || cpuStateView == NULL || disassemblyView == NULL || sourceCodeView == NULL)
	{
		snprintf(failureMsg, sizeof(failureMsg), "C64 Source Code placement IDs should resolve to current RetroDebugger views");
		return false;
	}

	CScopedDefaultWorkspaceTestViewState restoreViews;
	restoreViews.AddDebugInterface(viewC64->debugInterfaceC64);
	restoreViews.AddDebugInterface(viewC64->debugInterfaceAtari);
	restoreViews.AddDebugInterface(viewC64->debugInterfaceNes);

	screenView->SetVisible(false);
	cpuStateView->SetVisible(false);
	disassemblyView->SetVisible(false);
	sourceCodeView->SetVisible(false);

	if (!defaultLayouts.ApplyLayoutSpecFloating(layoutSpec, 580.0f, 360.0f))
	{
		snprintf(failureMsg, sizeof(failureMsg), "Applying C64 Source Code floating placement should succeed");
		return false;
	}

	if (!screenView->visible
		|| !cpuStateView->visible
		|| !disassemblyView->visible
		|| !sourceCodeView->visible)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Applying C64 Source Code should show all old visible current-equivalent views");
		return false;
	}

	if (!NearlyEqual(screenView->posX, 478.0f)
		|| !NearlyEqual(screenView->posY, 0.0f)
		|| !NearlyEqual(screenView->sizeX, 97.92f)
		|| !NearlyEqual(screenView->sizeY, 69.36f)
		|| !NearlyEqual(cpuStateView->posX, 220.0f)
		|| !NearlyEqual(cpuStateView->posY, 2.5f)
		|| !NearlyEqual(cpuStateView->sizeX, 24.0f)
		|| !NearlyEqual(cpuStateView->sizeY, 24.0f)
		|| !NearlyEqual(disassemblyView->posX, 1.0f)
		|| !NearlyEqual(disassemblyView->posY, 1.0f)
		|| !NearlyEqual(disassemblyView->sizeX, 110.6f)
		|| !NearlyEqual(disassemblyView->sizeY, 356.0f)
		|| !NearlyEqual(sourceCodeView->posX, 114.1f)
		|| !NearlyEqual(sourceCodeView->posY, 15.0f)
		|| !NearlyEqual(sourceCodeView->sizeX, 463.9f)
		|| !NearlyEqual(sourceCodeView->sizeY, 341.0f))
	{
		snprintf(failureMsg, sizeof(failureMsg), "Applying C64 Source Code should position views using old geometry");
		return false;
	}

	if (!VerifyQueuedPlacement(screenView, "c64.screen", 478.0f, 0.0f, 97.92f, 69.36f)
		|| !VerifyQueuedPlacement(cpuStateView, "c64.cpu_state", 220.0f, 2.5f, 24.0f, 24.0f)
		|| !VerifyQueuedPlacement(disassemblyView, "c64.disassembly", 1.0f, 1.0f, 110.6f, 356.0f)
		|| !VerifyQueuedPlacement(sourceCodeView, "c64.source_code", 114.1f, 15.0f, 463.9f, 341.0f))
	{
		return false;
	}

	return true;
}

static bool VerifyDefaultWorkspaceApplyC64AllGraphicsLayout()
{
	if (viewC64 == NULL)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Global viewC64 should exist before applying C64 All Graphics default workspace layouts");
		return false;
	}

	CDefaultWorkspaceLayouts defaultLayouts(viewC64, NULL);
	const SDefaultWorkspaceLayoutSpec *layoutSpec = FindDefaultWorkspaceLayoutSpec(DefaultWorkspacePlatform_C64, DefaultWorkspaceSlot_AllGraphics);
	CGuiView *screenView = defaultLayouts.FindViewForPlacement("c64.screen");
	CGuiView *bitmapsView = defaultLayouts.FindViewForPlacement("c64.all_graphics_bitmaps");
	CGuiView *screensView = defaultLayouts.FindViewForPlacement("c64.all_graphics_screens");
	CGuiView *charsetsView = defaultLayouts.FindViewForPlacement("c64.all_graphics_charsets");
	CGuiView *spritesView = defaultLayouts.FindViewForPlacement("c64.all_graphics_sprites");

	if (layoutSpec == NULL || screenView == NULL || bitmapsView == NULL || screensView == NULL || charsetsView == NULL || spritesView == NULL)
	{
		snprintf(failureMsg, sizeof(failureMsg), "C64 All Graphics placement IDs should resolve to current split graphics views");
		return false;
	}

	CScopedDefaultWorkspaceTestViewState restoreViews;
	restoreViews.AddDebugInterface(viewC64->debugInterfaceC64);
	restoreViews.AddDebugInterface(viewC64->debugInterfaceAtari);
	restoreViews.AddDebugInterface(viewC64->debugInterfaceNes);

	screenView->SetVisible(false);
	bitmapsView->SetVisible(false);
	screensView->SetVisible(false);
	charsetsView->SetVisible(false);
	spritesView->SetVisible(false);

	if (!defaultLayouts.ApplyLayoutSpecFloating(layoutSpec, 580.0f, 360.0f))
	{
		snprintf(failureMsg, sizeof(failureMsg), "Applying C64 All Graphics floating placement should succeed");
		return false;
	}

	if (!screenView->visible || !bitmapsView->visible || !screensView->visible || !charsetsView->visible || !spritesView->visible)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Applying C64 All Graphics should show the screen anchor and split graphics views");
		return false;
	}

	if (!NearlyEqual(bitmapsView->posX, 0.0f)
		|| !NearlyEqual(bitmapsView->posY, 0.0f)
		|| !NearlyEqual(bitmapsView->sizeX, 169.0f)
		|| !NearlyEqual(bitmapsView->sizeY, 180.0f)
		|| !NearlyEqual(screensView->posX, 169.0f)
		|| !NearlyEqual(screensView->posY, 0.0f)
		|| !NearlyEqual(charsetsView->posX, 0.0f)
		|| !NearlyEqual(charsetsView->posY, 180.0f)
		|| !NearlyEqual(spritesView->posX, 169.0f)
		|| !NearlyEqual(spritesView->posY, 180.0f)
		|| !NearlyEqual(screenView->posX, 458.0f)
		|| !NearlyEqual(screenView->posY, 15.0f)
		|| !NearlyEqual(screenView->sizeX, 115.2f)
		|| !NearlyEqual(screenView->sizeY, 81.6f))
	{
		snprintf(failureMsg, sizeof(failureMsg), "Applying C64 All Graphics should use the split graphics approximation and old screen anchor geometry");
		return false;
	}

	if (!VerifyQueuedPlacement(bitmapsView, "c64.all_graphics_bitmaps", 0.0f, 0.0f, 169.0f, 180.0f)
		|| !VerifyQueuedPlacement(spritesView, "c64.all_graphics_sprites", 169.0f, 180.0f, 169.0f, 180.0f)
		|| !VerifyQueuedPlacement(screenView, "c64.screen", 458.0f, 15.0f, 115.2f, 81.6f))
	{
		return false;
	}

	return true;
}

static bool VerifyDefaultWorkspaceApplyC64AllSidsLayout()
{
	if (viewC64 == NULL)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Global viewC64 should exist before applying C64 All SIDs default workspace layouts");
		return false;
	}

	CDefaultWorkspaceLayouts defaultLayouts(viewC64, NULL);
	const SDefaultWorkspaceLayoutSpec *layoutSpec = FindDefaultWorkspaceLayoutSpec(DefaultWorkspacePlatform_C64, DefaultWorkspaceSlot_AllSids);
	CGuiView *sidTrackerView = defaultLayouts.FindViewForPlacement("c64.sid_tracker_history");
	CGuiView *sidPianoView = defaultLayouts.FindViewForPlacement("c64.sid_piano_keyboard");
	CGuiView *sidStateView = defaultLayouts.FindViewForPlacement("c64.sid_state");
	CGuiView *cpuStateView = defaultLayouts.FindViewForPlacement("c64.cpu_state");
	CGuiView *dataDumpView = defaultLayouts.FindViewForPlacement("c64.data_dump");
	CGuiView *memoryMapView = defaultLayouts.FindViewForPlacement("c64.memory_map");
	CGuiView *disassemblyView = defaultLayouts.FindViewForPlacement("c64.disassembly");
	CGuiView *screenView = defaultLayouts.FindViewForPlacement("c64.screen");

	if (layoutSpec == NULL || sidTrackerView == NULL || sidPianoView == NULL || sidStateView == NULL
		|| cpuStateView == NULL || dataDumpView == NULL || memoryMapView == NULL || disassemblyView == NULL || screenView == NULL)
	{
		snprintf(failureMsg, sizeof(failureMsg), "C64 All SIDs placement IDs should resolve to SID state/tracker/piano views and old support views");
		return false;
	}

	if (sidTrackerView != viewC64->viewC64SidTrackerHistory
		|| sidPianoView != viewC64->viewC64SidPianoKeyboard
		|| sidStateView != viewC64->viewC64StateSID)
	{
		snprintf(failureMsg, sizeof(failureMsg), "C64 All SIDs placement IDs should map to the intended SID tracker, piano, and state views");
		return false;
	}

	CScopedDefaultWorkspaceTestViewState restoreViews;
	restoreViews.AddDebugInterface(viewC64->debugInterfaceC64);
	restoreViews.AddDebugInterface(viewC64->debugInterfaceAtari);
	restoreViews.AddDebugInterface(viewC64->debugInterfaceNes);

	sidTrackerView->SetVisible(false);
	sidPianoView->SetVisible(false);
	sidStateView->SetVisible(false);
	cpuStateView->SetVisible(false);
	dataDumpView->SetVisible(false);
	memoryMapView->SetVisible(false);
	disassemblyView->SetVisible(false);
	screenView->SetVisible(false);

	if (!defaultLayouts.ApplyLayoutSpecFloating(layoutSpec, 580.0f, 360.0f))
	{
		snprintf(failureMsg, sizeof(failureMsg), "Applying C64 All SIDs floating placement should succeed");
		return false;
	}

	if (!sidTrackerView->visible || !sidPianoView->visible || !sidStateView->visible
		|| !cpuStateView->visible || !dataDumpView->visible || !memoryMapView->visible || !disassemblyView->visible || !screenView->visible)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Applying C64 All SIDs should show SID tracker/piano/state and old support views");
		return false;
	}

	if (!NearlyEqual(sidTrackerView->posX, 0.0f)
		|| !NearlyEqual(sidTrackerView->posY, 0.0f)
		|| !NearlyEqual(sidTrackerView->sizeX, 350.0f)
		|| !NearlyEqual(sidTrackerView->sizeY, 126.0f)
		|| !NearlyEqual(sidPianoView->posX, 0.0f)
		|| !NearlyEqual(sidPianoView->posY, 128.0f)
		|| !NearlyEqual(sidPianoView->sizeX, 350.0f)
		|| !NearlyEqual(sidPianoView->sizeY, 58.0f)
		|| !NearlyEqual(sidStateView->posX, 0.0f)
		|| !NearlyEqual(sidStateView->posY, 190.0f)
		|| !NearlyEqual(sidStateView->sizeX, 350.0f)
		|| !NearlyEqual(sidStateView->sizeY, 168.0f)
		|| !NearlyEqual(cpuStateView->posX, 350.0f)
		|| !NearlyEqual(cpuStateView->posY, 2.5f)
		|| !NearlyEqual(cpuStateView->sizeX, 24.0f)
		|| !NearlyEqual(cpuStateView->sizeY, 24.0f)
		|| !NearlyEqual(dataDumpView->posX, 458.0f)
		|| !NearlyEqual(dataDumpView->posY, 100.0f)
		|| !NearlyEqual(dataDumpView->sizeX, 470.0f)
		|| !NearlyEqual(dataDumpView->sizeY, 135.0f)
		|| !NearlyEqual(memoryMapView->posX, 442.5f)
		|| !NearlyEqual(memoryMapView->posY, 239.0f)
		|| !NearlyEqual(memoryMapView->sizeX, 130.0f)
		|| !NearlyEqual(memoryMapView->sizeY, 119.0f)
		|| !NearlyEqual(disassemblyView->posX, 358.0f)
		|| !NearlyEqual(disassemblyView->posY, 239.0f)
		|| !NearlyEqual(disassemblyView->sizeX, 79.0f)
		|| !NearlyEqual(disassemblyView->sizeY, 119.0f)
		|| !NearlyEqual(screenView->posX, 458.0f)
		|| !NearlyEqual(screenView->posY, 15.0f)
		|| !NearlyEqual(screenView->sizeX, 115.2f)
		|| !NearlyEqual(screenView->sizeY, 81.6f))
	{
		snprintf(failureMsg, sizeof(failureMsg), "Applying C64 All SIDs should use SID tracker/piano approximation and old support-view geometry");
		return false;
	}

	if (!VerifyQueuedPlacement(sidTrackerView, "c64.sid_tracker_history", 0.0f, 0.0f, 350.0f, 126.0f)
		|| !VerifyQueuedPlacement(sidPianoView, "c64.sid_piano_keyboard", 0.0f, 128.0f, 350.0f, 58.0f)
		|| !VerifyQueuedPlacement(sidStateView, "c64.sid_state", 0.0f, 190.0f, 350.0f, 168.0f)
		|| !VerifyQueuedPlacement(screenView, "c64.screen", 458.0f, 15.0f, 115.2f, 81.6f))
	{
		return false;
	}

	return true;
}

static bool VerifyDefaultWorkspaceApplyC64MemoryDebuggerLayout()
{
	if (viewC64 == NULL)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Global viewC64 should exist before applying C64 Memory Debugger default workspace layouts");
		return false;
	}

	CDefaultWorkspaceLayouts defaultLayouts(viewC64, NULL);
	const SDefaultWorkspaceLayoutSpec *layoutSpec = FindDefaultWorkspaceLayoutSpec(DefaultWorkspacePlatform_C64, DefaultWorkspaceSlot_MemoryDebugger);
	CGuiView *cpuStateView = defaultLayouts.FindViewForPlacement("c64.cpu_state");
	CGuiView *disassemblyView = defaultLayouts.FindViewForPlacement("c64.disassembly");
	CGuiView *disassembly2View = defaultLayouts.FindViewForPlacement("c64.disassembly2");
	CGuiView *dataDumpView = defaultLayouts.FindViewForPlacement("c64.data_dump");
	CGuiView *dataDump2View = defaultLayouts.FindViewForPlacement("c64.data_dump2");
	CGuiView *dataDump3View = defaultLayouts.FindViewForPlacement("c64.data_dump3");
	CGuiView *memoryMapView = defaultLayouts.FindViewForPlacement("c64.memory_map");
	CGuiView *screenView = defaultLayouts.FindViewForPlacement("c64.screen");

	if (layoutSpec == NULL || cpuStateView == NULL || disassemblyView == NULL || disassembly2View == NULL
		|| dataDumpView == NULL || dataDump2View == NULL || dataDump3View == NULL || memoryMapView == NULL || screenView == NULL)
	{
		snprintf(failureMsg, sizeof(failureMsg), "C64 Memory Debugger placement IDs should resolve to current disassembly/data dump views");
		return false;
	}

	if (disassemblyView != viewC64->viewC64Disassembly
		|| disassembly2View != viewC64->viewC64Disassembly2
		|| dataDumpView != viewC64->viewC64MemoryDataDump
		|| dataDump2View != viewC64->viewC64MemoryDataDump2
		|| dataDump3View != viewC64->viewC64MemoryDataDump3)
	{
		snprintf(failureMsg, sizeof(failureMsg), "C64 Memory Debugger placement IDs should map to the intended primary/secondary disassembly and data dump views");
		return false;
	}

	CScopedDefaultWorkspaceTestViewState restoreViews;
	restoreViews.AddDebugInterface(viewC64->debugInterfaceC64);
	restoreViews.AddDebugInterface(viewC64->debugInterfaceAtari);
	restoreViews.AddDebugInterface(viewC64->debugInterfaceNes);

	cpuStateView->SetVisible(false);
	disassemblyView->SetVisible(false);
	disassembly2View->SetVisible(false);
	dataDumpView->SetVisible(false);
	dataDump2View->SetVisible(false);
	dataDump3View->SetVisible(false);
	memoryMapView->SetVisible(false);
	screenView->SetVisible(false);

	if (!defaultLayouts.ApplyLayoutSpecFloating(layoutSpec, 580.0f, 360.0f))
	{
		snprintf(failureMsg, sizeof(failureMsg), "Applying C64 Memory Debugger floating placement should succeed");
		return false;
	}

	if (!cpuStateView->visible || !disassemblyView->visible || !disassembly2View->visible
		|| !dataDumpView->visible || !dataDump2View->visible || !dataDump3View->visible || !memoryMapView->visible || !screenView->visible)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Applying C64 Memory Debugger should show the old memory-debugger data/disassembly windows and support views");
		return false;
	}

	if (!NearlyEqual(cpuStateView->posX, 350.0f)
		|| !NearlyEqual(cpuStateView->posY, 2.5f)
		|| !NearlyEqual(cpuStateView->sizeX, 24.0f)
		|| !NearlyEqual(cpuStateView->sizeY, 24.0f)
		|| !NearlyEqual(disassemblyView->posX, 1.0f)
		|| !NearlyEqual(disassemblyView->posY, 2.0f)
		|| !NearlyEqual(disassemblyView->sizeX, 65.6f)
		|| !NearlyEqual(disassemblyView->sizeY, 180.0f)
		|| !NearlyEqual(disassembly2View->posX, 1.0f)
		|| !NearlyEqual(disassembly2View->posY, 186.0f)
		|| !NearlyEqual(disassembly2View->sizeX, 65.6f)
		|| !NearlyEqual(disassembly2View->sizeY, 167.0f)
		|| !NearlyEqual(dataDumpView->posX, 67.1f)
		|| !NearlyEqual(dataDumpView->posY, 15.5f)
		|| !NearlyEqual(dataDumpView->sizeX, 387.0f)
		|| !NearlyEqual(dataDumpView->sizeY, 169.0f)
		|| !NearlyEqual(dataDump2View->posX, 67.1f)
		|| !NearlyEqual(dataDump2View->posY, 185.5f)
		|| !NearlyEqual(dataDump2View->sizeX, 387.0f)
		|| !NearlyEqual(dataDump2View->sizeY, 169.0f)
		|| !NearlyEqual(dataDump3View->posX, 458.0f)
		|| !NearlyEqual(dataDump3View->posY, 100.0f)
		|| !NearlyEqual(dataDump3View->sizeX, 470.0f)
		|| !NearlyEqual(dataDump3View->sizeY, 135.0f)
		|| !NearlyEqual(memoryMapView->posX, 460.0f)
		|| !NearlyEqual(memoryMapView->posY, 239.0f)
		|| !NearlyEqual(memoryMapView->sizeX, 112.0f)
		|| !NearlyEqual(memoryMapView->sizeY, 119.0f)
		|| !NearlyEqual(screenView->posX, 458.0f)
		|| !NearlyEqual(screenView->posY, 15.0f)
		|| !NearlyEqual(screenView->sizeX, 115.2f)
		|| !NearlyEqual(screenView->sizeY, 81.6f))
	{
		snprintf(failureMsg, sizeof(failureMsg), "Applying C64 Memory Debugger should use old data/disassembly geometry without the obsolete toolbar");
		return false;
	}

	if (!VerifyQueuedPlacement(disassemblyView, "c64.disassembly", 1.0f, 2.0f, 65.6f, 180.0f)
		|| !VerifyQueuedPlacement(disassembly2View, "c64.disassembly2", 1.0f, 186.0f, 65.6f, 167.0f)
		|| !VerifyQueuedPlacement(dataDumpView, "c64.data_dump", 67.1f, 15.5f, 387.0f, 169.0f)
		|| !VerifyQueuedPlacement(dataDump2View, "c64.data_dump2", 67.1f, 185.5f, 387.0f, 169.0f)
		|| !VerifyQueuedPlacement(screenView, "c64.screen", 458.0f, 15.0f, 115.2f, 81.6f))
	{
		return false;
	}

	return true;
}

static bool VerifyDefaultWorkspaceRecalculatesFontsFromScaledGeometry()
{
	if (viewC64 == NULL)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Global viewC64 should exist before testing default workspace font scaling");
		return false;
	}

	CDefaultWorkspaceLayouts defaultLayouts(viewC64, NULL);
	const SDefaultWorkspaceLayoutSpec *debuggerLayout = FindDefaultWorkspaceLayoutSpec(DefaultWorkspacePlatform_C64, DefaultWorkspaceSlot_Debugger);
	CViewDisassembly *disassemblyView = dynamic_cast<CViewDisassembly *>(defaultLayouts.FindViewForPlacement("c64.disassembly"));
	CViewDataDump *dataDumpView = dynamic_cast<CViewDataDump *>(defaultLayouts.FindViewForPlacement("c64.data_dump"));
	if (debuggerLayout == NULL || disassemblyView == NULL || dataDumpView == NULL)
	{
		snprintf(failureMsg, sizeof(failureMsg), "C64 Debugger font-scaling views should resolve to disassembly and data dump instances");
		return false;
	}

	CScopedDefaultWorkspaceTestViewState restoreViews;
	restoreViews.AddDebugInterface(viewC64->debugInterfaceC64);
	restoreViews.AddDebugInterface(viewC64->debugInterfaceAtari);
	restoreViews.AddDebugInterface(viewC64->debugInterfaceNes);
	disassemblyView->fontSize = 3.0f;
	disassemblyView->showHexCodes = true;
	disassemblyView->showCodeCycles = true;
	disassemblyView->showLabels = false;
	disassemblyView->labelNumCharacters = 15;
	disassemblyView->mnemonicsDisplayOffsetX = 0.0f;
	disassemblyView->codeCyclesDisplayOffsetX = -1.5f;
	disassemblyView->LayoutParameterChanged(NULL);
	dataDumpView->fontSize = 3.0f;
	dataDumpView->numberOfBytesPerLine = 8;
	dataDumpView->showDataCharacters = true;
	dataDumpView->showCharacters = false;
	dataDumpView->showSprites = false;
	dataDumpView->LayoutParameterChanged(NULL);

	if (!defaultLayouts.ApplyLayoutSpecFloating(debuggerLayout, 1160.0f, 720.0f))
	{
		snprintf(failureMsg, sizeof(failureMsg), "Applying 2x C64 Debugger floating placement should succeed");
		return false;
	}

	if (!NearlyEqual(disassemblyView->fontSize, 14.0f))
	{
		snprintf(failureMsg, sizeof(failureMsg), "2x C64 Debugger disassembly should scale old 7px font to 14px, got %.3f", disassemblyView->fontSize);
		return false;
	}

	if (!NearlyEqual(dataDumpView->fontSize, 10.0f) || dataDumpView->numberOfBytesPerLine != 16)
	{
		snprintf(failureMsg, sizeof(failureMsg), "2x C64 Debugger data dump should keep scaled 10px font and expand to 16 bytes, got font %.3f bytes %d",
				 dataDumpView->fontSize,
				 dataDumpView->numberOfBytesPerLine);
		return false;
	}

	const SDefaultWorkspaceLayoutSpec *monitorLayout = FindDefaultWorkspaceLayoutSpec(DefaultWorkspacePlatform_C64, DefaultWorkspaceSlot_MonitorConsole);
	if (monitorLayout == NULL)
	{
		snprintf(failureMsg, sizeof(failureMsg), "C64 Monitor Console layout should exist for narrow data dump font test");
		return false;
	}

	if (!defaultLayouts.ApplyLayoutSpecFloating(monitorLayout, 580.0f, 360.0f))
	{
		snprintf(failureMsg, sizeof(failureMsg), "Applying C64 Monitor Console floating placement should succeed for narrow data dump font test");
		return false;
	}

	if (!NearlyEqual(dataDumpView->fontSize, 5.0f) || dataDumpView->numberOfBytesPerLine != 8)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Narrow C64 Monitor Console data dump should keep 5px font and 8 bytes, got font %.3f bytes %d",
				 dataDumpView->fontSize,
				 dataDumpView->numberOfBytesPerLine);
		return false;
	}

	const SDefaultWorkspaceLayoutSpec *vicDisplayLayout = FindDefaultWorkspaceLayoutSpec(DefaultWorkspacePlatform_C64, DefaultWorkspaceSlot_VicDisplay);
	if (vicDisplayLayout == NULL)
	{
		snprintf(failureMsg, sizeof(failureMsg), "C64 VIC Display layout should exist for very narrow data dump font test");
		return false;
	}

	if (!defaultLayouts.ApplyLayoutSpecFloating(vicDisplayLayout, 580.0f, 360.0f))
	{
		snprintf(failureMsg, sizeof(failureMsg), "Applying C64 VIC Display floating placement should succeed for very narrow data dump font test");
		return false;
	}

	if (!NearlyEqual(dataDumpView->fontSize, 5.0f) || dataDumpView->numberOfBytesPerLine != 4)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Very narrow C64 VIC Display data dump should keep 5px font and reduce to 4 fitting bytes, got font %.3f bytes %d",
				 dataDumpView->fontSize,
				 dataDumpView->numberOfBytesPerLine);
		return false;
	}

	static const SDefaultWorkspaceViewPlacementSpec dataDumpGraphicsPlacements[] = {
		{ "c64.data_dump", 0.0f, 0.0f, 300.0f, 100.0f, 5.0f }
	};
	SDefaultWorkspaceLayoutSpec dataDumpGraphicsLayout = {
		DefaultWorkspacePlatform_C64,
		DefaultWorkspaceSlot_DataDump,
		"C64 Data Dump Graphics Fit Test",
		"test.c64.data_dump_graphics_fit",
		dataDumpGraphicsPlacements,
		(int)(sizeof(dataDumpGraphicsPlacements) / sizeof(dataDumpGraphicsPlacements[0]))
	};
	dataDumpView->numberOfBytesPerLine = 8;
	dataDumpView->showDataCharacters = true;
	dataDumpView->showCharacters = true;
	dataDumpView->showSprites = true;
	dataDumpView->LayoutParameterChanged(NULL);

	if (!defaultLayouts.ApplyLayoutSpecFloating(&dataDumpGraphicsLayout, 580.0f, 360.0f))
	{
		snprintf(failureMsg, sizeof(failureMsg), "Applying data dump graphics fitting layout should succeed");
		return false;
	}

	if (!NearlyEqual(dataDumpView->fontSize, 5.0f) || dataDumpView->numberOfBytesPerLine != 8)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Data dump with character and sprite previews should account for preview widths and keep 8 fitting bytes, got font %.3f bytes %d",
				 dataDumpView->fontSize,
				 dataDumpView->numberOfBytesPerLine);
		return false;
	}

	const SDefaultWorkspaceLayoutSpec *atariDebuggerLayout = FindDefaultWorkspaceLayoutSpec(DefaultWorkspacePlatform_Atari800, DefaultWorkspaceSlot_Debugger);
	CViewDisassembly *atariDisassemblyView = dynamic_cast<CViewDisassembly *>(defaultLayouts.FindViewForPlacement("atari.disassembly"));
	CViewDataDump *atariDataDumpView = dynamic_cast<CViewDataDump *>(defaultLayouts.FindViewForPlacement("atari.data_dump"));
	if (atariDebuggerLayout == NULL || atariDisassemblyView == NULL || atariDataDumpView == NULL)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Atari800 Debugger font-scaling views should resolve to disassembly and data dump instances");
		return false;
	}
	atariDisassemblyView->fontSize = 3.0f;
	atariDisassemblyView->showHexCodes = true;
	atariDisassemblyView->showCodeCycles = true;
	atariDisassemblyView->showLabels = false;
	atariDisassemblyView->labelNumCharacters = 15;
	atariDisassemblyView->mnemonicsDisplayOffsetX = 0.0f;
	atariDisassemblyView->codeCyclesDisplayOffsetX = -1.5f;
	atariDisassemblyView->LayoutParameterChanged(NULL);
	atariDataDumpView->fontSize = 3.0f;
	atariDataDumpView->numberOfBytesPerLine = 8;
	atariDataDumpView->showDataCharacters = true;
	atariDataDumpView->LayoutParameterChanged(NULL);

	if (!defaultLayouts.ApplyLayoutSpecFloating(atariDebuggerLayout, 1160.0f, 720.0f))
	{
		snprintf(failureMsg, sizeof(failureMsg), "Applying 2x Atari800 Debugger floating placement should succeed");
		return false;
	}

	if (!NearlyEqual(atariDisassemblyView->fontSize, 14.0f)
		|| !NearlyEqual(atariDataDumpView->fontSize, 10.0f)
		|| atariDataDumpView->numberOfBytesPerLine != 16)
	{
		snprintf(failureMsg, sizeof(failureMsg), "2x Atari800 Debugger should scale fonts and data dump columns, got disasm %.3f dump %.3f bytes %d",
				 atariDisassemblyView->fontSize,
				 atariDataDumpView->fontSize,
				 atariDataDumpView->numberOfBytesPerLine);
		return false;
	}

	const SDefaultWorkspaceLayoutSpec *nesDebuggerLayout = FindDefaultWorkspaceLayoutSpec(DefaultWorkspacePlatform_NES, DefaultWorkspaceSlot_Debugger);
	CViewDisassembly *nesDisassemblyView = dynamic_cast<CViewDisassembly *>(defaultLayouts.FindViewForPlacement("nes.disassembly"));
	CViewDataDump *nesDataDumpView = dynamic_cast<CViewDataDump *>(defaultLayouts.FindViewForPlacement("nes.data_dump"));
	if (nesDebuggerLayout == NULL || nesDisassemblyView == NULL || nesDataDumpView == NULL)
	{
		snprintf(failureMsg, sizeof(failureMsg), "NES Debugger font-scaling views should resolve to disassembly and data dump instances");
		return false;
	}
	nesDisassemblyView->fontSize = 3.0f;
	nesDisassemblyView->showHexCodes = true;
	nesDisassemblyView->showCodeCycles = true;
	nesDisassemblyView->showLabels = false;
	nesDisassemblyView->labelNumCharacters = 15;
	nesDisassemblyView->mnemonicsDisplayOffsetX = 0.0f;
	nesDisassemblyView->codeCyclesDisplayOffsetX = -1.5f;
	nesDisassemblyView->LayoutParameterChanged(NULL);
	nesDataDumpView->fontSize = 3.0f;
	nesDataDumpView->numberOfBytesPerLine = 8;
	nesDataDumpView->showDataCharacters = true;
	nesDataDumpView->LayoutParameterChanged(NULL);

	if (!defaultLayouts.ApplyLayoutSpecFloating(nesDebuggerLayout, 1160.0f, 720.0f))
	{
		snprintf(failureMsg, sizeof(failureMsg), "Applying 2x NES Debugger floating placement should succeed");
		return false;
	}

	if (!NearlyEqual(nesDisassemblyView->fontSize, 14.0f)
		|| !NearlyEqual(nesDataDumpView->fontSize, 10.0f)
		|| nesDataDumpView->numberOfBytesPerLine != 16)
	{
		snprintf(failureMsg, sizeof(failureMsg), "2x NES Debugger should scale fonts and data dump columns, got disasm %.3f dump %.3f bytes %d",
				 nesDisassemblyView->fontSize,
				 nesDataDumpView->fontSize,
				 nesDataDumpView->numberOfBytesPerLine);
		return false;
	}

	return true;
}

static bool VerifyDefaultWorkspaceShortcutResolution()
{
	CDefaultWorkspaceLayouts defaultLayouts(NULL, NULL);
	SDefaultWorkspaceActivePlatformState activePlatforms;
	activePlatforms.c64 = true;
	activePlatforms.atari800 = false;
	activePlatforms.nes = false;

	const SDefaultWorkspaceLayoutSpec *layoutSpec = defaultLayouts.ResolveShortcutLayout(activePlatforms, DefaultWorkspaceSlot_DataDump);
	if (layoutSpec == NULL || strcmp(layoutSpec->displayName, "C64 Data Dump") != 0)
	{
		snprintf(failureMsg, sizeof(failureMsg), "C64-only slot 2 should resolve to C64 Data Dump");
		return false;
	}

	layoutSpec = defaultLayouts.ResolveShortcutLayout(activePlatforms, DefaultWorkspaceSlot_FullScreenZoom);
	if (layoutSpec == NULL || strcmp(layoutSpec->displayName, "C64 Full Screen Zoom") != 0)
	{
		snprintf(failureMsg, sizeof(failureMsg), "C64-only slot 9 should resolve to C64 Full Screen Zoom");
		return false;
	}

	activePlatforms.c64 = false;
	activePlatforms.nes = true;
	layoutSpec = defaultLayouts.ResolveShortcutLayout(activePlatforms, DefaultWorkspaceSlot_1541MemoryMap);
	if (layoutSpec == NULL || strcmp(layoutSpec->displayName, "NES APU") != 0)
	{
		snprintf(failureMsg, sizeof(failureMsg), "NES-only slot 4 should resolve to NES APU");
		return false;
	}

	activePlatforms.nes = false;
	activePlatforms.atari800 = true;
	layoutSpec = defaultLayouts.ResolveShortcutLayout(activePlatforms, DefaultWorkspaceSlot_1541MemoryMap);
	if (layoutSpec != NULL)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Atari-only slot 4 should not resolve because Atari has no matching default layout");
		return false;
	}

	activePlatforms.c64 = true;
	activePlatforms.atari800 = true;
	activePlatforms.nes = false;
	layoutSpec = defaultLayouts.ResolveShortcutLayout(activePlatforms, DefaultWorkspaceSlot_DataDump);
	if (layoutSpec != NULL)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Shortcut resolution should not consume when more than one platform is active");
		return false;
	}

	activePlatforms.c64 = false;
	activePlatforms.atari800 = false;
	layoutSpec = defaultLayouts.ResolveShortcutLayout(activePlatforms, DefaultWorkspaceSlot_DataDump);
	if (layoutSpec != NULL)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Shortcut resolution should not consume when no platform is active");
		return false;
	}

	return true;
}

void CTestDefaultWorkspaceSpecs::Run(ITestCallback *cb)
{
	this->callback = cb;
	this->isRunning = true;
	this->currentStep = 0;
	failureMsg[0] = '\0';

	const std::vector<SDefaultWorkspaceLayoutSpec> &c64Specs = GetDefaultWorkspaceLayoutSpecs(DefaultWorkspacePlatform_C64);
	const std::vector<SDefaultWorkspaceLayoutSpec> &atariSpecs = GetDefaultWorkspaceLayoutSpecs(DefaultWorkspacePlatform_Atari800);
	const std::vector<SDefaultWorkspaceLayoutSpec> &nesSpecs = GetDefaultWorkspaceLayoutSpecs(DefaultWorkspacePlatform_NES);

	if (c64Specs.size() != 16 || atariSpecs.size() != 9 || nesSpecs.size() != 7)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Expected C64/Atari/NES spec counts 16/9/7, got %zu/%zu/%zu", c64Specs.size(), atariSpecs.size(), nesSpecs.size());
		TestCompleted(false, failureMsg);
		return;
	}
	StepCompleted(1, true, "Platform spec counts match old layout inventory");

	static const SExpectedDefaultWorkspaceLayoutSpec expectedC64Specs[] = {
		{ DefaultWorkspaceSlot_Only, "C64 Only", "retro.default.c64.only" },
		{ DefaultWorkspaceSlot_DataDump, "C64 Data Dump", "retro.default.c64.data_dump" },
		{ DefaultWorkspaceSlot_Debugger, "C64 Debugger", "retro.default.c64.debugger" },
		{ DefaultWorkspaceSlot_1541MemoryMap, "C64 1541 Memory Map", "retro.default.c64.1541_memory_map" },
		{ DefaultWorkspaceSlot_ShowStates, "C64 Show States", "retro.default.c64.show_states" },
		{ DefaultWorkspaceSlot_MemoryMap, "C64 Memory Map", "retro.default.c64.memory_map" },
		{ DefaultWorkspaceSlot_1541Debugger, "C64 1541 Debugger", "retro.default.c64.1541_debugger" },
		{ DefaultWorkspaceSlot_MonitorConsole, "C64 Monitor Console", "retro.default.c64.monitor_console" },
		{ DefaultWorkspaceSlot_FullScreenZoom, "C64 Full Screen Zoom", "retro.default.c64.full_screen_zoom" },
		{ DefaultWorkspaceSlot_Cycler, "C64 Cycler", "retro.default.c64.cycler" },
		{ DefaultWorkspaceSlot_VicDisplayLite, "C64 VIC Display Lite", "retro.default.c64.vic_display_lite" },
		{ DefaultWorkspaceSlot_VicDisplay, "C64 VIC Display", "retro.default.c64.vic_display" },
		{ DefaultWorkspaceSlot_SourceCode, "C64 Source Code", "retro.default.c64.source_code" },
		{ DefaultWorkspaceSlot_AllGraphics, "C64 All Graphics", "retro.default.c64.all_graphics" },
		{ DefaultWorkspaceSlot_AllSids, "C64 All SIDs", "retro.default.c64.all_sids" },
		{ DefaultWorkspaceSlot_MemoryDebugger, "C64 Memory Debugger", "retro.default.c64.memory_debugger" }
	};

	static const SExpectedDefaultWorkspaceLayoutSpec expectedAtariSpecs[] = {
		{ DefaultWorkspaceSlot_Only, "Atari800 Only", "retro.default.atari800.only" },
		{ DefaultWorkspaceSlot_DataDump, "Atari800 Data Dump", "retro.default.atari800.data_dump" },
		{ DefaultWorkspaceSlot_Debugger, "Atari800 Debugger", "retro.default.atari800.debugger" },
		{ DefaultWorkspaceSlot_ShowStates, "Atari800 Show States", "retro.default.atari800.show_states" },
		{ DefaultWorkspaceSlot_MemoryMap, "Atari800 Memory Map", "retro.default.atari800.memory_map" },
		{ DefaultWorkspaceSlot_MonitorConsole, "Atari800 Monitor Console", "retro.default.atari800.monitor_console" },
		{ DefaultWorkspaceSlot_Cycler, "Atari800 Cycler", "retro.default.atari800.cycler" },
		{ DefaultWorkspaceSlot_VicDisplayLite, "Atari800 Display Lite", "retro.default.atari800.display_lite" },
		{ DefaultWorkspaceSlot_SourceCode, "Atari800 Source Code", "retro.default.atari800.source_code" }
	};

	static const SExpectedDefaultWorkspaceLayoutSpec expectedNesSpecs[] = {
		{ DefaultWorkspaceSlot_Only, "NES Only", "retro.default.nes.only" },
		{ DefaultWorkspaceSlot_DataDump, "NES Data Dump", "retro.default.nes.data_dump" },
		{ DefaultWorkspaceSlot_Debugger, "NES Debugger", "retro.default.nes.debugger" },
		{ DefaultWorkspaceSlot_1541MemoryMap, "NES APU", "retro.default.nes.apu" },
		{ DefaultWorkspaceSlot_ShowStates, "NES Show States", "retro.default.nes.show_states" },
		{ DefaultWorkspaceSlot_MemoryMap, "NES Memory Map", "retro.default.nes.memory_map" },
		{ DefaultWorkspaceSlot_MonitorConsole, "NES Monitor Console", "retro.default.nes.monitor_console" }
	};

	if (!VerifyLayoutSpecs(c64Specs, expectedC64Specs, sizeof(expectedC64Specs) / sizeof(expectedC64Specs[0]), "C64")
		|| !VerifyLayoutSpecs(atariSpecs, expectedAtariSpecs, sizeof(expectedAtariSpecs) / sizeof(expectedAtariSpecs[0]), "Atari800")
		|| !VerifyLayoutSpecs(nesSpecs, expectedNesSpecs, sizeof(expectedNesSpecs) / sizeof(expectedNesSpecs[0]), "NES"))
	{
		TestCompleted(false, failureMsg);
		return;
	}

	const SDefaultWorkspaceLayoutSpec *c64DataDump = FindDefaultWorkspaceLayoutSpec(DefaultWorkspacePlatform_C64, DefaultWorkspaceSlot_DataDump);
	if (c64DataDump == NULL
		|| strcmp(c64DataDump->displayName, "C64 Data Dump") != 0
		|| strcmp(c64DataDump->predefinedId, "retro.default.c64.data_dump") != 0)
	{
		TestCompleted(false, "C64 Data Dump spec should have stable name and predefined id");
		return;
	}

	const SDefaultWorkspaceLayoutSpec *c64Only = FindDefaultWorkspaceLayoutSpec(DefaultWorkspacePlatform_C64, DefaultWorkspaceSlot_Only);
	if (c64Only == NULL
		|| c64Only->placements == NULL
		|| c64Only->numPlacements != 1
		|| !VerifyPlacementSpec(c64Only, "c64.screen", 35.102f, 0.0f, 508.235f, 360.0f))
	{
		TestCompleted(false, failureMsg[0] != '\0' ? failureMsg : "C64 Only should include old screen-only geometry");
		return;
	}

	if (c64DataDump->placements == NULL
		|| c64DataDump->numPlacements != 5
		|| !VerifyPlacementSpec(c64DataDump, "c64.screen", 317.416f, 10.5f, 259.584f, 183.872f)
		|| !VerifyPlacementSpec(c64DataDump, "c64.cpu_state", 317.416f, 0.0f, 0.0f, 0.0f)
		|| !VerifyPlacementSpec(c64DataDump, "c64.disassembly", 1.0f, 1.0f, 105.0f, 356.0f)
		|| !VerifyPlacementSpec(c64DataDump, "c64.memory_map", 112.0f, 1.0f, 199.0f, 192.0f)
		|| !VerifyPlacementSpec(c64DataDump, "c64.data_dump", 108.0f, 196.0f, 470.0f, 162.0f))
	{
		TestCompleted(false, failureMsg[0] != '\0' ? failureMsg : "C64 Data Dump should include the first old geometry placement set");
		return;
	}

	const SDefaultWorkspaceLayoutSpec *c64Debugger = FindDefaultWorkspaceLayoutSpec(DefaultWorkspacePlatform_C64, DefaultWorkspaceSlot_Debugger);
	if (c64Debugger == NULL
		|| c64Debugger->placements == NULL
		|| c64Debugger->numPlacements != 5
		|| !VerifyPlacementSpec(c64Debugger, "c64.screen", 180.0f, 10.0f, 257.28f, 182.24f)
		|| !VerifyPlacementSpec(c64Debugger, "c64.cpu_state", 181.0f, 0.0f, 0.0f, 0.0f)
		|| !VerifyPlacementSpec(c64Debugger, "c64.disassembly", 1.0f, 1.0f, 175.0f, 356.0f)
		|| !VerifyPlacementSpec(c64Debugger, "c64.data_dump", 178.0f, 195.0f, 470.0f, 165.0f)
		|| !VerifyPlacementSpec(c64Debugger, "c64.vic_state", 440.0f, 0.0f, 136.0f, 192.0f))
	{
		TestCompleted(false, failureMsg[0] != '\0' ? failureMsg : "C64 Debugger should include old debugger geometry including VIC state");
		return;
	}

	const SDefaultWorkspaceLayoutSpec *c64MemoryMap = FindDefaultWorkspaceLayoutSpec(DefaultWorkspacePlatform_C64, DefaultWorkspaceSlot_MemoryMap);
	if (c64MemoryMap == NULL
		|| c64MemoryMap->placements == NULL
		|| c64MemoryMap->numPlacements != 5
		|| !VerifyPlacementSpec(c64MemoryMap, "c64.screen", 420.0f, 10.0f, 157.44f, 111.52f)
		|| !VerifyPlacementSpec(c64MemoryMap, "c64.cpu_state", 78.0f, 0.0f, 0.0f, 0.0f)
		|| !VerifyPlacementSpec(c64MemoryMap, "c64.disassembly", 0.5f, 0.5f, 75.0f, 359.0f)
		|| !VerifyPlacementSpec(c64MemoryMap, "c64.memory_map", 77.0f, 15.0f, 340.5f, 340.5f)
		|| !VerifyPlacementSpec(c64MemoryMap, "c64.data_dump", 421.0f, 125.0f, 470.0f, 230.0f))
	{
		TestCompleted(false, failureMsg[0] != '\0' ? failureMsg : "C64 Memory Map should include old memory-map geometry");
		return;
	}

	const SDefaultWorkspaceLayoutSpec *c64DriveDebugger = FindDefaultWorkspaceLayoutSpec(DefaultWorkspacePlatform_C64, DefaultWorkspaceSlot_1541Debugger);
	if (c64DriveDebugger == NULL
		|| c64DriveDebugger->placements == NULL
		|| c64DriveDebugger->numPlacements != 9
		|| !VerifyPlacementSpec(c64DriveDebugger, "c64.screen", 80.0f, 10.0f, 418.56f, 296.48f)
		|| !VerifyPlacementSpec(c64DriveDebugger, "c64.cpu_state", 80.0f, 0.0f, 0.0f, 0.0f)
		|| !VerifyPlacementSpec(c64DriveDebugger, "c64.disassembly", 0.5f, 0.5f, 75.0f, 359.0f)
		|| !VerifyPlacementSpec(c64DriveDebugger, "drive1541.cpu_state", 350.0f, 0.0f, 0.0f, 0.0f)
		|| !VerifyPlacementSpec(c64DriveDebugger, "drive1541.disassembly", 500.0f, 0.5f, 75.0f, 359.0f)
		|| !VerifyPlacementSpec(c64DriveDebugger, "drive1541.via_state", 342.0f, 310.0f, 240.0f, 50.0f)
		|| !VerifyPlacementSpec(c64DriveDebugger, "c64.memory_map", 80.25f, 309.25f, 50.0f, 50.0f)
		|| !VerifyPlacementSpec(c64DriveDebugger, "drive1541.memory_map", 447.75f, 309.25f, 50.0f, 50.0f)
		|| !VerifyPlacementSpec(c64DriveDebugger, "c64.cia_state", 135.0f, 310.0f, 380.0f, 58.0f))
	{
		TestCompleted(false, failureMsg[0] != '\0' ? failureMsg : "C64 1541 Debugger should include old drive debugger geometry");
		return;
	}

	const SDefaultWorkspaceLayoutSpec *c64DriveMemoryMap = FindDefaultWorkspaceLayoutSpec(DefaultWorkspacePlatform_C64, DefaultWorkspaceSlot_1541MemoryMap);
	if (c64DriveMemoryMap == NULL
		|| c64DriveMemoryMap->placements == NULL
		|| c64DriveMemoryMap->numPlacements != 7
		|| !VerifyPlacementSpec(c64DriveMemoryMap, "c64.screen", 190.0f, 0.0f, 201.6f, 152.8f)
		|| !VerifyPlacementSpec(c64DriveMemoryMap, "c64.cpu_state", 0.5f, 0.0f, 178.5f, 10.0f, 3.5f)
		|| !VerifyPlacementSpec(c64DriveMemoryMap, "c64.disassembly", 0.5f, 17.0f, 178.5f, 338.0f, 5.0f)
		|| !VerifyPlacementSpec(c64DriveMemoryMap, "drive1541.cpu_state", 429.0f, 0.0f, 150.0f, 10.0f, 5.0f)
		|| !VerifyPlacementSpec(c64DriveMemoryMap, "drive1541.disassembly", 429.0f, 17.0f, 150.0f, 338.0f, 5.0f)
		|| !VerifyPlacementSpec(c64DriveMemoryMap, "c64.memory_map", 190.0f, 155.0f, 115.0f, 200.0f)
		|| !VerifyPlacementSpec(c64DriveMemoryMap, "drive1541.memory_map", 306.0f, 155.0f, 115.0f, 200.0f))
	{
		TestCompleted(false, failureMsg[0] != '\0' ? failureMsg : "C64 1541 Memory Map should include old dual memory-map geometry");
		return;
	}

	const SDefaultWorkspaceLayoutSpec *c64ShowStates = FindDefaultWorkspaceLayoutSpec(DefaultWorkspacePlatform_C64, DefaultWorkspaceSlot_ShowStates);
	if (c64ShowStates == NULL
		|| c64ShowStates->placements == NULL
		|| c64ShowStates->numPlacements != 9
		|| !VerifyPlacementSpec(c64ShowStates, "c64.screen", 317.416f, 10.5f, 259.584f, 183.872f)
		|| !VerifyPlacementSpec(c64ShowStates, "c64.cpu_state", 317.416f, 0.0f, 0.0f, 0.0f)
		|| !VerifyPlacementSpec(c64ShowStates, "c64.vic_state", 13.0f, 13.0f, 290.0f, 160.0f)
		|| !VerifyPlacementSpec(c64ShowStates, "c64.sid_state", 0.0f, 190.0f, 100.0f, 100.0f)
		|| !VerifyPlacementSpec(c64ShowStates, "c64.cia_state", 190.0f, 200.0f, 380.0f, 58.0f)
		|| !VerifyPlacementSpec(c64ShowStates, "c64.reu_state", 315.0f, 315.0f, 380.0f, 58.0f)
		|| !VerifyPlacementSpec(c64ShowStates, "c64.emulation_counters", 496.0f, 335.0f, 380.0f, 58.0f)
		|| !VerifyPlacementSpec(c64ShowStates, "drive1541.via_state", 190.0f, 265.0f, 240.0f, 50.0f)
		|| !VerifyPlacementSpec(c64ShowStates, "c64.emulation_state", 371.0f, 350.0f, 100.0f, 100.0f))
	{
		TestCompleted(false, failureMsg[0] != '\0' ? failureMsg : "C64 Show States should include old state-view geometry");
		return;
	}

	const SDefaultWorkspaceLayoutSpec *c64MonitorConsole = FindDefaultWorkspaceLayoutSpec(DefaultWorkspacePlatform_C64, DefaultWorkspaceSlot_MonitorConsole);
	if (c64MonitorConsole == NULL
		|| c64MonitorConsole->placements == NULL
		|| c64MonitorConsole->numPlacements != 6
		|| !VerifyPlacementSpec(c64MonitorConsole, "c64.screen", 317.416f, 10.5f, 259.584f, 183.872f)
		|| !VerifyPlacementSpec(c64MonitorConsole, "c64.cpu_state", 317.416f, 0.0f, 0.0f, 0.0f)
		|| !VerifyPlacementSpec(c64MonitorConsole, "c64.monitor_console", 1.0f, 1.0f, 310.0f, 194.372f)
		|| !VerifyPlacementSpec(c64MonitorConsole, "c64.disassembly", 1.0f, 195.5f, 125.0f, 159.5f)
		|| !VerifyPlacementSpec(c64MonitorConsole, "c64.data_dump", 128.0f, 195.5f, 252.0f, 165.0f)
		|| !VerifyPlacementSpec(c64MonitorConsole, "c64.memory_map", 381.0f, 195.5f, 199.0f, 164.0f))
	{
		TestCompleted(false, failureMsg[0] != '\0' ? failureMsg : "C64 Monitor Console should include old monitor-console geometry");
		return;
	}

	const SDefaultWorkspaceLayoutSpec *c64FullScreenZoom = FindDefaultWorkspaceLayoutSpec(DefaultWorkspacePlatform_C64, DefaultWorkspaceSlot_FullScreenZoom);
	if (c64FullScreenZoom == NULL
		|| c64FullScreenZoom->placements == NULL
		|| c64FullScreenZoom->numPlacements != 1
		|| !VerifyPlacementSpec(c64FullScreenZoom, "c64.screen", 35.882f, 0.0f, 508.235f, 360.0f))
	{
		TestCompleted(false, failureMsg[0] != '\0' ? failureMsg : "C64 Full Screen Zoom should approximate old zoomed-screen geometry with a screen-only placement");
		return;
	}

	const SDefaultWorkspaceLayoutSpec *c64Cycler = FindDefaultWorkspaceLayoutSpec(DefaultWorkspacePlatform_C64, DefaultWorkspaceSlot_Cycler);
	if (c64Cycler == NULL
		|| c64Cycler->placements == NULL
		|| c64Cycler->numPlacements != 5
		|| !VerifyPlacementSpec(c64Cycler, "c64.screen", 317.416f, 10.5f, 259.584f, 183.872f)
		|| !VerifyPlacementSpec(c64Cycler, "c64.cpu_state", 317.416f, 0.0f, 0.0f, 0.0f)
		|| !VerifyPlacementSpec(c64Cycler, "c64.disassembly", 1.0f, 1.0f, 315.0f, 208.0f)
		|| !VerifyPlacementSpec(c64Cycler, "c64.data_dump", 0.0f, 217.0f, 313.0f, 143.0f)
		|| !VerifyPlacementSpec(c64Cycler, "c64.vic_state", 320.0f, 330.0f, 256.0f, 28.0f))
	{
		TestCompleted(false, failureMsg[0] != '\0' ? failureMsg : "C64 Cycler should include old cycler geometry");
		return;
	}

	const SDefaultWorkspaceLayoutSpec *c64VicDisplayLite = FindDefaultWorkspaceLayoutSpec(DefaultWorkspacePlatform_C64, DefaultWorkspaceSlot_VicDisplayLite);
	if (c64VicDisplayLite == NULL
		|| c64VicDisplayLite->placements == NULL
		|| c64VicDisplayLite->numPlacements != 5
		|| !VerifyPlacementSpec(c64VicDisplayLite, "c64.disassembly", 1.0f, 1.0f, 180.6f, 356.0f)
		|| !VerifyPlacementSpec(c64VicDisplayLite, "c64.cpu_state", 185.5f, 2.5f, 0.0f, 0.0f)
		|| !VerifyPlacementSpec(c64VicDisplayLite, "c64.data_dump", 185.5f, 260.0f, 277.0f, 97.0f)
		|| !VerifyPlacementSpec(c64VicDisplayLite, "c64.screen", 185.5f, 13.0f, 345.6f, 244.8f)
		|| !VerifyPlacementSpec(c64VicDisplayLite, "c64.memory_map", 467.5f, 260.0f, 110.0f, 99.0f))
	{
		TestCompleted(false, failureMsg[0] != '\0' ? failureMsg : "C64 VIC Display Lite should include old display-lite geometry");
		return;
	}

	const SDefaultWorkspaceLayoutSpec *c64VicDisplay = FindDefaultWorkspaceLayoutSpec(DefaultWorkspacePlatform_C64, DefaultWorkspaceSlot_VicDisplay);
	if (c64VicDisplay == NULL
		|| c64VicDisplay->placements == NULL
		|| c64VicDisplay->numPlacements != 7
		|| !VerifyPlacementSpec(c64VicDisplay, "c64.screen", 444.6f, 15.0f, 134.4f, 95.2f)
		|| !VerifyPlacementSpec(c64VicDisplay, "c64.cpu_state", 354.6f, 2.5f, 0.0f, 0.0f)
		|| !VerifyPlacementSpec(c64VicDisplay, "c64.disassembly", 1.0f, 1.0f, 180.6f, 356.0f)
		|| !VerifyPlacementSpec(c64VicDisplay, "c64.vic_state", 186.5f, 1.0f, 256.5f, 144.0f)
		|| !VerifyPlacementSpec(c64VicDisplay, "c64.data_dump", 449.5f, 113.0f, 125.0f, 34.0f)
		|| !VerifyPlacementSpec(c64VicDisplay, "c64.vic_display", 185.5f, 150.0f, 329.989f, 204.280f)
		|| !VerifyPlacementSpec(c64VicDisplay, "c64.vic_control", 521.5f, 150.0f, 200.0f, 200.0f))
	{
		TestCompleted(false, failureMsg[0] != '\0' ? failureMsg : "C64 VIC Display should include old display geometry");
		return;
	}

	const SDefaultWorkspaceLayoutSpec *c64SourceCode = FindDefaultWorkspaceLayoutSpec(DefaultWorkspacePlatform_C64, DefaultWorkspaceSlot_SourceCode);
	if (c64SourceCode == NULL
		|| c64SourceCode->placements == NULL
		|| c64SourceCode->numPlacements != 4
		|| !VerifyPlacementSpec(c64SourceCode, "c64.disassembly", 1.0f, 1.0f, 110.6f, 356.0f)
		|| !VerifyPlacementSpec(c64SourceCode, "c64.source_code", 114.1f, 15.0f, 463.9f, 341.0f)
		|| !VerifyPlacementSpec(c64SourceCode, "c64.cpu_state", 220.0f, 2.5f, 0.0f, 0.0f)
		|| !VerifyPlacementSpec(c64SourceCode, "c64.screen", 478.0f, 0.0f, 97.92f, 69.36f))
	{
		TestCompleted(false, failureMsg[0] != '\0' ? failureMsg : "C64 Source Code should include old source-code geometry");
		return;
	}

	const SDefaultWorkspaceLayoutSpec *c64AllGraphics = FindDefaultWorkspaceLayoutSpec(DefaultWorkspacePlatform_C64, DefaultWorkspaceSlot_AllGraphics);
	if (c64AllGraphics == NULL
		|| c64AllGraphics->placements == NULL
		|| c64AllGraphics->numPlacements != 5
		|| !VerifyPlacementSpec(c64AllGraphics, "c64.all_graphics_bitmaps", 0.0f, 0.0f, 169.0f, 180.0f)
		|| !VerifyPlacementSpec(c64AllGraphics, "c64.all_graphics_screens", 169.0f, 0.0f, 169.0f, 180.0f)
		|| !VerifyPlacementSpec(c64AllGraphics, "c64.all_graphics_charsets", 0.0f, 180.0f, 169.0f, 180.0f)
		|| !VerifyPlacementSpec(c64AllGraphics, "c64.all_graphics_sprites", 169.0f, 180.0f, 169.0f, 180.0f)
		|| !VerifyPlacementSpec(c64AllGraphics, "c64.screen", 458.0f, 15.0f, 115.2f, 81.6f))
	{
		TestCompleted(false, failureMsg[0] != '\0' ? failureMsg : "C64 All Graphics should approximate the old aggregate with split graphics views and the old screen anchor");
		return;
	}

	const SDefaultWorkspaceLayoutSpec *c64AllSids = FindDefaultWorkspaceLayoutSpec(DefaultWorkspacePlatform_C64, DefaultWorkspaceSlot_AllSids);
	if (c64AllSids == NULL
		|| c64AllSids->placements == NULL
		|| c64AllSids->numPlacements != 8
		|| !VerifyPlacementSpec(c64AllSids, "c64.sid_tracker_history", 0.0f, 0.0f, 350.0f, 126.0f)
		|| !VerifyPlacementSpec(c64AllSids, "c64.sid_piano_keyboard", 0.0f, 128.0f, 350.0f, 58.0f)
		|| !VerifyPlacementSpec(c64AllSids, "c64.sid_state", 0.0f, 190.0f, 350.0f, 168.0f)
		|| !VerifyPlacementSpec(c64AllSids, "c64.cpu_state", 350.0f, 2.5f, 0.0f, 0.0f)
		|| !VerifyPlacementSpec(c64AllSids, "c64.data_dump", 458.0f, 100.0f, 470.0f, 135.0f)
		|| !VerifyPlacementSpec(c64AllSids, "c64.memory_map", 442.5f, 239.0f, 130.0f, 119.0f)
		|| !VerifyPlacementSpec(c64AllSids, "c64.disassembly", 358.0f, 239.0f, 79.0f, 119.0f)
		|| !VerifyPlacementSpec(c64AllSids, "c64.screen", 458.0f, 15.0f, 115.2f, 81.6f))
	{
		TestCompleted(false, failureMsg[0] != '\0' ? failureMsg : "C64 All SIDs should approximate old aggregate view with SID tracker/piano/state and old support geometry");
		return;
	}

	const SDefaultWorkspaceLayoutSpec *c64MemoryDebugger = FindDefaultWorkspaceLayoutSpec(DefaultWorkspacePlatform_C64, DefaultWorkspaceSlot_MemoryDebugger);
	if (c64MemoryDebugger == NULL
		|| c64MemoryDebugger->placements == NULL
		|| c64MemoryDebugger->numPlacements != 8
		|| !VerifyPlacementSpec(c64MemoryDebugger, "c64.cpu_state", 350.0f, 2.5f, 0.0f, 0.0f)
		|| !VerifyPlacementSpec(c64MemoryDebugger, "c64.disassembly", 1.0f, 2.0f, 65.6f, 180.0f)
		|| !VerifyPlacementSpec(c64MemoryDebugger, "c64.disassembly2", 1.0f, 186.0f, 65.6f, 167.0f)
		|| !VerifyPlacementSpec(c64MemoryDebugger, "c64.data_dump", 67.1f, 15.5f, 387.0f, 169.0f)
		|| !VerifyPlacementSpec(c64MemoryDebugger, "c64.data_dump2", 67.1f, 185.5f, 387.0f, 169.0f)
		|| !VerifyPlacementSpec(c64MemoryDebugger, "c64.data_dump3", 458.0f, 100.0f, 470.0f, 135.0f)
		|| !VerifyPlacementSpec(c64MemoryDebugger, "c64.memory_map", 460.0f, 239.0f, 112.0f, 119.0f)
		|| !VerifyPlacementSpec(c64MemoryDebugger, "c64.screen", 458.0f, 15.0f, 115.2f, 81.6f))
	{
		TestCompleted(false, failureMsg[0] != '\0' ? failureMsg : "C64 Memory Debugger should approximate the old layout with memory/debugger windows and no obsolete toolbar");
		return;
	}

	const SDefaultWorkspaceLayoutSpec *atariDataDump = FindDefaultWorkspaceLayoutSpec(DefaultWorkspacePlatform_Atari800, DefaultWorkspaceSlot_DataDump);
	if (atariDataDump == NULL
		|| strcmp(atariDataDump->displayName, "Atari800 Data Dump") != 0
		|| strcmp(atariDataDump->predefinedId, "retro.default.atari800.data_dump") != 0)
	{
		TestCompleted(false, "Atari800 Data Dump spec should have stable name and predefined id");
		return;
	}

	const SDefaultWorkspaceLayoutSpec *nesApu = FindDefaultWorkspaceLayoutSpec(DefaultWorkspacePlatform_NES, DefaultWorkspaceSlot_1541MemoryMap);
	if (nesApu == NULL
		|| strcmp(nesApu->displayName, "NES APU") != 0
		|| strcmp(nesApu->predefinedId, "retro.default.nes.apu") != 0)
	{
		TestCompleted(false, "NES slot 4 should be the APU default workspace");
		return;
	}
	StepCompleted(2, true, "Stable names and IDs are deterministic");

	SDefaultWorkspaceViewPlacementSpec placement = { "screen", 1.0f, 2.0f, 3.0f, 4.0f };
	if (strcmp(placement.viewId, "screen") != 0 || placement.x != 1.0f || placement.y != 2.0f || placement.width != 3.0f || placement.height != 4.0f)
	{
		TestCompleted(false, "View placement spec should expose old geometry fields");
		return;
	}

	SDefaultWorkspaceViewPlacementSpec scaledPlacement = ScaleDefaultWorkspacePlacement({ "screen", 10.0f, 20.0f, 100.0f, 50.0f }, 1160.0f, 720.0f);
	if (strcmp(scaledPlacement.viewId, "screen") != 0
		|| !NearlyEqual(scaledPlacement.x, 20.0f)
		|| !NearlyEqual(scaledPlacement.y, 40.0f)
		|| !NearlyEqual(scaledPlacement.width, 200.0f)
		|| !NearlyEqual(scaledPlacement.height, 100.0f))
	{
		TestCompleted(false, "Placement scaling should preserve old 580x360 proportions in the current viewport");
		return;
	}

	scaledPlacement = ScaleDefaultWorkspacePlacement({ "tiny", 1.0f, 1.0f, 2.0f, 3.0f }, 580.0f, 360.0f);
	if (strcmp(scaledPlacement.viewId, "tiny") != 0
		|| !NearlyEqual(scaledPlacement.x, 1.0f)
		|| !NearlyEqual(scaledPlacement.y, 1.0f)
		|| !NearlyEqual(scaledPlacement.width, 24.0f)
		|| !NearlyEqual(scaledPlacement.height, 24.0f))
	{
		TestCompleted(false, "Placement scaling should clamp tiny windows to the minimum safe size");
		return;
	}

	SDefaultWorkspacePopupState popupState;
	if (!popupState.lockedDefaultLayouts || !popupState.createContextShortcuts || !popupState.noTabBarsInGeneratedLayouts)
	{
		TestCompleted(false, "Popup state should default to locked layouts, context shortcuts, and NoTabBar enabled");
		return;
	}

	const std::vector<SDefaultWorkspaceShortcutSlotSpec> &shortcutSlots = GetDefaultWorkspaceShortcutSlotSpecs();
	const SDefaultWorkspaceShortcutSlotSpec *dataDumpShortcut = FindDefaultWorkspaceShortcutSlotSpec(DefaultWorkspaceSlot_DataDump);
	if (shortcutSlots.size() != 16 || dataDumpShortcut == NULL
		|| strcmp(dataDumpShortcut->roleName, "Data Dump") != 0
		|| strcmp(dataDumpShortcut->defaultShortcutLabel, "Ctrl+F2") != 0
		|| dataDumpShortcut->keyCode <= 0
		|| !dataDumpShortcut->isControl
		|| dataDumpShortcut->isShift)
	{
		TestCompleted(false, "Shortcut slot specs should define deterministic default shortcut metadata");
		return;
	}
	StepCompleted(3, true, "Chunk 1 helper structs are present with deterministic defaults");

	if (FindDefaultWorkspaceLayoutSpec(DefaultWorkspacePlatform_Atari800, DefaultWorkspaceSlot_1541MemoryMap) != NULL)
	{
		TestCompleted(false, "Atari800 should not invent a 1541 Memory Map default workspace");
		return;
	}

	if (FindDefaultWorkspaceLayoutSpec(DefaultWorkspacePlatform_NES, DefaultWorkspaceSlot_VicDisplay) != NULL)
	{
		TestCompleted(false, "NES should not invent a VIC Display default workspace");
		return;
	}

	const std::vector<SDefaultWorkspaceLayoutSpec> &allSpecs = GetAllDefaultWorkspaceLayoutSpecs();
	if (allSpecs.size() != 32)
	{
		snprintf(failureMsg, sizeof(failureMsg), "Expected 32 default workspace specs and no mixed layout, got %zu", allSpecs.size());
		TestCompleted(false, failureMsg);
		return;
	}

	std::set<std::string> ids;
	for (std::vector<SDefaultWorkspaceLayoutSpec>::const_iterator it = allSpecs.begin(); it != allSpecs.end(); ++it)
	{
		if (ContainsText(it->displayName, "And Atari") || ContainsText(it->predefinedId, "mixed"))
		{
			snprintf(failureMsg, sizeof(failureMsg), "Mixed default workspace should be absent, found '%s'", it->displayName);
			TestCompleted(false, failureMsg);
			return;
		}

		if (!ids.insert(it->predefinedId).second)
		{
			snprintf(failureMsg, sizeof(failureMsg), "Duplicate predefined id '%s'", it->predefinedId);
			TestCompleted(false, failureMsg);
			return;
		}
	}
	StepCompleted(4, true, "Mixed layout absent and predefined IDs are unique");

	if (!VerifyDefaultWorkspaceShortcutRegistration()
		|| !VerifyDefaultWorkspaceShortcutConflicts()
		|| !VerifyDefaultWorkspaceShortcutAssignment()
		|| !VerifyPreserveScanTabBarModeFlagPolicy()
		|| !VerifyDebugLogDefaultVisibilityPolicy()
		|| !VerifyDefaultWorkspaceCreatePreservesPopupAssignments()
		|| !VerifyDefaultWorkspaceShortcutPersistenceAcrossStartup()
		|| !VerifyDefaultWorkspaceLayoutDataCreation()
		|| !VerifyAllDefaultWorkspaceSpecsHaveResolvablePlacements()
		|| !VerifyDefaultWorkspaceShortcutRestoreAndReset()
		|| !VerifyDefaultWorkspacePopupCaptureState()
		|| !VerifyDefaultWorkspaceApplyC64DataDumpLayout()
		|| !VerifyDefaultWorkspaceHidesGlobalDebugLog()
		|| !VerifyDefaultWorkspaceApplyC64DebuggerLayout()
		|| !VerifyDefaultWorkspaceApplyC64MemoryMapLayout()
		|| !VerifyDefaultWorkspaceApplyC64DriveDebuggerLayout()
		|| !VerifyDefaultWorkspaceApplyC64DriveMemoryMapLayout()
		|| !VerifyDefaultWorkspaceC64DisassemblyParametersDoNotLeakBetweenLayouts()
		|| !VerifyDefaultWorkspaceApplyC64ShowStatesLayout()
		|| !VerifyDefaultWorkspaceApplyC64MonitorConsoleLayout()
		|| !VerifyDefaultWorkspaceApplyC64FullScreenZoomLayout()
		|| !VerifyDefaultWorkspaceApplyC64CyclerLayout()
		|| !VerifyDefaultWorkspaceApplyC64VicDisplayLiteLayout()
		|| !VerifyDefaultWorkspaceApplyC64VicDisplayLayout()
		|| !VerifyDefaultWorkspaceApplyC64SourceCodeLayout()
		|| !VerifyDefaultWorkspaceApplyC64AllGraphicsLayout()
		|| !VerifyDefaultWorkspaceApplyC64AllSidsLayout()
		|| !VerifyDefaultWorkspaceApplyC64MemoryDebuggerLayout()
		|| !VerifyDefaultWorkspaceRecalculatesFontsFromScaledGeometry()
		|| !VerifyDefaultWorkspaceShortcutResolution())
	{
		TestCompleted(false, failureMsg);
		return;
	}
	StepCompleted(5, true, "Shortcut slot registration, conflicts, and active-platform resolution work");

	TestCompleted(true, "Default workspace specs match old layout inventory");
}

void CTestDefaultWorkspaceSpecs::Cancel()
{
	isRunning = false;
	TestCompleted(false, "Cancelled");
}
