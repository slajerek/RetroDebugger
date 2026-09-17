#include "CViewGT2KeyboardSetup.h"
#include "C64DebuggerPluginGoatTracker.h"
#include "CViewC64.h"
#include "GT2RenderHelper.h"
#include "SYS_KeyCodes.h"
#include "CSlrString.h"
#include "imgui.h"
#include <cstdio>
#include <cstring>
#include <list>

extern "C" {
#include "goattrk2.h"
extern unsigned char notekeytbl1[];
extern unsigned char notekeytbl2[];
extern unsigned char dmckeytbl[];
extern unsigned char jankokeytbl1[];
extern unsigned char jankokeytbl2[];
extern unsigned char renoisekeytbl1[];
extern unsigned char renoisekeytbl2[];
}

static const char *noteNames[] = {
	"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B",
	"C+1", "C#+1", "D+1", "D#+1", "E+1"
};

CViewGT2KeyboardSetup::CViewGT2KeyboardSetup(const char *name, float posX, float posY, float posZ,
											   float sizeX, float sizeY)
: CGuiView(posX, posY, posZ, sizeX, sizeY)
{
	this->name = name;
	captureTarget = -1;
	isCapturing = false;
	isSaveDialog = false;
}

CViewGT2KeyboardSetup::~CViewGT2KeyboardSetup()
{
}

const char *CViewGT2KeyboardSetup::KeyName(unsigned char keyCode)
{
	static char buf[16];
	// Map SDL keycodes to readable names
	if (keyCode >= 'a' && keyCode <= 'z')
	{
		buf[0] = keyCode - 'a' + 'A';
		buf[1] = 0;
		return buf;
	}
	if (keyCode >= '0' && keyCode <= '9')
	{
		buf[0] = keyCode;
		buf[1] = 0;
		return buf;
	}
	switch (keyCode)
	{
		case SDLK_COMMA:     return ",";
		case SDLK_PERIOD:    return ".";
		case SDLK_SEMICOLON: return ";";
		case SDLK_SLASH:     return "/";
		case SDLK_BACKSLASH: return "\\";
		case SDLK_LEFTBRACKET:  return "[";
		case SDLK_RIGHTBRACKET: return "]";
		case SDLK_MINUS:     return "-";
		case SDLK_EQUALS:    return "=";
		case SDLK_GRAVE: return "`";
		case SDLK_APOSTROPHE:     return "'";
		default:
			sprintf(buf, "0x%02X", keyCode);
			return buf;
	}
}

void CViewGT2KeyboardSetup::RenderImGui()
{
	PreRenderImGui();
	float uiScale = GT2EffectiveUIScale();
	ImGui::SetWindowFontScale(uiScale);
	ImGuiStyle &style = ImGui::GetStyle();
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
		ImVec2(style.FramePadding.x * uiScale, style.FramePadding.y * uiScale));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
		ImVec2(style.ItemSpacing.x * uiScale, style.ItemSpacing.y * uiScale));

	ImGui::Text("Custom Keyboard Layout Editor");
	ImGui::Separator();

	if (isCapturing)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
		ImGui::Text(">>> Press a key to assign <<<");
		ImGui::PopStyleColor();
		ImGui::Separator();
	}

	// --- Lower Octave (Row 1) ---
	ImGui::Text("Lower Octave (Row 1)");
	if (ImGui::BeginTable("tbl1", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
	{
		ImGui::TableSetupColumn("Note", ImGuiTableColumnFlags_WidthFixed, 60.0f * uiScale);
		ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, 80.0f * uiScale);
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 80.0f * uiScale);
		ImGui::TableHeadersRow();

		for (int i = 0; i < 15; i++)
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::Text("%s", noteNames[i]);
			ImGui::TableNextColumn();

			bool isActive = isCapturing && captureTarget == i;
			if (isActive)
			{
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
				ImGui::Text("[waiting...]");
				ImGui::PopStyleColor();
			}
			else
			{
				ImGui::Text("%s", KeyName(customkeytbl1[i]));
			}

			ImGui::TableNextColumn();
			char label[32];
			sprintf(label, "Set##r1_%d", i);
			if (ImGui::SmallButton(label))
			{
				isCapturing = true;
				captureTarget = i;
			}
		}
		ImGui::EndTable();
	}

	ImGui::Spacing();

	// --- Upper Octave (Row 2) ---
	ImGui::Text("Upper Octave (Row 2)");
	if (ImGui::BeginTable("tbl2", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
	{
		ImGui::TableSetupColumn("Note", ImGuiTableColumnFlags_WidthFixed, 60.0f * uiScale);
		ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, 80.0f * uiScale);
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 80.0f * uiScale);
		ImGui::TableHeadersRow();

		for (int i = 0; i < 17; i++)
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::Text("%s", noteNames[i]);
			ImGui::TableNextColumn();

			bool isActive = isCapturing && captureTarget == (100 + i);
			if (isActive)
			{
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
				ImGui::Text("[waiting...]");
				ImGui::PopStyleColor();
			}
			else
			{
				ImGui::Text("%s", KeyName(customkeytbl2[i]));
			}

			ImGui::TableNextColumn();
			char label[32];
			sprintf(label, "Set##r2_%d", i);
			if (ImGui::SmallButton(label))
			{
				isCapturing = true;
				captureTarget = 100 + i;
			}
		}
		ImGui::EndTable();
	}

	ImGui::Spacing();
	ImGui::Separator();

	// --- Actions ---
	if (ImGui::Button("Copy from Tracker"))
	{
		memcpy(customkeytbl1, notekeytbl1, 15);
		memcpy(customkeytbl2, notekeytbl2, 17);
	}
	ImGui::SameLine();
	if (ImGui::Button("Copy from DMC"))
	{
		memcpy(customkeytbl1, dmckeytbl, 15);
		// DMC has 16 entries, fill remaining with 0
		if (15 > 16) customkeytbl1[15] = 0;
		memset(customkeytbl2, 0, 17);
	}
	ImGui::SameLine();
	if (ImGui::Button("Copy from Janko"))
	{
		memcpy(customkeytbl1, jankokeytbl1, 15);
		memcpy(customkeytbl2, jankokeytbl2, 17);
	}
	ImGui::SameLine();
	if (ImGui::Button("Copy from Renoise"))
	{
		memcpy(customkeytbl1, renoisekeytbl1, 15);
		memcpy(customkeytbl2, renoisekeytbl2, 17);
	}

	ImGui::Spacing();

	if (ImGui::Button("Save Layout..."))
	{
		OpenSaveDialog();
	}
	ImGui::SameLine();
	if (ImGui::Button("Load Layout..."))
	{
		OpenLoadDialog();
	}
	ImGui::SameLine();
	if (ImGui::Button("Use This Layout"))
	{
		keypreset = KEY_CUSTOM;
		PLUGIN_GoatTrackerSaveSettings();
	}

	ImGui::PopStyleVar(2);
	PostRenderImGui();
}

bool CViewGT2KeyboardSetup::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	if (isCapturing)
	{
		unsigned char key = (unsigned char)(keyCode & 0xFF);

		if (keyCode == SDLK_ESCAPE)
		{
			// Cancel capture
			isCapturing = false;
			captureTarget = -1;
			return true;
		}

		if (captureTarget >= 0 && captureTarget < 15)
		{
			customkeytbl1[captureTarget] = key;
		}
		else if (captureTarget >= 100 && captureTarget < 117)
		{
			customkeytbl2[captureTarget - 100] = key;
		}

		isCapturing = false;
		captureTarget = -1;
		PLUGIN_GoatTrackerSaveSettings();
		return true;
	}

	return false;
}

// --- File I/O ---

void CViewGT2KeyboardSetup::SaveLayout(const char *path)
{
	FILE *f = fopen(path, "wb");
	if (!f) return;

	char ident[] = {'G', 'T', 'K', '1'};
	fwrite(ident, 4, 1, f);
	fwrite(customkeytbl1, 15, 1, f);
	fwrite(customkeytbl2, 17, 1, f);
	fclose(f);
}

void CViewGT2KeyboardSetup::LoadLayout(const char *path)
{
	FILE *f = fopen(path, "rb");
	if (!f) return;

	char ident[4];
	fread(ident, 4, 1, f);
	if (memcmp(ident, "GTK1", 4) != 0)
	{
		fclose(f);
		return;
	}

	fread(customkeytbl1, 15, 1, f);
	fread(customkeytbl2, 17, 1, f);
	fclose(f);

	keypreset = KEY_CUSTOM;
	PLUGIN_GoatTrackerSaveSettings();
}

void CViewGT2KeyboardSetup::OpenSaveDialog()
{
	isSaveDialog = true;
	std::list<CSlrString *> extensions;
	extensions.push_back(new CSlrString("gt2keys"));

	CSlrString *defaultFolder = new CSlrString("/");
	CSlrString *defaultFilename = new CSlrString("custom");

	SYS_DialogSaveFile(this, &extensions, defaultFilename, defaultFolder, NULL);

	delete defaultFolder;
	delete defaultFilename;
	for (auto *ext : extensions) delete ext;
}

void CViewGT2KeyboardSetup::OpenLoadDialog()
{
	isSaveDialog = false;
	std::list<CSlrString *> extensions;
	extensions.push_back(new CSlrString("gt2keys"));

	CSlrString *defaultFolder = new CSlrString("/");
	SYS_DialogOpenFile(this, &extensions, defaultFolder, NULL);

	delete defaultFolder;
	for (auto *ext : extensions) delete ext;
}

void CViewGT2KeyboardSetup::SystemDialogFileOpenSelected(CSlrString *path)
{
	char *cPath = path->GetStdASCII();
	LoadLayout(cPath);
	delete[] cPath;
}

void CViewGT2KeyboardSetup::SystemDialogFileOpenCancelled()
{
}

void CViewGT2KeyboardSetup::SystemDialogFileSaveSelected(CSlrString *path)
{
	char *cPath = path->GetStdASCII();
	SaveLayout(cPath);
	delete[] cPath;
}

void CViewGT2KeyboardSetup::SystemDialogFileSaveCancelled()
{
}
