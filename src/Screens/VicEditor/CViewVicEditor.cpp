#include "VID_Main.h"
#include "CViewC64.h"
#include "CViewVicEditor.h"
#include "CViewC64VicDisplay.h"
#include "CViewDataDump.h"
#include "CSlrFileZlib.h"
#include "RES_ResourceManager.h"
#include "IMG_Filters.h"

#include "VID_Main.h"
#include "CGuiMain.h"
#include "CSlrString.h"
#include "C64Tools.h"
#include "SYS_KeyCodes.h"
#include "CSlrKeyboardShortcuts.h"
#include "CSlrFileFromOS.h"
#include "C64SettingsStorage.h"
#include "CViewC64StateVIC.h"
#include "C64CharMulti.h"
#include "C64CharHires.h"

#include "C64KeyboardShortcuts.h"
#include "CViewSnapshots.h"
#include "CDebugInterfaceC64.h"
#include "MTH_Random.h"
#include "C64Tools.h"

#include "CViewC64.h"
#include "CViewC64Screen.h"
#include "CViewC64VicControl.h"
#include "CViewMemoryMap.h"
#include "CViewC64Sprite.h"
#include "CViewVicEditorLayers.h"
#include "CViewVicEditorCreateNewPicture.h"
#include "CGuiViewToolBox.h"
#include "CViewTimeline.h"

#include "CGuiMain.h"

#include "C64VicDisplayCanvas.h"
#include "CVicEditorLayerC64Screen.h"
#include "CVicEditorLayerC64Canvas.h"
#include "CVicEditorLayerC64Sprites.h"
#include "CVicEditorLayerVirtualSprites.h"
#include "CVicEditorLayerUnrestrictedBitmap.h"
#include "CVicEditorLayerImage.h"

#include "C64DisplayListCodeGenerator.h"

#include "zlib.h"

extern "C" {
	extern void c64_glue_set_vbank(int vbank, int ddr_flag);
}

#define VIC_EDITOR_FILE_MAGIC	0x56434500
#define VIC_EDITOR_FILE_VERSION	0x00000003

#define NUMBER_OF_UNDO_LEVELS	100
#define ZOOMING_SPEED_FACTOR_QUICK	15

CViewVicEditor::CViewVicEditor(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY)
: CGuiViewWindowsManager(name, posX, posY, posZ, sizeX, sizeY)
{
	font = viewC64->fontCBMShifted;
	fontScale = 1.5;
	fontHeight = font->GetCharHeight('@', fontScale) + 2;
	
	strHeader = new CSlrString("VIC Editor");
	
	this->consumeTapBackground = false;
	
	/// colors
	tr = 0.64; //163/255;
	tg = 0.59; //151/255;
	tb = 1.0; //255/255;

	//
	isMovingPreviewFrame = false;
	isPaintingOnPreviewFrame = false;
	
	isKeyboardMovingDisplay = false;
	isPainting = false;
	prevRx = -1000;
	prevRy = -1000;

	brushSize = 1;
	this->currentBrush = CreateBrushRectangle(brushSize);

	//
	importFileExtensions.push_back(new CSlrString("vce"));
	importFileExtensions.push_back(new CSlrString("png"));
	importFileExtensions.push_back(new CSlrString("aas"));
	importFileExtensions.push_back(new CSlrString("art"));
	importFileExtensions.push_back(new CSlrString("dd"));
	importFileExtensions.push_back(new CSlrString("ddl"));
	importFileExtensions.push_back(new CSlrString("kla"));
	importFileExtensions.push_back(new CSlrString("d64"));
	importFileExtensions.push_back(new CSlrString("g64"));
	importFileExtensions.push_back(new CSlrString("prg"));
	importFileExtensions.push_back(new CSlrString("crt"));
	importFileExtensions.push_back(new CSlrString("reu"));
	importFileExtensions.push_back(new CSlrString("snap"));
	importFileExtensions.push_back(new CSlrString("vsf"));
	importFileExtensions.push_back(new CSlrString("sid"));
	
	exportVCEFileExtensions.push_back(new CSlrString("vce"));
	exportHyperBitmapFileExtensions.push_back(new CSlrString("bin"));
	exportMultiBitmapFileExtensions.push_back(new CSlrString("kla"));
	exportHiresBitmapFileExtensions.push_back(new CSlrString("art"));
	exportHiresTextFileExtensions.push_back(new CSlrString("rawtext"));
	exportPNGFileExtensions.push_back(new CSlrString("png"));

	//
	
	viewVicDisplayMain = new CViewC64VicDisplay("viewVicEditorDisplayMain", 0, 0, posZ, SCREEN_WIDTH, SCREEN_HEIGHT, viewC64->debugInterfaceC64,
												NULL, GUI_FRAME_NO_FRAME, NULL);
	
	// set mode
	
//	u8 borderType = VIC_DISPLAY_SHOW_BORDER_NONE;
	u8 borderType = VIC_DISPLAY_SHOW_BORDER_VISIBLE_AREA;
	
	LOGTODO("FIXME!");
	
//	viewVicDisplayMain->btnModeMulti->SetOn(true);
//	viewVicDisplayMain->btnModeBitmap->SetOn(true);
//	
	
	viewVicDisplayMain->SetShowDisplayBorderType(borderType);

	//	viewVicDisplayMain->SetDisplayPosition(0,0, 1.7f);
	viewVicDisplayMain->SetDisplayPosition(0,0, 11.7f, true);

	
	viewVicDisplayMain->InitGridLinesColorFromSettings();
	
	
	//viewVicDisplayMain->showGridLines = true;
	viewVicDisplayMain->showRasterCursor = false;
	
	viewVicDisplayMain->consumeTapBackground = false;
	
	viewVicDisplayMain->applyScrollRegister = true;

	
	//
	viewTopBar = new CGuiViewToolBox(0, 0, posZ, 17.5, 3.1, 12.8, 12.8, 24.6, 16, -1, NULL, this);
	this->AddGuiElement(viewTopBar);
	

	//
	viewVicDisplaySmall = new CViewVicEditorDisplayPreview("viewVicEditorDisplayPreview", 100, 100, posZ, SCREEN_WIDTH, SCREEN_HEIGHT, viewC64->debugInterfaceC64,
														   new CSlrString("Preview"), this);
	viewVicDisplaySmall->renderDisplayFrame = true;
	
	LOGD("viewVicDisplaySmall->viewFrame=%x", viewVicDisplaySmall->viewFrame);
	
	
	viewVicDisplaySmall->SetShowDisplayBorderType(borderType);
	
	viewVicDisplaySmall->SetDisplayPosition(340, 20, 0.7f, true);

	viewVicDisplaySmall->gridLinesColorA = 0.0f;
	viewVicDisplaySmall->gridLinesColorA2 = 0.0f;

	viewVicDisplaySmall->showRasterCursor = false;
	
	viewVicDisplaySmall->showSpritesFrames = false;
	
	viewVicDisplaySmall->applyScrollRegister = false;
	
	// will not render in main CGuiView::Render loop
//	viewVicDisplaySmall->manualRender = true;
	
	this->AddWindow(viewVicDisplaySmall);

	//
	viewVicDisplayMain->SetVicControlView(viewC64->viewC64VicControl);
	viewVicDisplaySmall->SetVicControlView(viewC64->viewC64VicControl);

	//	// set mode
	//	viewVicDisplaySmall->btnModeMulti->SetOn(true);
	//
	//	viewVicDisplaySmall->btnModeBitmap->SetOn(true);
	

	//
	// setup layers, from below:
	layerC64Screen = new CVicEditorLayerC64Screen(this);
	layers.push_back(layerC64Screen);
	
	layerReferenceImage = new CVicEditorLayerImage(this, "Reference");
	layers.push_back(layerReferenceImage);
	
	layerC64Canvas = new CVicEditorLayerC64Canvas(this);
	layers.push_back(layerC64Canvas);
	
	layerC64Canvas->isVisible = false;
	
	layerC64Sprites = new CVicEditorLayerC64Sprites(this);
	layers.push_back(layerC64Sprites);
	
	layerVirtualSprites = new CVicEditorLayerVirtualSprites(this);
	layers.push_back(layerVirtualSprites);
	
	layerUnrestrictedBitmap = new CVicEditorLayerUnrestrictedBitmap(this, "Unrestricted");
	layers.push_back(layerUnrestrictedBitmap);

	this->selectedLayer = NULL;
	
	// this is not selectable layer for showing tool preview
	layerToolPreview = new CVicEditorLayerUnrestrictedBitmap(this, "Tool preview");
	toolIsActive = false;
	
	//
	viewLayers = new CViewVicEditorLayers("Layers##CViewVicEditor", 470, 180, posZ, 80, 57, this);
	this->AddWindow(viewLayers);
	
	//
	float sp = 0.85f;
	viewPalette = new CViewC64Palette("Palette##CViewVicEditor", 420, 310, posZ, 150*sp, 30*sp, this);
	this->AddWindow(viewPalette);
	
	//
	viewCharset = new CViewC64Charset("Charset##CViewVicEditor", 50, 50, posZ, 200, 50, this);
	this->AddWindow(viewCharset);

	viewSprite = new CViewC64Sprite("Sprite##CViewVicEditor", 320, 180, posZ, 100, 64, this);
	this->AddWindow(viewSprite);

	//
	viewCreateNewPicture = new CViewVicEditorCreateNewPicture("New Picture##CViewVicEditor", 0,0, posZ, this); //56, this);
	viewCreateNewPicture->PositionCenterOnParentView();
	viewCreateNewPicture->SetVisible(false);
	this->AddGuiElement(viewCreateNewPicture);
	
	//
	viewToolBox = new CGuiViewToolBox(30, 80, posZ, 0, 0, 12.8, 12.8, 13, 15.5, 1, new CSlrString(""), this);

	// tool box is not ready, make it invisible in production:
	viewToolBox->visible = false;
	this->AddWindow(viewToolBox);
	
	//
	// loop of views for TAB & shift+TAB
	traversalOfViews.push_back(viewVicDisplaySmall);
	traversalOfViews.push_back(viewLayers);
	traversalOfViews.push_back(viewCharset);
	traversalOfViews.push_back(viewSprite);
	traversalOfViews.push_back(viewPalette);

	prevVisibleLayers = true;
	prevVisibleCharset = true;
	prevVisiblePalette = true;
	prevVisibleSprite = true;
	prevVisiblePreview = true;
	prevVisibleTopBar = true;
	prevVisibleToolBox = true;

	inPresentationScreen = false;
	
	backupRenderDataWithColors = false;
	
	resetDisplayScaleIndex = 2;
	
	exportFileDialogMode = VICEDITOR_EXPORT_UNKNOWN;
	
	// create undo pool
	for (int i = 0; i < NUMBER_OF_UNDO_LEVELS; i++)
	{
		CByteBuffer *byteBuffer = new CByteBuffer();
		poolList.push_back(byteBuffer);
	}
	
	// register for OS window size changes
	guiMain->AddGlobalOSWindowChangedCallback(this);
	

	// icons
	// read gfx
	//	imgConsoleFonts = RES_GetImage("/Engine/console-plain");
	//	imgConsoleFonts->ResourceSetPriority(RESOURCE_PRIORITY_STATIC);

	//	imgNew = RES_LoadImageFromFileOS("/Users/mars/develop/MTEngine/_RUNTIME_/Documents/gfx/ico_new.gfx", false);
	
	imgNew = RES_GetImage("/gfx/icon_new", false);
	viewTopBar->AddIcon(imgNew);
	imgOpen = RES_GetImage("/gfx/icon_open", false);
	viewTopBar->AddIcon(imgOpen);
	imgSave = RES_GetImage("/gfx/icon_save", false);
	viewTopBar->AddIcon(imgSave);
	imgImport = RES_GetImage("/gfx/icon_import", false);
	viewTopBar->AddIcon(imgImport);
	imgExport = RES_GetImage("/gfx/icon_export", false);
	viewTopBar->AddIcon(imgExport);
	imgSettings = RES_GetImage("/gfx/icon_settings", false);
	viewTopBar->AddIcon(imgSettings);
	imgClear = RES_GetImage("/gfx/icon_clear", false);
	viewTopBar->AddIcon(imgClear);

	imgToolAlwaysOnTopOn = RES_GetImage("/gfx/icon_tool_on_top_on", false);
	imgToolAlwaysOnTopOff = RES_GetImage("/gfx/icon_tool_on_top_off", false);
	btnToolAlwaysOnTop = viewTopBar->AddIcon(c64SettingsWindowAlwaysOnTop ? imgToolAlwaysOnTopOn : imgToolAlwaysOnTopOff,
											 563, viewTopBar->nextIconY);

	viewTopBar->SetPosition(0, 0, posZ, SCREEN_WIDTH, viewTopBar->sizeY);
	
	// tool box
	imgToolPen = RES_GetImage("/gfx/icon_tool_pen", false);
	viewToolBox->AddIcon(imgToolPen);
	
	imgToolBrushSquare = RES_GetImage("/gfx/icon_tool_brush_square", false);
	viewToolBox->AddIcon(imgToolBrushSquare);
	
	imgToolBrushCircle = RES_GetImage("/gfx/icon_tool_brush_circle", false);
	viewToolBox->AddIcon(imgToolBrushCircle);

	imgToolFill = RES_GetImage("/gfx/icon_tool_fill", false);
	viewToolBox->AddIcon(imgToolFill);
	
	imgToolRectangle = RES_GetImage("/gfx/icon_tool_rectangle", false);
	viewToolBox->AddIcon(imgToolRectangle);
	
	imgToolCircle = RES_GetImage("/gfx/icon_tool_circle", false);
	viewToolBox->AddIcon(imgToolCircle);

	imgToolLine = RES_GetImage("/gfx/icon_tool_line", false);
	viewToolBox->AddIcon(imgToolLine);

	imgToolDither = RES_GetImage("/gfx/icon_tool_dither", false);
	viewToolBox->AddIcon(imgToolDither);

	viewToolBox->SetPosition(60, 85, posZ, viewToolBox->sizeX, viewToolBox->sizeY);

	
	//
	SetTopBarVisible(true);
	
	//	ZoomDisplay(15.0f);
	ZoomDisplay(5.0f);
	

	//
//	imgMock = RES_LoadImageFromFileOS("/Users/mars/develop/MTEngine/_RUNTIME_/Documents/gfx/viceditor01.gfx", false);
}

void CViewVicEditor::InitPaintGridColors()
{
	this->viewVicDisplayMain->InitGridLinesColorFromSettings();
}

void CViewVicEditor::InitPaintGridShowZoomLevel()
{
	if (viewVicDisplayMain->scale < c64SettingsPaintGridShowZoomLevel)
	{
		viewVicDisplayMain->showGridLines = false;
	}
	else
	{
		viewVicDisplayMain->showGridLines = true;
	}

}

void CViewVicEditor::SetShowDisplayBorderType(u8 newBorderType)
{
	viewVicDisplayMain->SetShowDisplayBorderType(newBorderType);
	viewVicDisplaySmall->SetShowDisplayBorderType(newBorderType);
}

void CViewVicEditor::InitAddresses()
{
	
}


CViewVicEditor::~CViewVicEditor()
{
}

void CViewVicEditor::DoLogic()
{
	
}

void CViewVicEditor::Render()
{
	guiMain->LockMutex();
	
	if (viewSprite->btnScanForSprites->IsOn())
	{
		layerVirtualSprites->ClearSprites();
		layerVirtualSprites->SimpleScanSpritesInThisFrame();
	}

	// copy current state of VIC
	c64d_vicii_copy_state(&(viewC64->currentViciiState));
	
	// TODO: this is crude hack, temporary copy parameters, this is temporary workaround
	//       this view should copy data itself based on a new class for VIC Display UI
	//       (TODO: create view for VIC Display UI, leaving VIC Display generic class)
	
	float rasterX, rasterY;
	
	if (!viewC64->viewC64VicDisplay->isCursorLocked)
	{
		if (viewVicDisplaySmall->visible && viewVicDisplaySmall->IsInsideScreen(guiMain->mousePosX, guiMain->mousePosY))
		{
			viewVicDisplaySmall->GetRasterPosFromScreenPos2(guiMain->mousePosX, guiMain->mousePosY, &rasterX, &rasterY);
			
			viewVicDisplayMain->rasterCursorPosX = rasterX;
			viewVicDisplayMain->rasterCursorPosY = rasterY;
		}
		else
		{
			viewVicDisplayMain->GetRasterPosFromScreenPos2(guiMain->mousePosX, guiMain->mousePosY, &rasterX, &rasterY);
			
			viewVicDisplaySmall->rasterCursorPosX = rasterX;
			viewVicDisplaySmall->rasterCursorPosY = rasterY;
			
			viewVicDisplayMain->rasterCursorPosX = rasterX;
			viewVicDisplayMain->rasterCursorPosY = rasterY;
		}
		
		viewC64->viewC64VicDisplay->rasterCursorPosX = rasterX;
		viewC64->viewC64VicDisplay->rasterCursorPosY = rasterY;
	}
	
	// update just the VIC state for main C64View screen to correctly render C64 Sprites
	vicii_cycle_state_t *displayVicState = viewC64->viewC64VicDisplay->UpdateViciiStateNonVisible(viewC64->viewC64VicDisplay->rasterCursorPosX,
															   viewC64->viewC64VicDisplay->rasterCursorPosY);
	viewC64->viewC64VicDisplay->RefreshScreenStateOnly(displayVicState);
	viewVicDisplaySmall->RefreshScreenStateOnly(displayVicState);
	
	// this is done by CViewC64 now
//	viewC64->viewC64StateVIC->UpdateSpritesImages();
	
	// main render routine:
	BlitFilledRectangle(0, 0, -1, sizeX, sizeY, 0.0, 0.0, 0.0, 1.0);
	
	// * render main screen *
	vicii_cycle_state_t *viciiState = &(viewC64->viciiStateToShow);
	
	viewPalette->colorD020 = viciiState->regs[0x20];
	viewPalette->colorD021 = viciiState->regs[0x21];
	

	viewVicDisplayMain->RefreshScreen(viciiState);

	if (toolIsActive)
	{
		layerToolPreview->RenderMain(viciiState);
	}
	
	// render main screen layers from top to bottom
	for (std::list<CVicEditorLayer *>::iterator it = this->layers.begin();
		 it != this->layers.end(); it++)
	{
		CVicEditorLayer *layer = *it;
		
		if (layer->isVisible)
		{
			layer->RenderMain(viciiState);
		}
	}

	// render grid lines
	for (std::list<CVicEditorLayer *>::iterator it = this->layers.begin();
		 it != this->layers.end(); it++)
	{
		CVicEditorLayer *layer = *it;
		
		layer->RenderGridMain(viciiState);
	}
	
	
	if (viewC64->isShowingRasterCross)
//	if (viewVicDisplayMain->showRasterCursor)
	{
		if (viewC64->viewC64VicDisplay->isCursorLocked)
		{
			viewVicDisplayMain->RenderCursor(viewC64->viewC64VicDisplay->rasterCursorPosX,
											 viewC64->viewC64VicDisplay->rasterCursorPosY);
		}
		else
		{
			viewVicDisplayMain->RenderCursor();
		}
		
	}
	
	//
	viewVicDisplaySmall->SetViciiState(viciiState);
	
	// * render UI *
//	BlitFilledRectangle(0, 0, posZ, SCREEN_WIDTH, SCREEN_HEIGHT, 1, 0, 0, 1);
//	imgMock->Render(0, -50, posZ, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, 0.703125, 0.87890625);

	
	CGuiView::Render();

	guiMain->UnlockMutex();

	/////////////
}

void CViewVicEditor::Render(float posX, float posY)
{
	CGuiView::Render(posX, posY);
}

void CViewVicEditor::MoveDisplayToPreviewScreenPos(float x, float y)
{
	LOGD("CViewVicEditor::MoveDisplayToPreviewScreenPos: %f %f", x, y);
	
	guiMain->LockMutex();
	
	float rx, ry;
	viewVicDisplaySmall->GetRasterPosFromScreenPosWithoutScroll(x, y, &rx, &ry);

	LOGD("  r=%f %f  dfrs=%f %f", rx, ry, viewVicDisplaySmall->displayFrameRasterSizeX, viewVicDisplaySmall->displayFrameRasterSizeY);
	
	rx -= viewVicDisplaySmall->displayFrameRasterSizeX/2.0f;
	ry -= viewVicDisplaySmall->displayFrameRasterSizeY/2.0f;

	LOGD("  r=%f %f  rsf=%f %f", rx, ry, viewVicDisplayMain->rasterScaleFactorX, viewVicDisplayMain->rasterScaleFactorY);

	float px = rx * viewVicDisplayMain->rasterScaleFactorX;
	float py = ry * viewVicDisplayMain->rasterScaleFactorY;

	if (viewVicDisplaySmall->showDisplayBorderType == VIC_DISPLAY_SHOW_BORDER_VISIBLE_AREA)
	{
		//    frame  |visible   interior  visible|  frame
		// X: 0x000  | 0x068  0x088 0x1C8  0x1E8 |  0x1F8
		//        0             136   456             504
		// Y: 0x000  | 0x010  0x032 0x0FA  0x120 |  0x138
		//        0              50   251             312
		
		px += 0x40 * viewVicDisplayMain->rasterScaleFactorX;
		py += 0x22 * viewVicDisplayMain->rasterScaleFactorY;
	}
	

	this->MoveDisplayToScreenPos(-px, -py);
	
	guiMain->UnlockMutex();
}


void CViewVicEditor::UpdateDisplayFrame()
{
	guiMain->LockMutex();
	
	// check boundaries
	float px = viewVicDisplayMain->posX;
	float py = viewVicDisplayMain->posY;
	
	float borderX = 0.0f;
	
	if (viewVicDisplaySmall->showDisplayBorderType == VIC_DISPLAY_SHOW_BORDER_VISIBLE_AREA)
	{
		//    frame  |visible   interior  visible|  frame
		// X: 0x000  | 0x068  0x088 0x1C8  0x1E8 |  0x1F8
		//        0             136   456             504
		// Y: 0x000  | 0x010  0x032 0x0FA  0x120 |  0x138
		//        0              50   251             312
		
		borderX = 0x21 * viewVicDisplayMain->rasterScaleFactorX;
	}
	
	if (px > -borderX)
		px = -borderX;
	
	if (py > 0.0f)
		py = 0.0f;
	
	if (px + (mainDisplayPosX + viewVicDisplayMain->visibleScreenSizeX + borderX + viewVicDisplayMain->rasterScaleFactorX) < mainDisplaySizeX)
	{
		px = mainDisplaySizeX - (mainDisplayPosX + viewVicDisplayMain->visibleScreenSizeX + borderX + viewVicDisplayMain->rasterScaleFactorX);

		// keep it centered
//		LOGD("   px=%f", px);
		if (px > -borderX/4.0f)
		{
			px = -borderX/4.0f + viewVicDisplayMain->rasterScaleFactorX*2.0f;
		}
	}
	
	if (py + (mainDisplayPosY + viewVicDisplayMain->visibleScreenSizeY) < mainDisplaySizeY)
	{
		py = mainDisplaySizeY - (mainDisplayPosY + viewVicDisplayMain->visibleScreenSizeY);
	}
	
	viewVicDisplayMain->SetPosition(px, py);
	
	float rx, ry;
	viewVicDisplayMain->GetRasterPosFromScreenPosWithoutScroll(mainDisplayPosX, mainDisplayPosY, &rx, &ry);
	
	float rx2, ry2;
	
//	LOGD("viewVicDisplayMain->sizeX=%f", viewVicDisplayMain->sizeX);
//	viewVicDisplayMain->GetRasterPosFromScreenPos(viewVicDisplayMain->posX + viewVicDisplayMain->sizeX,
//												  viewVicDisplayMain->posY + viewVicDisplayMain->sizeY, &rx2, &ry2);

	viewVicDisplayMain->GetRasterPosFromScreenPosWithoutScroll(mainDisplayPosEndX-1,
												  mainDisplayPosEndY-1, &rx2, &ry2);

	
	
	float rsx = rx2 - rx;
	float rsy = ry2 - ry;
	
	viewVicDisplaySmall->SetDisplayFrameRaster((int)rx+1, (int)ry+1, (int)rsx-1, (int)rsy-1);

//	viewVicDisplaySmall->SetDisplayFrameRaster(rx+1, ry+1, rsx-1, rsy-1);

	guiMain->UnlockMutex();
}


void CViewVicEditor::ShowPaintMessage(u8 result)
{
	if (result == PAINT_RESULT_ERROR)
	{
		guiMain->ShowMessage("Can't paint");
	}
	else if (result == PAINT_RESULT_BLOCKED)
	{
		guiMain->ShowMessage("Paint blocked");
	}
	else if (result == PAINT_RESULT_REPLACED_COLOR)
	{
		guiMain->ShowMessage("Replaced color");
	}
	else if (result == PAINT_RESULT_OUTSIDE)
	{
		guiMain->ShowMessage("Outside");
	}

}

////////
void CViewVicEditor::PaintBrushLineWithMessage(CImageData *brush, int rx1, int ry1, int rx2, int ry2, u8 colorSource)
{
	int result = PaintBrushLine(brush, rx1, ry1, rx2, ry2, colorSource);
	ShowPaintMessage(result);
}

u8 CViewVicEditor::PaintBrushLine(CImageData *brush, int rx0, int ry0, int rx1, int ry1, u8 colorSource)
{
	int result = PAINT_RESULT_OK;

	int dx = abs(rx1-rx0), sx = rx0<rx1 ? 1 : -1;
	int dy = abs(ry1-ry0), sy = ry0<ry1 ? 1 : -1;
	int err = (dx>dy ? dx : -dy)/2, e2;
	
	for(;;)
	{
		int r = PaintBrush(brush, rx0, ry0, colorSource);
		if (r < result)
			result = r;

		if (rx0==rx1 && ry0==ry1)
		{
			break;
		}
		e2 = err;
		
		if (e2 >-dx)
		{
			err -= dy;
			rx0 += sx;
		}
		if (e2 < dy)
		{
			err += dx;
			ry0 += sy;
		}
	}
	return result;
}

void CViewVicEditor::PaintBrushWithMessage(CImageData *brush, int rx, int ry, u8 colorSource)
{
	int result = PaintBrush(brush, rx, ry, colorSource);
	ShowPaintMessage(result);
}

bool CViewVicEditor::IsColorReplace()
{
//	LOGD("IsColorReplace: return %d | c64SettingsVicEditorForceReplaceColor %d = %d",
//		 guiMain->isControlPressed, c64SettingsVicEditorForceReplaceColor,
//		 (guiMain->isControlPressed | c64SettingsVicEditorForceReplaceColor));

	return (guiMain->isControlPressed | c64SettingsVicEditorForceReplaceColor);
}

u8 CViewVicEditor::PaintBrush(CImageData *brush, int rx, int ry, u8 colorSource)
{
	LOGD("CViewVicEditor::PaintBrush: rx=%d ry=%d", rx, ry);
	int brushCenterX = floor((float)brush->width / 2.0f);
	int brushCenterY = floor((float)brush->height / 2.0f);
	
	int sx = rx - brushCenterX;
	int sy = ry - brushCenterY;
	
	LOGD("   ..sx=%d sy=%d", sx, sy);
	
	int result = PAINT_RESULT_OK;
	
	for (int y = 0; y < brush->height; y++)
	{
		for (int x = 0; x < brush->width; x++)
		{
			if (brush->GetPixelResultByte(x, y) > 0)
			{
				int r;
				
				if (selectedLayer != NULL && selectedLayer->isPaintingLocked == false)
				{
					bool forceColorReplace = IsColorReplace();
					
					r = selectedLayer->Paint(forceColorReplace, guiMain->isAltPressed,
											 sx + x, sy + y,
											 viewPalette->colorLMB, viewPalette->colorRMB, colorSource,
											 this->viewCharset->selectedChar);
				}
				else
				{
					// try to paint on each layer counting from the top to bottom
					for (std::list<CVicEditorLayer *>::reverse_iterator it = layers.rbegin();
						 it != layers.rend(); it++)
					{
						CVicEditorLayer *layer = *it;
						
						if (layer->isVisible && layer->isPaintingLocked == false)
						{
							LOGD(" ...sx=%d +x=%d =%d  | sy=%d +y=%d =%d", sx, x, (sx+x), sy, y, (sy+y));
							
							bool forceColorReplace = IsColorReplace();

							r = layer->Paint(forceColorReplace, guiMain->isAltPressed,
											 sx + x, sy + y,
											 viewPalette->colorLMB, viewPalette->colorRMB, colorSource,
											 this->viewCharset->selectedChar);

							if (r >= PAINT_RESULT_BLOCKED)
							{
//								LOGD("layer %s: r=%d", layer->layerName, r);
								break;
							}
						}
					}
				}

				if (r < result)
					result = r;
				
			}
		}
	}
	
	return result;
}

u8 CViewVicEditor::PaintPixel(int rx, int ry, u8 colorSource)
{
	LOGD("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! CViewVicEditor::PaintPixel, colorSource=%d", colorSource);
	
	guiMain->LockMutex();
	
	int result;
	
	bool forceColorReplace = IsColorReplace();

	result = viewVicDisplayMain->currentCanvas->Paint(forceColorReplace, guiMain->isAltPressed,
													  rx, ry,
													  viewPalette->colorLMB, viewPalette->colorRMB, colorSource,
													  this->viewCharset->selectedChar);
	
	guiMain->UnlockMutex();
	
	return result;
}

// pure pixel paiting (for external API)
u8 CViewVicEditor::PaintPixelColor(bool forceColorReplace, int rx, int ry, u8 color, int selectedChar)
{
	guiMain->LockMutex();
	
	int result;
	
	result = viewVicDisplayMain->currentCanvas->Paint(forceColorReplace, false,
													  rx, ry,
													  color, color, VICEDITOR_COLOR_SOURCE_LMB,
													  selectedChar);
	
	guiMain->UnlockMutex();
	
	return result;
}

// pure pixel paiting (for external API)
u8 CViewVicEditor::PaintPixelColor(int rx, int ry, u8 color)
{
	guiMain->LockMutex();
	
	int result;
	
	result = viewVicDisplayMain->currentCanvas->Paint(true, false,
													  rx, ry,
													  color, color, VICEDITOR_COLOR_SOURCE_LMB,
													  0x00);
	
	guiMain->UnlockMutex();
	
	return result;
}


//@returns is consumed
bool CViewVicEditor::DoTap(float x, float y)
{
	LOGG("CViewVicEditor::DoTap:  x=%f y=%f", x, y);
	
	if (CGuiView::DoTap(x, y) == false)
	{
		if (viewVicDisplaySmall->visible && viewVicDisplaySmall->IsInsideView(x, y))
		{
			float fRasterX, fRasterY;
			viewVicDisplaySmall->GetRasterPosFromScreenPos2(x, y, &fRasterX, &fRasterY);
			
			int rx = floor(fRasterX);
			int ry = floor(fRasterY);
			
			if (guiMain->isShiftPressed)
			{
				// get pixel color
				u8 color;
				if (GetColorAtRasterPos(rx, ry, &color))
				{
					viewPalette->colorLMB = color;
				}
				return true;
			}
			else
			{
				LOGD("PAINT! %f %f (%d %d)", x ,y, rx, ry);
				
				if (isPainting == false)
				{
					StoreUndo();
				}
				
				prevRx = rx;
				prevRy = ry;
				PaintBrushWithMessage(this->currentBrush, rx, ry, VICEDITOR_COLOR_SOURCE_LMB);
				
				isPainting = true;
				isPaintingOnPreviewFrame = true;
				return true;
			}
		}
		
		if (isPaintingOnPreviewFrame)
			return true;
		
		// clicked on the background / vic main display?
		if (viewVicDisplayMain->IsInside(x, y))
		{
			float fRasterX, fRasterY;
			viewVicDisplayMain->GetRasterPosFromScreenPosWithoutScroll(x, y, &fRasterX, &fRasterY);
		
			int rx = floor(fRasterX);
			int ry = floor(fRasterY);
			
			LOGD("ps:   rx=%d ry=%d", rx, ry);
			
			if (guiMain->isShiftPressed && guiMain->isControlPressed)
			{
				C64Sprite *sprite = layerVirtualSprites->FindSpriteByRasterPos(rx, ry);
				
				if (sprite == NULL)
				{
					viewSprite->isSpriteLocked = false;
					return true;
				}
				
				if (viewSprite->isSpriteLocked == false)
				{
					viewSprite->isSpriteLocked = true;
					return true;
				}
				
//				if (viewSprite->sprite != sprite)
//				{
//					viewSprite->isSpriteLocked = true;
//					return true;
//				}
				viewSprite->isSpriteLocked = false;
				return true;
			}
			
			viewVicDisplayMain->GetRasterPosFromScreenPos2(x, y, &fRasterX, &fRasterY);
			
			if (guiMain->isShiftPressed)
			{
				// get pixel color
				u8 color;
				if (GetColorAtRasterPos(rx, ry, &color))
				{
					viewPalette->colorLMB = color;
				}
				return true;
			}
			else
			{
				LOGD("PAINT! %f %f (%d %d)", x ,y, rx, ry);
				
				if (isPainting == false)
				{
					StoreUndo();
				}
				
				PaintBrush(this->currentBrush, rx, ry, VICEDITOR_COLOR_SOURCE_LMB);
				
				isPainting = true;
				return true;
			}
		}
		
		return false;
	}
	return true;
}

bool CViewVicEditor::GetColorAtRasterPos(int rx, int ry, u8 *color)
{
	// try to get color from each layer counting from the top to bottom
	for (std::list<CVicEditorLayer *>::reverse_iterator it = layers.rbegin();
		 it != layers.rend(); it++)
	{
		CVicEditorLayer *layer = *it;
		
		if (layer->isVisible)
		{
			if (layer->GetColorAtPixel(rx, ry, color))
				return true;
		}
	}
	
	return false;

}

bool CViewVicEditor::DoMove(float x, float y, float distX, float distY, float diffX, float diffY)
{
	if (CGuiView::DoMove(x, y, distX, distY, diffX, diffY) == false)
	{
		if (viewVicDisplaySmall->visible &&
			!viewVicDisplaySmall->viewFrame->movingView
			&& viewVicDisplaySmall->IsInsideView(x, y))
		{
			float fRasterX, fRasterY;
			viewVicDisplaySmall->GetRasterPosFromScreenPos2(x, y, &fRasterX, &fRasterY);
			
			int rx = floor(fRasterX);
			int ry = floor(fRasterY);
			
			//LOGD("PAINT! %f %f (%d %d)", x ,y, rx, ry);
			
			if (guiMain->isShiftPressed)
			{
				LOGD("        !! get color at %d %d", rx, ry);
				u8 color;
				if (GetColorAtRasterPos(rx, ry, &color))
				{
					viewPalette->colorLMB = color;
				}
				return true;
			}
			else
			{
				if (isPainting == false)
				{
					StoreUndo();
				}
				
				if (prevRx < -999)
				{
					prevRx = rx;
					prevRy = ry;
				}
				
//				PaintBrush(this->currentBrush, rx, ry, VICEDITOR_COLOR_SOURCE_LMB);
				PaintBrushLineWithMessage(this->currentBrush, prevRx, prevRy, rx, ry, VICEDITOR_COLOR_SOURCE_LMB);
				
				prevRx = rx;
				prevRy = ry;
				
				isPainting = true;
				isPaintingOnPreviewFrame = true;
				
				//		viewVicDisplaySmall->mousePosX = x;
				//		viewVicDisplaySmall->mousePosY = y;
				
				return true;
			}
			
		}
		
		if (isPaintingOnPreviewFrame)
			return true;
		
		// update sprite
		if (viewSprite->visible && !(viewSprite->isSpriteLocked))
		{
			float rx, ry;
			viewVicDisplayMain->GetRasterPosFromScreenPosWithoutScroll(x, y, &rx, &ry);
			layerVirtualSprites->UpdateSpriteView((int)rx, (int)ry);
		}
		

		// clicked on the background / vic main display?
		if (viewVicDisplayMain->IsInside(x, y))
		{
			// TODO: paint line
			float fRasterX, fRasterY;
			viewVicDisplayMain->GetRasterPosFromScreenPos2(x, y, &fRasterX, &fRasterY);
			
			int rx = floor(fRasterX);
			int ry = floor(fRasterY);

			if (guiMain->isShiftPressed)
			{
				LOGD("        !! get color at %d %d", rx, ry);
				u8 color;
				if (GetColorAtRasterPos(rx, ry, &color))
				{
					viewPalette->colorLMB = color;
				}
				return true;
			}
			else
			{
				//			LOGD("PAINT! %f %f (%d %d)", x ,y, rx, ry);
				if (prevRx < -999)
				{
					prevRx = rx;
					prevRy = ry;
				}
				
				if (isPainting == false)
				{
					StoreUndo();
				}
				
//				PaintBrush(this->currentBrush, rx, ry, VICEDITOR_COLOR_SOURCE_LMB);
				PaintBrushLineWithMessage(this->currentBrush, prevRx, prevRy, rx, ry, VICEDITOR_COLOR_SOURCE_LMB);

				prevRx = rx;
				prevRy = ry;

				isPainting = true;
				return true;
	
			}
		}
		
		return false;
	}
	return true;
}

void CViewVicEditor::ToolPaintPixel(int px, int py, u8 colorSource)
{
	PaintPixel(px, py, colorSource);
}

void CViewVicEditor::ToolPaintBrush(int px, int py, u8 colorSource)
{
	PaintBrush(this->currentBrush, px, py, colorSource);
}

void CViewVicEditor::ToolClearDraftImage()
{
	layerToolPreview->ClearScreen();
}

void CViewVicEditor::ToolMakeDraftActive()
{
	this->toolIsActive = true;
}

void CViewVicEditor::ToolMakeDraftNotActive()
{
	this->toolIsActive = false;
}

void CViewVicEditor::ToolPaintPixelOnDraft(int px, int py, u8 colorSource)
{
	layerToolPreview->Paint(true, guiMain->isAltPressed,
							px, py,
							viewPalette->colorLMB, viewPalette->colorRMB, colorSource,
							this->viewCharset->selectedChar);
}

void CViewVicEditor::ToolPaintBrushOnDraft(int px, int py, u8 colorSource)
{
	LOGTODO("CViewVicEditor::ToolPaintBrushOnDraft: not implemented");
}

//
bool CViewVicEditor::DoRightClick(float x, float y)
{
	LOGI("CViewVicEditor::DoRightClick");
	if (viewVicDisplaySmall->visible && viewVicDisplaySmall->IsInsideView(x, y))
	{
		MoveDisplayToPreviewScreenPos(x, y);
		isMovingPreviewFrame = true;
		return true;
	}
	
	if (isMovingPreviewFrame)
		return true;
	
	if (isPaintingOnPreviewFrame)
		return true;
	
	LOGD("CViewVicEditor: .................. CGuiView::DoRightClick(x, y)");
	bool ret = CGuiView::DoRightClick(x, y);

	LOGD("ret=%d", ret);

	if (ret == false)
	{
		LOGD("viewVicDisplayMain->IsInside: %f %f | %f %f %f %f", x, y, viewVicDisplayMain->posX, viewVicDisplayMain->posY, viewVicDisplayMain->sizeX, viewVicDisplayMain->sizeY);

		// clicked on the background / vic main display?
		if (viewVicDisplayMain->IsInside(x, y))
		{
			LOGD("          !!INSIDE viewVicDisplayMain");
			float fRasterX, fRasterY;
			viewVicDisplayMain->GetRasterPosFromScreenPos2(x, y, &fRasterX, &fRasterY);
			
			int rx = floor(fRasterX);
			int ry = floor(fRasterY);
			
			if (guiMain->isShiftPressed)
			{
				LOGD("        !! get color at %d %d", rx, ry);
				u8 color;
				if (GetColorAtRasterPos(rx, ry, &color))
				{
					viewPalette->colorRMB = color;
				}
				return true;
			}
			else
			{
				LOGD("PAINT! %f %f (%d %d)", x ,y, rx, ry);
				
				if (isPainting == false)
				{
					StoreUndo();
				}
				
				prevRx = rx;
				prevRy = ry;

				PaintBrushWithMessage(this->currentBrush, rx, ry, VICEDITOR_COLOR_SOURCE_RMB);

				isPainting = true;
				return true;
			}
		}
		
		LOGD("    ... false | NOT INSIDE");
		return false;
	}
	return true;
	
	
	
	return CGuiView::DoRightClick(x, y);
}

bool CViewVicEditor::DoRightClickMove(float x, float y, float distX, float distY, float diffX, float diffY)
{
	if (viewVicDisplaySmall->visible && viewVicDisplaySmall->IsInsideView(x, y))
	{
		MoveDisplayToPreviewScreenPos(x, y);
		isMovingPreviewFrame = true;

		viewVicDisplaySmall->DoRightClickMove(x, y, distX, distY, diffX, diffY);
		return true;
	}
	
	if (isMovingPreviewFrame)
		return true;
	
	if (isPaintingOnPreviewFrame)
		return true;
	
	// update sprite
	if (viewSprite->visible && !(viewSprite->isSpriteLocked))
	{
		float rx, ry;
		viewVicDisplayMain->GetRasterPosFromScreenPosWithoutScroll(x, y, &rx, &ry);
		layerVirtualSprites->UpdateSpriteView((int)rx, (int)ry);
	}
	
	// clicked on the background / vic main display?
	if (viewVicDisplayMain->IsInside(x, y))
	{
		// TODO: paint line
		float fRasterX, fRasterY;
		viewVicDisplayMain->GetRasterPosFromScreenPos2(x, y, &fRasterX, &fRasterY);
		
		int rx = floor(fRasterX);
		int ry = floor(fRasterY);
		
		if (guiMain->isShiftPressed)
		{
			LOGD("        !! get color at %d %d", rx, ry);
			u8 color;
			if (GetColorAtRasterPos(rx, ry, &color))
			{
				viewPalette->colorRMB = color;
			}
			return true;
		}
		else
		{
			//			LOGD("PAINT! %f %f (%d %d)", x ,y, rx, ry);
			if (prevRx < -999)
			{
				prevRx = rx;
				prevRy = ry;
			}
			
			if (isPainting == false)
			{
				StoreUndo();
			}
			
//			PaintBrush(this->currentBrush, rx, ry, VICEDITOR_COLOR_SOURCE_RMB);
			PaintBrushLineWithMessage(this->currentBrush, prevRx, prevRy, rx, ry, VICEDITOR_COLOR_SOURCE_RMB);

			prevRx = rx;
			prevRy = ry;
			
			isPainting = true;
			return true;
			
		}
	}

	
	return CGuiView::DoRightClickMove(x, y, distX, distY, diffX, diffY);
}


//


bool CViewVicEditor::FinishMove(float x, float y, float distX, float distY, float accelerationX, float accelerationY)
{
	isPainting = false;
	prevRx = -1000;
	prevRy = -1000;
	isMovingPreviewFrame = false;
	isPaintingOnPreviewFrame = false;
	
	return CGuiView::FinishMove(x, y, distX, distY, accelerationX, accelerationY);
}

bool CViewVicEditor::FinishRightClickMove(float x, float y, float distX, float distY, float accelerationX, float accelerationY)
{
	isPainting = false;
	prevRx = -1000;
	prevRy = -1000;
	isMovingPreviewFrame = false;
	isPaintingOnPreviewFrame = false;
	return CGuiView::FinishRightClickMove(x, y, distX, distY, accelerationX, accelerationY);
}




/////////////// TODO: FinishTouches event does not exist anyore







void CViewVicEditor::FinishTouches()
{
	
	isPainting = false;
	prevRx = -1000;
	prevRy = -1000;
	isMovingPreviewFrame = false;
	isPaintingOnPreviewFrame = false;
	
//	return CGuiView::FinishTouches();
}



bool CViewVicEditor::DoScrollWheel(float deltaX, float deltaY)
{
	if (c64SettingsUseMultiTouchInMemoryMap)
	{
		float f = 2.0f;
		MoveDisplayDiff(deltaX * f, deltaY * f);
	}
	else
	{
		if (c64SettingsMemoryMapInvertControl)
		{
			deltaY = -deltaY;
		}
		
		// scale
		float dy;
		
		if (guiMain->isShiftPressed)
		{
			// quicker zoom with shift
			dy = deltaY * 0.05f * ZOOMING_SPEED_FACTOR_QUICK;
		}
		else
		{
			dy = deltaY * 0.05f;
		}
		
		float newScale = viewVicDisplayMain->scale + dy;
		
		LOGG("CViewVicEditor::DoScrollWheel:  %f %f %f", viewVicDisplayMain->scale, dy, newScale);
		
		ZoomDisplay(newScale);
	}
	
	return true;
}

bool CViewVicEditor::InitZoom()
{
	return CGuiView::InitZoom();
}

bool CViewVicEditor::DoZoomBy(float x, float y, float zoomValue, float difference)
{
	// scale
	float dy;
	
	if (guiMain->isShiftPressed)
	{
		// quicker zoom with shift
		dy = difference * 0.25f * ZOOMING_SPEED_FACTOR_QUICK;
	}
	else
	{
		dy = difference * 0.25f;
	}

	float newScale = viewVicDisplayMain->scale + dy;
	
	LOGG("CViewVicEditor::DoZoomBy:  %f %f %f", viewVicDisplayMain->scale, dy, newScale);
	
	ZoomDisplay(newScale);
	
	
	return true;
}

bool CViewVicEditor::DoNotTouchedMove(float x, float y)
{
	//LOGG("CViewVicEditor::DoNotTouchedMove: isKeyboardMovingDisplay=%d", isKeyboardMovingDisplay);
	if (isKeyboardMovingDisplay)
	{
		float dx = prevMousePosX - x;
		float dy = prevMousePosY - y;
		
		MoveDisplayDiff(-dx, -dy);
		
		prevMousePosX = x;
		prevMousePosY = y;
		return true;
	}
	
	if (viewPalette->IsInside(x, y))
	{
		//LOGD(" ======= INSIDE PALETTE");
		return viewPalette->DoNotTouchedMove(x, y);
	}
	
	if (viewVicDisplaySmall->visible && viewVicDisplaySmall->IsInside(x, y))
	{
		//LOGD(" ======= INSIDE SMALL=%x", viewVicDisplaySmall);
		return viewVicDisplaySmall->DoNotTouchedMove(x, y);
	}
	
	// update sprite
	if (viewSprite->visible && !(viewSprite->isSpriteLocked))
	{
		float rx, ry;
		viewVicDisplayMain->GetRasterPosFromScreenPosWithoutScroll(x, y, &rx, &ry);
		layerVirtualSprites->UpdateSpriteView((int)rx, (int)ry);
	}
	
	//LOGD(" ====== MAIN=%x", viewVicDisplayMain);
	if (!viewC64->viewC64VicDisplay->isCursorLocked)
	{
		viewVicDisplayMain->DoNotTouchedMove(x, y);
	}
	
	return true;
}


void CViewVicEditor::MoveDisplayDiff(float diffX, float diffY)
{
	LOGG("CViewVicEditor::MoveDisplayDiff: diffX=%f diffY=%f posX=%f posY=%f posOffsetX=%f posOffsetY=%f", diffX, diffY, viewVicDisplayMain->posX, viewVicDisplayMain->posY, viewVicDisplayMain->posOffsetX, viewVicDisplayMain->posOffsetY);
	float px = viewVicDisplayMain->posX - mainDisplayPosX - viewVicDisplayMain->posOffsetX;
	float py = viewVicDisplayMain->posY - mainDisplayPosY - viewVicDisplayMain->posOffsetY;
	
	px += diffX;
	py += diffY;
	
	MoveDisplayToScreenPos(px, py);
}

void CViewVicEditor::MoveDisplayToScreenPos(float px, float py)
{
	LOGG("CViewVicEditor::MoveDisplayToScreenPos: %f %f", px, py);

	guiMain->LockMutex();
	
	float borderX = 0.0f;
	
	if (viewVicDisplaySmall->showDisplayBorderType == VIC_DISPLAY_SHOW_BORDER_VISIBLE_AREA)
	{
		//    frame  |visible   interior  visible|  frame
		// X: 0x000  | 0x068  0x088 0x1C8  0x1E8 |  0x1F8
		//        0             136   456             504
		// Y: 0x000  | 0x010  0x032 0x0FA  0x120 |  0x138
		//        0              50   251             312

		borderX = 0x21 * viewVicDisplayMain->rasterScaleFactorX;
	}
	
	if (px > -borderX)
		px = -borderX;
	
	if (py > 0.0f)
		py = 0.0f;
	
	if (px + (mainDisplayPosX + viewVicDisplayMain->visibleScreenSizeX + borderX) < mainDisplaySizeX)
	{
//		LOGD("mainDisplaySizeX=%f visibleScreenSizeX=%f borderX=%f (%f)", mainDisplaySizeX, viewVicDisplayMain->visibleScreenSizeX, borderX,
//			 (viewVicDisplayMain->visibleScreenSizeX + borderX));
		px = mainDisplaySizeX - (mainDisplayPosX + viewVicDisplayMain->visibleScreenSizeX + borderX);
		
//		LOGD("   px=%f", px);
		if (px > -borderX/4.0f)
		{
			px = -borderX/4.0f + viewVicDisplayMain->rasterScaleFactorX*2.0f;
		}
	}
	
	if (py + (mainDisplayPosY + viewVicDisplayMain->visibleScreenSizeY) < mainDisplaySizeY)
	{
		py = mainDisplaySizeY - (mainDisplayPosY + viewVicDisplayMain->visibleScreenSizeY);
	}
	
	LOGG("------> SetPosition: px=%f py=%f", px, py);
	viewVicDisplayMain->SetPosition(px, py);
	UpdateDisplayFrame();
	
	guiMain->UnlockMutex();
}

void CViewVicEditor::ZoomDisplay(float newScale)
{
	guiMain->LockMutex();
	
	if (newScale < 1.80f)
		newScale = 1.80f;
	
	if (newScale > 60.0f)
		newScale = 60.0f;
	
	if (newScale < c64SettingsPaintGridShowZoomLevel)
	{
		viewVicDisplayMain->showGridLines = false;
	}
	else
	{
		viewVicDisplayMain->showGridLines = true;
	}

	float cx = guiMain->mousePosX;
	float cy = guiMain->mousePosY;
	
	if (viewVicDisplaySmall->IsInsideView(cx, cy))
	{
		viewVicDisplayMain->SetDisplayScale(newScale);
		
		MoveDisplayToPreviewScreenPos(cx, cy);
	}
	else
	{
		float px, py;
		viewVicDisplayMain->GetRasterPosFromScreenPosWithoutScroll(cx, cy, &px, &py);
		
		viewVicDisplayMain->SetDisplayScale(newScale);
		
		float pcx, pcy;
		viewVicDisplayMain->GetScreenPosFromRasterPosWithoutScroll(px, py, &pcx, &pcy);
		
		MoveDisplayDiff(cx-pcx-viewVicDisplayMain->posOffsetX, cy-pcy-viewVicDisplayMain->posOffsetY);
	}

	
	UpdateDisplayFrame();
	
	guiMain->UnlockMutex();
}

bool CViewVicEditor::DoFinishTap(float x, float y)
{
	LOGG("CViewVicEditor::DoFinishTap: %f %f", x, y);

	return CGuiView::DoFinishTap(x, y);
}

//@returns is consumed
bool CViewVicEditor::DoDoubleTap(float x, float y)
{
	LOGG("CViewVicEditor::DoDoubleTap:  x=%f y=%f", x, y);
	return CGuiView::DoDoubleTap(x, y);
}

bool CViewVicEditor::DoFinishDoubleTap(float x, float y)
{
	LOGG("CViewVicEditor::DoFinishTap: %f %f", x, y);
	return CGuiView::DoFinishDoubleTap(x, y);
}


bool CViewVicEditor::DoMultiTap(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiTap(touch, x, y);
}

bool CViewVicEditor::DoMultiMove(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiMove(touch, x, y);
}

bool CViewVicEditor::DoMultiFinishTap(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiFinishTap(touch, x, y);
}

void CViewVicEditor::SwitchToVicEditor()
{
//	if (viewC64->debugInterfaceC64)
//	{
//		c64SettingsIsInVicEditor = true;
//		guiMain->SetView(this);
//	}
}


void CViewVicEditor::UpdateSmallDisplayScale()
{
	double vicEditorPreviewWindowScales[5] = { 0.25, 0.50, 1.0, 2.0, 4.0 };

	ResetSmallDisplayScale(vicEditorPreviewWindowScales[resetDisplayScaleIndex]);
	
}

void CViewVicEditor::GlobalOSWindowChangedCallback()
{
	UpdateSmallDisplayScale();
}

void CViewVicEditor::SaveScreenshotAsPNG()
{
	exportMode = VICEDITOR_EXPORT_PNG;
	this->OpenDialogExportFile();
}

void CViewVicEditor::SetSpritesFramesVisible(bool showSpriteFrames)
{
	viewVicDisplayMain->showSpritesFrames = showSpriteFrames;
	viewVicDisplaySmall->showSpritesFrames = showSpriteFrames;
	layerVirtualSprites->showSpriteFrames = showSpriteFrames;
}

void CViewVicEditor::SetTopBarVisible(bool isTopBarVisible)
{
	guiMain->LockMutex();
	viewTopBar->SetVisible(isTopBarVisible);
	
	mainDisplayPosX = 0;
	
	if (isTopBarVisible)
	{
		mainDisplayPosY = 16;
	}
	else
	{
		mainDisplayPosY = 0;
	}
	mainDisplaySizeX = SCREEN_WIDTH - mainDisplayPosX;
	mainDisplaySizeY = SCREEN_HEIGHT - mainDisplayPosY;
	mainDisplayPosEndX = SCREEN_WIDTH;
	mainDisplayPosEndY = SCREEN_HEIGHT;
	viewVicDisplayMain->posOffsetX = mainDisplayPosX;
	viewVicDisplayMain->posOffsetY = mainDisplayPosY;
	
	UpdateDisplayFrame();

	guiMain->UnlockMutex();
}

bool CViewVicEditor::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	LOGI("CViewVicEditor::KeyDown: keyCode=%d", keyCode);
//	guiMain->SetView(viewC64->viewC64MainMenu);

	// TODO: create customizable shortcuts for this
	
//	if (keyCode == MTKEY_ENTER)
//	{
//		C64GenerateDisplayListCode(this);
//		return true;
//	}
	
	LOGTODO("VIC EDITOR KEYBOARD SHORTCUTS NOT DONE");
	
	/*
	
	keyCode = SYS_GetBareKey(keyCode, isShift, isAlt, isControl, isSuper);
	
	std::list<u32> zones;
	zones.push_back(KBZONE_VIC_EDITOR);
	zones.push_back(KBZONE_GLOBAL);
	CSlrKeyboardShortcut *shortcut = viewC64->keyboardShortcuts->FindShortcut(zones, keyCode, isShift, isAlt, isControl);
	
	if (shortcut == viewC64->keyboardShortcuts->kbsVicEditorCreateNewPicture)
	{
		this->CreateNewPicture();
		return true;
	}
	else if (shortcut == viewC64->keyboardShortcuts->kbsVicEditorPreviewScale)
	{
		resetDisplayScaleIndex++;
		if (resetDisplayScaleIndex == 5)
		{
			resetDisplayScaleIndex = 0;
		}
		
		UpdateSmallDisplayScale();
		return true;
	}
	else if (shortcut == viewC64->keyboardShortcuts->kbsVicEditorShowCursor)
	{
		viewVicDisplaySmall->showRasterCursor = !viewVicDisplaySmall->showRasterCursor;
		return true;
	}
	else if (shortcut == viewC64->keyboardShortcuts->kbsVicEditorDoUndo)
	{
		guiMain->LockMutex();
		DoUndo();
		guiMain->UnlockMutex();
		return true;
	}
	else if (shortcut == viewC64->keyboardShortcuts->kbsVicEditorDoRedo)
	{
		guiMain->LockMutex();
		DoRedo();
		guiMain->UnlockMutex();
		return true;
	}
	else if (shortcut == viewC64->keyboardShortcuts->kbsVicEditorOpenFile)
	{
		OpenDialogImportFile();
		return true;
	}
	else if (shortcut == viewC64->keyboardShortcuts->kbsVicEditorExportFile)
	{
		exportMode = VICEDITOR_EXPORT_UNKNOWN;
		OpenDialogExportFile();
		return true;
	}
	else if (shortcut == viewC64->keyboardShortcuts->kbsVicEditorSaveVCE)
	{
		OpenDialogSaveVCE();
		return true;
	}
	else if (shortcut == viewC64->keyboardShortcuts->kbsVicEditorLeaveEditor)
	{
		c64SettingsIsInVicEditor = false;
		C64DebuggerStoreSettings();
		this->DeactivateView();
		guiMain->SetView(viewC64);
		return true;
	}
	else if (shortcut == viewC64->keyboardShortcuts->kbsVicEditorClearScreen)
	{
		this->ClearScreen();
		return true;
	}
	else if (shortcut == viewC64->keyboardShortcuts->kbsVicEditorRectangleBrushSizePlus)
	{
		if (brushSize <= 2001)
		{
			guiMain->LockMutex();
			if (currentBrush)
				delete currentBrush;
			
			brushSize += 2;
			
			char *buf = SYS_GetCharBuf();
			
			currentBrush = CreateBrushRectangle(brushSize);
			sprintf(buf, "Rectangle brush size %d", brushSize);

			guiMain->UnlockMutex();

			guiMain->ShowMessage(buf);
			SYS_ReleaseCharBuf(buf);
		}
		return true;
	}
	else if (shortcut == viewC64->keyboardShortcuts->kbsVicEditorRectangleBrushSizeMinus)
	{
		if (brushSize >= 3)
		{
			guiMain->LockMutex();
			if (currentBrush)
				delete currentBrush;
			
			brushSize -= 2;
			
			char *buf = SYS_GetCharBuf();
			
			currentBrush = CreateBrushRectangle(brushSize);
			sprintf(buf, "Rectangle brush size %d", brushSize);

			guiMain->UnlockMutex();

			guiMain->ShowMessage(buf);
			SYS_ReleaseCharBuf(buf);
		}
		return true;
	}
	else if (shortcut == viewC64->keyboardShortcuts->kbsVicEditorCircleBrushSizePlus)
	{
		if (brushSize <= 2001)
		{
			guiMain->LockMutex();
			if (currentBrush)
				delete currentBrush;
			
			brushSize += 2;
			
			char *buf = SYS_GetCharBuf();
			
			currentBrush = CreateBrushCircle(brushSize);
			sprintf(buf, "Circle brush size %d", brushSize);

			guiMain->UnlockMutex();

			guiMain->ShowMessage(buf);
			SYS_ReleaseCharBuf(buf);
		}
		return true;
	}
	else if (shortcut == viewC64->keyboardShortcuts->kbsVicEditorCircleBrushSizeMinus)
	{
		if (brushSize >= 3)
		{
			guiMain->LockMutex();
			if (currentBrush)
				delete currentBrush;
			
			brushSize -= 2;
			
			char *buf = SYS_GetCharBuf();
			
			currentBrush = CreateBrushCircle(brushSize);
			sprintf(buf, "Circle brush size %d", brushSize);

			guiMain->UnlockMutex();

			guiMain->ShowMessage(buf);
			SYS_ReleaseCharBuf(buf);
		}
		return true;
	}
	else if (shortcut == viewC64->keyboardShortcuts->kbsVicEditorToggleAllWindows)
	{
		if (inPresentationScreen)
		{
			viewPalette->SetVisible(prevVisiblePalette);
			viewCharset->SetVisible(prevVisibleCharset);
			viewSprite->SetVisible(prevVisibleSprite);
			viewLayers->SetVisible(prevVisibleLayers);
			
			// TODO: toolbox is disabled in production
			//viewToolBox->SetVisible(prevVisibleToolBox);
			
			SetTopBarVisible(prevVisibleTopBar);
			viewVicDisplaySmall->SetVisible(prevVisiblePreview);
			inPresentationScreen = false;
		}
		else
		{
			prevVisiblePalette = viewPalette->visible;
			prevVisibleCharset = viewCharset->visible;
			prevVisibleSprite = viewSprite->visible;
			prevVisibleLayers = viewLayers->visible;
			prevVisibleTopBar = viewTopBar->visible;
			
			// TODO: toolbox is disabled in production
			//prevVisibleToolBox = viewToolBox->visible;
			
			prevVisiblePreview = viewVicDisplaySmall->visible;
			
			viewPalette->SetVisible(false);
			viewCharset->SetVisible(false);
			viewSprite->SetVisible(false);
			viewLayers->SetVisible(false);
			
			// TODO: toolbox is disabled in production
			//viewToolBox->SetVisible(false);
			
			SetTopBarVisible(false);
			viewVicDisplaySmall->SetVisible(false);
			inPresentationScreen = true;
		}
		return true;
	}
	else if (shortcut == viewC64->keyboardShortcuts->kbsVicEditorToggleWindowPreview)
	{
		viewVicDisplaySmall->SetVisible(!viewVicDisplaySmall->visible);
		return true;
	}
	else if (shortcut == viewC64->keyboardShortcuts->kbsVicEditorToggleWindowPalette)
	{
		viewPalette->SetVisible(!viewPalette->visible);
		return true;
	}
	else if (shortcut == viewC64->keyboardShortcuts->kbsVicEditorToggleWindowLayers)
	{
		viewLayers->SetVisible(!viewLayers->visible);
		return true;
	}
	else if (shortcut == viewC64->keyboardShortcuts->kbsVicEditorToggleWindowCharset)
	{
		viewCharset->SetVisible(!viewCharset->visible);
		prevVisibleCharset = viewCharset->visible;
		return true;
	}
	else if (shortcut == viewC64->keyboardShortcuts->kbsVicEditorToggleWindowSprite)
	{
		viewSprite->SetVisible(!viewSprite->visible);
		return true;
	}
	else if (shortcut == viewC64->keyboardShortcuts->kbsVicEditorToggleTopBar)
	{
		SetTopBarVisible(!viewTopBar->visible);
		return true;
	}
	// TODO: toolbox is disabled in production
//	else if (shortcut == viewC64->keyboardShortcuts->kbsVicEditorToggleToolBox)
//	{
//		viewToolBox->SetVisible(!viewToolBox->visible);
//		return true;
//	}
	else if (shortcut == viewC64->keyboardShortcuts->kbsVicEditorToggleSpriteFrames)
	{
		bool show = !viewVicDisplayMain->showSpritesFrames;
		SetSpritesFramesVisible(show);		
		return true;
	}
	else if (shortcut == viewC64->keyboardShortcuts->kbsVicEditorSelectNextLayer)
	{
		viewLayers->SelectNextLayer();
		return true;
	}
	else if (shortcut == viewC64->viewC64MainMenu->kbsMoveFocusToNextView)
	{
		guiMain->LockMutex();
		MoveFocusToNextView();
		guiMain->UnlockMutex();
	}
	else if (shortcut == viewC64->viewC64MainMenu->kbsMoveFocusToPreviousView)
	{
		guiMain->LockMutex();
		MoveFocusToPrevView();
		guiMain->UnlockMutex();
	}
	
	
//	if ((keyCode == 'h' || keyCode == 'H') && isShift && isControl)
//	{
//		exportMode = VICEDITOR_EXPORT_HYPER;
//		OpenDialogExportFile();
//		return true;
//	}
	
//	if ((keyCode == 's' || keyCode == 'S') && isAlt && !isControl && !isShift)
//	{
//		guiMain->LockMutex();
//		viewSprite->isSpriteLocked = false;
//		layerVirtualSprites->ClearSprites();
//		layerVirtualSprites->FullScanSpritesInThisFrame();
//		guiMain->UnlockMutex();
//		return true;
//	}

	if (keyCode == MTKEY_SPACEBAR)
	{
		isKeyboardMovingDisplay = true;
		prevMousePosX = guiMain->mousePosX;
		prevMousePosY = guiMain->mousePosY;
		return true;
	}

	if (guiMain->isShiftPressed == false
		&& guiMain->isControlPressed == false
		&& guiMain->isAltPressed == false)
	{
		/// "interesting"
		//              0    1    2    3    4    5    6    7
		char codes[0x10] =
		
		//              0    1    2    3    4    5    6    7
		{ '1', '2', '3', '4', '5', '6', '7', '8',
			'q', 'w', 'e', 'r', 't', 'y', 'u', 'i' };
		
		
		for (int i = 0; i < 0x10; i++)
		{
			if (keyCode == codes[i])
			{
				LOGD("Set color %1X", i);
				if (guiMain->isAltPressed == false)
				{
					viewPalette->colorLMB = i;
				}
				else
				{
					viewPalette->colorRMB = i;
				}
				
				return true;
			}
		}

	}
	
	if (keyCode == '0' || keyCode == ')')
	{
		
		if (guiMain->isShiftPressed)
		{
			if (viewPalette->IsInsideView(guiMain->mousePosX, guiMain->mousePosY))
			{
				int color = viewPalette->GetColorIndex(guiMain->mousePosX, guiMain->mousePosY);
				if (color != -1)
				{
					StoreUndo();
					
					viewC64->debugInterfaceC64->SetVicRegister(0x21, color);
					viewPalette->colorD021 = color;
					viewC64->debugInterfaceC64->SetVicRegister(0x20, color);
					viewPalette->colorD020 = color;
				}
				
				return true;
			}
			
			//set $d021 to color at mouse cursor
			
			float fRasterX, fRasterY;
			viewVicDisplayMain->GetRasterPosFromScreenPos2(guiMain->mousePosX, guiMain->mousePosY, &fRasterX, &fRasterY);
			
			int rx = floor(fRasterX);
			int ry = floor(fRasterY);
			
			// get pixel color
			StoreUndo();

			u8 color = viewVicDisplayMain->currentCanvas->GetColorAtPixel(rx, ry);
			
			viewC64->debugInterfaceC64->SetVicRegister(0x21, color);
			viewPalette->colorD021 = color;
			viewC64->debugInterfaceC64->SetVicRegister(0x20, color);
			viewPalette->colorD020 = color;
			return true;

		}

		else
		{
			viewPalette->colorLMB = viewPalette->colorD021;
			return true;
		}
		
	}
	
	 */

	//let's do this manual to walkaround bug with vic display
	//	if (CGuiView::KeyDown(keyCode, isShift, isAlt, isControl, isSuper))
	//	{
	//		return true;
	//	}
	

	if (this->viewPalette->KeyDown(keyCode, isShift, isAlt, isControl, isSuper))
	{
		return true;
	}
	
	if (this->viewSprite->KeyDown(keyCode, isShift, isAlt, isControl, isSuper))
	{
		return true;
	}

	if (this->viewCharset->KeyDown(keyCode, isShift, isAlt, isControl, isSuper))
	{
		return true;
	}
	
	//

//	// TODO: this is a temporary UX workaround for step over jsr
//	if (viewC64->ProcessGlobalKeyboardShortcut(keyCode, isShift, isAlt, isControl, isSuper))
//	{
//		return true;
//	}
	
	if (viewC64->viewC64Screen->KeyDown(keyCode, isShift, isAlt, isControl, isSuper))
	{
		return true;
	}


	return viewC64->KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
}

void CViewVicEditor::CreateNewPicture()
{
	this->BringToFront(viewCreateNewPicture);
	viewCreateNewPicture->SetVisible(!viewCreateNewPicture->visible);
	if (viewCreateNewPicture->IsVisible())
	{
		viewCreateNewPicture->ActivateView();
	}
}

void CViewVicEditor::ClearScreen()
{
	guiMain->LockMutex();
	for (std::list<CVicEditorLayer *>::iterator it = this->layers.begin();
		 it != this->layers.end(); it++)
	{
		CVicEditorLayer *layer = *it;
		layer->ClearScreen();
	}
	guiMain->UnlockMutex();
}

//
bool CViewVicEditor::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	isKeyboardMovingDisplay = false;
	
	if (keyCode == MTKEY_LALT || keyCode == MTKEY_RALT)
	{
		viewVicDisplayMain->currentCanvas->ClearDitherMask();
	}
	
	return viewC64->KeyUp(keyCode, isShift, isAlt, isControl, isSuper);
//	return CGuiView::KeyUp(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewVicEditor::KeyPressed(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return viewC64->KeyPressed(keyCode, isShift, isAlt, isControl, isSuper);
//	return CGuiView::KeyPressed(keyCode, isShift, isAlt, isControl, isSuper);
}

void CViewVicEditor::ActivateView()
{
	LOGG("CViewVicEditor::ActivateView()");
	
	guiMain->LockMutex();
	
	// force color rendering
	backupRenderDataWithColors = viewC64->viewC64MemoryDataDump->renderDataWithColors;
	viewC64->viewC64MemoryDataDump->renderDataWithColors = true;
	
//	viewC64->viewC64VicControl->lstBitmapAddresses->SetElement(1, true, false);
//	viewC64->viewC64VicControl->lstBitmapAddresses->SetListLocked(true);
//	viewC64->viewC64VicControl->btnModeText->SetOn(false);
//	viewC64->viewC64VicControl->btnModeBitmap->SetOn(true);
//	viewC64->viewC64VicControl->btnModeHires->SetOn(false);
//	viewC64->viewC64VicControl->btnModeMulti->SetOn(true);
//
	

	// other border types are not supported now
//	if (viewC64->viewC64VicDisplay->showDisplayBorderType == VIC_DISPLAY_SHOW_BORDER_FULL)
//	{
//		this->SetShowDisplayBorderType(VIC_DISPLAY_SHOW_BORDER_VISIBLE_AREA);
//	}
//	else
//	{
//		this->SetShowDisplayBorderType(viewC64->viewC64VicDisplay->showDisplayBorderType);
//	}
	
	this->SetShowDisplayBorderType(VIC_DISPLAY_SHOW_BORDER_VISIBLE_AREA);
	
	UpdateDisplayFrame();

	///
	vicii_cycle_state_t *viciiState = &(viewC64->viciiStateToShow);
	
	u8 mc;
	u8 eb;
	u8 bm;
	u8 blank;
	
	mc = (viciiState->regs[0x16] & 0x10) >> 4;
	bm = (viciiState->regs[0x11] & 0x20) >> 5;
	eb = (viciiState->regs[0x11] & 0x40) >> 6;
	
	viewC64->viewC64VicControl->RefreshStateButtonsUI(&mc, &eb, &bm, &blank);
	
	bool isBitmap = bm;
	
	if (isBitmap == false)
	{
		viewCharset->visible = prevVisibleCharset;
	}
	else
	{
		viewCharset->visible = false;
	}
	
//	viewPalette->visible = true;
//	viewVicDisplaySmall->visible = true;
//	inPresentationScreen = false;

	
	bool applyScroll = viewC64->viewC64VicControl->btnApplyScrollRegister->IsOn();
	this->viewVicDisplayMain->applyScrollRegister = applyScroll;
	this->viewVicDisplaySmall->applyScrollRegister = applyScroll;
	
	guiMain->UnlockMutex();

	viewC64->ShowMouseCursor();
}

void CViewVicEditor::DeactivateView()
{
	LOGG("CViewVicEditor::DeactivateView()");
	
	viewC64->viewC64MemoryDataDump->renderDataWithColors = backupRenderDataWithColors;
}

//
void CViewVicEditor::ToolBoxIconPressed(CSlrImage *imgIcon)
{
	LOGD("CViewVicEditor::ToolBoxIconPressed");
	
	if (imgIcon == imgNew)
	{
		CreateNewPicture();
	}
	else if (imgIcon == imgOpen)
	{
		OpenDialogImportFile();
	}
	else if (imgIcon == imgSave)
	{
		OpenDialogSaveVCE();
	}
	else if (imgIcon == imgImport)
	{
		OpenDialogImportFile();
	}
	else if (imgIcon == imgExport)
	{
		OpenDialogExportFile();
	}
	else if (imgIcon == imgSettings)
	{
//		guiMain->SetView(viewC64->viewC64SettingsMenu);
		return;
	}
	else if (imgIcon == imgClear)
	{
		this->ClearScreen();
	}
	else if (imgIcon == imgToolAlwaysOnTopOff
			 || imgIcon == imgToolAlwaysOnTopOn)
	{
		viewC64->viewC64SettingsMenu->menuItemWindowAlwaysOnTop->SetSelectedOption(c64SettingsWindowAlwaysOnTop ? false : true, true);
		btnToolAlwaysOnTop->SetImage(c64SettingsWindowAlwaysOnTop ? imgToolAlwaysOnTopOn : imgToolAlwaysOnTopOff);
	}
	else if (imgIcon == imgToolAlwaysOnTopOn)
	{
		btnToolAlwaysOnTop->SetImage(imgToolAlwaysOnTopOff);
	}

}

//
CImageData *CViewVicEditor::CreateBrushCircle(int newSize)
{
	CImageData *brush = new CImageData(newSize, newSize, IMG_TYPE_GRAYSCALE);
	brush->AllocImage(false, true);
	
	float r = (float)newSize / 2.0f;
	int ir = (int)r;

	float r2 = r*r;
	
	for (int y = -r; y < r; y++)
	{
		for (int x = -r; x < r; x++)
		{
			if ( (x*x + y*y) < r2 )
			{
				brush->SetPixelResultByte(x + ir, y + ir, 1);
			}
		}
	}
	
	//brush->debugPrint();
	return brush;
}

CImageData *CViewVicEditor::CreateBrushRectangle(int newSize)
{
	CImageData *brush = new CImageData(newSize, newSize, IMG_TYPE_GRAYSCALE);
	brush->AllocImage(false, true);
	
	for (int y = 0; y < newSize; y++)
	{
		for (int x = 0; x < newSize; x++)
		{
			brush->SetPixelResultByte(x, y, 1);
		}
	}

	//brush->debugPrint();
	return brush;
}

// import export
void CViewVicEditor::OpenDialogImportFile()
{
	LOGM("OpenDialogImportFile");
	CSlrString *windowTitle = new CSlrString("Open image file to import");
	windowTitle->DebugPrint("windowTitle=");
	viewC64->ShowDialogOpenFile(this, &importFileExtensions, c64SettingsDefaultVicEditorFolder, windowTitle);
	delete windowTitle;
}

void CViewVicEditor::SystemDialogFileOpenSelected(CSlrString *path)
{
	LOGM("CViewVicEditor::SystemDialogFileOpenSelected, path=%x", path);
	path->DebugPrint("path=");
	
	if (c64SettingsDefaultVicEditorFolder != NULL)
		delete c64SettingsDefaultVicEditorFolder;
	c64SettingsDefaultVicEditorFolder = path->GetFilePathWithoutFileNameComponentFromPath();
	c64SettingsDefaultVicEditorFolder->DebugPrint("c64SettingsDefaultVicEditorFolder=");
	C64DebuggerStoreSettings();

	CSlrString *ext = path->GetFileExtensionComponentFromPath();
	
	if (ext->CompareWith("png") || ext->CompareWith("PNG"))
	{
		StoreUndo();
		ImportPNG(path);
	}
	else if (ext->CompareWith("kla") || ext->CompareWith("KLA"))
	{
		StoreUndo();
		ImportKoala(path, true);
	}
	else if (ext->CompareWith("dd") || ext->CompareWith("DD")
			 || ext->CompareWith("ddl") || ext->CompareWith("DDL"))
	{
		StoreUndo();
		ImportDoodle(path);
	}
	else if (ext->CompareWith("aas") || ext->CompareWith("AAS")
			 || ext->CompareWith("art") || ext->CompareWith("ART"))
	{
		StoreUndo();
		ImportArtStudio(path);
	}
	else if (ext->CompareWith("vce") || ext->CompareWith("VCE"))
	{
		StoreUndo();
		ImportVCE(path);
	}
	else
	{
		// TODO: move this into common open file interface, path will be deleted by SystemDialogFileOpenSelected
		viewC64->viewC64MainMenu->SystemDialogFileOpenSelected(path);
	}

	/*
	else
	{
		char *cExt = ext->GetStdASCII();
		
		char *buf = SYS_GetCharBuf();
		
		sprintf (buf, "Unknown extension: %s", cExt);
		guiMain->ShowMessage(buf);
		
		delete [] cExt;
		SYS_ReleaseCharBuf(buf);
	}*/
	
}

void CViewVicEditor::SystemDialogFileOpenCancelled()
{
	LOGD("CViewVicEditor::SystemDialogFileOpenCancelled");
}

// export
u8 CViewVicEditor::GetExportModeFromVicState(vicii_cycle_state_t *viciiState)
{
	// check current VIC Display mode
	u8 mc;
	u8 eb;
	u8 bm;
	u8 blank;
	
	mc = (viciiState->regs[0x16] & 0x10) >> 4;
	bm = (viciiState->regs[0x11] & 0x20) >> 5;
	eb = (viciiState->regs[0x11] & 0x40) >> 6;
	
	viewC64->viewC64VicControl->RefreshStateButtonsUI(&mc, &eb, &bm, &blank);
	
	bool isMultiColor = mc;
	bool isBitmap = bm;
	bool isExtColor = eb;

	if (isBitmap == true && isMultiColor == true)
	{
		// multi bitmap
		return VICEDITOR_EXPORT_KOALA;
	}
	else if (isBitmap == true && isMultiColor == false)
	{
		// hires bitmap
		return VICEDITOR_EXPORT_ART_STUDIO;
	}
	// raw text
	return VICEDITOR_EXPORT_RAW_TEXT;
}

void CViewVicEditor::ExportScreen(CSlrString *path)
{
	path->DebugPrint("CViewVicEditor::ExportScreen path=");
	
	// export current screen to a path based on selected screen mode
	u8 exportMode = GetExportModeFromVicState(&(viewC64->viciiStateToShow));
	
	if (exportMode == VICEDITOR_EXPORT_KOALA)
	{
		ExportSpritesData(path);

		CSlrString *strPathKla = new CSlrString(path);
		strPathKla->Concatenate(".kla");
		ExportKoala(strPathKla);
		delete strPathKla;
	}
	else if (exportMode == VICEDITOR_EXPORT_ART_STUDIO)
	{
		ExportSpritesData(path);

		CSlrString *strPathArt = new CSlrString(path);
		strPathArt->Concatenate(".art");
		ExportArtStudio(strPathArt);
		delete strPathArt;
	}
	else //if (exportFileDialogMode == VICEDITOR_EXPORT_RAW_TEXT)
	{
		ExportSpritesData(path);

		CSlrString *strPathRaw = new CSlrString(path);
		strPathRaw->Concatenate(".raw");
		ExportRawText(strPathRaw);
		delete strPathRaw;
		
		CSlrString *strPathCharset = new CSlrString(path);
		strPathRaw->Concatenate(".charset");
		ExportCharset(strPathCharset);
		delete strPathCharset;
	}
}

void CViewVicEditor::OpenDialogExportFile()
{
	LOGM("CViewVicEditor::OpenDialogExportFile");
	
	if (exportMode == VICEDITOR_EXPORT_HYPER)
	{
		LOGD(" ..... export hyper screen");
		CSlrString *defaultFileName = new CSlrString("picture");

		exportFileDialogMode = VICEDITOR_EXPORT_HYPER;

		CSlrString *windowTitle = new CSlrString("Export HyperScreen Picture");
		viewC64->ShowDialogSaveFile(this, &exportHyperBitmapFileExtensions, defaultFileName, c64SettingsDefaultVicEditorFolder, windowTitle);
		delete windowTitle;
		delete defaultFileName;
		return;
	}
	else if (exportMode == VICEDITOR_EXPORT_PNG)
	{
		LOGD(" ..... export png");
		CSlrString *defaultFileName = new CSlrString("screenshot");

		// multi bitmap
		exportFileDialogMode = VICEDITOR_EXPORT_PNG;
		
		CSlrString *windowTitle = new CSlrString("Export screen to PNG");
		viewC64->ShowDialogSaveFile(this, &exportPNGFileExtensions, defaultFileName, c64SettingsDefaultVicEditorFolder, windowTitle);
		delete windowTitle;
		delete defaultFileName;
		return;
	}
	
	// automatic mode based on current vicii state
	exportMode = GetExportModeFromVicState(&(viewC64->viciiStateToShow));
	
	if (exportMode == VICEDITOR_EXPORT_KOALA)
	{
		LOGD(" ..... export Koala");
		CSlrString *defaultFileName = new CSlrString("picture");

		// multi bitmap
		exportFileDialogMode = VICEDITOR_EXPORT_KOALA;

		CSlrString *windowTitle = new CSlrString("Export Multi-Color KOALA Picture");
		viewC64->ShowDialogSaveFile(this, &exportMultiBitmapFileExtensions, defaultFileName, c64SettingsDefaultVicEditorFolder, windowTitle);
		delete windowTitle;
		delete defaultFileName;
		return;
	}
	else if (exportMode == VICEDITOR_EXPORT_ART_STUDIO)
	{
		LOGD(" ..... export Art Studio");
		CSlrString *defaultFileName = new CSlrString("picture");

		// hires bitmap
		exportFileDialogMode = VICEDITOR_EXPORT_ART_STUDIO;

		CSlrString *windowTitle = new CSlrString("Export Hires ART STUDIO Picture");
		viewC64->ShowDialogSaveFile(this, &exportHiresBitmapFileExtensions, defaultFileName, c64SettingsDefaultVicEditorFolder, windowTitle);
		delete windowTitle;
		delete defaultFileName;
		return;
	}
	else
	{
		LOGD(" ..... export Raw text");
		CSlrString *defaultFileName = new CSlrString("picture");

		// raw text
		exportFileDialogMode = VICEDITOR_EXPORT_RAW_TEXT;

		CSlrString *windowTitle = new CSlrString("Export RAW TEXT Picture");
		viewC64->ShowDialogSaveFile(this, &exportHiresTextFileExtensions, defaultFileName, c64SettingsDefaultVicEditorFolder, windowTitle);
		delete windowTitle;
		delete defaultFileName;
		return;
	}
	
}

// import export
void CViewVicEditor::OpenDialogSaveVCE()
{
	LOGM("CViewVicEditor::OpenDialogSaveVCE");
	CSlrString *defaultFileName = new CSlrString("picture");
	
	// multi bitmap
	exportFileDialogMode = VICEDITOR_EXPORT_VCE;
		
	CSlrString *windowTitle = new CSlrString("Export VicEditor Picture");
	viewC64->ShowDialogSaveFile(this, &exportVCEFileExtensions, defaultFileName, c64SettingsDefaultVicEditorFolder, windowTitle);
	delete windowTitle;
	delete defaultFileName;
}

void CViewVicEditor::SystemDialogFileSaveSelected(CSlrString *path)
{
	LOGD("CViewVicEditor::SystemDialogFileSaveSelected");
	path->DebugPrint("path=");
	
	if (c64SettingsDefaultVicEditorFolder != NULL)
		delete c64SettingsDefaultVicEditorFolder;
	c64SettingsDefaultVicEditorFolder = path->GetFilePathWithoutFileNameComponentFromPath();
	c64SettingsDefaultVicEditorFolder->DebugPrint("c64SettingsDefaultVicEditorFolder=");
	C64DebuggerStoreSettings();

	if (exportFileDialogMode == VICEDITOR_EXPORT_VCE)
	{
		ExportVCE(path);
	}
	else if (exportFileDialogMode == VICEDITOR_EXPORT_HYPER)
	{
		ExportHyper(path);
		ExportSpritesData(path);
	}
	else if (exportFileDialogMode == VICEDITOR_EXPORT_KOALA)
	{
		ExportKoala(path);
		ExportSpritesData(path);
	}
	else if (exportFileDialogMode == VICEDITOR_EXPORT_ART_STUDIO)
	{
		ExportArtStudio(path);
		ExportSpritesData(path);
	}
	else if (exportFileDialogMode == VICEDITOR_EXPORT_RAW_TEXT)
	{
		ExportRawText(path);
		ExportSpritesData(path);
		ExportCharset(path);
	}
	else if (exportFileDialogMode == VICEDITOR_EXPORT_PNG)
	{
		ExportPNG(path);
	}
	
#ifdef WIN32
	// we all love windows, don't we?
	viewC64->KeyUp(MTKEY_LSHIFT, true, false, false, false);
	viewC64->KeyUp(MTKEY_RSHIFT, true, false, false, false);
#endif

}

void CViewVicEditor::SystemDialogFileSaveCancelled()
{
}

///

void CViewVicEditor::EnsureCorrectScreenAndBitmapAddr()
{
	u16 screenBase = viewVicDisplayMain->screenAddress;
	u16 bitmapBase = viewVicDisplayMain->bitmapAddress;

	if (screenBase == 0x0000 || bitmapBase == 0x0000)
	{
//		viewC64->viewC64VicControl->lstScreenAddresses->SetListLocked(true);
//		viewC64->viewC64VicControl->lstScreenAddresses->SetElement(1, true, false);
		
		screenBase = 0x0400;
		viewVicDisplayMain->screenAddress = 0x0400;

//		viewC64->viewC64VicControl->lstBitmapAddresses->SetListLocked(true);
//		viewC64->viewC64VicControl->lstBitmapAddresses->SetElement(1, true, false);
		
		bitmapBase = 0x2000;
		viewVicDisplayMain->bitmapAddress = 0x2000;

		SetVicAddresses(0x0000, screenBase, 0x1000, bitmapBase);
	}

}

void CViewVicEditor::SetVicMode(bool isBitmapMode, bool isMultiColor, bool isExtendedBackground)
{
	this->SetVicModeRegsOnly(isBitmapMode, isMultiColor, isExtendedBackground);

	vicii_cycle_state_t *viciiState = &(viewC64->viciiStateToShow);
	viewVicDisplayMain->SetCurrentCanvas(isBitmapMode, isMultiColor, isExtendedBackground, 1);
	viewVicDisplayMain->currentCanvas->SetViciiState(viciiState);
	viewVicDisplaySmall->SetCurrentCanvas(isBitmapMode, isMultiColor, isExtendedBackground, 1);
	viewVicDisplaySmall->currentCanvas->SetViciiState(viciiState);
}

void CViewVicEditor::SetVicModeRegsOnly(bool isBitmapMode, bool isMultiColor, bool isExtendedBackground)
{
	vicii_cycle_state_t *viciiState = &(viewC64->viciiStateToShow);
	
	u8 d011 = viciiState->regs[0x11];
	u8 d016 = viciiState->regs[0x16];
	
	if (isBitmapMode)
	{
		d011 = (d011 & 0xDF) | 0x20;
	}
	else
	{
		d011 = (d011 & 0xDF);
	}
	
	if (isMultiColor)
	{
		d016 = (d016 & 0xEF) | 0x10;
	}
	else
	{
		d016 = (d016 & 0xEF);
	}

	if (isExtendedBackground)
	{
		d011 = (d011 & 0xBF) | 0x40;
	}
	else
	{
		d011 = (d011 & 0xBF);
	}
	
	viewC64->debugInterfaceC64->SetVicRegister(0x11, d011);
	viewC64->debugInterfaceC64->SetVicRegister(0x16, d016);
}

void CViewVicEditor::SetVicAddresses(int vbank, int screenAddr, int charsetAddr, int bitmapAddr)
{
	LOGD("CViewVicEditor::SetVicAddresses: vbank=%04x screen=%04x charset=%04x bitmap=%04x", vbank, screenAddr, charsetAddr, bitmapAddr);
	vbank = vbank >> 14;
	c64_glue_set_vbank(vbank, 0);

	int screen = (screenAddr - vbank) / 0x0400;
	int charset = (charsetAddr - vbank) / 0x0800;
	int bitmap = (bitmapAddr - vbank) / 0x2000;
	
	int d018 = ((screen << 4) & 0xF0) | ((bitmap << 3) & 0x08) | ((charset << 1) & 0x0E);
	
	viewC64->debugInterfaceC64->SetVicRegister(0x18, d018);

	viewVicDisplayMain->screenAddress = screenAddr;
	viewVicDisplayMain->charsetAddress = charsetAddr;
	viewVicDisplayMain->bitmapAddress = bitmapAddr;

	viewVicDisplaySmall->screenAddress = screenAddr;
	viewVicDisplaySmall->charsetAddress = charsetAddr;
	viewVicDisplaySmall->bitmapAddress = bitmapAddr;

}

bool CViewVicEditor::ImportPNG(CSlrString *path)
{
	guiMain->LockMutex();
	
	// import png
	char *cPath = path->GetStdASCII();
	
	CImageData *imageData = new CImageData(cPath);
	delete [] cPath;

	if (this->selectedLayer == layerReferenceImage || layerReferenceImage->isVisible)
	{
		// load png as reference image
		layerReferenceImage->LoadFrom(imageData);
		
		if (this->selectedLayer == layerReferenceImage)
		{
			CSlrString *str = path->GetFileNameComponentFromPath();
			str->Concatenate(" imported");
			guiMain->ShowMessage(str);
			delete str;

			delete imageData;
			guiMain->UnlockMutex();
			return true;
		}
	}
	
	if (this->selectedLayer == layerC64Canvas ||
		this->selectedLayer == layerC64Screen ||
		layerC64Canvas->isVisible || layerC64Screen->isVisible)
	{
		// import to C64
		if (imageData->width == 320 && imageData->height == 200)
		{
			if (viewVicDisplayMain->currentCanvas->ConvertFrom(imageData) == PAINT_RESULT_OK)
			{
				CSlrString *str = path->GetFileNameComponentFromPath();
				str->Concatenate(" imported");
				guiMain->ShowMessage(str);
				delete str;
			}
			else
			{
				guiMain->ShowMessage("Import failed");
			}
		}
		else if (imageData->width == 384 && imageData->height == 272)
		{
			//		//-32.000000 -35.000000
			CImageData *interiorImage = IMG_CropImageRGBA(imageData, 32, 35, 320, 200);
			
			if (viewVicDisplayMain->currentCanvas->ConvertFrom(interiorImage) != PAINT_RESULT_OK)
			{
				guiMain->UnlockMutex();
				guiMain->ShowMessage("Import failed");
				return false;
			}
			
			delete interiorImage;
			
			viewSprite->selectedColor = -1;
			
			CDebugInterfaceC64 *debugInterface = viewVicDisplayMain->debugInterface;
			
			CImageData *image = viewVicDisplayMain->currentCanvas->ReducePalette(imageData, viewVicDisplayMain);
			
			for (int ry = 0; ry < 272; ry++)
			{
				for (int rx = 0; rx < 384; rx++)
				{
					int px = rx - 32;
					int py = ry - 35;
					u8 paintColor = image->GetPixelResultByte(rx, ry);
					
					C64Sprite *sprite = layerVirtualSprites->FindSpriteByRasterPos(px, py);
					
					if (sprite == NULL)
						continue;
					
					int spc = (px + 0x18)/8.0f;
					int spy = py + 0x32;
					
					vicii_cycle_state_t *viciiState = NULL;
					
					if (spy >= 0 && spy < 312
						&& spc >= 0 && spc < 64)
					{
						viciiState = c64d_get_vicii_state_for_raster_cycle(spy+2, spc);
					}
					
					// TODO: calculate histograms and select colors for spirtes based on most used color pixels
					if (viciiState != NULL)
					{
						u8 paintColorD021	= viciiState->regs[0x21];
						u8 paintColorSprite = viciiState->regs[0x27 + (sprite->spriteId)];
						
						if (sprite->isMulti)
						{
							u8 paintColorD025	= viciiState->regs[0x25];
							u8 paintColorD026	= viciiState->regs[0x26];
							
							if (paintColor == paintColorD021
								|| paintColor == paintColorD025
								|| paintColor == paintColorSprite
								|| paintColor == paintColorD026)
							{
								layerVirtualSprites->Paint(false, false, px, py, paintColor, paintColor, VICEDITOR_COLOR_SOURCE_LMB, 0);
							}
							else
							{
								float dist[4];
								
								dist[0] = GetC64ColorDistance(paintColor, paintColorD021, debugInterface);
								dist[1] = GetC64ColorDistance(paintColor, paintColorD025, debugInterface);
								dist[2] = GetC64ColorDistance(paintColor, paintColorSprite, debugInterface);
								dist[3] = GetC64ColorDistance(paintColor, paintColorD026, debugInterface);
								
								float minDist = 9999.9f;
								int minColorNum = 0;
								
								for (int i = 0; i < 4; i++)
								{
									if (dist[i] < minDist)
									{
										minDist = dist[i];
										minColorNum = i;
									}
								}
								
								paintColor = viewSprite->GetPaintColorByNum(minColorNum);
								layerVirtualSprites->Paint(false, false, px, py, paintColor, paintColor, VICEDITOR_COLOR_SOURCE_LMB, 0);
							}
						}
						else
						{
							if (paintColor == paintColorD021)
							{
								layerVirtualSprites->Paint(false, false, px, py,
														   viewSprite->paintColorD021, paintColorD021, VICEDITOR_COLOR_SOURCE_LMB, 0);
							}
							else
							{
								layerVirtualSprites->Paint(false, false, px, py,
														   viewSprite->paintColorSprite, paintColorSprite, VICEDITOR_COLOR_SOURCE_LMB, 0);
							}
						}
					}
					
					
				}
			}
			
			delete image;
		}
		else
		{
			guiMain->UnlockMutex();
			guiMain->ShowMessage("Image size should be 320x200 or 384x272");

			// scale
			
			delete imageData;
			return false;
		}
	}
	
	CSlrString *str = path->GetFileNameComponentFromPath();
	str->Concatenate(" imported");
	guiMain->ShowMessage(str);
	delete str;

	delete imageData;
	
	guiMain->UnlockMutex();
	
	return true;
}

bool CViewVicEditor::ImportKoala(CSlrString *path, bool showMessage)
{
	guiMain->LockMutex();
	
	// sanity-check default VIC Display settings
	u8 *screen_ptr;
	u8 *color_ram_ptr;
	u8 *chargen_ptr;
	u8 *bitmap_low_ptr;
	u8 *bitmap_high_ptr;
	u8 d020colors[0x0F];
	
	viewVicDisplayMain->GetViciiPointers(&(viewC64->viciiStateToShow),
										 &screen_ptr, &color_ram_ptr, &chargen_ptr, &bitmap_low_ptr, &bitmap_high_ptr, d020colors);
	
	
	EnsureCorrectScreenAndBitmapAddr();

	u16 bitmapBase = viewVicDisplayMain->bitmapAddress;
	u16 screenBase = viewVicDisplayMain->screenAddress;
	
//	viewC64->viewC64VicControl->btnModeText->SetOn(false);
//	viewC64->viewC64VicControl->btnModeBitmap->SetOn(true);
//	viewC64->viewC64VicControl->btnModeHires->SetOn(false);
//	viewC64->viewC64VicControl->btnModeMulti->SetOn(true);

	SetVicMode(true, true, false);
	
	u8 bkgD020, bkgD021;
	ImportKoala(path, bitmapBase, screenBase, 0xD800, &bkgD020, &bkgD021);
	
	viewC64->debugInterfaceC64->SetVicRegister(0x20, bkgD020);
	viewC64->debugInterfaceC64->SetVicRegister(0x21, bkgD021);
	
	guiMain->UnlockMutex();
	
	if (showMessage)
	{
		CSlrString *str = path->GetFileNameComponentFromPath();
		str->Concatenate(" loaded");
		guiMain->ShowMessage(str);
		delete str;
	}

	return true;
}

bool CViewVicEditor::ImportKoala(CSlrString *path, u16 bitmapAddress, u16 screenAddress, u16 colorRamAddress, u8 *colorD020, u8 *colorD021)
{
	// load file
	char *cPath = path->GetStdASCII();
	CSlrFileFromOS *file = new CSlrFileFromOS(cPath);
	delete [] cPath;
	
	if (!file->Exists())
	{
		delete file;
		guiMain->ShowMessage("File not found");
		return false;
	}
	
	CByteBuffer *byteBuffer = new CByteBuffer();
	byteBuffer->readFromFileNoHeader(file);

	// 2-bytes loading addr
	byteBuffer->GetU8();
	byteBuffer->GetU8();

	u16 ptr;
	
	// load bitmap data
	ptr = bitmapAddress;
	for (int i = 0; i < 0x1F40; i++)
	{
		u8 v = byteBuffer->GetU8();
		
		viewVicDisplayMain->debugInterface->SetByteC64(ptr, v);
		ptr++;
	}
	
	ptr = screenAddress;
	for (int i = 0; i < 0x03E8; i++)
	{
		u8 v = byteBuffer->GetU8();
		
		viewVicDisplayMain->debugInterface->SetByteC64(ptr, v);
		ptr++;
	}

	ptr = colorRamAddress;
	for (int i = 0; i < 0x03E8; i++)
	{
		u8 v = byteBuffer->GetU8();
		
		viewVicDisplayMain->debugInterface->SetByteC64(ptr, v);
		ptr++;
	}

	u8 bkg = byteBuffer->GetU8();
	
	*colorD020 = bkg;
	*colorD021 = bkg;
	
	return true;
}

bool CViewVicEditor::ImportDoodle(CSlrString *path)
{
	char *cPath = path->GetStdASCII();
	CSlrFileFromOS *file = new CSlrFileFromOS(cPath);
	delete [] cPath;
	
	if (!file->Exists())
	{
		delete file;
		guiMain->ShowMessage("File not found");
		return false;
	}
	
	CByteBuffer *byteBuffer = new CByteBuffer();
	byteBuffer->readFromFileNoHeader(file);
	
	guiMain->LockMutex();
	
	// sanity-check default VIC Display settings
	u8 *screen_ptr;
	u8 *color_ram_ptr;
	u8 *chargen_ptr;
	u8 *bitmap_low_ptr;
	u8 *bitmap_high_ptr;
	u8 d020colors[0x0F];
	
	viewVicDisplayMain->GetViciiPointers(&(viewC64->viciiStateToShow),
										 &screen_ptr, &color_ram_ptr, &chargen_ptr, &bitmap_low_ptr, &bitmap_high_ptr, d020colors);
	
	EnsureCorrectScreenAndBitmapAddr();
	u16 bitmapBase = viewVicDisplayMain->bitmapAddress;
	u16 screenBase = viewVicDisplayMain->screenAddress;
	
//	viewC64->viewC64VicControl->btnModeText->SetOn(false);
//	viewC64->viewC64VicControl->btnModeBitmap->SetOn(true);
//	viewC64->viewC64VicControl->btnModeHires->SetOn(true);
//	viewC64->viewC64VicControl->btnModeMulti->SetOn(false);
	
	SetVicMode(true, false, false);

	// 2-bytes loading addr
	byteBuffer->GetU8();
	byteBuffer->GetU8();

	u16 ptr;

	// load screen data $5C00-$5FE7
	ptr = screenBase;
	for (int i = 0; i < 0x03E8; i++)
	{
		u8 v = byteBuffer->GetU8();
		
		viewVicDisplayMain->debugInterface->SetByteC64(ptr, v);
		ptr++;
	}
	
	// unused 8 bytes
	for (int i = 0; i < 8; i++)
	{
		byteBuffer->GetU8();
	}
	
	// load bitmap data $6000-$7F3F
	ptr = bitmapBase;
	for (int i = 0; i < 0x1F40; i++)
	{
		u8 v = byteBuffer->GetU8();
		
		viewVicDisplayMain->debugInterface->SetByteC64(ptr, v);
		ptr++;
	}
	
	guiMain->UnlockMutex();
	
	CSlrString *str = path->GetFileNameComponentFromPath();
	str->Concatenate(" loaded");
	guiMain->ShowMessage(str);
	delete str;

	return true;
}

bool CViewVicEditor::ImportArtStudio(CSlrString *path)
{
	char *cPath = path->GetStdASCII();
	CSlrFileFromOS *file = new CSlrFileFromOS(cPath);
	delete [] cPath;
	
	if (!file->Exists())
	{
		delete file;
		guiMain->ShowMessage("File not found");
		return false;
	}
	
	CByteBuffer *byteBuffer = new CByteBuffer();
	byteBuffer->readFromFileNoHeader(file);
	
	guiMain->LockMutex();
	
	// sanity-check default VIC Display settings
	u8 *screen_ptr;
	u8 *color_ram_ptr;
	u8 *chargen_ptr;
	u8 *bitmap_low_ptr;
	u8 *bitmap_high_ptr;
	u8 d020colors[0x0F];
	
	viewVicDisplayMain->GetViciiPointers(&(viewC64->viciiStateToShow),
										 &screen_ptr, &color_ram_ptr, &chargen_ptr, &bitmap_low_ptr, &bitmap_high_ptr, d020colors);
	
	EnsureCorrectScreenAndBitmapAddr();
	u16 bitmapBase = viewVicDisplayMain->bitmapAddress;
	u16 screenBase = viewVicDisplayMain->screenAddress;
	
//	viewC64->viewC64VicControl->btnModeText->SetOn(false);
//	viewC64->viewC64VicControl->btnModeBitmap->SetOn(true);
//	viewC64->viewC64VicControl->btnModeHires->SetOn(true);
//	viewC64->viewC64VicControl->btnModeMulti->SetOn(false);
	
	SetVicMode(true, false, false);

	// 2-bytes loading addr
	byteBuffer->GetU8();
	byteBuffer->GetU8();
	
	u16 ptr;
	
	// load bitmap data $2000-$3F3F
	ptr = bitmapBase;
	for (int i = 0; i < 0x1F40; i++)
	{
		u8 v = byteBuffer->GetU8();
		
		viewVicDisplayMain->debugInterface->SetByteC64(ptr, v);
		ptr++;
	}
	
	// load screen data $3F40-$4327
	ptr = screenBase;
	for (int i = 0; i < 0x03E8; i++)
	{
		u8 v = byteBuffer->GetU8();
		
		viewVicDisplayMain->debugInterface->SetByteC64(ptr, v);
		ptr++;
	}
	
	u8 bkg = byteBuffer->GetU8();

	viewC64->debugInterfaceC64->SetVicRegister(0x20, bkg);
	viewC64->debugInterfaceC64->SetVicRegister(0x21, bkg);
	
	guiMain->UnlockMutex();
	
	CSlrString *str = path->GetFileNameComponentFromPath();
	str->Concatenate(" loaded");
	guiMain->ShowMessage(str);
	delete str;

	return true;
}

bool CViewVicEditor::ExportKoala(CSlrString *path)
{
	LOGD("CViewVicEditor::ExportKoala");
	path->DebugPrint("ExportKoala path=");
	
	guiMain->LockMutex();

	char *cPath = path->GetStdASCII();
	LOGD(" ..... cPath='%s'", cPath);

	CSlrFileFromOS *file = new CSlrFileFromOS(cPath, SLR_FILE_MODE_WRITE);
	delete [] cPath;
	
	u8 *screen_ptr;
	u8 *color_ram_ptr;
	u8 *chargen_ptr;
	u8 *bitmap_low_ptr;
	u8 *bitmap_high_ptr;
	u8 d020colors[0x0F];
	
	viewVicDisplayMain->GetViciiPointers(&(viewC64->viciiStateToShow),
										 &screen_ptr, &color_ram_ptr, &chargen_ptr, &bitmap_low_ptr, &bitmap_high_ptr, d020colors);
	
	// load address
	file->WriteByte(0x00);
	file->WriteByte(0x60);
	
	//
	file->Write(bitmap_low_ptr,  0x1000);
	file->Write(bitmap_high_ptr, 0x0F40);

	//
	file->Write(screen_ptr, 0x03E8);
	
	//
	file->Write(color_ram_ptr, 0x03E8);
	
	file->WriteByte(d020colors[1]);

	file->Close();

	guiMain->UnlockMutex();

	CSlrString *str = path->GetFileNameComponentFromPath();
	str->Concatenate(" saved");
	guiMain->ShowMessage(str);
	delete str;
	
	LOGM("CViewVicEditor::ExportKoala: file saved");

	return true;
}

bool CViewVicEditor::ExportArtStudio(CSlrString *path)
{
	LOGD("CViewVicEditor::ExportArtStudio");

	path->DebugPrint("ExportArtStudio path=");

	guiMain->LockMutex();
	
	char *cPath = path->GetStdASCII();
	LOGD(" ..... cPath='%s'", cPath);

	CSlrFileFromOS *file = new CSlrFileFromOS(cPath, SLR_FILE_MODE_WRITE);
	delete [] cPath;
	
	u8 *screen_ptr;
	u8 *color_ram_ptr;
	u8 *chargen_ptr;
	u8 *bitmap_low_ptr;
	u8 *bitmap_high_ptr;
	u8 d020colors[0x0F];
	
	viewVicDisplayMain->GetViciiPointers(&(viewC64->viciiStateToShow),
										 &screen_ptr, &color_ram_ptr, &chargen_ptr, &bitmap_low_ptr, &bitmap_high_ptr, d020colors);
	
	// load address
	file->WriteByte(0x00);
	file->WriteByte(0x20);
	
	//
	file->Write(bitmap_low_ptr,  0x1000);
	file->Write(bitmap_high_ptr, 0x0F40);
	
	//
	file->Write(screen_ptr, 0x03E8);
	
	file->WriteByte(d020colors[1]);
	
	file->Close();
	
	guiMain->UnlockMutex();
	
	CSlrString *str = path->GetFileNameComponentFromPath();
	str->Concatenate(" saved");
	guiMain->ShowMessage(str);
	delete str;
	
	LOGM("CViewVicEditor::ExportArtStudio: file saved");

	return true;
}

bool CViewVicEditor::ExportRawText(CSlrString *path)
{
	LOGM("CViewVicEditor::ExportRawText");

	path->DebugPrint("ExportRawText path=");

	guiMain->LockMutex();
	
	char *cPath = path->GetStdASCII();
	LOGD(" ..... cPath='%s'", cPath);

	CSlrFileFromOS *file = new CSlrFileFromOS(cPath, SLR_FILE_MODE_WRITE);
	delete [] cPath;
	
	u8 *screen_ptr;
	u8 *color_ram_ptr;
	u8 *chargen_ptr;
	u8 *bitmap_low_ptr;
	u8 *bitmap_high_ptr;
	u8 d020colors[0x0F];
	
	viewVicDisplayMain->GetViciiPointers(&(viewC64->viciiStateToShow),
										 &screen_ptr, &color_ram_ptr, &chargen_ptr, &bitmap_low_ptr, &bitmap_high_ptr, d020colors);
	
	//
	file->Write(screen_ptr, 0x03E8);
	file->Write(color_ram_ptr, 0x03E8);

	file->WriteByte(d020colors[1]);
	file->WriteByte(d020colors[0]);
	
	file->Close();
	
	guiMain->UnlockMutex();
	
	CSlrString *str = path->GetFileNameComponentFromPath();
	str->Concatenate(" saved");
	guiMain->ShowMessage(str);
	delete str;
	
	LOGM("CViewVicEditor::ExportRawText: file saved");

	return true;

}

bool CViewVicEditor::ExportCharset(CSlrString *path)
{
	LOGM("CViewVicEditor::ExportCharset");
	
	path->DebugPrint("ExportCharset path=");

	guiMain->LockMutex();
	
	char *cPath;
	
	//
	CSlrString *pathNoExt = path->GetFilePathWithoutExtension();
	pathNoExt->DebugPrint("pathNoExt=");
	
	CSlrString *pathCharset = new CSlrString(pathNoExt);
	char buf[64];
	sprintf(buf, ".charset"); //-charset.data"); //.charset"); //-charset.data");
	
	pathCharset->Concatenate(buf);
	cPath = pathCharset->GetStdASCII();
	LOGD(" ..... cPath='%s'", cPath);
	
	
	CSlrFileFromOS *file = new CSlrFileFromOS(cPath, SLR_FILE_MODE_WRITE);
	delete [] cPath;
	delete pathCharset;
	delete pathNoExt;

	u8 *screen_ptr;
	u8 *color_ram_ptr;
	u8 *chargen_ptr;
	u8 *bitmap_low_ptr;
	u8 *bitmap_high_ptr;
	u8 d020colors[0x0F];
	
	viewVicDisplayMain->GetViciiPointers(&(viewC64->viciiStateToShow),
										 &screen_ptr, &color_ram_ptr, &chargen_ptr, &bitmap_low_ptr, &bitmap_high_ptr, d020colors);
	
	//
	file->Write(chargen_ptr, 0x0800);
	
	file->Close();
	
	guiMain->UnlockMutex();
	
	LOGM("CViewVicEditor::ExportCharset: file saved");
	
	return true;
	
}

bool CViewVicEditor::ExportHyper(CSlrString *path)
{
	guiMain->LockMutex();
	
	char *cPath = path->GetStdASCII();
	CSlrFileFromOS *file = new CSlrFileFromOS(cPath, SLR_FILE_MODE_WRITE);
	delete [] cPath;
	
	u8 *screen_ptr;
	u8 *color_ram_ptr;
	u8 *chargen_ptr;
	u8 *bitmap_low_ptr;
	u8 *bitmap_high_ptr;
	u8 d020colors[0x0F];
	
	viewVicDisplayMain->GetViciiPointers(&(viewC64->viciiStateToShow),
										 &screen_ptr, &color_ram_ptr, &chargen_ptr, &bitmap_low_ptr, &bitmap_high_ptr, d020colors);
	
//	// load address
//	file->WriteByte(0x00);
//	file->WriteByte(0x20);
	
	// sprites
	for (int i = 0x4C00; i < 0x5C00; i++)
	{
		u8 v = viewVicDisplayMain->debugInterface->GetByteFromRamC64(i);
		file->WriteByte(v);
	}
	
	
	// screen 5c00
	file->Write(screen_ptr, 0x03E8);
	
	for (int i = 0; i < 0x18; i++)
	{
		file->WriteByte(0x00);
	}
	
	// bitmap 6000
	file->Write(bitmap_low_ptr,  0x1000);
	file->Write(bitmap_high_ptr, 0x0F40);
	
	//
	file->Write(color_ram_ptr, 0x03E8);
	
//	FILE *fp = fopen("/Users/mars/Desktop/c", "wb");
//	fwrite(color_ram_ptr, 1, 0x03E8, fp);
//	fclose(fp);
	
	file->Close();
	
	guiMain->UnlockMutex();
	
	guiMain->ShowMessage("File saved");
	
	return true;
}

//
bool CViewVicEditor::ExportPNG(CSlrString *path)
{
	LOGD("CViewVicEditor::ExportPNG");
	path->DebugPrint("ExportPNG path=");
	
	guiMain->LockMutex();
	viewC64->debugInterfaceC64->LockRenderScreenMutex();
	
	// refresh texture of C64's screen
	CImageData *c64Screen = viewC64->debugInterfaceC64->GetScreenImageData();
	CImageData *imageCrop = IMG_CropSupersampleImageRGBA(c64Screen,
														 viewC64->debugInterfaceC64->screenSupersampleFactor,
														 0, 0, 384, 272);

	viewVicDisplayMain->RefreshScreenImageData(&(viewC64->viciiStateToShow), 255, 255);
	layerVirtualSprites->SimpleScanSpritesInThisFrame();
	
	char *cPath;
	
	// first save c64 screen as-is
	cPath = path->GetStdASCII();
	LOGD(" ..... cPath='%s'", cPath);

	imageCrop->Save(cPath);
	
	delete [] cPath;
	delete imageCrop;

	//
	CSlrString *pathNoExt = path->GetFilePathWithoutExtension();
	pathNoExt->DebugPrint("pathNoExt=");

	
	// then save display
	CSlrString *pathDisplay = new CSlrString(pathNoExt);
	pathDisplay->Concatenate("-display.png");
	cPath = pathDisplay->GetStdASCII();
	LOGD(" ..... display cPath='%s'", cPath);

	imageCrop = IMG_CropImageRGBA(viewVicDisplayMain->imageDataScreen, 0, 0, 320, 200);
	
	imageCrop->Save(cPath);
	
	delete [] cPath;
	delete pathDisplay;
	delete imageCrop;

	// then unrestricted
	if (layerUnrestrictedBitmap->NumVisiblePixels() > 0)
	{
		CSlrString *pathUnrestricted = new CSlrString(pathNoExt);
		pathDisplay->Concatenate("-unrestricted.png");
		cPath = pathUnrestricted->GetStdASCII();
		LOGD(" ..... unrestricted cPath='%s'", cPath);
		
		imageCrop = IMG_CropImageRGBA(layerUnrestrictedBitmap->imageDataUnrestricted, 0, 0, 320, 200);
		
		imageCrop->Save(cPath);
		
		delete [] cPath;
		delete pathUnrestricted;
		delete imageCrop;		
	}

	// and sprites
	for (std::list<C64Sprite *>::iterator it = this->layerVirtualSprites->sprites.begin();
		 it != this->layerVirtualSprites->sprites.end(); it++)
	{
		C64Sprite *sprite = *it;

		int spc = (sprite->posX + 0x18)/8.0f;
		int spy = sprite->posY + 0x32;

		vicii_cycle_state_t *viciiState = NULL;
		
		if (spy >= 0 && spy < 312
			&& spc >= 0 && spc < 64)
		{
			viciiState = c64d_get_vicii_state_for_raster_cycle(spy+2, spc);
		}

		if (viciiState != NULL)
		{
			u8 paintColorD021	= viciiState->regs[0x21];
			u8 paintColorD025	= viciiState->regs[0x25];
			u8 paintColorD026	= viciiState->regs[0x26];
			
			u8 paintColorSprite = viciiState->regs[0x27 + (sprite->spriteId)];
			
			int spriteId = sprite->spriteId;
			
			int addr1 = sprite->pointerAddr;
			
			int v_bank = viciiState->vbank_phi1;
			int addr2 = v_bank + viciiState->sprite[sprite->spriteId].pointer * 64;
			
			int sprx = sprite->posX + 0x18;
			int spry = sprite->posY + 0x32;
			
			if (sprx < 0)
				sprx += 504;
			
			int addr = addr2;
			
			uint8 spriteData[63];
			for (int i = 0; i < 63; i++)
			{
				u8 v = viewVicDisplayMain->debugInterface->GetByteFromRamC64(addr);
				spriteData[i] = v;
				addr++;
			}
			
			CImageData *imageDataSprite = new CImageData(24, 21, IMG_TYPE_RGBA);
			imageDataSprite->AllocImage(false, true);

			bool isColor = false;
			if (viciiState->regs[0x1c] & (1 << (sprite->spriteId)))
			{
				isColor = true;
			}
			if (isColor == false)
			{
				uint8 spriteColor = viciiState->regs[0x27+(sprite->spriteId)];
				
				ConvertSpriteDataToImage(spriteData, imageDataSprite, paintColorD021, spriteColor, viewVicDisplayMain->debugInterface, 0);
			}
			else
			{
				ConvertColorSpriteDataToImage(spriteData, imageDataSprite,
											  paintColorD021, paintColorD025, paintColorD026, paintColorSprite,
											  viewVicDisplayMain->debugInterface, 0, 0);
			}
			
			//
			CSlrString *pathSprite = new CSlrString(pathNoExt);
			char buf[64];
			sprintf(buf, "-sprite-%03d-%03d-%d.png", sprx, spry, sprite->spriteId);
			
			pathSprite->Concatenate(buf);
			cPath = pathSprite->GetStdASCII();
			LOGD(" ..... sprite cPath='%s'", cPath);
			
			imageDataSprite->Save(cPath);
			
			delete [] cPath;
			delete pathSprite;
			delete imageDataSprite;

			// export raw data
			pathSprite = new CSlrString(pathNoExt);
			sprintf(buf, "-sprite-%03d-%03d-%d.sprite", sprx, spry, sprite->spriteId);
			
			pathSprite->Concatenate(buf);
			cPath = pathSprite->GetStdASCII();
			LOGD(" ..... sprite raw cPath='%s'", cPath);
			
			FILE *fp = fopen(cPath, "wb");
			if (fp)
			{
				fwrite(spriteData, 64, 1, fp);
				fclose(fp);
			}

			// TODO: export sprite colors and flags
			LOGTODO("export sprite colors and flags");

			delete [] cPath;
			delete pathSprite;

		}
		
	}
	

	
	//
	viewC64->debugInterfaceC64->UnlockRenderScreenMutex();
	guiMain->UnlockMutex();
	
	CSlrString *str = path->GetFileNameComponentFromPath();
	str->Concatenate(" exported");
	guiMain->ShowMessage(str);
	delete str;
	
	LOGM("CViewVicEditor::ExportPNG: file saved");
	
	return true;
}

//
bool CViewVicEditor::ExportSpritesData(CSlrString *path)
{
	LOGD("CViewVicEditor::ExportSpritesData");
	
	guiMain->LockMutex();
	viewC64->debugInterfaceC64->LockRenderScreenMutex();
	
	// refresh texture of C64's screen
	layerVirtualSprites->SimpleScanSpritesInThisFrame();
	
	char *cPath;
	
	//
	CSlrString *pathNoExt = path->GetFilePathWithoutExtension();
	pathNoExt->DebugPrint("pathNoExt=");
	
	// export sprites
	for (std::list<C64Sprite *>::iterator it = this->layerVirtualSprites->sprites.begin();
		 it != this->layerVirtualSprites->sprites.end(); it++)
	{
		C64Sprite *sprite = *it;
		
		int spc = (sprite->posX + 0x18)/8.0f;
		int spy = sprite->posY + 0x32;
		
		vicii_cycle_state_t *viciiState = NULL;
		
		if (spy >= 0 && spy < 312
			&& spc >= 0 && spc < 64)
		{
			viciiState = c64d_get_vicii_state_for_raster_cycle(spy+2, spc);
		}
		
		if (viciiState != NULL)
		{
			int v_bank = viciiState->vbank_phi1;
			int addr2 = v_bank + viciiState->sprite[sprite->spriteId].pointer * 64;
			
			int sprx = sprite->posX + 0x18;
			int spry = sprite->posY + 0x32;
			
			if (sprx < 0)
				sprx += 504;
			
			int addr = addr2;
			
			uint8 spriteData[63];
			for (int i = 0; i < 63; i++)
			{
				u8 v = viewVicDisplayMain->debugInterface->GetByteFromRamC64(addr);
				spriteData[i] = v;
				addr++;
			}
			
			CSlrString *pathSprite = new CSlrString(pathNoExt);
			char buf[64];
			sprintf(buf, "-sprite-%03d-%03d-%d.sprite", sprx, spry, sprite->spriteId);
			
			pathSprite->Concatenate(buf);
			cPath = pathSprite->GetStdASCII();
			LOGD(" ..... cPath='%s'", cPath);
			
			FILE *fp = fopen(cPath, "wb");
			if (fp)
			{
				fwrite(spriteData, 64, 1, fp);
				fclose(fp);
			}
			
			// TODO: export sprite colors and flags
			LOGTODO("export sprite colors and flags");
			
			delete [] cPath;
			delete pathSprite;
		}
		
	}
	
	
	
	//
	viewC64->debugInterfaceC64->UnlockRenderScreenMutex();
	guiMain->UnlockMutex();
	
	LOGM("CViewVicEditor::ExportSpritesData: file saved");
	
	return true;
}

void CViewVicEditor::ResetSmallDisplayScale(double newRealScale)
{
	LOGD("CViewVicEditor::ResetSmallDisplayScale, scale=%f", newRealScale);
	
	// OS size of pixels in the viewport
	double realPixelSizeX, realPixelSizeY;

	GUI_GetRealScreenPixelSizes(&realPixelSizeX, &realPixelSizeY);

	double newScale = newRealScale * realPixelSizeX;
	
	if (viewVicDisplaySmall->showDisplayBorderType == VIC_DISPLAY_SHOW_BORDER_VISIBLE_AREA)
	{
		newScale /= 0.7353;
	}
	
	float px = viewVicDisplaySmall->posX;
	float py = viewVicDisplaySmall->posY;
	viewVicDisplaySmall->SetDisplayPosition(px, py, newScale, true);
	
	char *buf = SYS_GetCharBuf();
	sprintf(buf, "%d%%", (int)(newRealScale * 100.0));
	CSlrString *barTitle = new CSlrString(buf);
	viewVicDisplaySmall->viewFrame->SetBarTitle(barTitle);
	SYS_ReleaseCharBuf(buf);
}

// undo
void CViewVicEditor::DebugPrintUndo(char *header)
{
	return;
	
//	int i = 0;
//	
//	LOGD(" ######## %s #######", header);
//	LOGD("    --- undo list (%d):", undoList.size());
//	i = 0;
//	for (std::list<CByteBuffer *>::iterator it = undoList.begin(); it != undoList.end(); it++)
//	{
//		CByteBuffer *b = *it;
//		LOGD("%d:    %x", i++, b);
//	}
//	LOGD("    --- redo list (%d):", redoList.size());
//	i = 0;
//	for (std::list<CByteBuffer *>::iterator it = redoList.begin(); it != redoList.end(); it++)
//	{
//		CByteBuffer *b = *it;
//		LOGD("%d:    %x", i++, b);
//	}
//	LOGD(" ###############################");
}

void CViewVicEditor::StoreUndo()
{
	LOGD(">> CViewVicEditor::StoreUndo <<");
	
	guiMain->LockMutex();
	
	DebugPrintUndo("before StoreUndo");

	// move all redo back to pool
	while(!redoList.empty())
	{
		CByteBuffer *b = redoList.back();
		redoList.pop_back();
		poolList.push_back(b);
	}
	
	CByteBuffer *buffer = NULL;
	
	if (poolList.empty())
	{
		if (undoList.empty())
		{
			// should never happen
			buffer = new CByteBuffer();
			LOGError("CViewVicEditor::StoreUndo: no buffers available");
		}
		else
		{
			buffer = undoList.front();
			undoList.pop_front();
		}
	}
	else
	{
		buffer = poolList.front();
		poolList.pop_front();
	}
	
	LOGD("   store now undo buffer=%x", buffer);
	
	buffer->Reset();
	
	this->Serialise(buffer, true, true, true);
	
	undoList.push_back(buffer);

	DebugPrintUndo("after StoreUndo");

	guiMain->UnlockMutex();
	
}

void CViewVicEditor::DoUndo()
{
	LOGD("<< CViewVicEditor::DoUndo >>");
	
	int i = 0;
	
	DebugPrintUndo("before DoUndo");

	if (undoList.empty())
	{
		LOGD("   no undos");
		return;
	}
	
	EnsureCorrectScreenAndBitmapAddr();

	// store current state for redo
	CByteBuffer *buffer = NULL;
	
	if (poolList.empty())
	{
		if (redoList.empty())
		{
			// should never happen
			buffer = new CByteBuffer();
			LOGError("CViewVicEditor::DoUndo: no buffers available");
		}
		else
		{
			buffer = redoList.front();
			redoList.pop_front();
		}
	}
	else
	{
		buffer = poolList.front();
		poolList.pop_front();
	}
	
	LOGD("   store now redo buffer=%x", buffer);
	
	buffer->Reset();
	
	this->Serialise(buffer, true, true, true);
	
	redoList.push_back(buffer);

	///
	
	buffer = undoList.back();
	undoList.pop_back();

	
	DebugPrintUndo("after DoUndo");

	
	LOGD("   restore now buffer is=%x", buffer);

	buffer->Rewind();
	
	this->Deserialise(buffer, VIC_EDITOR_FILE_VERSION);
	
	poolList.push_back(buffer);
}

void CViewVicEditor::DoRedo()
{
	LOGD("<< CViewVicEditor::DoRedo >>");
	int i = 0;
	
	DebugPrintUndo("before DoRedo");


	if (redoList.empty())
	{
		LOGD("   no redos");
		return;
	}
	
	EnsureCorrectScreenAndBitmapAddr();

	//
	CByteBuffer *buffer = NULL;
	
	if (poolList.empty())
	{
		if (undoList.empty())
		{
			// should never happen
			buffer = new CByteBuffer();
			LOGError("CViewVicEditor::StoreUndo: no buffers available");
		}
		else
		{
			buffer = undoList.front();
			undoList.pop_front();
		}
	}
	else
	{
		buffer = poolList.front();
		poolList.pop_front();
	}
	
	LOGD("   store now undo buffer=%x", buffer);
	
	buffer->Reset();
	
	this->Serialise(buffer, true, true, true);
	
	undoList.push_back(buffer);

	//
	
	buffer = redoList.back();
	redoList.pop_back();

	
	DebugPrintUndo("after DoRedo");

	
	LOGD("   redo restore now buffer=%x", buffer);

	buffer->Rewind();

	this->Deserialise(buffer, VIC_EDITOR_FILE_VERSION);
	
	poolList.push_back(buffer);
}

////////////

bool CViewVicEditor::ExportVCE(CSlrString *path)
{
	LOGD("CViewVicEditor::ExportVCE");
	
	guiMain->LockMutex();
	
	char *cPath = path->GetStdASCII();
	LOGD(" ..... cPath='%s'", cPath);
	
	CSlrFileFromOS *file = new CSlrFileFromOS(cPath, SLR_FILE_MODE_WRITE);
	delete [] cPath;
	
	//
	
	CByteBuffer *byteBuffer = new CByteBuffer();
	
	byteBuffer->PutU32(VIC_EDITOR_FILE_MAGIC);
	byteBuffer->PutU32(VIC_EDITOR_FILE_VERSION);
	
	CByteBuffer *serialiseBuffer = new CByteBuffer();
	this->Serialise(serialiseBuffer, true, true, true);
	
	// uncompressed size
	byteBuffer->PutU32(serialiseBuffer->length);
	
	// compress
	uLong outBufferSize = compressBound(serialiseBuffer->length);
	u8 *outBuffer = new u8[outBufferSize];
	
	int result = compress2(outBuffer, &outBufferSize, serialiseBuffer->data, serialiseBuffer->length, 9);
	
	if (result != Z_OK)
	{
		guiMain->UnlockMutex();

		LOGError("zlib error: %d", result);
		guiMain->ShowMessage("zlib error");
		delete [] outBuffer;
		delete serialiseBuffer;
		delete byteBuffer;
		delete file;
		return false;
	}
	
	u32 outSize = (u32)outBufferSize;
	
	LOGD("..original size=%d compressed=%d", serialiseBuffer->length, outSize);
	
	//byteBuffer->putU32(outSize);
	byteBuffer->putBytes(outBuffer, outSize);
	
	//
	byteBuffer->storeToFileNoHeader(file);
	
	file->Close();
	
	delete [] outBuffer;
	delete serialiseBuffer;
	delete byteBuffer;
	delete file;
	
	guiMain->UnlockMutex();
	
	CSlrString *str = path->GetFileNameComponentFromPath();
	str->Concatenate(" saved");
	guiMain->ShowMessage(str);
	delete str;
	
	LOGM("CViewVicEditor::ExportKoala: file saved");
	
	return true;
}

bool CViewVicEditor::ImportVCE(CSlrString *path)
{
	LOGD("CViewVicEditor::ImportVCE");
	
	char *cPath = path->GetStdASCII();
	CSlrFileFromOS *file = new CSlrFileFromOS(cPath);
	delete [] cPath;
	
	if (!file->Exists())
	{
		delete file;
		guiMain->ShowMessage("File not found");
		return false;
	}
	
	guiMain->LockMutex();
	
	u32 magic = file->ReadUnsignedInt();
	if (magic != VIC_EDITOR_FILE_MAGIC)
	{
		guiMain->UnlockMutex();

		guiMain->ShowMessage("VCE format error");
		delete file;
		return false;
	}
	
	u32 fileVersion = file->ReadUnsignedInt();
	if (fileVersion > VIC_EDITOR_FILE_VERSION)
	{
		guiMain->UnlockMutex();

		char *buf = SYS_GetCharBuf();
		sprintf(buf, "File v%d higher than supported %d.", fileVersion, VIC_EDITOR_FILE_VERSION);
		guiMain->ShowMessage(buf);
		SYS_ReleaseCharBuf(buf);
		delete file;
		return false;
	}
	
	// load
	u32 uncompressedSize = file->ReadUnsignedInt();
	u8 *uncompressedData = new u8[uncompressedSize];
	
	CSlrFileZlib *fileZlib = new CSlrFileZlib(file);
	fileZlib->Read(uncompressedData, uncompressedSize);
	
	delete fileZlib;
	
	LOGD("... uncompressedSize=%d", uncompressedSize);
	
	
	// load
	viewC64->debugInterfaceC64->SetDebugMode(DEBUGGER_MODE_RUNNING);
	
	viewC64->viewC64VicControl->UnlockAll();
	
	viewC64->debugInterfaceC64->SetPatchKernalFastBoot(true);
	
	viewC64->debugInterfaceC64->DetachCartridge();
	//	viewC64->debugInterfaceC64->HardReset();

	SYS_Sleep(350);
	
	this->RunC64EndlessLoop();
	
	CByteBuffer *serialiseBuffer = new CByteBuffer(uncompressedData, uncompressedSize);
	this->Deserialise(serialiseBuffer, fileVersion);
	
	delete serialiseBuffer;
	
	delete file;
	
	UpdateReferenceLayers();
	
	
	guiMain->UnlockMutex();
	
	CSlrString *str = path->GetFileNameComponentFromPath();
	str->Concatenate(" loaded");
	guiMain->ShowMessage(str);
	delete str;

	return true;
}

void CViewVicEditor::RunC64EndlessLoop()
{
	// add at 0002 78	SEI		0003 4C 01 09 JMP $0003
	viewC64->debugInterfaceC64->SetByteToRamC64(0x0002, 0x78);	// SEI
	viewC64->debugInterfaceC64->SetByteToRamC64(0x0003, 0x4C);	// JMP $0901
	viewC64->debugInterfaceC64->SetByteToRamC64(0x0004, 0x03);
	viewC64->debugInterfaceC64->SetByteToRamC64(0x0005, 0x00);
	viewC64->debugInterfaceC64->MakeJmpC64(0x0002);
}

void CViewVicEditor::Serialise(CByteBuffer *byteBuffer, bool storeVicRegisters, bool storeC64Memory, bool storeVicDisplayControlState)
{
	vicii_cycle_state_t *viciiState = &(viewC64->viciiStateToShow);
	
	///
	u8 mc;
	u8 eb;
	u8 bm;
	u8 blank;
	
	mc = (viciiState->regs[0x16] & 0x10) >> 4;
	bm = (viciiState->regs[0x11] & 0x20) >> 5;
	eb = (viciiState->regs[0x11] & 0x40) >> 6;
	
	viewC64->viewC64VicControl->RefreshStateButtonsUI(&mc, &eb, &bm, &blank);

	bool isMultiColor = mc;
	bool isBitmap = bm;
	bool isExtColor = eb;

	///
	
	// screen addr
	uint16 screen_addr = viciiState->vbank_phi2 + ((viciiState->regs[0x18] & 0xf0) << 6);
	screen_addr = (screen_addr & viciiState->vaddr_mask_phi2) | viciiState->vaddr_offset_phi2;
	
	// charset addr
	int charset_addr = (viciiState->regs[0x18] & 0xe) << 10;
	charset_addr = (charset_addr + viciiState->vbank_phi1);
	charset_addr &= viciiState->vaddr_mask_phi1;
	charset_addr |= viciiState->vaddr_offset_phi1;
	
	// bitmap addr
	int bitmap_addr = charset_addr & 0xe000;

	viewC64->viewC64VicControl->SetViciiPointersFromUI(&screen_addr, &charset_addr, &bitmap_addr);

	// store current VIC Display mode
	byteBuffer->PutBool(isBitmap);
	byteBuffer->PutBool(isMultiColor);
	byteBuffer->PutBool(isExtColor);
	
	// put vbank
	byteBuffer->putInt(viciiState->vbank_phi1);

	// put addresses
	byteBuffer->PutI32(screen_addr);
	byteBuffer->PutI32(charset_addr);
	byteBuffer->PutI32(bitmap_addr);

	// put colors
	byteBuffer->PutU8(this->viewPalette->colorD020);
	byteBuffer->PutU8(this->viewPalette->colorD021);
	
	LOGD(".....> serialising layers, index=%d", byteBuffer->index);
	// serialise layers
	for (std::list<CVicEditorLayer *>::iterator it = this->layers.begin();
		 it != this->layers.end(); it++)
	{
		CVicEditorLayer *layer = *it;
		
		LOGD("....... serialise layer '%s', index=%d", layer->layerName, byteBuffer->index);
		layer->Serialise(byteBuffer);
	}
	
	// put current sprite pointers
	LOGD("....... serialise spritePointers, index=%d", byteBuffer->index);

	for (int i = 0; i < 8; i++)
	{
		u8 spritePointer = viewC64->debugInterfaceC64->GetByteFromRamC64(screen_addr + 0x03F8 + i);
		byteBuffer->PutU8(spritePointer);
	}
	
	if (storeVicRegisters)
	{
		// put VIC registers
		byteBuffer->PutBool(true);
		byteBuffer->PutBytes(viciiState->regs, 0x40);
	}
	else
	{
		byteBuffer->PutBool(false);
	}
	
	if (storeC64Memory)
	{
		byteBuffer->PutBool(true);
		
		u8 *c64memory = new u8[0x10000];
		viewC64->debugInterfaceC64->GetWholeMemoryMapFromRam(c64memory);

		byteBuffer->PutBytes(c64memory, 0x10000);
		delete [] c64memory;
	}
	else
	{
		byteBuffer->PutBool(false);
	}

	if (storeVicDisplayControlState)
	{
		// VicDisplayControlState was not intended originally to be stored in VCE file
		// but Isildur/Samar is using it to set correct VIC registers for his images
		// this *should* be replaced in the future by a NEW PICTURE dialog that
		// will prepare VIC registers to mode selected by the user
		byteBuffer->PutBool(true);
		
		byteBuffer->PutBool(viewC64->viewC64VicControl->btnModeText->IsOn());
		byteBuffer->PutBool(viewC64->viewC64VicControl->btnModeBitmap->IsOn());
		byteBuffer->PutBool(viewC64->viewC64VicControl->btnModeHires->IsOn());
		byteBuffer->PutBool(viewC64->viewC64VicControl->btnModeMulti->IsOn());
		byteBuffer->PutBool(viewC64->viewC64VicControl->btnModeStandard->IsOn());
		byteBuffer->PutBool(viewC64->viewC64VicControl->btnModeExtended->IsOn());
		
		byteBuffer->PutBool(viewC64->viewC64VicControl->lstScreenAddresses->isLocked);
		if (viewC64->viewC64VicControl->lstScreenAddresses->isLocked)
		{
			byteBuffer->PutI16(viewC64->viewC64VicControl->lstScreenAddresses->selectedElement);
		}
		
		byteBuffer->PutBool(viewC64->viewC64VicControl->lstCharsetAddresses->isLocked);
		if (viewC64->viewC64VicControl->lstCharsetAddresses->isLocked)
		{
			byteBuffer->PutI16(viewC64->viewC64VicControl->lstCharsetAddresses->selectedElement);
		}

		byteBuffer->PutBool(viewC64->viewC64VicControl->lstBitmapAddresses->isLocked);
		if (viewC64->viewC64VicControl->lstBitmapAddresses->isLocked)
		{
			byteBuffer->PutI16(viewC64->viewC64VicControl->lstBitmapAddresses->selectedElement);
		}

	}
	else
	{
		byteBuffer->PutBool(false);
	}
	
}

void CViewVicEditor::Deserialise(CByteBuffer *byteBuffer, int version)
{
	// restore VIC Display mode
	bool isBitmap = byteBuffer->GetBool();
//	viewC64->viewC64VicControl->btnModeBitmap->SetOn(isBitmap);
//	viewC64->viewC64VicControl->btnModeText->SetOn(!isBitmap);
	
	bool isMultiColor = byteBuffer->GetBool();
//	viewC64->viewC64VicControl->btnModeMulti->SetOn(isMultiColor);
//	viewC64->viewC64VicControl->btnModeHires->SetOn(!isMultiColor);
	
	bool isExtColor = byteBuffer->GetBool();
//	viewC64->viewC64VicControl->btnModeExtended->SetOn(isExtColor);
//	viewC64->viewC64VicControl->btnModeStandard->SetOn(!isExtColor);

	SetVicModeRegsOnly(isBitmap, isMultiColor, isExtColor);
	
	// set VBank
	int vbank = byteBuffer->getInt();
	
	//
	int screenAddr = byteBuffer->GetI32();
//	viewC64->viewC64VicControl->lstScreenAddresses->SetListLocked(true);
//	viewC64->viewC64VicControl->lstScreenAddresses->SetElement(screenAddr / 0x0400, true, false);
	viewVicDisplayMain->screenAddress = screenAddr;

	int charsetAddr = byteBuffer->GetI32();
//	viewC64->viewC64VicControl->lstCharsetAddresses->SetListLocked(true);
//	viewC64->viewC64VicControl->lstCharsetAddresses->SetElement(charsetAddr / 0x0800, true, false);

	int bitmapAddr = byteBuffer->GetI32();
//	viewC64->viewC64VicControl->lstBitmapAddresses->SetListLocked(true);
//	viewC64->viewC64VicControl->lstBitmapAddresses->SetElement(bitmapAddr / 0x2000, true, false);
	viewVicDisplayMain->bitmapAddress = bitmapAddr;
	
	SetVicAddresses(vbank, screenAddr, charsetAddr, bitmapAddr);
	
	//
		
	u8 colorD020 = byteBuffer->GetU8();
	viewC64->debugInterfaceC64->SetVicRegister(0x20, colorD020);

	u8 colorD021 = byteBuffer->GetU8();
	viewC64->debugInterfaceC64->SetVicRegister(0x21, colorD021);
	
	LOGD(" ......... deserialising layers, index=%d", byteBuffer->index);

	// deserialise layers
	for (std::list<CVicEditorLayer *>::iterator it = this->layers.begin();
		 it != this->layers.end(); it++)
	{
		CVicEditorLayer *layer = *it;
		
		LOGD(" ......... deserialising layer '%s', index=%d", layer->layerName, byteBuffer->index);

		layer->Deserialise(byteBuffer, version);
	}
	
	LOGD(" ..... deserialising sprite pointers, index=%d", byteBuffer->index);

	// get current sprite pointers
	for (int i = 0; i < 8; i++)
	{
		u8 spritePointer = byteBuffer->GetU8();
		viewC64->debugInterfaceC64->SetByteToRamC64(screenAddr + 0x03F8 + i, spritePointer);
	}

	//
	LOGD("restoreVicRegisters? index=%d", byteBuffer->index);
	bool restoreVicRegisters = byteBuffer->GetBool();
	if (restoreVicRegisters)
	{
		u8 regs[0x40];
		byteBuffer->GetBytes(regs, 0x40);

		// set VIC registers
		for (int i = 0; i < 0x40; i++)
		{
			viewC64->debugInterfaceC64->SetVicRegister(i, regs[i]);
		}
	}
	
	//////////
	
	LOGD("restoreC64Memory? index=%d", byteBuffer->index);

	bool restoreC64Memory = byteBuffer->GetBool();
	if (restoreC64Memory)
	{
		LOGD("...restoreC64Memory... index=%d", byteBuffer->index);
		u8 *c64memory = new u8[0x10000];
		byteBuffer->getBytes(c64memory, 0x10000);
		
		// leave zero pages as-is
		for (int i = 0x0400; i < 0xFFF0; i++)
		{
			viewC64->debugInterfaceC64->SetByteToRamC64(i, c64memory[i]);
		}
		delete [] c64memory;
	}
	
	if (version < 2)
	{
		// no more fields in version 1
		return;
	}

	bool restoreVicDisplayControlState = byteBuffer->GetBool();
	if (restoreVicDisplayControlState)
	{
		LOGD("...restoreVicDisplayControlState");
		
		viewC64->viewC64VicControl->btnModeText->SetOn(byteBuffer->GetBool());
		viewC64->viewC64VicControl->btnModeBitmap->SetOn(byteBuffer->GetBool());
		viewC64->viewC64VicControl->btnModeHires->SetOn(byteBuffer->GetBool());
		viewC64->viewC64VicControl->btnModeMulti->SetOn(byteBuffer->GetBool());
		viewC64->viewC64VicControl->btnModeStandard->SetOn(byteBuffer->GetBool());
		viewC64->viewC64VicControl->btnModeExtended->SetOn(byteBuffer->GetBool());
		
		if (byteBuffer->GetBool())
		{
			viewC64->viewC64VicControl->lstScreenAddresses->SetListLocked(true);
			viewC64->viewC64VicControl->lstScreenAddresses->SetElement(byteBuffer->GetI16(), true, true);
		}
		else
		{
			viewC64->viewC64VicControl->lstScreenAddresses->SetListLocked(false);
		}
		
		if (byteBuffer->GetBool())
		{
			viewC64->viewC64VicControl->lstCharsetAddresses->SetListLocked(true);
			viewC64->viewC64VicControl->lstCharsetAddresses->SetElement(byteBuffer->GetI16(), true, true);
		}
		else
		{
			viewC64->viewC64VicControl->lstCharsetAddresses->SetListLocked(false);
		}
		
		if (byteBuffer->GetBool())
		{
			viewC64->viewC64VicControl->lstBitmapAddresses->SetListLocked(true);
			viewC64->viewC64VicControl->lstBitmapAddresses->SetElement(byteBuffer->GetI16(), true, true);
		}
		else
		{
			viewC64->viewC64VicControl->lstBitmapAddresses->SetListLocked(false);
		}
	}
	
	//
	this->viewLayers->UpdateVisibleSwitchButtons();
}



void CViewVicEditor::PaletteColorChanged(u8 colorSource, u8 newColorValue)
{
	viewSprite->PaletteColorChanged(colorSource, newColorValue);
}

void CViewVicEditor::SelectLayer(CVicEditorLayer *layer)
{
	guiMain->LockMutex();
	if (this->selectedLayer != NULL)
	{
		this->selectedLayer->LayerSelected(false);
	}
	
	this->selectedLayer = layer;
	
	if (this->selectedLayer != NULL)
	{
		this->selectedLayer->LayerSelected(true);
	}
	
	guiMain->UnlockMutex();
}

///
bool CViewVicEditor::CanSelectView(CGuiView *view)
{
	if (view->visible)
		return true;
	
	return false;
}


void CViewVicEditor::MoveFocusToNextView()
{
	// TODO: N/A, traversal of views
	return;
	
	if (focusElement == NULL)
	{
		SetFocus(traversalOfViews[0]);
		BringToFront(traversalOfViews[0]);
		return;
	}
	
	int selectedViewNum = -1;
	
	for (int i = 0; i < traversalOfViews.size(); i++)
	{
		CGuiView *view = traversalOfViews[i];
		if (view == focusElement)
		{
			selectedViewNum = i;
			break;
		}
	}
	
	CGuiView *newView = NULL;
	for (int z = 0; z < traversalOfViews.size(); z++)
	{
		selectedViewNum++;
		if (selectedViewNum == traversalOfViews.size())
		{
			selectedViewNum = 0;
		}
		
		newView = traversalOfViews[selectedViewNum];
		if (CanSelectView(newView))
			break;
	}
	
	if (CanSelectView(newView))
	{
		SetFocus(traversalOfViews[selectedViewNum]);
		BringToFront(traversalOfViews[selectedViewNum]);
	}
	else
	{
		LOGError("CViewC64::MoveFocusToNextView: no visible views");
	}
	
}

void CViewVicEditor::MoveFocusToPrevView()
{
	// TODO: N/A, traversal of views
	return;

	if (focusElement == NULL)
	{
		SetFocus(traversalOfViews[0]);
		return;
	}
	
	int selectedViewNum = -1;
	
	for (int i = 0; i < traversalOfViews.size(); i++)
	{
		CGuiView *view = traversalOfViews[i];
		if (view == focusElement)
		{
			selectedViewNum = i;
			break;
		}
	}
	
	if (selectedViewNum == -1)
	{
		LOGError("CViewC64::MoveFocusToPrevView: selected view not found");
		return;
	}
	
	CGuiView *newView = NULL;
	for (int z = 0; z < traversalOfViews.size(); z++)
	{
		selectedViewNum--;
		if (selectedViewNum == -1)
		{
			selectedViewNum = traversalOfViews.size()-1;
		}
		
		newView = traversalOfViews[selectedViewNum];
		if (CanSelectView(newView))
			break;
	}
	
	if (CanSelectView(newView))
	{
		SetFocus(traversalOfViews[selectedViewNum]);
		BringToFront(traversalOfViews[selectedViewNum]);
	}
	else
	{
		LOGError("CViewC64::MoveFocusToNextView: no visible views");
	}
}

void CViewVicEditor::UpdateReferenceLayers()
{
	if (layerReferenceImage->isVisible)
	{
		viewVicDisplayMain->backgroundColorAlpha = 0;
		viewVicDisplaySmall->backgroundColorAlpha = 0;
	}
	else
	{
		viewVicDisplayMain->backgroundColorAlpha = 255;
		viewVicDisplaySmall->backgroundColorAlpha = 255;
	}
}

///
void CViewVicEditor::RunDebug()
{
//	SYS_Sleep(300);
//	LOGD("");
//	LOGD("%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%  CViewVicEditor::RunDebug %%%%%%%%%%%%%%%%%");
//	
//	if (viewSprite->btnScanForSprites->IsOn() == false)
//	{
//		layerVirtualSprites->ClearSprites();
//		layerVirtualSprites->FullScanSpritesInThisFrame();
//	}
//	
//	LOGD("%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%  CViewVicEditor::RunDebug DONE %%%%%%%%%%%%%%%%%");
}

