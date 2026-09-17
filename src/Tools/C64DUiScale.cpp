#include "C64DUiScale.h"

#include "DBG_Log.h"
#include "SYS_Main.h"
#include "SYS_FileSystem.h"
#include "SYS_DefaultConfig.h"
#include "CByteBuffer.h"
#include "CSlrString.h"
#include "CConfigStorageHjson.h"
#include "VID_Main.h"
#include "CGuiMain.h"
#include "CGuiView.h"
#include "CLayoutManager.h"
#include "CLayoutParameter.h"
#include "MT_Theme.h"

#include "imgui.h"
#include "imgui_internal.h"

#include <SDL3/SDL.h>

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <string>

// ---------------------------------------------------------------------------
// App-level state. The SCALE itself is the engine's (MT_UiScale.h); what this
// app owns is where the value came from, so the menu can tick "Auto" and so
// the migration knows whether it has run.
// ---------------------------------------------------------------------------

static bool gUiScaleIsAuto = true;
static bool gUiScaleInitialised = false;

#define C64D_UI_SCALE_CONFIG_AUTO		"uiScaleAuto"
#define C64D_UI_SCALE_CONFIG_VALUE		"uiScale"
#define C64D_UI_SCALE_CONFIG_GEOMETRY	"uiGeometryScale"

bool C64D_IsUiScaleAuto()
{
	return gUiScaleIsAuto;
}

// ---------------------------------------------------------------------------
// The ImGui .ini transformer
// ---------------------------------------------------------------------------

bool C64D_UiScaleIsSizeLikeLayoutParameter(const char *parameterName)
{
	if (parameterName == NULL)
		return false;

	// The float layout parameters that carry a pixel size. "Font Scale" is a
	// multiplier on a bitmap font's cell height, so it is a size in effect.
	// The VicEditor canvas zoom and pan are pixel quantities too.
	static const char *sizeLike[] =
	{
		"Font Size",
		"Font Scale",
		"Display zoom",
		"Display pos X",
		"Display pos Y",
	};

	for (int i = 0; i < (int)(sizeof(sizeLike) / sizeof(sizeLike[0])); i++)
	{
		if (!strcmp(parameterName, sizeLike[i]))
			return true;
	}

	return false;
}

static bool C64D_UiScaleIsIniTokenBoundary(const char *ini, size_t pos)
{
	if (pos == 0)
		return true;
	char c = ini[pos - 1];
	return (c == ' ' || c == '\t' || c == '\n' || c == '\r');
}

// Reads "<number>,<number>" at `p`, scales both, appends the result to `out`
// and returns the number of characters consumed. Returns 0 when the text does
// not look like a pair, in which case nothing was appended.
static size_t C64D_UiScaleAppendScaledPair(const char *p, float factor, std::string &out)
{
	char *end = NULL;
	double x = strtod(p, &end);
	if (end == p || *end != ',')
		return 0;

	const char *q = end + 1;
	char *end2 = NULL;
	double y = strtod(q, &end2);
	if (end2 == q)
		return 0;

	char buf[96];
	// The values ImGui writes are integers for Pos/Size and integers for
	// SizeRef too, so print them the way ImGui does rather than inventing
	// decimals it would then have to parse back.
	snprintf(buf, sizeof(buf), "%d,%d",
			 (int)floor(x * factor + 0.5), (int)floor(y * factor + 0.5));
	out += buf;

	return (size_t)(end2 - p);
}

char *C64D_UiScaleTransformImGuiIni(const char *ini, float factor)
{
	if (ini == NULL)
		return NULL;

	// The keys whose value is a scalable "x,y" pair. Order matters: the longer
	// "ViewportPos=" must be tested before "Pos=", because "Pos=" is a
	// substring of it -- and it is only a token boundary check that keeps
	// "ViewportId=" and "DockId=" out of this list by accident.
	static const char *scalableKeys[] =
	{
		"ViewportPos=",
		"Pos=",
		"Size=",
		"SizeRef=",
	};
	static const int scalableKeyCount = (int)(sizeof(scalableKeys) / sizeof(scalableKeys[0]));

	std::string out;
	size_t len = strlen(ini);
	out.reserve(len + len / 8 + 16);

	size_t i = 0;
	while (i < len)
	{
		bool matched = false;

		if (C64D_UiScaleIsIniTokenBoundary(ini, i))
		{
			for (int k = 0; k < scalableKeyCount; k++)
			{
				size_t keyLen = strlen(scalableKeys[k]);
				if (strncmp(ini + i, scalableKeys[k], keyLen) != 0)
					continue;

				std::string scaled;
				size_t consumed = C64D_UiScaleAppendScaledPair(ini + i + keyLen, factor, scaled);
				if (consumed == 0)
					break;   // not a pair after all -- copy verbatim

				out += scalableKeys[k];
				out += scaled;
				i += keyLen + consumed;
				matched = true;
				break;
			}
		}

		if (!matched)
		{
			out += ini[i];
			i++;
		}
	}

	char *result = new char[out.length() + 1];
	memcpy(result, out.c_str(), out.length() + 1);
	return result;
}

// ---------------------------------------------------------------------------
// The layout buffer transformer
// ---------------------------------------------------------------------------

// One view's record: visible flag, rect, then name/value parameter pairs. The
// wire format carries a parameter's NAME but not its TYPE, so the live view is
// what tells us how many bytes the value is and whether it is scalable. Every
// view exists by the time this runs (CViewC64's constructor builds them all,
// plugins included), so a lookup miss means a workspace from a build with a
// view this one does not have -- in which case the record cannot be walked and
// the whole migration for that layout is abandoned rather than half-applied.
static bool C64D_UiScaleTransformViewRecord(CByteBuffer *in, CByteBuffer *out,
										   const char *viewName, float factor)
{
	out->PutBool(in->GetBool());

	// The rect is re-derived from the ImGui window every frame
	// (CGuiView::UpdateImGuiWindowPositionAndSize), so scaling it changes
	// nothing that survives the first frame -- but a stored layout that a
	// future engine reads back directly should still be self-consistent.
	for (int i = 0; i < 4; i++)
		out->PutFloat(in->GetFloat() * factor);

	u32 numParameters = in->GetU32();
	out->PutU32(numParameters);

	CGuiView *view = NULL;
	if (viewName != NULL)
	{
		u64 hash = GetHashCode64(viewName);
		std::map<u64, CGuiView *>::iterator it = guiMain->layoutViews.find(hash);
		if (it != guiMain->layoutViews.end())
			view = it->second;
	}

	for (u32 i = 0; i < numParameters; i++)
	{
		char *parameterName = in->GetString();
		if (in->error || parameterName == NULL)
		{
			if (parameterName != NULL)
				STRFREE(parameterName);
			return false;
		}

		out->PutString(parameterName);

		CLayoutParameter *parameter = NULL;
		if (view != NULL)
		{
			u64 hash = GetHashCode64(parameterName);
			std::map<u64, CLayoutParameter *>::iterator it = view->layoutParametersByHash.find(hash);
			if (it != view->layoutParametersByHash.end())
				parameter = it->second;
		}

		if (parameter == NULL)
		{
			// Unknown name: its value's width is unknowable, so every byte
			// after it would be misread.
			LOGWarning("C64D_UiScale: layout parameter '%s' of view '%s' is unknown, layout not migrated",
					   parameterName, viewName ? viewName : "<null>");
			STRFREE(parameterName);
			return false;
		}

		bool scalable = C64D_UiScaleIsSizeLikeLayoutParameter(parameterName);
		STRFREE(parameterName);

		if (dynamic_cast<CLayoutParameterFloat *>(parameter) != NULL)
		{
			float value = in->GetFloat();
			out->PutFloat(scalable ? value * factor : value);
		}
		else if (dynamic_cast<CLayoutParameterDouble *>(parameter) != NULL)
		{
			// CLayoutParameterDouble serializes through PutFloat -- four bytes,
			// not eight.
			float value = in->GetFloat();
			out->PutFloat(scalable ? value * factor : value);
		}
		else if (dynamic_cast<CLayoutParameterBool *>(parameter) != NULL)
		{
			out->PutBool(in->GetBool());
		}
		else if (dynamic_cast<CLayoutParameterInt *>(parameter) != NULL)
		{
			out->putInt(in->getInt());
		}
		else if (dynamic_cast<CLayoutParameterCombo *>(parameter) != NULL)
		{
			out->PutU32(in->GetU32());
		}
		else
		{
			// CLayoutParameterPath and the base class both serialize nothing.
		}

		if (in->error)
			return false;
	}

	return !in->error;
}

bool C64D_UiScaleTransformLayoutBuffer(CByteBuffer *buffer, float factor)
{
	if (buffer == NULL || buffer->length < 1)
		return false;

	buffer->Rewind();
	buffer->error = false;

	u32 version = buffer->GetU32();
	if (version < 2)
	{
		// Version 0 does not wrap each view's record in its own sub-buffer, so
		// a parse failure cannot be contained; version 1 has no view rect, so
		// the four floats below are not there to read. Both predate any build
		// that could have produced a HiDPI problem, and a wrong walk is worse
		// than an unscaled workspace.
		LOGWarning("C64D_UiScale: layout version %d is not migrated", version);
		buffer->Rewind();
		return false;
	}

	CByteBuffer *out = new CByteBuffer();
	out->PutU32(version);

	u32 iniLength = buffer->GetU32();
	u8 *iniData = buffer->GetBytes(iniLength);
	bool ok = (iniData != NULL && !buffer->error);

	if (ok)
	{
		// The stored block is NUL-terminated (SerializeLayout writes len+1).
		std::string ini((const char *)iniData, iniLength);
		size_t nul = ini.find('\0');
		if (nul != std::string::npos)
			ini.resize(nul);

		char *scaledIni = C64D_UiScaleTransformImGuiIni(ini.c_str(), factor);
		u32 scaledLength = (u32)strlen(scaledIni) + 1;
		out->PutU32(scaledLength);
		out->PutBytes((u8 *)scaledIni, scaledLength);
		delete[] scaledIni;
	}

	if (iniData != NULL)
		delete[] iniData;

	if (ok)
	{
		u32 numViews = buffer->GetU32();
		out->PutU32(numViews);

		for (u32 i = 0; i < numViews && ok; i++)
		{
			char *viewName = buffer->GetString();
			if (buffer->error || viewName == NULL)
			{
				if (viewName != NULL)
					STRFREE(viewName);
				ok = false;
				break;
			}
			out->PutString(viewName);

			CByteBuffer *viewIn = buffer->GetByteBuffer();
			if (viewIn == NULL || buffer->error)
			{
				STRFREE(viewName);
				if (viewIn)
					delete viewIn;
				ok = false;
				break;
			}

			viewIn->Rewind();
			CByteBuffer *viewOut = new CByteBuffer();
			if (C64D_UiScaleTransformViewRecord(viewIn, viewOut, viewName, factor))
			{
				out->PutByteBuffer(viewOut);
			}
			else
			{
				// Each view's record is wrapped in its own sub-buffer, which is
				// what makes a failure containable: copy the bytes through
				// verbatim. That view keeps its old sizes -- one view unscaled
				// beats a whole workspace lost, and it is exactly what happens
				// for a record written by a build that had a view this one does
				// not (CGuiMain::DeserializeLayout skips those too).
				LOGWarning("C64D_UiScale: view '%s' record copied unscaled", viewName);
				out->PutByteBuffer(viewIn);
			}

			delete viewOut;
			delete viewIn;
			STRFREE(viewName);
		}
	}

	if (!ok)
	{
		LOGError("C64D_UiScale: could not migrate a workspace, leaving it unchanged");
		delete out;
		buffer->Rewind();
		return false;
	}

	buffer->Clear();
	buffer->PutBytes(out->data, out->length);
	buffer->Rewind();
	delete out;

	return true;
}

// ---------------------------------------------------------------------------
// Live UI
// ---------------------------------------------------------------------------

void C64D_UiScaleRescaleLiveViewParameters(float factor)
{
	if (guiMain == NULL)
		return;

	for (std::map<u64, CGuiView *>::iterator it = guiMain->layoutViews.begin();
		 it != guiMain->layoutViews.end(); it++)
	{
		CGuiView *view = it->second;
		if (view == NULL)
			continue;

		for (std::list<CLayoutParameter *>::iterator pit = view->layoutParameters.begin();
			 pit != view->layoutParameters.end(); pit++)
		{
			CLayoutParameter *parameter = *pit;
			if (!C64D_UiScaleIsSizeLikeLayoutParameter(parameter->name))
				continue;

			CLayoutParameterFloat *floatParameter = dynamic_cast<CLayoutParameterFloat *>(parameter);
			if (floatParameter == NULL || floatParameter->value == NULL)
				continue;

			*(floatParameter->value) *= factor;
			view->LayoutParameterChanged(parameter);
		}
	}
}

// Moves live windows, dock nodes AND the settings of windows that have not
// been created yet, in one step: ImGui's own .ini text is the only
// representation that covers all three.
static void C64D_UiScaleRescaleLiveImGuiWindows(float factor)
{
	if (ImGui::GetCurrentContext() == NULL)
		return;

	size_t iniLength = 0;
	const char *ini = ImGui::SaveIniSettingsToMemory(&iniLength);
	if (ini == NULL)
		return;

	char *scaled = C64D_UiScaleTransformImGuiIni(ini, factor);
	ImGui::LoadIniSettingsFromMemory(scaled);
	delete[] scaled;
}

// ---------------------------------------------------------------------------
// Persisted geometry
// ---------------------------------------------------------------------------

static void C64D_UiScaleBackupFile(const char *filePath)
{
	FILE *src = fopen(filePath, "rb");
	if (src == NULL)
		return;

	char *backupPath = new char[strlen(filePath) + 32];
	sprintf(backupPath, "%s.pre-hidpi-backup", filePath);

	// Write it ONCE and never again. The name promises the state before HiDPI
	// support, and migration is not a one-time event -- a laptop whose display
	// scale changes runs it again in the other direction. Overwriting on the
	// second run would replace the genuine original with an already-migrated
	// copy, which is the one thing the backup exists to prevent. (Observed:
	// a 1.0 -> 2.5 run followed by a 2.5 -> 1.0 run left a 2.5-scaled
	// "original" behind.)
	FILE *existing = fopen(backupPath, "rb");
	if (existing != NULL)
	{
		fclose(existing);
		fclose(src);
		delete[] backupPath;
		return;
	}

	FILE *dst = fopen(backupPath, "wb");
	if (dst != NULL)
	{
		u8 chunk[8192];
		size_t got;
		while ((got = fread(chunk, 1, sizeof(chunk), src)) > 0)
			fwrite(chunk, 1, got, dst);
		fclose(dst);
		LOGM("C64D_UiScale: backed up %s", backupPath);
	}

	fclose(src);
	delete[] backupPath;
}

// imgui.ini has not been read yet when this runs (ImGui loads it lazily inside
// the first NewFrame and MT_PostInit is before the first frame), so rewriting
// the file is enough -- no need to also touch live state.
static void C64D_UiScaleMigrateImGuiIniFile(float factor)
{
	ImGuiIO &io = ImGui::GetIO();
	if (io.IniFilename == NULL)
		return;

	FILE *fp = fopen(io.IniFilename, "rb");
	if (fp == NULL)
		return;   // fresh settings folder, nothing to upgrade

	fseek(fp, 0, SEEK_END);
	long size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	if (size <= 0)
	{
		fclose(fp);
		return;
	}

	char *content = new char[size + 1];
	size_t got = fread(content, 1, (size_t)size, fp);
	content[got] = 0;
	fclose(fp);

	C64D_UiScaleBackupFile(io.IniFilename);

	char *scaled = C64D_UiScaleTransformImGuiIni(content, factor);
	delete[] content;

	fp = fopen(io.IniFilename, "wb");
	if (fp != NULL)
	{
		fwrite(scaled, 1, strlen(scaled), fp);
		fclose(fp);
		LOGM("C64D_UiScale: imgui.ini scaled by %.3f", factor);
	}

	delete[] scaled;
}

static int C64D_UiScaleMigrateStoredLayouts(float factor)
{
	if (guiMain == NULL || guiMain->layoutManager == NULL)
		return 0;

	int migrated = 0;
	for (std::list<CLayoutData *>::iterator it = guiMain->layoutManager->layouts.begin();
		 it != guiMain->layoutManager->layouts.end(); it++)
	{
		CLayoutData *layout = *it;
		if (layout == NULL || layout->serializedLayoutBuffer == NULL)
			continue;

		if (C64D_UiScaleTransformLayoutBuffer(layout->serializedLayoutBuffer, factor))
		{
			migrated++;
		}
		else
		{
			LOGWarning("C64D_UiScale: workspace '%s' was not migrated",
					   layout->layoutName ? layout->layoutName : "<unnamed>");
		}
	}

	return migrated;
}

// ---------------------------------------------------------------------------
// Startup
// ---------------------------------------------------------------------------

void C64D_UiScaleInitEarly()
{
	if (gApplicationDefaultConfig == NULL)
	{
		gUiScaleInitialised = true;
		return;
	}

	bool isAuto = true;
	gApplicationDefaultConfig->GetBool(C64D_UI_SCALE_CONFIG_AUTO, &isAuto, true);

	float scale;
	if (gHeadlessMode)
	{
		// Headless runs are pinned to 1.0 whatever the settings say. The suites
		// assert on pixel geometry, and a developer who had picked 200% for
		// their own monitor would otherwise move every baseline in the run --
		// a failure that reproduces on one machine and nowhere else. Reading
		// the SETTING and not just the display is the part that matters here.
		scale = 1.0f;
	}
	else if (isAuto)
	{
		scale = MT_DetectDisplayUiScale();
	}
	else
	{
		float stored = 1.0f;
		gApplicationDefaultConfig->GetFloat(C64D_UI_SCALE_CONFIG_VALUE, &stored, 1.0f);
		scale = MT_ThemeClampGuiScale(stored);
	}

	gUiScaleIsAuto = isAuto;
	gUiScaleInitialised = true;

	// The engine owns the scale and applies it to the ImGui style itself,
	// from VID_FinishStyleChange. This is the app deciding WHAT the scale
	// should be; the engine decides what to do with it.
	MT_SetUiScale(scale);

	LOGM("C64D_UiScale: scale=%.2f (%s)", MT_GetUiScale(),
		 gUiScaleIsAuto ? "auto" : "manual");
}

void C64D_UiScaleMigratePersistedGeometry()
{
	if (gApplicationDefaultConfig == NULL)
		return;

	// Headless runs must not rewrite the developer's settings folder.
	if (gHeadlessMode)
		return;

	// Absent means "written before this app knew about HiDPI", and everything
	// such a build produced is at scale 1.0.
	bool hasGeometryScale = gApplicationDefaultConfig->E_x_i_s_t_s(C64D_UI_SCALE_CONFIG_GEOMETRY);

	float geometryScale = 1.0f;
	if (hasGeometryScale)
		gApplicationDefaultConfig->GetFloat(C64D_UI_SCALE_CONFIG_GEOMETRY, &geometryScale, 1.0f);

	if (!(geometryScale > 0.0f))
		geometryScale = 1.0f;

	if (fabsf(geometryScale - MT_GetUiScale()) < 0.001f)
	{
		if (!hasGeometryScale)
			{
				float appliedScale = MT_GetUiScale();
				gApplicationDefaultConfig->SetFloat(C64D_UI_SCALE_CONFIG_GEOMETRY, &appliedScale);
			}
		return;
	}

	float factor = MT_GetUiScale() / geometryScale;
	LOGM("C64D_UiScale: upgrading persisted layouts from scale %.2f to %.2f (factor %.3f)",
		 geometryScale, MT_GetUiScale(), factor);

	if (settingsPathToLayoutsFile == NULL && gCPathToSettings != NULL)
	{
		char *layoutsPath = new char[strlen(gCPathToSettings) + strlen(C64D_LAYOUTS_FILE_NAME) + 2];
		sprintf(layoutsPath, "%s%s", gCPathToSettings, C64D_LAYOUTS_FILE_NAME);
		C64D_UiScaleBackupFile(layoutsPath);
		delete[] layoutsPath;
	}

	int migrated = C64D_UiScaleMigrateStoredLayouts(factor);
	C64D_UiScaleMigrateImGuiIniFile(factor);

	float appliedScale = MT_GetUiScale();
	gApplicationDefaultConfig->SetFloat(C64D_UI_SCALE_CONFIG_GEOMETRY, &appliedScale);

	if (guiMain != NULL)
	{
		// Say what happened AND how to undo it. The one case this gets wrong is
		// a user who already re-arranged everything under an SDL3 build before
		// this feature existed -- their layout is already physical and gets
		// doubled. Settings > UI Scale > 100% rescales it straight back, and
		// the .pre-hidpi-backup copies are the belt to that brace.
		char message[320];
		snprintf(message, sizeof(message),
				 "%d workspace%s rescaled for this %d%% display.\n"
				 "Settings > UI Scale to change. Originals kept as *.pre-hidpi-backup.",
				 migrated, migrated == 1 ? "" : "s", (int)(MT_GetUiScale() * 100.0f + 0.5f));
		guiMain->ShowNotification("UI scale", message);
	}
}

// ---------------------------------------------------------------------------
// Changing it at runtime
// ---------------------------------------------------------------------------

// The heavy half of a scale change -- rescaling live views, ImGui's own window
// and dock state, and every stored workspace -- runs OUTSIDE the frame.
//
// It is triggered from a menu item, which is drawn between NewFrame() and
// Render() with a popup's window stack live on top of it. LoadIniSettingsFromMemory
// moves and re-docks windows; doing that from inside the very frame that is
// drawing the menu is asking ImGui to reorganise the tree it is walking. The
// engine's own layout switching takes the same precaution.
class CUiThreadTaskApplyUiScale : public CUiThreadTaskCallback
{
public:
	CUiThreadTaskApplyUiScale(float factor) { this->factor = factor; }
	float factor;

	virtual void RunUIThreadTask()
	{
		C64D_UiScaleRescaleLiveViewParameters(factor);
		C64D_UiScaleRescaleLiveImGuiWindows(factor);
		C64D_UiScaleMigrateStoredLayouts(factor);

		if (gApplicationDefaultConfig != NULL)
		{
			float scale = MT_GetUiScale();
			gApplicationDefaultConfig->SetFloat(C64D_UI_SCALE_CONFIG_GEOMETRY, &scale);
		}

		if (guiMain != NULL && guiMain->layoutManager != NULL)
			guiMain->layoutManager->StoreLayouts();

		delete this;
	}
};

void C64D_UiScaleSet(float newScale, bool isAuto)
{
	newScale = MT_ThemeClampGuiScale(newScale);

	float oldScale = MT_GetUiScale();
	gUiScaleIsAuto = isAuto;

	if (gApplicationDefaultConfig != NULL)
	{
		gApplicationDefaultConfig->SetBool(C64D_UI_SCALE_CONFIG_AUTO, &gUiScaleIsAuto);
		gApplicationDefaultConfig->SetFloat(C64D_UI_SCALE_CONFIG_VALUE, &newScale);
	}

	if (fabsf(newScale - oldScale) < 0.001f)
		return;

	MT_SetUiScale(newScale);
	LOGM("C64D_UiScale: scale changed to %.2f", MT_GetUiScale());

	float factor = newScale / oldScale;

	if (guiMain != NULL)
	{
		guiMain->AddUiThreadTask(new CUiThreadTaskApplyUiScale(factor));
	}
	else
	{
	}
}
