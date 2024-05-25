#include "CViewC64Screen.h"
#include "CDebugInterface.h"
#include "CViewC64.h"
#include "CColorsTheme.h"
#include "C64SettingsStorage.h"

CViewC64Screen::CViewC64Screen(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY,
									 CDebugInterface *debugInterface)
: CViewEmulatorScreen(name, posX, posY, posZ, sizeX, sizeY, debugInterface)
{
	shiftDown = false;
	renderRasterOnForeground = viewC64->config->GetBool("CViewC64Screen::renderRasterOnForeground", true);
	InitRasterColorsFromScheme();
}

bool CViewC64Screen::IsSkipKey(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	// debugger keys, not used in c64
	if (keyCode == MTKEY_F9 || keyCode == MTKEY_F10 || keyCode == MTKEY_F11 || keyCode == MTKEY_F12 || keyCode == MTKEY_F13 || keyCode == MTKEY_F14 || keyCode == MTKEY_F15 || keyCode == MTKEY_F16)
		return true;
	
	return false;
}

u32 CViewC64Screen::ConvertKeyCode(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return SYS_KeyCodeConvertSpecial(keyCode, isShift, isAlt, isControl, isSuper);
}

void CViewC64Screen::PostDebugInterfaceKeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	if (keyCode == MTKEY_LSHIFT
		|| keyCode == MTKEY_RSHIFT)
	{
		shiftDown = true;
	}
}

extern "C"
{
	// workaround
	void c64d_keyboard_force_key_up_latch(signed long key);
}

void CViewC64Screen::PostDebugInterfaceKeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	// workaround for keeping pressed shift
	if (keyCode != MTKEY_LSHIFT
		&& keyCode != MTKEY_RSHIFT
		&& isShift)
	{
		if (guiMain->isLeftShiftPressed)
		{
			LOGI("workaround: send keydown MTKEY_LSHIFT");
			debugInterface->KeyboardDown(MTKEY_LSHIFT);
			shiftDown = true;
		}
		if (guiMain->isRightShiftPressed)
		{
			LOGI("workaround: send keydown MTKEY_RSHIFT");
			debugInterface->KeyboardDown(MTKEY_RSHIFT);
			shiftDown = true;
		}
	}
	else if (!isShift && shiftDown)
	{
		LOGI("workaround: send key UP L/R SHIFT");
		c64d_keyboard_force_key_up_latch(MTKEY_LSHIFT);
		c64d_keyboard_force_key_up_latch(MTKEY_RSHIFT);
		shiftDown = false;
	}
	
	if ((keyCode == MTKEY_LALT || keyCode == MTKEY_RALT) && !isShift)
	{
		LOGI("workaround 2: send key UP L/R SHIFT");
		c64d_keyboard_force_key_up_latch(MTKEY_LSHIFT);
		c64d_keyboard_force_key_up_latch(MTKEY_RSHIFT);
		shiftDown = false;
	}
}

// raster
void CViewC64Screen::InitRasterColorsFromScheme()
{
	// grid lines
	GetColorsFromScheme(c64SettingsScreenGridLinesColorScheme, 0.0f,
						 &gridLinesColorR, &gridLinesColorG, &gridLinesColorB);
	
	gridLinesColorA = c64SettingsScreenGridLinesAlpha;

	// raster long screen line
	GetColorsFromScheme(c64SettingsScreenRasterCrossLinesColorScheme, 0.0f,
						 &rasterLongScrenLineR, &rasterLongScrenLineG, &rasterLongScrenLineB);
	rasterLongScrenLineA = c64SettingsScreenRasterCrossLinesAlpha;
	
	//c64SettingsScreenRasterCrossAlpha = 0.85

	// exterior
	GetColorsFromScheme(c64SettingsScreenRasterCrossExteriorColorScheme, 0.1f,
						 &rasterCrossExteriorR, &rasterCrossExteriorG, &rasterCrossExteriorB);
	
	rasterCrossExteriorA = 0.8235f * c64SettingsScreenRasterCrossAlpha;		// 0.7
	
	// tip
	GetColorsFromScheme(c64SettingsScreenRasterCrossTipColorScheme, 0.1f,
						 &rasterCrossEndingTipR, &rasterCrossEndingTipG, &rasterCrossEndingTipB);
	
	rasterCrossEndingTipA = 0.1765f * c64SettingsScreenRasterCrossAlpha;	// 0.15
	
	// white interior cross
	GetColorsFromScheme(c64SettingsScreenRasterCrossInteriorColorScheme, 0.1f,
						 &rasterCrossInteriorR, &rasterCrossInteriorG, &rasterCrossInteriorB);

	rasterCrossInteriorA = c64SettingsScreenRasterCrossAlpha;	// 0.85
	
}

void CViewC64Screen::UpdateRasterCrossFactors()
{
	if (debugInterface->GetEmulatorType() == EMULATOR_TYPE_C64_VICE)
	{
		this->rasterScaleFactorX = sizeX / (float)320;
		this->rasterScaleFactorY = sizeY / (float)200;
		rasterCrossOffsetX = 0.0f; // -103.787 * rasterScaleFactorX;
		rasterCrossOffsetY = 0.0f; //-15.500 * rasterScaleFactorY;
	}
	
	rasterCrossWidth = 1.0f;
	rasterCrossWidth2 = rasterCrossWidth/2.0f;
	
	rasterCrossSizeX = 25.0f * rasterScaleFactorX;
	rasterCrossSizeY = 25.0f * rasterScaleFactorY;
	rasterCrossSizeX2 = rasterCrossSizeX/2.0f;
	rasterCrossSizeY2 = rasterCrossSizeY/2.0f;
	rasterCrossSizeX3 = rasterCrossSizeX/3.0f;
	rasterCrossSizeY3 = rasterCrossSizeY/3.0f;
	rasterCrossSizeX4 = rasterCrossSizeX/4.0f;
	rasterCrossSizeY4 = rasterCrossSizeY/4.0f;
	rasterCrossSizeX6 = rasterCrossSizeX/6.0f;
	rasterCrossSizeY6 = rasterCrossSizeY/6.0f;
	
	rasterCrossSizeX34 = rasterCrossSizeX2+rasterCrossSizeX4;
	rasterCrossSizeY34 = rasterCrossSizeY2+rasterCrossSizeY4;
}
CViewC64Screen::~CViewC64Screen()
{
	
}

