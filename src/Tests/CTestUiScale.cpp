#include "CTestUiScale.h"

#include "C64DUiScale.h"
#include "SYS_Main.h"
#include "CByteBuffer.h"
#include "CGuiMain.h"
#include "CGuiView.h"
#include "CLayoutManager.h"
#include "CLayoutParameter.h"
#include "CViewC64.h"
#include "CViewDisassembly.h"
#include "MT_Theme.h"
#include "MT_UiScale.h"
#include "VID_Main.h"
#include "imgui.h"

#include <cstdio>
#include <cstring>
#include <cmath>
#include <string>
#include <map>
#include <list>

static char failureMsg[1024];

static bool NearlyEqual(float a, float b, float epsilon = 0.001f)
{
	float diff = a - b;
	if (diff < 0.0f)
		diff = -diff;
	return diff < epsilon;
}

// ---------------------------------------------------------------------------
// #1 -- the .ini transformer
// ---------------------------------------------------------------------------

static bool IniContains(const char *ini, const char *needle)
{
	return ini != NULL && strstr(ini, needle) != NULL;
}

static bool VerifyIniTransformScalesGeometry()
{
	const char *input =
		"[Window][C64 Disassembly]\n"
		"Pos=10,20\n"
		"Size=278,437\n"
		"Collapsed=0\n"
		"DockId=0x00000003,0\n"
		"ViewportPos=-7,56\n"
		"ViewportId=0x1A2B3C4D\n"
		"\n"
		"[Docking][Data]\n"
		"DockNode ID=0x00000003 Parent=0x8B93E3BD SizeRef=278,437 Selected=0xDEADBEEF\n";

	char *out = C64D_UiScaleTransformImGuiIni(input, 2.0f);
	if (out == NULL)
	{
		snprintf(failureMsg, sizeof(failureMsg), "ini transform returned NULL");
		return false;
	}

	bool ok = true;

	// Geometry doubled...
	if (!IniContains(out, "Pos=20,40\n"))		{ snprintf(failureMsg, sizeof(failureMsg), "Pos was not scaled: %s", out); ok = false; }
	else if (!IniContains(out, "Size=556,874"))	{ snprintf(failureMsg, sizeof(failureMsg), "Size was not scaled: %s", out); ok = false; }
	else if (!IniContains(out, "SizeRef=556,874")) { snprintf(failureMsg, sizeof(failureMsg), "SizeRef was not scaled: %s", out); ok = false; }
	else if (!IniContains(out, "ViewportPos=-14,112")) { snprintf(failureMsg, sizeof(failureMsg), "ViewportPos (negative) was not scaled: %s", out); ok = false; }

	// ...and identifiers untouched. ViewportId is the one that would break if
	// "Pos=" were matched without a token-boundary test, because "Pos=" is a
	// substring of "ViewportPos=" and "Id" of neither.
	else if (!IniContains(out, "DockId=0x00000003,0"))				{ snprintf(failureMsg, sizeof(failureMsg), "DockId was rewritten: %s", out); ok = false; }
	else if (!IniContains(out, "ViewportId=0x1A2B3C4D"))			{ snprintf(failureMsg, sizeof(failureMsg), "ViewportId was rewritten: %s", out); ok = false; }
	else if (!IniContains(out, "ID=0x00000003"))					{ snprintf(failureMsg, sizeof(failureMsg), "node ID was rewritten: %s", out); ok = false; }
	else if (!IniContains(out, "Parent=0x8B93E3BD"))				{ snprintf(failureMsg, sizeof(failureMsg), "Parent was rewritten: %s", out); ok = false; }
	else if (!IniContains(out, "Selected=0xDEADBEEF"))				{ snprintf(failureMsg, sizeof(failureMsg), "Selected was rewritten: %s", out); ok = false; }
	else if (!IniContains(out, "Collapsed=0"))						{ snprintf(failureMsg, sizeof(failureMsg), "Collapsed was rewritten: %s", out); ok = false; }

	delete[] out;
	return ok;
}

static bool VerifyIniTransformRoundTrips()
{
	// Scaling up and back down must land on the original for the integer sizes
	// ImGui actually writes.
	const char *input = "[Window][X]\nPos=100,200\nSize=640,480\n";

	char *up = C64D_UiScaleTransformImGuiIni(input, 2.0f);
	char *down = C64D_UiScaleTransformImGuiIni(up, 0.5f);

	bool ok = (strcmp(down, input) == 0);
	if (!ok)
		snprintf(failureMsg, sizeof(failureMsg), "ini round trip 2.0 then 0.5 changed the text: '%s'", down);

	delete[] up;
	delete[] down;
	return ok;
}

static bool VerifyIniTransformLeavesNonPairsAlone()
{
	// A key that looks scalable but whose value is not an "x,y" pair must be
	// copied verbatim rather than mangled or dropped.
	const char *input = "[Window][X]\nSize=notapair\nPos=5,6\n";

	char *out = C64D_UiScaleTransformImGuiIni(input, 3.0f);
	bool ok = IniContains(out, "Size=notapair") && IniContains(out, "Pos=15,18");
	if (!ok)
		snprintf(failureMsg, sizeof(failureMsg), "non-pair value was not preserved: '%s'", out);

	delete[] out;
	return ok;
}

// ---------------------------------------------------------------------------
// #2 -- which layout parameters are sizes
// ---------------------------------------------------------------------------

static bool VerifySizeLikeParameterClassification()
{
	const char *sizes[] = { "Font Size", "Font Scale", "Display zoom", "Display pos X", "Display pos Y" };
	for (int i = 0; i < (int)(sizeof(sizes) / sizeof(sizes[0])); i++)
	{
		if (!C64D_UiScaleIsSizeLikeLayoutParameter(sizes[i]))
		{
			snprintf(failureMsg, sizeof(failureMsg), "'%s' should be classified as a size", sizes[i]);
			return false;
		}
	}

	// GT2RenoiseUIScale is a USER ZOOM that GT2EffectiveUIScale() already
	// multiplies by the UI scale at render time. Migrating it too would apply
	// the display scale twice to the GT2 grid.
	const char *notSizes[] = { "GT2RenoiseUIScale", "Visible", "Show hex", NULL };
	for (int i = 0; notSizes[i] != NULL; i++)
	{
		if (C64D_UiScaleIsSizeLikeLayoutParameter(notSizes[i]))
		{
			snprintf(failureMsg, sizeof(failureMsg), "'%s' must NOT be classified as a size", notSizes[i]);
			return false;
		}
	}

	if (C64D_UiScaleIsSizeLikeLayoutParameter(NULL))
	{
		snprintf(failureMsg, sizeof(failureMsg), "NULL parameter name must not be classified as a size");
		return false;
	}

	return true;
}

// ---------------------------------------------------------------------------
// #3 -- the layout buffer migration, end to end against live views
// ---------------------------------------------------------------------------

static bool VerifyLayoutBufferMigration()
{
	if (guiMain == NULL)
	{
		snprintf(failureMsg, sizeof(failureMsg), "guiMain is NULL");
		return false;
	}

	// Serialize the real UI into a real layout buffer, so the test exercises
	// the format the app actually writes rather than a hand-built one.
	CLayoutData *layout = new CLayoutData();
	layout->layoutName = STRALLOC("__uiscale_test__");
	guiMain->SerializeLayout(layout);

	CByteBuffer *buffer = layout->serializedLayoutBuffer;
	if (buffer == NULL || buffer->length < 8)
	{
		snprintf(failureMsg, sizeof(failureMsg), "serialized layout buffer is empty");
		delete layout;
		return false;
	}

	// Remember every float parameter's value so the migration can be checked
	// name by name -- sizes scaled, everything else identical.
	std::map<std::string, float> before;
	for (std::map<u64, CGuiView *>::iterator it = guiMain->layoutViews.begin();
		 it != guiMain->layoutViews.end(); it++)
	{
		CGuiView *view = it->second;
		for (std::list<CLayoutParameter *>::iterator pit = view->layoutParameters.begin();
			 pit != view->layoutParameters.end(); pit++)
		{
			CLayoutParameterFloat *f = dynamic_cast<CLayoutParameterFloat *>(*pit);
			if (f == NULL || f->value == NULL)
				continue;
			std::string key = std::string(view->name) + "/" + f->name;
			before[key] = *(f->value);
		}
	}

	if (!C64D_UiScaleTransformLayoutBuffer(buffer, 2.0f))
	{
		snprintf(failureMsg, sizeof(failureMsg), "C64D_UiScaleTransformLayoutBuffer refused a buffer this app just wrote");
		delete layout;
		return false;
	}

	// Read the migrated buffer back through the ordinary deserialize path.
	if (!guiMain->DeserializeLayout(layout))
	{
		snprintf(failureMsg, sizeof(failureMsg), "migrated layout buffer failed to deserialize");
		delete layout;
		return false;
	}

	bool ok = true;
	for (std::map<u64, CGuiView *>::iterator it = guiMain->layoutViews.begin();
		 it != guiMain->layoutViews.end() && ok; it++)
	{
		CGuiView *view = it->second;
		for (std::list<CLayoutParameter *>::iterator pit = view->layoutParameters.begin();
			 pit != view->layoutParameters.end(); pit++)
		{
			CLayoutParameterFloat *f = dynamic_cast<CLayoutParameterFloat *>(*pit);
			if (f == NULL || f->value == NULL)
				continue;

			std::string key = std::string(view->name) + "/" + f->name;
			std::map<std::string, float>::iterator bit = before.find(key);
			if (bit == before.end())
				continue;

			float expected = bit->second;
			if (C64D_UiScaleIsSizeLikeLayoutParameter(f->name))
				expected *= 2.0f;

			// A view may clamp what it is given (CViewC64StateCIA recomputes
			// from its own rect), so only fail when the value moved AWAY from
			// the expectation rather than towards a clamp.
			if (!NearlyEqual(*(f->value), expected, 0.01f)
				&& !NearlyEqual(*(f->value), bit->second, 0.01f))
			{
				snprintf(failureMsg, sizeof(failureMsg),
						 "%s: expected %.3f after a 2x migration (was %.3f), got %.3f",
						 key.c_str(), expected, bit->second, *(f->value));
				ok = false;
				break;
			}
		}
	}

	// Put the values back, whatever happened, so the rest of the suite runs
	// against an unchanged UI.
	C64D_UiScaleTransformLayoutBuffer(buffer, 0.5f);
	guiMain->DeserializeLayout(layout);

	delete layout;
	return ok;
}

static bool VerifyUnknownViewRecordIsCopiedVerbatim()
{
	// A workspace can name a view this build does not have -- an older or newer
	// release, a plugin that is not compiled in. Its parameter value widths are
	// unknowable, so the record cannot be walked; but because each record is
	// wrapped in its own sub-buffer it CAN be copied through byte for byte. The
	// rest of the workspace still migrates.
	CByteBuffer *buffer = new CByteBuffer();
	buffer->PutU32(2);                 // layout version
	const char *ini = "[Window][X]\nPos=1,2\n";
	buffer->PutU32((u32)strlen(ini) + 1);
	buffer->PutBytes((u8 *)ini, (u32)strlen(ini) + 1);
	buffer->PutU32(1);                 // one view
	buffer->PutString("__no_such_view__");
	CByteBuffer *viewBuffer = new CByteBuffer();
	viewBuffer->PutBool(true);
	viewBuffer->PutFloat(1.0f); viewBuffer->PutFloat(2.0f);
	viewBuffer->PutFloat(3.0f); viewBuffer->PutFloat(4.0f);
	viewBuffer->PutU32(1);
	viewBuffer->PutString("Font Size");
	viewBuffer->PutFloat(7.0f);
	buffer->PutByteBuffer(viewBuffer);
	delete viewBuffer;

	bool ok = C64D_UiScaleTransformLayoutBuffer(buffer, 2.0f);
	if (!ok)
	{
		snprintf(failureMsg, sizeof(failureMsg), "a workspace naming an unknown view should still migrate its ini");
		delete buffer;
		return false;
	}

	// Walk the result: the ini must be scaled, the opaque record must not.
	buffer->Rewind();
	buffer->GetU32();                                  // version
	u32 iniLength = buffer->GetU32();
	u8 *iniData = buffer->GetBytes(iniLength);
	bool iniScaled = (iniData != NULL && strstr((const char *)iniData, "Pos=2,4") != NULL);
	if (iniData != NULL)
		delete[] iniData;

	buffer->GetU32();                                  // numViews
	char *viewName = buffer->GetString();
	STRFREE(viewName);
	CByteBuffer *record = buffer->GetByteBuffer();
	record->Rewind();
	record->GetBool();
	for (int i = 0; i < 4; i++)
		record->GetFloat();
	record->GetU32();
	char *parameterName = record->GetString();
	float fontSize = record->GetFloat();
	STRFREE(parameterName);
	delete record;
	delete buffer;

	if (!iniScaled)
	{
		snprintf(failureMsg, sizeof(failureMsg), "the ini of a workspace with an unknown view was not scaled");
		return false;
	}
	if (!NearlyEqual(fontSize, 7.0f))
	{
		snprintf(failureMsg, sizeof(failureMsg),
				 "an unknown view's record must be copied verbatim, got Font Size %.3f instead of 7.0", fontSize);
		return false;
	}
	return true;
}

static bool VerifyPreVersion2LayoutIsRefused()
{
	// Version 1 has no view rect, so the four floats the walker reads are not
	// there; version 0 does not wrap records at all. Both must be refused
	// whole rather than mis-walked.
	CByteBuffer *buffer = new CByteBuffer();
	buffer->PutU32(1);
	buffer->PutU32(1);
	buffer->PutBytes((u8 *)"", 1);
	buffer->PutU32(0);

	int lengthBefore = buffer->length;
	bool refused = !C64D_UiScaleTransformLayoutBuffer(buffer, 2.0f);
	bool unchanged = (buffer->length == lengthBefore);
	delete buffer;

	if (!refused || !unchanged)
	{
		snprintf(failureMsg, sizeof(failureMsg), "a version-1 layout must be refused and left untouched");
		return false;
	}
	return true;
}

// ---------------------------------------------------------------------------
// #4 -- the style must not compound
// ---------------------------------------------------------------------------

static bool VerifyApplyingTheStyleTwiceDoesNotCompound()
{
	if (ImGui::GetCurrentContext() == NULL)
	{
		snprintf(failureMsg, sizeof(failureMsg), "no ImGui context");
		return false;
	}

	// The failure this guards is "the UI grows every time you open Settings":
	// ScaleAllSizes multiplies in place, so a second apply on the same style
	// would square the scale.
	//
	// Forced to 2.0 for the duration: at 1.0 the geometry pass is skipped
	// entirely, so the assertion below would pass without exercising anything.
	// A build machine at 100% is the normal case, which is exactly why the
	// test must not depend on the display.
	ImGuiStyle backup = ImGui::GetStyle();
	float restoreScale = MT_GetUiScale();

	MT_SetUiScale(2.0f);

	MT_UiScaleApplyToImGuiStyle();
	ImGuiStyle once = ImGui::GetStyle();

	MT_UiScaleApplyToImGuiStyle();
	ImGuiStyle twice = ImGui::GetStyle();

	// A scale of 2 must actually have changed something, or "unchanged twice"
	// proves nothing. The one legitimate exception is a user's own custom
	// style, whose geometry the app deliberately never scales.
	bool geometryIsOurs = (VID_GetDefaultImGuiStyle() != IMGUI_STYLE_CUSTOM);
	bool actuallyScaled = !geometryIsOurs
					   || !NearlyEqual(once.FramePadding.y, backup.FramePadding.y)
					   || !NearlyEqual(once.ScrollbarSize, backup.ScrollbarSize);

	bool ok = NearlyEqual(once.FramePadding.x, twice.FramePadding.x)
			&& NearlyEqual(once.FramePadding.y, twice.FramePadding.y)
			&& NearlyEqual(once.ScrollbarSize, twice.ScrollbarSize)
			&& NearlyEqual(once.WindowPadding.x, twice.WindowPadding.x)
			&& NearlyEqual(once.ItemSpacing.y, twice.ItemSpacing.y);

	if (!ok)
	{
		snprintf(failureMsg, sizeof(failureMsg),
				 "applying the UI scale twice compounded: FramePadding %.2f -> %.2f, ScrollbarSize %.2f -> %.2f",
				 once.FramePadding.y, twice.FramePadding.y, once.ScrollbarSize, twice.ScrollbarSize);
	}
	else if (!actuallyScaled)
	{
		snprintf(failureMsg, sizeof(failureMsg),
				 "a UI scale of 2.0 left the geometry untouched, so the idempotence check proved nothing");
		ok = false;
	}

	MT_SetUiScale(restoreScale);
	ImGui::GetStyle() = backup;
	return ok;
}

static bool VerifyScaleIsOnTheEngineLadder()
{
	float scale = MT_GetUiScale();
	if (!NearlyEqual(scale, MT_ThemeClampGuiScale(scale)))
	{
		snprintf(failureMsg, sizeof(failureMsg),
				 "UI scale %.4f is not a rung of MT_kGuiScaleSteps", scale);
		return false;
	}

#if defined(MACOS)
	// macOS gets points from SDL, so the app must never scale on top of what
	// the OS already did.
	if (!NearlyEqual(MT_DetectDisplayUiScale(), 1.0f))
	{
		snprintf(failureMsg, sizeof(failureMsg), "detected scale on macOS must be 1.0");
		return false;
	}
#endif

	// Headless runs must not inherit the build machine's monitor OR the
	// developer's chosen setting, or every pixel baseline in the suite moves
	// from one machine to the next.
	if (gHeadlessMode && !NearlyEqual(scale, 1.0f))
	{
		snprintf(failureMsg, sizeof(failureMsg),
				 "UI scale in headless mode must be 1.0, got %.2f", scale);
		return false;
	}
	if (gHeadlessMode && !NearlyEqual(MT_DetectDisplayUiScale(), 1.0f))
	{
		snprintf(failureMsg, sizeof(failureMsg), "detected scale in headless mode must be 1.0");
		return false;
	}

	return true;
}

// ---------------------------------------------------------------------------

void CTestUiScale::Run(ITestCallback *callback)
{
	this->callback = callback;
	isRunning = true;

	if (!VerifyIniTransformScalesGeometry()
		|| !VerifyIniTransformRoundTrips()
		|| !VerifyIniTransformLeavesNonPairsAlone())
	{
		TestCompleted(false, failureMsg);
		return;
	}
	StepCompleted(1, true, "ImGui .ini geometry scales and identifiers survive");

	if (!VerifySizeLikeParameterClassification())
	{
		TestCompleted(false, failureMsg);
		return;
	}
	StepCompleted(2, true, "Size-like layout parameters are classified correctly");

	if (!VerifyPreVersion2LayoutIsRefused()
		|| !VerifyUnknownViewRecordIsCopiedVerbatim()
		|| !VerifyLayoutBufferMigration())
	{
		TestCompleted(false, failureMsg);
		return;
	}
	StepCompleted(3, true, "Workspace buffers migrate and round-trip through the real views");

	if (!VerifyApplyingTheStyleTwiceDoesNotCompound()
		|| !VerifyScaleIsOnTheEngineLadder())
	{
		TestCompleted(false, failureMsg);
		return;
	}
	StepCompleted(4, true, "Style scaling is idempotent and the scale is on the engine ladder");

	TestCompleted(true, "HiDPI UI scaling behaves");
}

void CTestUiScale::Cancel()
{
	isRunning = false;
	TestCompleted(false, "Cancelled");
}
