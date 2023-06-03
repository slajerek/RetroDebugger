#include "RetroDebuggerEmbeddedData.h"
#include "RES_ResourceManager.h"
#include "GUI_Main.h"
#include "imgui_freetype.h"

#include "icon_new_gfx.h"
#include "icon_clear_gfx.h"
#include "icon_open_gfx.h"
#include "icon_export_gfx.h"
#include "icon_import_gfx.h"
#include "icon_tool_rectangle_gfx.h"
#include "icon_tool_pen_gfx.h"
#include "icon_tool_line_gfx.h"
#include "icon_tool_fill_gfx.h"
#include "icon_tool_dither_gfx.h"
#include "icon_tool_circle_gfx.h"
#include "icon_tool_brush_square_gfx.h"
#include "icon_tool_brush_circle_gfx.h"
#include "icon_tool_on_top_on_gfx.h"
#include "icon_tool_on_top_off_gfx.h"
#include "icon_settings_gfx.h"
#include "icon_save_gfx.h"
#include "icon_close_gfx.h"
#include "icon_small_export_gfx.h"
#include "icon_small_import_gfx.h"
//#include "icon_toolbox_import_gfx.h"
//#include "icon_toolbox_export_gfx.h"
#include "icon_raw_export_gfx.h"
#include "icon_raw_import_gfx.h"
#include "gamecontrollerdb_txt_zlib.h"
#include "FontProFontIIx.h"
#include "FontSweet16mono.h"
#include "FontCousineRegular.h"
#include "FontDroidSans.h"
#include "FontKarlaRegular.h"
#include "FontRobotoMedium.h"

void RetroDebuggerEmbeddedAddData()
{
	RES_AddEmbeddedDataToDeploy("/gfx/icon_new", DEPLOY_FILE_TYPE_GFX, icon_new_gfx, icon_new_gfx_length);
	RES_AddEmbeddedDataToDeploy("/gfx/icon_clear", DEPLOY_FILE_TYPE_GFX, icon_clear_gfx, icon_clear_gfx_length);
	RES_AddEmbeddedDataToDeploy("/gfx/icon_open", DEPLOY_FILE_TYPE_GFX, icon_open_gfx, icon_open_gfx_length);
	RES_AddEmbeddedDataToDeploy("/gfx/icon_save", DEPLOY_FILE_TYPE_GFX, icon_save_gfx, icon_save_gfx_length);
	RES_AddEmbeddedDataToDeploy("/gfx/icon_export", DEPLOY_FILE_TYPE_GFX, icon_export_gfx, icon_export_gfx_length);
	RES_AddEmbeddedDataToDeploy("/gfx/icon_import", DEPLOY_FILE_TYPE_GFX, icon_import_gfx, icon_import_gfx_length);
	RES_AddEmbeddedDataToDeploy("/gfx/icon_settings", DEPLOY_FILE_TYPE_GFX, icon_settings_gfx, icon_settings_gfx_length);
	RES_AddEmbeddedDataToDeploy("/gfx/icon_tool_rectangle", DEPLOY_FILE_TYPE_GFX, icon_tool_rectangle_gfx, icon_tool_rectangle_gfx_length);
	RES_AddEmbeddedDataToDeploy("/gfx/icon_tool_pen", DEPLOY_FILE_TYPE_GFX, icon_tool_pen_gfx, icon_tool_pen_gfx_length);
	RES_AddEmbeddedDataToDeploy("/gfx/icon_tool_line", DEPLOY_FILE_TYPE_GFX, icon_tool_line_gfx, icon_tool_line_gfx_length);
	RES_AddEmbeddedDataToDeploy("/gfx/icon_tool_fill", DEPLOY_FILE_TYPE_GFX, icon_tool_fill_gfx, icon_tool_fill_gfx_length);
	RES_AddEmbeddedDataToDeploy("/gfx/icon_tool_dither", DEPLOY_FILE_TYPE_GFX, icon_tool_dither_gfx, icon_tool_dither_gfx_length);
	RES_AddEmbeddedDataToDeploy("/gfx/icon_tool_circle", DEPLOY_FILE_TYPE_GFX, icon_tool_circle_gfx, icon_tool_circle_gfx_length);
	RES_AddEmbeddedDataToDeploy("/gfx/icon_tool_brush_square", DEPLOY_FILE_TYPE_GFX, icon_tool_brush_square_gfx, icon_tool_brush_square_gfx_length);
	RES_AddEmbeddedDataToDeploy("/gfx/icon_tool_brush_circle", DEPLOY_FILE_TYPE_GFX, icon_tool_brush_circle_gfx, icon_tool_brush_circle_gfx_length);
	RES_AddEmbeddedDataToDeploy("/gfx/icon_tool_on_top_on", DEPLOY_FILE_TYPE_GFX, icon_tool_on_top_on_gfx, icon_tool_on_top_on_gfx_length);
	RES_AddEmbeddedDataToDeploy("/gfx/icon_tool_on_top_off", DEPLOY_FILE_TYPE_GFX, icon_tool_on_top_off_gfx, icon_tool_on_top_off_gfx_length);

	RES_AddEmbeddedDataToDeploy("/gfx/icon_close", DEPLOY_FILE_TYPE_GFX, icon_close_gfx, icon_close_gfx_length);
	
	//	RES_AddEmbeddedDataToDeploy("/gfx/icon_toolbox_export", DEPLOY_FILE_TYPE_GFX, icon_toolbox_export_gfx, icon_toolbox_export_gfx_length);
	//	RES_AddEmbeddedDataToDeploy("/gfx/icon_toolbox_import", DEPLOY_FILE_TYPE_GFX, icon_toolbox_import_gfx, icon_toolbox_import_gfx_length);
	
	RES_AddEmbeddedDataToDeploy("/gfx/icon_raw_export", DEPLOY_FILE_TYPE_GFX, icon_raw_export_gfx, icon_raw_export_gfx_length);
	RES_AddEmbeddedDataToDeploy("/gfx/icon_raw_import", DEPLOY_FILE_TYPE_GFX, icon_raw_import_gfx, icon_raw_import_gfx_length);
	
	RES_AddEmbeddedDataToDeploy("/gfx/icon_small_export", DEPLOY_FILE_TYPE_GFX, icon_small_export_gfx, icon_small_export_gfx_length);
	RES_AddEmbeddedDataToDeploy("/gfx/icon_small_import", DEPLOY_FILE_TYPE_GFX, icon_small_import_gfx, icon_small_import_gfx_length);

	// controllers data
	RES_AddEmbeddedDataToDeploy("/gamecontrollerdb", DEPLOY_FILE_TYPE_DATA, gamecontrollerdb_txt_zlib, gamecontrollerdb_txt_zlib_length);
	
}

//
ImFont* AddEmbeddedImGuiFont(float fontSize, int oversample, const unsigned int *data, const unsigned int size, const char *name)
{
	ImFontConfig font_cfg = ImFontConfig();
	
	font_cfg.OversampleH = font_cfg.OversampleV = oversample;
	font_cfg.PixelSnapH = true;
	font_cfg.SizePixels = fontSize * 1.0f;
	if (font_cfg.Name[0] == '\0')
	{
		ImFormatString(font_cfg.Name, IM_ARRAYSIZE(font_cfg.Name), name, (int)font_cfg.SizePixels);
	}
	
	font_cfg.FontBuilderFlags = ImGuiFreeTypeBuilderFlags_LightHinting; //ImGuiFreeTypeBuilderFlags_NoHinting;
//	font_cfg.EllipsisChar = (ImWchar)0x0085;
	
	ImGuiIO& io = ImGui::GetIO();
//	const ImWchar* glyph_ranges = io.Fonts->GetGlyphRangesDefault();
	static const ImWchar glyph_ranges[] =
	{
		0x0020, 0x017F, // Basic Latin + Latin supplement + Latin extended A
		0,
	};

	ImFont* font = io.Fonts->AddFontFromMemoryCompressedTTF(data, size, font_cfg.SizePixels, &font_cfg, glyph_ranges);
//	font->DisplayOffset.y = 1.0f;
	return font;
}

ImFont* AddProFontIIx(float fontSize, int oversample)
{
	return AddEmbeddedImGuiFont(fontSize, oversample, ProFontIIxFont_compressed_data, ProFontIIxFont_compressed_size, "ProFontIIx.ttf, %dpx");
}

ImFont *AddSweet16MonoFont(float fontSize, int oversample)
{
	static const ImWchar Sweet16mono_ranges[] =
	{
		0x0020, 0x017F, // Basic Latin + Latin supplement + Latin extended A
		0,
	};

	ImFontConfig config;
	config.OversampleH = oversample;
	config.OversampleV = oversample;
	config.PixelSnapH = true;
	config.SizePixels = fontSize;
//	config.FontBuilderFlags = ImGuiFreeTypeBuilderFlags_NoHinting;

	// copy font name manually to avoid warnings
	const char *name = "Sweet16mono.ttf, 16px";
	char *dst = config.Name;

	while (*name)
		*dst++ = *name++;
	*dst = '\0';

	return ImGui::GetIO().Fonts->AddFontFromMemoryCompressedBase85TTF(Sweet16mono_compressed_data_base85, config.SizePixels, &config, Sweet16mono_ranges);
}

ImFont* AddCousineRegularFont(float fontSize, int oversample)
{
	return AddEmbeddedImGuiFont(fontSize, oversample, CousineRegularFont_compressed_data, CousineRegularFont_compressed_size, "Cousine Regular, %dpx");
}

ImFont* AddDroidSansFont(float fontSize, int oversample)
{
	return AddEmbeddedImGuiFont(fontSize, oversample, DroidSans_compressed_data, DroidSans_compressed_size, "Droid Sans, %dpx");
}

ImFont* AddKarlaRegularFont(float fontSize, int oversample)
{
	return AddEmbeddedImGuiFont(fontSize, oversample, KarlaRegularFont_compressed_data, KarlaRegularFont_compressed_size, "Karla Regular, %dpx");
}

ImFont* AddRobotoMediumFont(float fontSize, int oversample)
{
	return AddEmbeddedImGuiFont(fontSize, oversample, RobotoMediumFont_compressed_data, RobotoMediumFont_compressed_size, "Roboto Medium, %dpx");
}
