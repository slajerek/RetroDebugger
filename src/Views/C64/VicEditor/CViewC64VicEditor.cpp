#include "VID_Main.h"
#include "CViewC64VicEditor.h"
#include "CViewC64.h"
#include "CSlrFont.h"
#include "CViewC64VicDisplay.h"
#include "CVicEditorBrush.h"
#include "CVicEditorLayer.h"
#include "SYS_Threading.h"
#include "CByteBuffer.h"
#include "CSlrFileFromOS.h"
#include "CSlrFileZlib.h"
#include "C64SettingsStorage.h"
#include "C64KeyboardShortcuts.h"
#include "IMG_Filters.h"
#include "IMG_Scale.h"
#include "C64Tools.h"
#include "CMainMenuBar.h"

#include "CViewC64Screen.h"

#include "CDebugInterfaceC64.h"
#include "CViewC64VicControl.h"

#include "CViewC64Sprite.h"
#include "CViewC64Charset.h"
#include "CViewC64VicEditorLayers.h"
#include "CViewC64VicEditorCreateNewPicture.h"

#include "C64VicDisplayCanvas.h"
#include "CVicEditorLayerC64Screen.h"
#include "CVicEditorLayerC64Canvas.h"
#include "CVicEditorLayerC64Sprites.h"
#include "CVicEditorLayerVirtualSprites.h"
#include "CVicEditorLayerUnrestrictedBitmap.h"
#include "CVicEditorLayerImage.h"

extern "C" {
	extern void c64_glue_set_vbank(int vbank, int ddr_flag);
}

#define VIC_EDITOR_FILE_MAGIC	0x56434500
#define VIC_EDITOR_FILE_VERSION	0x00000003

#define NUMBER_OF_UNDO_LEVELS	100
#define ZOOMING_SPEED_FACTOR_QUICK	15

enum
{
	VICEDITOR_EXPORT_UNKNOWN	= 0,
	VICEDITOR_EXPORT_VCE		= 1,
	VICEDITOR_EXPORT_HYPER		= 2,
	VICEDITOR_EXPORT_KOALA		= 3,
	VICEDITOR_EXPORT_ART_STUDIO	= 4,
	VICEDITOR_EXPORT_RAW_TEXT	= 5,
	VICEDITOR_EXPORT_PNG		= 6
};


CViewC64VicEditor::CViewC64VicEditor(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY)
: CGuiView(name, posX, posY, posZ, sizeX, sizeY)
{
	mutex = new CSlrMutex("CViewVicEditor");
	
	debugInterface = viewC64->debugInterfaceC64;
	recentlyOpened = new CRecentlyOpenedFiles(new CSlrString("recents-viceditor"), this);

	imGuiNoWindowPadding = true;
	imGuiNoScrollbar = true;

	font = viewC64->fontCBMShifted;
	fontScale = 5.0f;//1.5;
	fontHeight = font->GetCharHeight('@', fontScale) + 2;

	isPainting = false;
	isKeyboardMovingDisplay = false;
	prevRx = -1000;
	prevRy = -1000;
	prevMousePosX = 0;
	prevMousePosY = 0;

	brushSize = 1;
	currentBrush = new CVicEditorBrush();
	currentBrush->CreateBrushRectangle(brushSize);
	
	viewVicDisplay = new CViewC64VicDisplay("CViewVicEditor::viewVicDisplay", posX, posY, posZ, sizeX, sizeY, viewC64->debugInterfaceC64);
	viewVicDisplay->SetVicControlView(viewC64->viewC64VicControl);
	viewVicDisplay->SetShowDisplayBorderType(VIC_DISPLAY_SHOW_BORDER_VISIBLE_AREA);

	viewVicDisplay->SetDisplayPosition(0,0, 1.0f, true);
	viewVicDisplay->InitGridLinesColorFromSettings();

	//viewVicDisplay->showGridLines = true;
	viewVicDisplay->showRasterCursor = false;
	viewVicDisplay->consumeTapBackground = false;
	viewVicDisplay->applyScrollRegister = true;
	
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
	
	//
	importFileExtensions.push_back(new CSlrString("vce"));
	
	importFileExtensions.push_back(new CSlrString("png"));
	importFileExtensions.push_back(new CSlrString("jpg"));
	importFileExtensions.push_back(new CSlrString("jpeg"));
	importFileExtensions.push_back(new CSlrString("bmp"));
	importFileExtensions.push_back(new CSlrString("gif"));
	importFileExtensions.push_back(new CSlrString("psd"));
	importFileExtensions.push_back(new CSlrString("pic"));
	importFileExtensions.push_back(new CSlrString("hdr"));
	importFileExtensions.push_back(new CSlrString("tga"));

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

	// this will be created elsewhere
	viewLayers = NULL;
	viewCharset = NULL;
	viewPalette = NULL;
	viewSprite = NULL;
	
	//
	viewCreateNewPicture = new CViewC64VicEditorCreateNewPicture("Create New Picture##c64", 100, 100, -1, this);
//	viewCreateNewPicture->SetVisible(false);
//	guiMain->AddView(viewCreateNewPicture);
	
	// keyboard shortcuts
	kbsVicEditorCreateNewPicture = new CSlrKeyboardShortcut(KBZONE_VIC_EDITOR, "VIC Editor: New Picture", 'n', false, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsVicEditorCreateNewPicture);

	kbsVicEditorPreviewScale = new CSlrKeyboardShortcut(KBZONE_VIC_EDITOR, "VIC Editor: Preview scale", '/', false, false, false, false, this);
	guiMain->AddKeyboardShortcut(kbsVicEditorPreviewScale);

	kbsVicEditorShowCursor = new CSlrKeyboardShortcut(KBZONE_VIC_EDITOR, "Show cursor", '\'', false, false, false, false, this);
	guiMain->AddKeyboardShortcut(kbsVicEditorShowCursor);

	kbsVicEditorShowGrid = new CSlrKeyboardShortcut(KBZONE_VIC_EDITOR, "Show grid", 'g', false, false, false, false, this);
	guiMain->AddKeyboardShortcut(kbsVicEditorShowGrid);

	kbsVicEditorDoUndo = new CSlrKeyboardShortcut(KBZONE_VIC_EDITOR, "Undo", 'z', false, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsVicEditorDoUndo);

	kbsVicEditorDoRedo = new CSlrKeyboardShortcut(KBZONE_VIC_EDITOR, "Redo", 'z', true, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsVicEditorDoRedo);

	kbsVicEditorOpenFile = new CSlrKeyboardShortcut(KBZONE_VIC_EDITOR, "Open file", 'o', false, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsVicEditorOpenFile);

	kbsVicEditorExportFile = new CSlrKeyboardShortcut(KBZONE_GLOBAL, "Export screen to file", 'e', true, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsVicEditorExportFile);

	kbsVicEditorSaveVCE = new CSlrKeyboardShortcut(KBZONE_VIC_EDITOR, "Save as VCE", 's', false, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsVicEditorSaveVCE);

	kbsVicEditorClearScreen = new CSlrKeyboardShortcut(KBZONE_VIC_EDITOR, "Clear screen", MTKEY_BACKSPACE, false, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsVicEditorClearScreen);

	kbsVicEditorRectangleBrushSizePlus = new CSlrKeyboardShortcut(KBZONE_VIC_EDITOR, "Rectangle brush size +", ']', false, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsVicEditorRectangleBrushSizePlus);

	kbsVicEditorRectangleBrushSizeMinus = new CSlrKeyboardShortcut(KBZONE_VIC_EDITOR, "Rectangle brush size -", '[', false, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsVicEditorRectangleBrushSizeMinus);

	kbsVicEditorCircleBrushSizePlus = new CSlrKeyboardShortcut(KBZONE_VIC_EDITOR, "Circle brush size +", ']', false, false, false, false, this);
	guiMain->AddKeyboardShortcut(kbsVicEditorCircleBrushSizePlus);

	kbsVicEditorCircleBrushSizeMinus = new CSlrKeyboardShortcut(KBZONE_VIC_EDITOR, "Circle brush size -", '[', false, false, false, false, this);
	guiMain->AddKeyboardShortcut(kbsVicEditorCircleBrushSizeMinus);

	kbsVicEditorToggleAllWindows = new CSlrKeyboardShortcut(KBZONE_VIC_EDITOR, "Toggle all windows", 'f', false, false, false, false, this);
	guiMain->AddKeyboardShortcut(kbsVicEditorToggleAllWindows);

	kbsVicEditorToggleWindowPreview = new CSlrKeyboardShortcut(KBZONE_VIC_EDITOR, "Toggle preview", 'd', false, false, false, false, this);
	guiMain->AddKeyboardShortcut(kbsVicEditorToggleWindowPreview);

	kbsVicEditorToggleWindowPalette = new CSlrKeyboardShortcut(KBZONE_VIC_EDITOR, "Toggle palette", 'p', false, false, false, false, this);
	guiMain->AddKeyboardShortcut(kbsVicEditorToggleWindowPalette);

	kbsVicEditorSwitchPaletteColors = new CSlrKeyboardShortcut(KBZONE_VIC_EDITOR, "Switch palette colors", 'x', false, false, false, false, this);
	guiMain->AddKeyboardShortcut(kbsVicEditorSwitchPaletteColors);

	kbsVicEditorToggleWindowLayers = new CSlrKeyboardShortcut(KBZONE_VIC_EDITOR, "Toggle layers", 'l', false, false, false, false, this);
	guiMain->AddKeyboardShortcut(kbsVicEditorToggleWindowLayers);

	kbsVicEditorToggleWindowCharset = new CSlrKeyboardShortcut(KBZONE_VIC_EDITOR, "Toggle charset", 'c', false, false, false, false, this);
	guiMain->AddKeyboardShortcut(kbsVicEditorToggleWindowCharset);

	kbsVicEditorToggleWindowSprite = new CSlrKeyboardShortcut(KBZONE_VIC_EDITOR, "Toggle sprite", 's', false, false, false, false, this);
	guiMain->AddKeyboardShortcut(kbsVicEditorToggleWindowSprite);

	kbsVicEditorToggleSpriteFrames = new CSlrKeyboardShortcut(KBZONE_VIC_EDITOR, "Toggle sprite frames", 'g', false, false, true, false, this);
	guiMain->AddKeyboardShortcut(kbsVicEditorToggleSpriteFrames);

	kbsVicEditorToggleTopBar = NULL; //new CSlrKeyboardShortcut(KBZONE_VIC_EDITOR, "Toggle top bar", 'b', false, false, true, false, this);
//	guiMain->AddKeyboardShortcut(kbsVicEditorToggleTopBar);

	kbsVicEditorSelectNextLayer = new CSlrKeyboardShortcut(KBZONE_VIC_EDITOR, "Select next layer", '`', false, false, false, false, this);
	guiMain->AddKeyboardShortcut(kbsVicEditorSelectNextLayer);
	//
	exportFileDialogMode = VICEDITOR_EXPORT_UNKNOWN;
	
	// create undo pool
	for (int i = 0; i < NUMBER_OF_UNDO_LEVELS; i++)
	{
		CByteBuffer *byteBuffer = new CByteBuffer();
		poolList.push_back(byteBuffer);
	}
	
	// register for OS window size changes
	guiMain->AddGlobalOSWindowChangedCallback(this);

	prevDisplayPosX = 0;
	prevDisplayPosY = 0;
	
	//
	this->viewVicDisplay->InitGridLinesColorFromSettings();

	//
	ZoomDisplay(3.0f);
	UpdateDisplayFrame();
	viewVicDisplay->UpdateGridLinesVisibleOnCurrentZoom();

	this->viewVicDisplay->showGridLines = true;
}

CViewC64VicEditor::~CViewC64VicEditor()
{
}

void CViewC64VicEditor::SetHelperViews(CViewC64VicControl *viewVicControl, CViewC64VicEditorLayers *viewLayers, CViewC64Charset *viewCharset, CViewC64Palette *viewPalette, CViewC64Sprite *viewSprite)
{
	this->viewVicControl = viewVicControl;
	this->viewLayers = viewLayers;
	this->viewCharset = viewCharset;
	this->viewPalette = viewPalette;
	this->viewSprite = viewSprite;
}

void CViewC64VicEditor::InitPaintGridColors()
{
	// TODO: make this a global setting for this view
	this->viewVicDisplay->InitGridLinesColorFromSettings();
}

//void CViewVicEditor::GlobalOSWindowChangedCallback()
//{
//	UpdateSmallDisplayScale();
//}


void CViewC64VicEditor::UpdateReferenceLayers()
{
	if (layerReferenceImage->isVisible)
	{
		viewVicDisplay->backgroundColorAlpha = 0;
	}
	else
	{
		viewVicDisplay->backgroundColorAlpha = 255;
	}
}

void CViewC64VicEditor::SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY)
{
	LOGD("CViewC64VicEditor::SetPosition: %f %f", posX, posY);

	float diffX = posX - prevDisplayPosX;
	float diffY = posY - prevDisplayPosY;
	float px = viewVicDisplay->posX + diffX;
	float py = viewVicDisplay->posY + diffY;
	viewVicDisplay->SetPosition(px, py);
	
	prevDisplayPosX = posX;
	prevDisplayPosY = posY;
	
	UpdateDisplayFrame();
	CGuiView::SetPosition(posX, posY, posZ, sizeX, sizeY);
}

void CViewC64VicEditor::DoLogic()
{
	CGuiView::DoLogic();
}

void CViewC64VicEditor::Render()
{
//	guiMain->fntConsole->BlitText("CViewVicEditor", posX, posY, 0, 11, 1.0);

	guiMain->LockMutex();
	
	if (viewSprite->btnScanForSprites->IsOn())
	{
		layerVirtualSprites->ClearSprites();
		layerVirtualSprites->SimpleScanSpritesInThisFrame();
	}

	// copy current state of VIC
	c64d_vicii_copy_state(&(viewC64->currentViciiState));
	
	UpdateDisplayRasterPos();
	
//	viewC64->viewC64VicDisplay->RefreshScreenStateOnly(displayVicState);
//	viewVicDisplaySmall->RefreshScreenStateOnly(displayVicState);
	
	// this is done by CViewC64 now
//	viewC64->viewC64StateVIC->UpdateSpritesImages();
	
	// main render routine:
	BlitFilledRectangle(posX, posY, -1, sizeX, sizeY, 0.0, 0.0, 0.0, 1.0);
	
	// * render main screen *
	vicii_cycle_state_t *viciiState = &(viewC64->viciiStateToShow);
	
	viewPalette->colorD020 = viciiState->regs[0x20];
	viewPalette->colorD021 = viciiState->regs[0x21];
	

	viewVicDisplay->RefreshScreen(viciiState);

//	if (toolIsActive)
//	{
//		layerToolPreview->RenderMain(viciiState);
//	}
	
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
	{
		// now we have 2 different vic displays, render cursor based on original vic display
		viewVicDisplay->RenderCursor(viewC64->viewC64VicDisplay->rasterCursorPosX,
										 viewC64->viewC64VicDisplay->rasterCursorPosY);
	}
	
	//
//	viewVicDisplaySmall->SetViciiState(viciiState);
	
	// * render UI *
//	BlitFilledRectangle(0, 0, posZ, SCREEN_WIDTH, SCREEN_HEIGHT, 1, 0, 0, 1);
//	imgMock->Render(0, -50, posZ, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, 0.703125, 0.87890625);

	
	CGuiView::Render();

	guiMain->UnlockMutex();
}

void CViewC64VicEditor::RenderImGui()
{
	PreRenderImGui();
	Render();
	PostRenderImGui();
}

void CViewC64VicEditor::UpdateDisplayRasterPos()
{
	// TODO: this is crude hack, temporary copy parameters, this is temporary workaround
	//       this view should copy data itself based on a new class for VIC Display UI
	//       (TODO: create view for VIC Display UI, leaving VIC Display class as generic)
	
	float mouseX = guiMain->mousePosX;
	float mouseY = guiMain->mousePosY;
	
	if (!viewC64->viewC64VicDisplay->isCursorLocked)
	{
		if ( (guiMain->FindTopWindow(mouseX, mouseY) == this)
			&& IsInsideView(mouseX, mouseY)
			&& viewVicDisplay->IsInsideScreen(mouseX, mouseY))
		{
			float rasterX, rasterY;
			viewVicDisplay->GetRasterPosFromScreenPos(mouseX, mouseY, &rasterX, &rasterY);
			
			
			//viewVicDisplaySmall->rasterCursorPosX = rasterX;
			//viewVicDisplaySmall->rasterCursorPosY = rasterY;
			
			viewVicDisplay->rasterCursorPosX = rasterX;
			viewVicDisplay->rasterCursorPosY = rasterY;
			viewC64->viewC64VicDisplay->rasterCursorPosX = rasterX;
			viewC64->viewC64VicDisplay->rasterCursorPosY = rasterY;
			
			// update just the VIC state for main C64View screen to correctly render C64 Sprites
			vicii_cycle_state_t *displayVicState = viewC64->viewC64VicDisplay->UpdateViciiStateNonVisible(viewC64->viewC64VicDisplay->rasterCursorPosX,
																	   viewC64->viewC64VicDisplay->rasterCursorPosY);

			viewC64->viewC64VicDisplay->UpdateAutoscrollDisassembly(false);
//			LOGD("update %f %f", rasterX, rasterY);			
		}
		else if (viewC64->viewC64VicDisplay->IsVisible() == false)
		{
//			LOGD("UpdateDisplayRasterPos4");
			viewC64->viewC64VicDisplay->CopyCurrentViciiStateAndUnlock();
		}
	}
}


//@returns is consumed
bool CViewC64VicEditor::DoDoubleTap(float x, float y)
{
	LOGG("CViewVicEditor::DoDoubleTap:  x=%f y=%f", x, y);
	return CGuiView::DoDoubleTap(x, y);
}

bool CViewC64VicEditor::DoFinishDoubleTap(float x, float y)
{
	LOGG("CViewVicEditor::DoFinishTap: %f %f", x, y);
	return CGuiView::DoFinishDoubleTap(x, y);
}


bool CViewC64VicEditor::FinishMove(float x, float y, float distX, float distY, float accelerationX, float accelerationY)
{
	return CGuiView::FinishMove(x, y, distX, distY, accelerationX, accelerationY);
}

bool CViewC64VicEditor::DoMultiTap(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiTap(touch, x, y);
}

bool CViewC64VicEditor::DoMultiMove(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiMove(touch, x, y);
}

bool CViewC64VicEditor::DoMultiFinishTap(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiFinishTap(touch, x, y);
}

bool CViewC64VicEditor::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
//	LOGI("CViewVicEditor::KeyDown");
	
	if (!IsInside(guiMain->mousePosX, guiMain->mousePosY))
	{
		// this view has special UX case: we are processing shortcuts even in not focused view (just having coursor on the view area is enough)
		// this is case when we are in focus, but the mouse is not inside the view
		return KeyDownOnMouseHover(keyCode, isShift, isAlt, isControl, isSuper);
	}
	
	if (viewC64->mainMenuBar->KeyDown(keyCode, isShift, isAlt, isControl, isSuper))
	{
		return true;
	}
	
	if (viewC64->viewC64Screen->KeyDown(keyCode, isShift, isAlt, isControl, isSuper))
	{
		return true;
	}

	return false; //viewC64->KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
	return CGuiView::KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewC64VicEditor::KeyDownRepeat(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return CGuiView::KeyDownRepeat(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewC64VicEditor::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return CGuiView::KeyUp(keyCode, isShift, isAlt, isControl, isSuper);
}

// In VicEditor all keyboard shortcuts are on mouse hover, not on click
bool CViewC64VicEditor::ProcessKeyboardShortcut(u32 zone, u8 actionType, CSlrKeyboardShortcut *keyboardShortcut)
{
	// vic editor is special and processes shortcuts on KeyDownOnMouseHover
	LOGError("CViewVicEditor::ProcessKeyboardShortcut: unhandled keyboardShortcut=%s", keyboardShortcut->name);
	return false;
}

bool CViewC64VicEditor::KeyboardShortcut(CSlrKeyboardShortcut *shortcut)
{
	if (shortcut == kbsVicEditorCreateNewPicture)
	{
		this->CreateNewPicture();
		return true;
	}
//	else if (shortcut == viewC64->keyboardShortcuts->kbsVicEditorPreviewScale)
//	{
//		resetDisplayScaleIndex++;
//		if (resetDisplayScaleIndex == 5)
//		{
//			resetDisplayScaleIndex = 0;
//		}
//
//		UpdateSmallDisplayScale();
//		return true;
//	}
//	else
		if (shortcut == kbsVicEditorShowCursor)
	{
//		viewVicDisplaySmall->showRasterCursor = !viewVicDisplaySmall->showRasterCursor;
		viewC64->viewC64VicDisplay->showRasterCursor = !viewC64->viewC64VicDisplay->showRasterCursor;
		viewVicDisplay->showRasterCursor = viewC64->viewC64VicDisplay->showRasterCursor;
		return true;
	}
	else if (shortcut == kbsVicEditorShowGrid)
	{
		viewVicDisplay->showGridLines = !viewVicDisplay->showGridLines;
		return true;
	}
	else if (shortcut == kbsVicEditorDoUndo)
	{
		guiMain->LockMutex();
		DoUndo();
		guiMain->UnlockMutex();
		return true;
	}
	else if (shortcut == kbsVicEditorDoRedo)
	{
		guiMain->LockMutex();
		DoRedo();
		guiMain->UnlockMutex();
		return true;
	}
	else if (shortcut == kbsVicEditorOpenFile)
	{
		OpenDialogImportFile();
		return true;
	}
	else if (shortcut == kbsVicEditorExportFile)
	{
		exportMode = VICEDITOR_EXPORT_UNKNOWN;
		OpenDialogExportFile();
		return true;
	}
	else if (shortcut == kbsVicEditorSaveVCE)
	{
		OpenDialogSaveVCE();
		return true;
	}
	else if (shortcut == kbsVicEditorClearScreen)
	{
		this->ClearScreen();
		return true;
	}
	else if (shortcut == kbsVicEditorRectangleBrushSizePlus)
	{
		if (brushSize <= 2001)
		{
			guiMain->LockMutex();
			if (currentBrush)
				delete currentBrush;
			
			brushSize += 2;
			
			char *buf = SYS_GetCharBuf();
			
			currentBrush = new CVicEditorBrush();
			currentBrush->CreateBrushRectangle(brushSize);
			sprintf(buf, "Rectangle brush size %d", brushSize);

			guiMain->UnlockMutex();

			viewC64->ShowMessageInfo(buf);
			SYS_ReleaseCharBuf(buf);
		}
		return true;
	}
	else if (shortcut == kbsVicEditorRectangleBrushSizeMinus)
	{
		if (brushSize >= 3)
		{
			guiMain->LockMutex();
			if (currentBrush)
				delete currentBrush;
			
			brushSize -= 2;
			
			char *buf = SYS_GetCharBuf();
			
			currentBrush = new CVicEditorBrush();
			currentBrush->CreateBrushRectangle(brushSize);
			sprintf(buf, "Rectangle brush size %d", brushSize);

			guiMain->UnlockMutex();

			viewC64->ShowMessageInfo(buf);
			SYS_ReleaseCharBuf(buf);
		}
		return true;
	}
	else if (shortcut == kbsVicEditorCircleBrushSizePlus)
	{
		if (brushSize <= 2001)
		{
			guiMain->LockMutex();
			if (currentBrush)
				delete currentBrush;
			
			brushSize += 2;
			
			char *buf = SYS_GetCharBuf();
			
			currentBrush = new CVicEditorBrush();
			currentBrush->CreateBrushCircle(brushSize);
			sprintf(buf, "Circle brush size %d", brushSize);

			guiMain->UnlockMutex();

			viewC64->ShowMessageInfo(buf);
			SYS_ReleaseCharBuf(buf);
		}
		return true;
	}
	else if (shortcut == kbsVicEditorCircleBrushSizeMinus)
	{
		if (brushSize >= 3)
		{
			guiMain->LockMutex();
			if (currentBrush)
				delete currentBrush;
			
			brushSize -= 2;
			
			char *buf = SYS_GetCharBuf();
			
			currentBrush = new CVicEditorBrush();
			currentBrush->CreateBrushCircle(brushSize);
			sprintf(buf, "Circle brush size %d", brushSize);

			guiMain->UnlockMutex();

			viewC64->ShowMessageInfo(buf);
			SYS_ReleaseCharBuf(buf);
		}
		return true;
	}
//	else if (shortcut == kbsVicEditorToggleAllWindows)
//	{
//		if (inPresentationScreen)
//		{
//			viewPalette->SetVisible(prevVisiblePalette);
//			viewCharset->SetVisible(prevVisibleCharset);
//			viewSprite->SetVisible(prevVisibleSprite);
//			viewLayers->SetVisible(prevVisibleLayers);
//
//			// TODO: toolbox is disabled in production
//			//viewToolBox->SetVisible(prevVisibleToolBox);
//
//			SetTopBarVisible(prevVisibleTopBar);
//			viewVicDisplaySmall->SetVisible(prevVisiblePreview);
//			inPresentationScreen = false;
//		}
//		else
//		{
//			prevVisiblePalette = viewPalette->visible;
//			prevVisibleCharset = viewCharset->visible;
//			prevVisibleSprite = viewSprite->visible;
//			prevVisibleLayers = viewLayers->visible;
//			prevVisibleTopBar = viewTopBar->visible;
//
//			// TODO: toolbox is disabled in production
//			//prevVisibleToolBox = viewToolBox->visible;
//
//			prevVisiblePreview = viewVicDisplaySmall->visible;
//
//			viewPalette->SetVisible(false);
//			viewCharset->SetVisible(false);
//			viewSprite->SetVisible(false);
//			viewLayers->SetVisible(false);
//
//			// TODO: toolbox is disabled in production
//			//viewToolBox->SetVisible(false);
//
//			SetTopBarVisible(false);
//			viewVicDisplaySmall->SetVisible(false);
//			inPresentationScreen = true;
//		}
//		return true;
//	}
//	else if (shortcut == kbsVicEditorToggleWindowPreview)
//	{
//		viewVicDisplaySmall->SetVisible(!viewVicDisplaySmall->visible);
//		return true;
//	}
	else if (shortcut == kbsVicEditorToggleWindowPalette)
	{
		viewPalette->SetVisible(!viewPalette->visible);
		return true;
	}
	else if (shortcut == kbsVicEditorSwitchPaletteColors)
	{
		viewPalette->SwitchSelectedColors();
		return true;
	}

	else if (shortcut == kbsVicEditorToggleWindowLayers)
	{
		viewLayers->SetVisible(!viewLayers->visible);
		return true;
	}
	else if (shortcut == kbsVicEditorToggleWindowCharset)
	{
		viewCharset->SetVisible(!viewCharset->visible);
//		prevVisibleCharset = viewCharset->visible;
		return true;
	}
	else if (shortcut == kbsVicEditorToggleWindowSprite)
	{
		viewSprite->SetVisible(!viewSprite->visible);
		return true;
	}
//	else if (shortcut == kbsVicEditorToggleTopBar)
//	{
//		SetTopBarVisible(!viewTopBar->visible);
//		return true;
//	}
	// TODO: toolbox is disabled in production
//	else if (shortcut == viewC64->keyboardShortcuts->kbsVicEditorToggleToolBox)
//	{
//		viewToolBox->SetVisible(!viewToolBox->visible);
//		return true;
//	}
	else if (shortcut == kbsVicEditorToggleSpriteFrames)
	{
		bool show = !viewVicDisplay->showSpritesFrames;
		SetSpritesFramesVisible(show);
		return true;
	}
	else if (shortcut == kbsVicEditorSelectNextLayer)
	{
		viewLayers->SelectNextLayer();
		return true;
	}
	if (shortcut == kbsVicEditorSwitchPaletteColors)
	{
		viewPalette->SwitchSelectedColors();
	}
//	else if (shortcut == viewC64->viewC64MainMenu->kbsMoveFocusToNextView)
//	{
//		guiMain->LockMutex();
//		MoveFocusToNextView();
//		guiMain->UnlockMutex();
//	}
//	else if (shortcut == viewC64->viewC64MainMenu->kbsMoveFocusToPreviousView)
//	{
//		guiMain->LockMutex();
//		MoveFocusToPrevView();
//		guiMain->UnlockMutex();
//	}
	
	return false;
}


bool CViewC64VicEditor::KeyDownOnMouseHover(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	keyCode = SYS_GetBareKey(keyCode, isShift, isAlt, isControl, isSuper);
	
	std::list<u32> zones;
	zones.push_back(KBZONE_VIC_EDITOR);
	zones.push_back(KBZONE_GLOBAL);
	CSlrKeyboardShortcut *shortcut = guiMain->keyboardShortcuts->FindShortcut(zones, keyCode, isShift, isAlt, isControl, isSuper);

	if (KeyboardShortcut(shortcut))
	{
		return true;
	}
	
//	if ((keyCode == 'h' || keyCode == 'H') && isShift && isControl)
//	{
//		exportMode = VICEDITOR_EXPORT_HYPER;
//		OpenDialogExportFile();
//		return true;
//	}
	
	if ((keyCode == 's' || keyCode == 'S') && isAlt && !isControl && !isShift)
	{
		guiMain->LockMutex();
		viewSprite->isSpriteLocked = false;
		layerVirtualSprites->ClearSprites();
		layerVirtualSprites->FullScanSpritesInThisFrame();
		guiMain->UnlockMutex();
		return true;
	}

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
			viewVicDisplay->GetRasterPosFromScreenPos(guiMain->mousePosX, guiMain->mousePosY, &fRasterX, &fRasterY);
			
			int rx = floor(fRasterX);
			int ry = floor(fRasterY);
			
			// get pixel color
			StoreUndo();

			u8 color = viewVicDisplay->currentCanvas->GetColorAtPixel(rx, ry);
			
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

	return false;

	if (viewC64->mainMenuBar->KeyDown(keyCode, isShift, isAlt, isControl, isSuper))
	{
		return true;
	}
	
	if (viewC64->viewC64Screen->KeyDown(keyCode, isShift, isAlt, isControl, isSuper))
	{
		return true;
	}

	return false;
}

bool CViewC64VicEditor::KeyUpOnMouseHover(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return false;
}

bool CViewC64VicEditor::KeyDownGlobal(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return false;
}

bool CViewC64VicEditor::KeyUpGlobal(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	isKeyboardMovingDisplay = false;
	
	if (keyCode == MTKEY_LALT || keyCode == MTKEY_RALT)
	{
		viewVicDisplay->currentCanvas->ClearDitherMask();
	}
	return false;
}

bool CViewC64VicEditor::KeyPressed(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return CGuiView::KeyPressed(keyCode, isShift, isAlt, isControl, isSuper);
}

void CViewC64VicEditor::ActivateView()
{
	LOGG("CViewVicEditor::ActivateView()");
}

void CViewC64VicEditor::DeactivateView()
{
	LOGG("CViewVicEditor::DeactivateView()");
}

void CViewC64VicEditor::LockMutex()
{
	mutex->Lock();
}

void CViewC64VicEditor::UnlockMutex()
{
	mutex->Unlock();
}


////
bool CViewC64VicEditor::DoScrollWheel(float deltaX, float deltaY)
{
	if (IsInsideView(guiMain->mousePosX, guiMain->mousePosY))
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
			
			float newScale = viewVicDisplay->scale + dy;
			
			LOGG("CViewVicEditor::DoScrollWheel:  %f %f %f", viewVicDisplay->scale, dy, newScale);
			
			ZoomDisplay(newScale);
		}
		return true;
	}
	return false;
}

bool CViewC64VicEditor::InitZoom()
{
	return CGuiView::InitZoom();
}

bool CViewC64VicEditor::DoZoomBy(float x, float y, float zoomValue, float difference)
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

	float newScale = viewVicDisplay->scale + dy;
	
	LOGG("CViewVicEditor::DoZoomBy:  %f %f %f", viewVicDisplay->scale, dy, newScale);
	
	ZoomDisplay(newScale);
	
	return true;
}

void CViewC64VicEditor::RunC64EndlessLoop()
{
	// prepare C64 code to run in endless loop. note: do we need to reset C64 first?
	// add at 0002 78	SEI		0003 4C 01 09 JMP $0003
	viewC64->debugInterfaceC64->SetByteToRamC64(0x0002, 0x78);	// SEI
	viewC64->debugInterfaceC64->SetByteToRamC64(0x0003, 0x4C);	// JMP $0901
	viewC64->debugInterfaceC64->SetByteToRamC64(0x0004, 0x03);
	viewC64->debugInterfaceC64->SetByteToRamC64(0x0005, 0x00);
	viewC64->debugInterfaceC64->MakeJmpC64(0x0002);
}

void CViewC64VicEditor::CreateNewPicture()
{
	viewCreateNewPicture->SetVisible(true);
	guiMain->AddView(viewCreateNewPicture);
//	guiMain->AddViewToLayout(viewCreateNewPicture);
//	guiMain->AddViewSkippingLayout(viewCreateNewPicture);
	viewCreateNewPicture->ActivateView();
}

void CViewC64VicEditor::ClearScreen()
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

bool CViewC64VicEditor::PaintUsingLeftMouseButton(float x, float y)
{
	// clicked on the background / vic main display?
//	if (viewVicDisplay->IsInside(x, y))
	{
		// TODO: paint line
		float fRasterX, fRasterY;
		viewVicDisplay->GetRasterPosFromScreenPos(x, y, &fRasterX, &fRasterY);

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
}

bool CViewC64VicEditor::PaintUsingRightMouseButton(float x, float y)
{
//	if (viewVicDisplaySmall->visible && viewVicDisplaySmall->IsInsideView(x, y))
//	{
//		MoveDisplayToPreviewScreenPos(x, y);
//		isMovingPreviewFrame = true;
//
//		viewVicDisplaySmall->DoRightClickMove(x, y, distX, distY, diffX, diffY);
//		return true;
//	}
//
//	if (isMovingPreviewFrame)
//		return true;
//
//	if (isPaintingOnPreviewFrame)
//		return true;
	
	// update sprite
	if (viewSprite->visible && !(viewSprite->isSpriteLocked))
	{
		float rx, ry;
		viewVicDisplay->GetRasterPosFromScreenPosWithoutScroll(x, y, &rx, &ry);
		layerVirtualSprites->UpdateSpriteView((int)rx, (int)ry);
	}
//
//		// clicked on the background / vic main display?
//		if (IsInsideView(x, y) && viewVicDisplay->IsInside(x, y))
	{
		// TODO: paint line
		float fRasterX, fRasterY;
		viewVicDisplay->GetRasterPosFromScreenPos(x, y, &fRasterX, &fRasterY);
		
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
	
	return false;
}

bool CViewC64VicEditor::DoNotTouchedMove(float x, float y)
{
//	LOGG("CViewVicEditor::DoNotTouchedMove: isKeyboardMovingDisplay=%d", isKeyboardMovingDisplay);
	if (isKeyboardMovingDisplay)
	{
		float dx = prevMousePosX - x;
		float dy = prevMousePosY - y;
		
		MoveDisplayDiff(-dx, -dy);
		
		prevMousePosX = x;
		prevMousePosY = y;
		return true;
	}
	
	if (IsInsideView(x, y))
	{
		CGuiView *view = guiMain->FindTopWindow(x, y);
		if (view != NULL)
		{
//			LOGD("FindTopWindow: %s", view->name);
			
			if (view == this)
			{
				// update sprite
				if (viewSprite->visible && !(viewSprite->isSpriteLocked))
				{
					float rx, ry;
					viewVicDisplay->GetRasterPosFromScreenPosWithoutScroll(x, y, &rx, &ry);
					layerVirtualSprites->UpdateSpriteView((int)rx, (int)ry);
				}
				
			//	LOGD(" ====== MAIN viewVicDisplay=%x", viewVicDisplay);
			//	if (!viewC64->viewC64VicDisplay->isCursorLocked)
			//	{
			//		viewVicDisplay->DoNotTouchedMove(x, y);
			//	}
				
				UpdateDisplayRasterPos();

				// does not work when other window is moved
//				if (guiMain->isLeftMouseButtonPressed)
//				{
//					PaintUsingLeftMouseButton(x, y);
//				}

				/*
				if (guiMain->isRightMouseButtonPressed)
				{
					PaintUsingRightMouseButton(x, y);
				}*/
			}
		}
		else
		{
//			LOGD("FindTopWindow NULL");
		}
	}
//	 return true;
//
	
	return false;
}


void CViewC64VicEditor::MoveDisplayDiff(float diffX, float diffY)
{
	LOGG("CViewVicEditor::MoveDisplayDiff: diffX=%f diffY=%f posX=%f posY=%f posOffsetX=%f posOffsetY=%f", diffX, diffY, viewVicDisplay->posX, viewVicDisplay->posY, viewVicDisplay->posOffsetX, viewVicDisplay->posOffsetY);
	float px = viewVicDisplay->posX - viewVicDisplay->posOffsetX;
	float py = viewVicDisplay->posY - viewVicDisplay->posOffsetY;
	
	px += diffX;
	py += diffY;
	
	MoveDisplayToScreenPos(px, py);
}

void CViewC64VicEditor::MoveDisplayToScreenPos(float px, float py)
{
	LOGG("CViewVicEditor::MoveDisplayToScreenPos: %f %f", px, py);

	guiMain->LockMutex();
	
	CheckDisplayBoundaries(&px, &py);
	
	LOGG("------> SetPosition: px=%f py=%f", px, py);
	viewVicDisplay->SetPosition(px, py);
	UpdateDisplayFrame();
	UpdateDisplayRasterPos();

	guiMain->UnlockMutex();
}

void CViewC64VicEditor::UpdateDisplayFrame()
{
	guiMain->LockMutex();
	
	// check boundaries
	float px = viewVicDisplay->posX;
	float py = viewVicDisplay->posY;
	
	CheckDisplayBoundaries(&px, &py);
	
	viewVicDisplay->SetPosition(px, py);
	
//	float rx, ry;
//	viewVicDisplay->GetRasterPosFromScreenPosWithoutScroll(mainDisplayPosX, mainDisplayPosY, &rx, &ry);
//
//	float rx2, ry2;
	
//	LOGD("viewVicDisplay->sizeX=%f", viewVicDisplay->sizeX);
//	viewVicDisplay->GetRasterPosFromScreenPos(viewVicDisplay->posX + viewVicDisplay->sizeX,
//												  viewVicDisplay->posY + viewVicDisplay->sizeY, &rx2, &ry2);

//	viewVicDisplay->GetRasterPosFromScreenPosWithoutScroll(mainDisplayPosEndX-1,
//												  mainDisplayPosEndY-1, &rx2, &ry2);

	
	
//	float rsx = rx2 - rx;
//	float rsy = ry2 - ry;
	
//	viewVicDisplaySmall->SetDisplayFrameRaster((int)rx+1, (int)ry+1, (int)rsx-1, (int)rsy-1);

//	viewVicDisplaySmall->SetDisplayFrameRaster(rx+1, ry+1, rsx-1, rsy-1);

	guiMain->UnlockMutex();
}

void CViewC64VicEditor::CheckDisplayBoundaries(float *px, float *py)
{
	float borderX = 0.0f;
	
	if (viewVicDisplay->showDisplayBorderType == VIC_DISPLAY_SHOW_BORDER_VISIBLE_AREA)
	{
		//    frame  |visible   interior  visible|  frame
		// X: 0x000  | 0x068  0x088 0x1C8  0x1E8 |  0x1F8
		//        0             136   456             504
		// Y: 0x000  | 0x010  0x032 0x0FA  0x120 |  0x138
		//        0              50   251             312
		
		borderX = 0x21 * viewVicDisplay->rasterScaleFactorX;
	}

	LOGD("CheckDisplayBoundaries: px=%f py=%f posX=%f posY=%f displayX=%f displayY=%f",
		 *px, *py, posX, posY, viewVicDisplay->posX, viewVicDisplay->posY);

//	float lx = mainDisplayPosX - borderX;
//	if (*px < lx)
//	{
//		*px = lx;
//	}
	
//	if (*px > -borderX)
//		*px = -borderX;
//
//	if (*py > 0.0f)
//		*py = 0.0f;
	
//	if (*px + (mainDisplayPosX + viewVicDisplay->visibleScreenSizeX + viewVicDisplay->offsetPosX
//			  + borderX + viewVicDisplay->rasterScaleFactorX) < mainDisplaySizeX)
//	{
//		*px = mainDisplaySizeX - (mainDisplayPosX + viewVicDisplay->visibleScreenSizeX + viewVicDisplay->offsetPosX
//			  + borderX + viewVicDisplay->rasterScaleFactorX);
//
//		// keep it centered
////		LOGD("   px=%f", px);
////		if (px > -borderX/4.0f)
////		{
////			px = -borderX/4.0f + viewVicDisplay->rasterScaleFactorX*2.0f;
////		}
//	}
//
//	if (*py + (mainDisplayPosY + viewVicDisplay->visibleScreenSizeY) < mainDisplaySizeY)
//	{
//		*py = mainDisplaySizeY - (mainDisplayPosY + viewVicDisplay->visibleScreenSizeY);
//	}
}

// TODO: this is almost equal as VicDisplay's ZoomDisplay, make this one method.
void CViewC64VicEditor::ZoomDisplay(float newScale)
{
	ZoomDisplay(newScale, guiMain->mousePosX, guiMain->mousePosY);
}

void CViewC64VicEditor::ZoomDisplay(float newScale, float anchorX, float anchorY)
{
	guiMain->LockMutex();

	if (newScale < 0.20f)
		newScale = 0.20f;

//	if (newScale < 1.80f)
//		newScale = 1.80f;
//
//	if (newScale > 60.0f)
//		newScale = 60.0f;
		
	float cx = anchorX;
	float cy = anchorY;
	
//	if (viewVicDisplaySmall->IsInsideView(cx, cy))
//	{
//		viewVicDisplay->SetDisplayScale(newScale);
//
//		MoveDisplayToPreviewScreenPos(cx, cy);
//	}
//	else
	{
		float px, py;
		viewVicDisplay->GetRasterPosFromScreenPosWithoutScroll(cx, cy, &px, &py);
		
		viewVicDisplay->SetDisplayScale(newScale);
		
		float pcx, pcy;
		viewVicDisplay->GetScreenPosFromRasterPosWithoutScroll(px, py, &pcx, &pcy);
		
		MoveDisplayDiff(cx-pcx-viewVicDisplay->posOffsetX, cy-pcy-viewVicDisplay->posOffsetY);
	}
	
	UpdateDisplayFrame();
	viewVicDisplay->UpdateGridLinesVisibleOnCurrentZoom();

	guiMain->UnlockMutex();
}

///////
void CViewC64VicEditor::SelectLayer(CVicEditorLayer *layer)
{
	LockMutex();
	if (this->selectedLayer != NULL)
	{
		this->selectedLayer->LayerSelected(false);
	}
	
	this->selectedLayer = layer;
	
	if (this->selectedLayer != NULL)
	{
		this->selectedLayer->LayerSelected(true);
	}
	
	UnlockMutex();
}

//
void CViewC64VicEditor::SetSpritesFramesVisible(bool showSpriteFrames)
{
	viewVicDisplay->showSpritesFrames = showSpriteFrames;
//	viewVicDisplaySmall->showSpritesFrames = showSpriteFrames;
	layerVirtualSprites->showSpriteFrames = showSpriteFrames;
}


//
void CViewC64VicEditor::PaletteColorChanged(u8 colorSource, u8 newColorValue)
{
	viewSprite->PaletteColorChanged(colorSource, newColorValue);
}

//
void CViewC64VicEditor::ShowPaintMessage(u8 result)
{
	if (result == PAINT_RESULT_ERROR)
	{
		viewC64->ShowMessageError("Can't paint now here");
	}
	else if (result == PAINT_RESULT_BLOCKED)
	{
		viewC64->ShowMessageWarning("Paint blocked");
	}
	else if (result == PAINT_RESULT_REPLACED_COLOR)
	{
		viewC64->ShowMessageInfo("Replaced color");
	}
	else if (result == PAINT_RESULT_OUTSIDE)
	{
//		viewC64->ShowMessageWarning("Outside");
	}
}

void CViewC64VicEditor::PaintBrushLineWithMessage(CVicEditorBrush *brush, int rx1, int ry1, int rx2, int ry2, u8 colorSource)
{
	int result = PaintBrushLine(brush, rx1, ry1, rx2, ry2, colorSource);
	ShowPaintMessage(result);
}

u8 CViewC64VicEditor::PaintBrushLine(CVicEditorBrush *brush, int rx0, int ry0, int rx1, int ry1, u8 colorSource)
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

void CViewC64VicEditor::PaintBrushWithMessage(CVicEditorBrush *brush, int rx, int ry, u8 colorSource)
{
	int result = PaintBrush(brush, rx, ry, colorSource);
	ShowPaintMessage(result);
}

bool CViewC64VicEditor::IsColorReplace()
{
//	LOGD("IsColorReplace: return %d | c64SettingsVicEditorForceReplaceColor %d = %d",
//		 guiMain->isControlPressed, c64SettingsVicEditorForceReplaceColor,
//		 (guiMain->isControlPressed | c64SettingsVicEditorForceReplaceColor));

	return (guiMain->isControlPressed | c64SettingsVicEditorForceReplaceColor);
}

u8 CViewC64VicEditor::PaintBrush(CVicEditorBrush *brush, int rx, int ry, u8 colorSource)
{
	LOGD("CViewVicEditor::PaintBrush: rx=%d ry=%d", rx, ry);
	int brushCenterX = floor((float)brush->GetWidth() / 2.0f);
	int brushCenterY = floor((float)brush->GetHeight() / 2.0f);
	
	int sx = rx - brushCenterX;
	int sy = ry - brushCenterY;
	
	LOGD("   ..sx=%d sy=%d", sx, sy);
	
	int result = PAINT_RESULT_OK;
	
	for (int y = 0; y < brush->GetWidth(); y++)
	{
		for (int x = 0; x < brush->GetHeight(); x++)
		{
			if (brush->GetPixel(x, y) > 0)
			{
				int r = PAINT_RESULT_OK;
				
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

u8 CViewC64VicEditor::PaintPixel(int rx, int ry, u8 colorSource)
{
	LOGD("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! CViewVicEditor::PaintPixel, colorSource=%d", colorSource);
	
	guiMain->LockMutex();
	
	int result;
	
	bool forceColorReplace = IsColorReplace();

	result = viewVicDisplay->currentCanvas->Paint(forceColorReplace, guiMain->isAltPressed,
												  rx, ry,
												  viewPalette->colorLMB, viewPalette->colorRMB, colorSource,
												  this->viewCharset->selectedChar);
	
	guiMain->UnlockMutex();
	
	return result;
}

// pure pixel paiting (for external API)
u8 CViewC64VicEditor::PaintPixelColor(bool forceColorReplace, int rx, int ry, u8 color, int selectedChar)
{
	guiMain->LockMutex();
	
	int result;
	
	result = viewVicDisplay->currentCanvas->Paint(forceColorReplace, false,
												  rx, ry,
												  color, color, VICEDITOR_COLOR_SOURCE_LMB,
												  selectedChar);
	
	guiMain->UnlockMutex();
	
	return result;
}

// pure pixel paiting (for external API)
u8 CViewC64VicEditor::PaintPixelColor(int rx, int ry, u8 color)
{
	guiMain->LockMutex();
	
	int result;
	
	result = viewVicDisplay->currentCanvas->Paint(true, false,
												  rx, ry,
												  color, color, VICEDITOR_COLOR_SOURCE_LMB,
												  0x00);
	
	guiMain->UnlockMutex();
	
	return result;
}


//@returns is consumed
bool CViewC64VicEditor::DoTap(float x, float y)
{
	LOGG("CViewVicEditor::DoTap:  x=%f y=%f", x, y);
	
	if (IsInsideView(x, y) && viewVicDisplay->IsInside(x, y))
	{
		if (guiMain->isShiftPressed && guiMain->isControlPressed)
		{
			float fRasterX, fRasterY;
			viewVicDisplay->GetRasterPosFromScreenPosWithoutScroll(x, y, &fRasterX, &fRasterY);

			int rx = floor(fRasterX);
			int ry = floor(fRasterY);

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

		return PaintUsingLeftMouseButton(x, y);
	}
	
//	if (CGuiView::DoTap(x, y) == false)
	{
//		if (viewVicDisplaySmall->visible && viewVicDisplaySmall->IsInsideView(x, y))
//		{
//			float fRasterX, fRasterY;
//			viewVicDisplaySmall->GetRasterPosFromScreenPos(x, y, &fRasterX, &fRasterY);
//
//			int rx = floor(fRasterX);
//			int ry = floor(fRasterY);
//
//			if (guiMain->isShiftPressed)
//			{
//				// get pixel color
//				u8 color;
//				if (GetColorAtRasterPos(rx, ry, &color))
//				{
//					viewPalette->colorLMB = color;
//				}
//				return true;
//			}
//			else
//			{
//				LOGD("PAINT! %f %f (%d %d)", x ,y, rx, ry);
//
//				if (isPainting == false)
//				{
//					StoreUndo();
//				}
//
//				prevRx = rx;
//				prevRy = ry;
//				PaintBrushWithMessage(this->currentBrush, rx, ry, VICEDITOR_COLOR_SOURCE_LMB);
//
//				isPainting = true;
//				isPaintingOnPreviewFrame = true;
//				return true;
//			}
//		}
		
//		if (isPaintingOnPreviewFrame)
//			return true;
	}
	return false;
}


bool CViewC64VicEditor::DoMove(float x, float y, float distX, float distY, float diffX, float diffY)
{
	LOGG("CViewVicEditor::DoMove: %f %f", x, y);
	
	if (!isPainting)
		return false;
	
	if (IsInsideView(x, y))
	{
		// update sprite
		if (viewSprite->visible && !(viewSprite->isSpriteLocked))
		{
			float rx, ry;
			viewVicDisplay->GetRasterPosFromScreenPosWithoutScroll(x, y, &rx, &ry);
			layerVirtualSprites->UpdateSpriteView((int)rx, (int)ry);
		}
								
		UpdateDisplayRasterPos();

		PaintUsingLeftMouseButton(x, y);
	}
	
	
	/*   THIS WAS HERE REMOVED AFTER REFACTOR */
//	if (CGuiView::DoMove(x, y, distX, distY, diffX, diffY) == false)
//	{
//		if (viewVicDisplaySmall->visible &&
//			!viewVicDisplaySmall->viewFrame->movingView
//			&& viewVicDisplaySmall->IsInsideView(x, y))
//		{
//			float fRasterX, fRasterY;
//			viewVicDisplaySmall->GetRasterPosFromScreenPos2(x, y, &fRasterX, &fRasterY);
//
//			int rx = floor(fRasterX);
//			int ry = floor(fRasterY);
//
//			//LOGD("PAINT! %f %f (%d %d)", x ,y, rx, ry);
//
//			if (guiMain->isShiftPressed)
//			{
//				LOGD("        !! get color at %d %d", rx, ry);
//				u8 color;
//				if (GetColorAtRasterPos(rx, ry, &color))
//				{
//					viewPalette->colorLMB = color;
//				}
//				return true;
//			}
//			else
//			{
//				if (isPainting == false)
//				{
//					StoreUndo();
//				}
//
//				if (prevRx < -999)
//				{
//					prevRx = rx;
//					prevRy = ry;
//				}
//
////				PaintBrush(this->currentBrush, rx, ry, VICEDITOR_COLOR_SOURCE_LMB);
//				PaintBrushLineWithMessage(this->currentBrush, prevRx, prevRy, rx, ry, VICEDITOR_COLOR_SOURCE_LMB);
//
//				prevRx = rx;
//				prevRy = ry;
//
//				isPainting = true;
//				isPaintingOnPreviewFrame = true;
//
//				//		viewVicDisplaySmall->mousePosX = x;
//				//		viewVicDisplaySmall->mousePosY = y;
//
//				return true;
//			}
//
//		}
//
//		if (isPaintingOnPreviewFrame)
//			return true;
//
//	if (IsInsideView(x,y) && viewVicDisplay->IsInside(x, y))
//	{
//		// update sprite
//		if (viewSprite->visible && !(viewSprite->isSpriteLocked))
//		{
//			float rx, ry;
//			viewVicDisplay->GetRasterPosFromScreenPosWithoutScroll(x, y, &rx, &ry);
//			layerVirtualSprites->UpdateSpriteView((int)rx, (int)ry);
//		}
//
//		return false;
//	}
	return false;
}

bool CViewC64VicEditor::DoRightClick(float x, float y)
{
	LOGD("CViewVicEditor::DoRightClick: %f %f", x, y);
	
	LOGD("IsInsideView: %s", STRBOOL(IsInsideView(x,y)));
	LOGD("viewVicDisplay->IsInside: %s", STRBOOL(viewVicDisplay->IsInside(x, y)));

	if (IsInsideView(x, y) && viewVicDisplay->IsInside(x, y))
	{
		return PaintUsingRightMouseButton(x, y);
	}

	if (IsInside(x, y))
	{
		isShowingContextMenu = true;
		return true;
	}
	
//	if (viewVicDisplaySmall->visible && viewVicDisplaySmall->IsInsideView(x, y))
//	{
//		MoveDisplayToPreviewScreenPos(x, y);
//		isMovingPreviewFrame = true;
//		return true;
//	}
//
//	if (isMovingPreviewFrame)
//		return true;
//
//	if (isPaintingOnPreviewFrame)
//		return true;
	
//	LOGD("CViewVicEditor: .................. CGuiView::DoRightClick(x, y)");
//	bool ret = CGuiView::DoRightClick(x, y);
//
//	LOGD("ret=%d", ret);
//
//	if (ret == false)
//	if (IsInsideView(x, y) && viewVicDisplay->IsInside(x, y))
//	{
//		LOGD("viewVicDisplay->IsInside: %f %f | %f %f %f %f", x, y, viewVicDisplay->posX, viewVicDisplay->posY, viewVicDisplay->sizeX, viewVicDisplay->sizeY);
//
//		// clicked on the background / vic main display?
//		if (viewVicDisplay->IsInside(x, y))
//		{
//			LOGD("          !!INSIDE viewVicDisplay");
//			float fRasterX, fRasterY;
//			viewVicDisplay->GetRasterPosFromScreenPos(x, y, &fRasterX, &fRasterY);
//
//			int rx = floor(fRasterX);
//			int ry = floor(fRasterY);
//
//			if (guiMain->isShiftPressed)
//			{
//				LOGD("        !! get color at %d %d", rx, ry);
//				u8 color;
//				if (GetColorAtRasterPos(rx, ry, &color))
//				{
//					viewPalette->colorRMB = color;
//				}
//				return true;
//			}
//			else
//			{
//				LOGD("PAINT! %f %f (%d %d)", x ,y, rx, ry);
//
//				if (isPainting == false)
//				{
//					StoreUndo();
//				}
//
//				prevRx = rx;
//				prevRy = ry;
//
//				PaintBrushWithMessage(this->currentBrush, rx, ry, VICEDITOR_COLOR_SOURCE_RMB);
//
//				isPainting = true;
//				return true;
//			}
//		}
//
//		LOGD("    ... false | NOT INSIDE");
//		return false;
//	}
	return false;
}

bool CViewC64VicEditor::DoRightClickMove(float x, float y, float distX, float distY, float diffX, float diffY)
{
	return false;
//	return CGuiView::DoRightClickMove(x, y, distX, distY, diffX, diffY);
}

//@returns is consumed
bool CViewC64VicEditor::DoFinishTap(float x, float y)
{
	LOGG("CViewVicEditor::DoFinishTap: %f %f", x, y);
	
	isPainting = false;
	prevRx = -1000;
	prevRy = -1000;
	
	return CGuiView::DoFinishTap(x, y);
}

bool CViewC64VicEditor::DoFinishRightClick(float x, float y)
{
	isPainting = false;
	prevRx = -1000;
	prevRy = -1000;
	return CGuiView::DoFinishRightClick(x, y);
}

//

bool CViewC64VicEditor::GetColorAtRasterPos(int rx, int ry, u8 *color)
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

// undo
void CViewC64VicEditor::DebugPrintUndo(char *header)
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

void CViewC64VicEditor::StoreUndo()
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
	
	this->Serialize(buffer, true, true, true);
	
	undoList.push_back(buffer);

	DebugPrintUndo("after StoreUndo");

	guiMain->UnlockMutex();
	
}

void CViewC64VicEditor::DoUndo()
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
	
	this->Serialize(buffer, true, true, true);
	
	redoList.push_back(buffer);

	///
	
	buffer = undoList.back();
	undoList.pop_back();

	
	DebugPrintUndo("after DoUndo");

	
	LOGD("   restore now buffer is=%x", buffer);

	buffer->Rewind();
	
	this->Deserialize(buffer, VIC_EDITOR_FILE_VERSION);
	
	poolList.push_back(buffer);
}

void CViewC64VicEditor::DoRedo()
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
	
	this->Serialize(buffer, true, true, true);
	
	undoList.push_back(buffer);

	//
	
	buffer = redoList.back();
	redoList.pop_back();

	
	DebugPrintUndo("after DoRedo");

	
	LOGD("   redo restore now buffer=%x", buffer);

	buffer->Rewind();

	this->Deserialize(buffer, VIC_EDITOR_FILE_VERSION);
	
	poolList.push_back(buffer);
}

//
void CViewC64VicEditor::EnsureCorrectScreenAndBitmapAddr()
{
	u16 screenBase = viewVicDisplay->screenAddress;
	u16 bitmapBase = viewVicDisplay->bitmapAddress;

	if (screenBase == 0x0000 || bitmapBase == 0x0000)
	{
//		viewC64->viewC64VicControl->lstScreenAddresses->SetListLocked(true);
//		viewC64->viewC64VicControl->lstScreenAddresses->SetElement(1, true, false);
		
		screenBase = 0x0400;
		viewVicDisplay->screenAddress = 0x0400;

//		viewC64->viewC64VicControl->lstBitmapAddresses->SetListLocked(true);
//		viewC64->viewC64VicControl->lstBitmapAddresses->SetElement(1, true, false);
		
		bitmapBase = 0x2000;
		viewVicDisplay->bitmapAddress = 0x2000;

		SetVicAddresses(0x0000, screenBase, 0x1000, bitmapBase);
	}
}

void CViewC64VicEditor::SetVicMode(bool isBitmapMode, bool isMultiColor, bool isExtendedBackground)
{
	this->SetVicModeRegsOnly(isBitmapMode, isMultiColor, isExtendedBackground);

	vicii_cycle_state_t *viciiState = &(viewC64->viciiStateToShow);
	viewVicDisplay->SetCurrentCanvas(isBitmapMode, isMultiColor, isExtendedBackground, 1);
	viewVicDisplay->currentCanvas->SetViciiState(viciiState);
//	viewVicDisplaySmall->SetCurrentCanvas(isBitmapMode, isMultiColor, isExtendedBackground, 1);
//	viewVicDisplaySmall->currentCanvas->SetViciiState(viciiState);
}

void CViewC64VicEditor::SetVicModeRegsOnly(bool isBitmapMode, bool isMultiColor, bool isExtendedBackground)
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

void CViewC64VicEditor::SetVicAddresses(int vbank, int screenAddr, int charsetAddr, int bitmapAddr)
{
	LOGD("CViewVicEditor::SetVicAddresses: vbank=%04x screen=%04x charset=%04x bitmap=%04x", vbank, screenAddr, charsetAddr, bitmapAddr);
	vbank = vbank >> 14;
	c64_glue_set_vbank(vbank, 0);

	int screen = (screenAddr - vbank) / 0x0400;
	int charset = (charsetAddr - vbank) / 0x0800;
	int bitmap = (bitmapAddr - vbank) / 0x2000;
	
	int d018 = ((screen << 4) & 0xF0) | ((bitmap << 3) & 0x08) | ((charset << 1) & 0x0E);
	
	debugInterface->SetVicRegister(0x18, d018);

	viewVicDisplay->screenAddress = screenAddr;
	viewVicDisplay->charsetAddress = charsetAddr;
	viewVicDisplay->bitmapAddress = bitmapAddr;

//	viewVicDisplaySmall->screenAddress = screenAddr;
//	viewVicDisplaySmall->charsetAddress = charsetAddr;
//	viewVicDisplaySmall->bitmapAddress = bitmapAddr;
}

//
void CViewC64VicEditor::Serialize(CByteBuffer *byteBuffer, bool storeVicRegisters, bool storeC64Memory, bool storeVicDisplayControlState)
{
//	vicii_cycle_state_t *viciiState = &(viewC64->viciiStateToShow);
	vicii_cycle_state_t *viciiState = &(viewC64->currentViciiState);

	///
	u8 mc;
	u8 eb;
	u8 bm;
	u8 blank;
	
	mc = (viciiState->regs[0x16] & 0x10) >> 4;
	bm = (viciiState->regs[0x11] & 0x20) >> 5;
	eb = (viciiState->regs[0x11] & 0x40) >> 6;
	
	viewVicControl->RefreshStateButtonsUI(&mc, &eb, &bm, &blank);

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

	viewVicControl->SetViciiPointersFromUI(&screen_addr, &charset_addr, &bitmap_addr);

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
	
	LOGD(".....> serializing layers, index=%d", byteBuffer->index);
	// serialize layers
	for (std::list<CVicEditorLayer *>::iterator it = this->layers.begin();
		 it != this->layers.end(); it++)
	{
		CVicEditorLayer *layer = *it;
		
		LOGD("....... serialize layer '%s', index=%d", layer->layerName, byteBuffer->index);
		layer->Serialize(byteBuffer);
	}
	
	// put current sprite pointers
	LOGD("....... serialize spritePointers, index=%d", byteBuffer->index);

	for (int i = 0; i < 8; i++)
	{
		u8 spritePointer = debugInterface->GetByteFromRamC64(screen_addr + 0x03F8 + i);
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
		debugInterface->GetWholeMemoryMapFromRam(c64memory);

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
		
		viewVicControl->SerializeState(byteBuffer);

	}
	else
	{
		byteBuffer->PutBool(false);
	}
	
}

void CViewC64VicEditor::Deserialize(CByteBuffer *byteBuffer, int version)
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
	viewVicDisplay->screenAddress = screenAddr;

	int charsetAddr = byteBuffer->GetI32();
//	viewC64->viewC64VicControl->lstCharsetAddresses->SetListLocked(true);
//	viewC64->viewC64VicControl->lstCharsetAddresses->SetElement(charsetAddr / 0x0800, true, false);

	int bitmapAddr = byteBuffer->GetI32();
//	viewC64->viewC64VicControl->lstBitmapAddresses->SetListLocked(true);
//	viewC64->viewC64VicControl->lstBitmapAddresses->SetElement(bitmapAddr / 0x2000, true, false);
	viewVicDisplay->bitmapAddress = bitmapAddr;
	
	SetVicAddresses(vbank, screenAddr, charsetAddr, bitmapAddr);
	
	//
		
	u8 colorD020 = byteBuffer->GetU8();
	debugInterface->SetVicRegister(0x20, colorD020);

	u8 colorD021 = byteBuffer->GetU8();
	debugInterface->SetVicRegister(0x21, colorD021);
	
	LOGD(" ......... deserializing layers, index=%d", byteBuffer->index);

	// deserialize layers
	for (std::list<CVicEditorLayer *>::iterator it = this->layers.begin();
		 it != this->layers.end(); it++)
	{
		CVicEditorLayer *layer = *it;
		
		LOGD(" ......... deserializing layer '%s', index=%d", layer->layerName, byteBuffer->index);

		layer->Deserialize(byteBuffer, version);
	}
	
	LOGD(" ..... deserializing sprite pointers, index=%d", byteBuffer->index);

	// get current sprite pointers
	for (int i = 0; i < 8; i++)
	{
		u8 spritePointer = byteBuffer->GetU8();
		debugInterface->SetByteToRamC64(screenAddr + 0x03F8 + i, spritePointer);
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
			debugInterface->SetVicRegister(i, regs[i]);
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
			debugInterface->SetByteToRamC64(i, c64memory[i]);
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
		viewVicControl->DeserializeState(byteBuffer);
	}
	
	//
	this->viewLayers->UpdateVisibleSwitchButtons();
}

//
// import export
void CViewC64VicEditor::OpenDialogImportFile()
{
	LOGM("OpenDialogImportFile");
	CSlrString *windowTitle = new CSlrString("Open image file to import");
	windowTitle->DebugPrint("windowTitle=");
	viewC64->ShowDialogOpenFile(this, &importFileExtensions, c64SettingsDefaultVicEditorFolder, windowTitle);
	delete windowTitle;
}

void CViewC64VicEditor::RecentlyOpenedFilesCallbackSelectedMenuItem(CSlrString *filePath)
{
	ImportImage(filePath, false);
}

void CViewC64VicEditor::SystemDialogFileOpenSelected(CSlrString *filePath)
{
	LOGM("CViewVicEditor::SystemDialogFileOpenSelected, filePath=%x", filePath);
	filePath->DebugPrint("filePath=");
	
	if (c64SettingsDefaultVicEditorFolder != NULL)
		delete c64SettingsDefaultVicEditorFolder;
	c64SettingsDefaultVicEditorFolder = filePath->GetFilePathWithoutFileNameComponentFromPath();
	c64SettingsDefaultVicEditorFolder->DebugPrint("c64SettingsDefaultVicEditorFolder=");
	C64DebuggerStoreSettings();

	ImportImage(filePath, false);
}

void CViewC64VicEditor::ImportImage(CSlrString *filePath)
{
	ImportImage(filePath, true);
}

void CViewC64VicEditor::ImportImage(CSlrString *filePath, bool onlyVicEditorFormats)
{
	CSlrString *ext = filePath->GetFileExtensionComponentFromPath();
	
	if (   ext->CompareWith("png") || ext->CompareWith("PNG")
		|| ext->CompareWith("jpg") || ext->CompareWith("JPG")
		|| ext->CompareWith("jpeg") || ext->CompareWith("JPEG")
		|| ext->CompareWith("bmp") || ext->CompareWith("BMP")
		|| ext->CompareWith("gif") || ext->CompareWith("GIF")
		|| ext->CompareWith("psd") || ext->CompareWith("PSD")
		|| ext->CompareWith("pic") || ext->CompareWith("PIC")
		|| ext->CompareWith("hdr") || ext->CompareWith("HDR")
		|| ext->CompareWith("tga") || ext->CompareWith("TGA"))
	{
		StoreUndo();
		ImportPNG(filePath);
		recentlyOpened->Add(filePath);
	}
	else if (ext->CompareWith("kla") || ext->CompareWith("KLA"))
	{
		StoreUndo();
		ImportKoala(filePath, true);
		recentlyOpened->Add(filePath);
	}
	else if (ext->CompareWith("dd") || ext->CompareWith("DD")
			 || ext->CompareWith("ddl") || ext->CompareWith("DDL"))
	{
		StoreUndo();
		ImportDoodle(filePath);
		recentlyOpened->Add(filePath);
	}
	else if (ext->CompareWith("aas") || ext->CompareWith("AAS")
			 || ext->CompareWith("art") || ext->CompareWith("ART"))
	{
		StoreUndo();
		ImportArtStudio(filePath);
		recentlyOpened->Add(filePath);
	}
	else if (ext->CompareWith("vce") || ext->CompareWith("VCE"))
	{
		StoreUndo();
		ImportVCE(filePath);
		recentlyOpened->Add(filePath);
	}
	else if (!onlyVicEditorFormats)
	{
		// TODO: move this into common open file interface, path will be deleted by SystemDialogFileOpenSelected
		viewC64->viewC64MainMenu->SystemDialogFileOpenSelected(filePath);
	}
	else
	{
		char *cExt = ext->GetStdASCII();
		
		char *buf = SYS_GetCharBuf();
		
		sprintf (buf, "VicEditor: Unknown image extension: %s", cExt);
		viewC64->ShowMessageError(buf);
		
		delete [] cExt;
		SYS_ReleaseCharBuf(buf);
	}
}

void CViewC64VicEditor::SystemDialogFileOpenCancelled()
{
	LOGD("CViewVicEditor::SystemDialogFileOpenCancelled");
}

// export
void CViewC64VicEditor::OpenDialogExportFile()
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

void CViewC64VicEditor::OpenDialogSaveVCE()
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

void CViewC64VicEditor::SystemDialogFileSaveSelected(CSlrString *path)
{
	LOGD("CViewVicEditor::SystemDialogFileSaveSelected");
	path->DebugPrint("path=");
	
	if (c64SettingsDefaultVicEditorFolder != NULL)
		delete c64SettingsDefaultVicEditorFolder;
	c64SettingsDefaultVicEditorFolder = path->GetFilePathWithoutFileNameComponentFromPath();
	c64SettingsDefaultVicEditorFolder->DebugPrint("c64SettingsDefaultVicEditorFolder=");
	C64DebuggerStoreSettings();

	recentlyOpened->Add(path);
	
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

void CViewC64VicEditor::SystemDialogFileSaveCancelled()
{
}

u8 CViewC64VicEditor::GetExportModeFromVicState(vicii_cycle_state_t *viciiState)
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

// TODO: refactor these below and move to custom import/export classes (ie. CVicEditorFormatKoala,...)
void CViewC64VicEditor::ExportScreen(CSlrString *path)
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

bool CViewC64VicEditor::ImportPNG(CSlrString *path)
{
	// import png
	char *cPath = path->GetStdASCII();
	
	CImageData *imageData = new CImageData(cPath);
	delete [] cPath;
	if (imageData->width < 1 || imageData->height < 1)
	{
		const char *errorStr = imageData->GetLoadError();
		viewC64->ShowMessageError("Failed to load image: %s", errorStr);
		delete imageData;
		return false;
	}
	bool ret = ImportImage(imageData);
	delete imageData;

	if (ret)
	{
		CSlrString *str = path->GetFileNameComponentFromPath();
		str->Concatenate(" imported");
		viewC64->ShowMessageSuccess(str);
		delete str;
	}
	return ret;
}

bool CViewC64VicEditor::ImportImage(CImageData *imageData)
{
	guiMain->LockMutex();

	if (this->selectedLayer == layerReferenceImage || layerReferenceImage->isVisible)
	{
		// load png as reference image
		layerReferenceImage->LoadFrom(imageData);
		
		if (this->selectedLayer == layerReferenceImage)
		{
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
			if (viewVicDisplay->currentCanvas->ConvertFrom(imageData) == PAINT_RESULT_OK)
			{
			}
			else
			{
				viewC64->ShowMessageError("Import failed");
			}
		}
		else if (imageData->width == 384 && imageData->height == 272)
		{
			//		//-32.000000 -35.000000
			CImageData *interiorImage = IMG_CropImageRGBA(imageData, 32, 35, 320, 200);
			
			if (viewVicDisplay->currentCanvas->ConvertFrom(interiorImage) != PAINT_RESULT_OK)
			{
				guiMain->UnlockMutex();
				viewC64->ShowMessageError("Import failed");
				return false;
			}
			
			delete interiorImage;
			
			viewSprite->selectedColor = -1;
			
			CDebugInterfaceC64 *debugInterface = viewVicDisplay->debugInterface;
			
			CImageData *image = viewVicDisplay->currentCanvas->ReducePalette(imageData, viewVicDisplay);
			
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
//			guiMain->ShowMessageBox("Can't import image", "Image size should be 320x200 or 384x272");
			viewC64->ShowMessageWarning("Imported image is scaled, default resolution is 320x200 or 384x272.");
			
			CImageData *imageScaled;
			float isx, isy, px, py;
			if ( (float)(imageData->width)/320.0f > ((float)imageData->height)/200.0f)
			{
				isx = 320.0f;
				isy = 320.0f * (float)imageData->height / (float)imageData->width;
				px = 0;
				py = (200.0f - isy) / 2.0f;
			}
			else
			{
				isx = 200.0f * (float)imageData->width / (float)imageData->height;
				isy = 200.0f;
				px = (320.0f - isx) / 2.0f;
				py = 0;
			}
			
			LOGD("image w=%d h=%d", imageData->width, imageData->height);
			LOGD("px=%f py=%f", px, py);
			LOGD("isx=%f isy=%f", isx, isy);
			imageScaled = IMG_Scale(imageData, isx, isy);
			
			CImageData *imageToImport;
			
			if (isx == 320 && isy == 200)
			{
				imageToImport = imageScaled;
			}
			else
			{
				imageToImport = new CImageData(320, 200);
				for (int x = 0; x < isx; x++)
				{
					for (int y = 0; y < isy; y++)
					{
						u8 r,g,b,a;
						imageScaled->GetPixel(x, y, &r, &g, &b, &a);
						imageToImport->SetPixel(x + px, y + py, r, g, b, a);
					}
				}
				delete imageScaled;
			}

			bool ret = ImportImage(imageToImport);
			delete imageToImport;
			
			if (ret == false)
			{
				guiMain->UnlockMutex();
				return false;
			}
			
		}
	}
	
	guiMain->UnlockMutex();
	
	return true;
}

bool CViewC64VicEditor::ImportKoala(CSlrString *path, bool showMessage)
{
	guiMain->LockMutex();
	
	// sanity-check default VIC Display settings
	u8 *screen_ptr;
	u8 *color_ram_ptr;
	u8 *chargen_ptr;
	u8 *bitmap_low_ptr;
	u8 *bitmap_high_ptr;
	u8 d020colors[0x0F];
	
	viewVicDisplay->GetViciiPointers(&(viewC64->viciiStateToShow),
										 &screen_ptr, &color_ram_ptr, &chargen_ptr, &bitmap_low_ptr, &bitmap_high_ptr, d020colors);
	
	
	EnsureCorrectScreenAndBitmapAddr();

	u16 bitmapBase = viewVicDisplay->bitmapAddress;
	u16 screenBase = viewVicDisplay->screenAddress;
	
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
		viewC64->ShowMessageSuccess(str);
		delete str;
	}

	return true;
}

bool CViewC64VicEditor::ImportKoala(CSlrString *path, u16 bitmapAddress, u16 screenAddress, u16 colorRamAddress, u8 *colorD020, u8 *colorD021)
{
	// load file
	char *cPath = path->GetStdASCII();
	CSlrFileFromOS *file = new CSlrFileFromOS(cPath);
	delete [] cPath;
	
	if (!file->Exists())
	{
		delete file;
		viewC64->ShowMessageError("File not found");
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
		
		viewVicDisplay->debugInterface->SetByteC64(ptr, v);
		ptr++;
	}
	
	ptr = screenAddress;
	for (int i = 0; i < 0x03E8; i++)
	{
		u8 v = byteBuffer->GetU8();
		
		viewVicDisplay->debugInterface->SetByteC64(ptr, v);
		ptr++;
	}

	ptr = colorRamAddress;
	for (int i = 0; i < 0x03E8; i++)
	{
		u8 v = byteBuffer->GetU8();
		
		viewVicDisplay->debugInterface->SetByteC64(ptr, v);
		ptr++;
	}

	u8 bkg = byteBuffer->GetU8();
	
	*colorD020 = bkg;
	*colorD021 = bkg;
	
	return true;
}

bool CViewC64VicEditor::ImportDoodle(CSlrString *path)
{
	char *cPath = path->GetStdASCII();
	CSlrFileFromOS *file = new CSlrFileFromOS(cPath);
	delete [] cPath;
	
	if (!file->Exists())
	{
		delete file;
		viewC64->ShowMessageError("File not found");
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
	
	viewVicDisplay->GetViciiPointers(&(viewC64->viciiStateToShow),
										 &screen_ptr, &color_ram_ptr, &chargen_ptr, &bitmap_low_ptr, &bitmap_high_ptr, d020colors);
	
	EnsureCorrectScreenAndBitmapAddr();
	u16 bitmapBase = viewVicDisplay->bitmapAddress;
	u16 screenBase = viewVicDisplay->screenAddress;
	
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
		
		viewVicDisplay->debugInterface->SetByteC64(ptr, v);
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
		
		viewVicDisplay->debugInterface->SetByteC64(ptr, v);
		ptr++;
	}
	
	guiMain->UnlockMutex();
	
	CSlrString *str = path->GetFileNameComponentFromPath();
	str->Concatenate(" loaded");
	viewC64->ShowMessageSuccess(str);
	delete str;

	return true;
}

bool CViewC64VicEditor::ImportArtStudio(CSlrString *path)
{
	char *cPath = path->GetStdASCII();
	CSlrFileFromOS *file = new CSlrFileFromOS(cPath);
	delete [] cPath;
	
	if (!file->Exists())
	{
		delete file;
		viewC64->ShowMessageError("File not found");
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
	
	viewVicDisplay->GetViciiPointers(&(viewC64->viciiStateToShow),
										 &screen_ptr, &color_ram_ptr, &chargen_ptr, &bitmap_low_ptr, &bitmap_high_ptr, d020colors);
	
	EnsureCorrectScreenAndBitmapAddr();
	u16 bitmapBase = viewVicDisplay->bitmapAddress;
	u16 screenBase = viewVicDisplay->screenAddress;
	
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
		
		viewVicDisplay->debugInterface->SetByteC64(ptr, v);
		ptr++;
	}
	
	// load screen data $3F40-$4327
	ptr = screenBase;
	for (int i = 0; i < 0x03E8; i++)
	{
		u8 v = byteBuffer->GetU8();
		
		viewVicDisplay->debugInterface->SetByteC64(ptr, v);
		ptr++;
	}
	
	u8 bkg = byteBuffer->GetU8();

	viewC64->debugInterfaceC64->SetVicRegister(0x20, bkg);
	viewC64->debugInterfaceC64->SetVicRegister(0x21, bkg);
	
	guiMain->UnlockMutex();
	
	CSlrString *str = path->GetFileNameComponentFromPath();
	str->Concatenate(" loaded");
	viewC64->ShowMessageSuccess(str);
	delete str;

	return true;
}

bool CViewC64VicEditor::ExportKoala(CSlrString *path)
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
	
	viewVicDisplay->GetViciiPointers(&(viewC64->viciiStateToShow),
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
	viewC64->ShowMessageSuccess(str);
	delete str;
	
	LOGM("CViewVicEditor::ExportKoala: file saved");

	return true;
}

bool CViewC64VicEditor::ExportArtStudio(CSlrString *path)
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
	
	viewVicDisplay->GetViciiPointers(&(viewC64->viciiStateToShow),
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
	viewC64->ShowMessageSuccess(str);
	delete str;
	
	LOGM("CViewVicEditor::ExportArtStudio: file saved");

	return true;
}

bool CViewC64VicEditor::ExportRawText(CSlrString *path)
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
	
	viewVicDisplay->GetViciiPointers(&(viewC64->viciiStateToShow),
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
	viewC64->ShowMessageSuccess(str);
	delete str;
	
	LOGM("CViewVicEditor::ExportRawText: file saved");

	return true;

}

bool CViewC64VicEditor::ExportCharset(CSlrString *path)
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
	
	viewVicDisplay->GetViciiPointers(&(viewC64->viciiStateToShow),
										 &screen_ptr, &color_ram_ptr, &chargen_ptr, &bitmap_low_ptr, &bitmap_high_ptr, d020colors);
	
	//
	file->Write(chargen_ptr, 0x0800);
	
	file->Close();
	
	guiMain->UnlockMutex();
	
	LOGM("CViewVicEditor::ExportCharset: file saved");
	
	return true;
	
}

bool CViewC64VicEditor::ExportHyper(CSlrString *path)
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
	
	viewVicDisplay->GetViciiPointers(&(viewC64->viciiStateToShow),
										 &screen_ptr, &color_ram_ptr, &chargen_ptr, &bitmap_low_ptr, &bitmap_high_ptr, d020colors);
	
//	// load address
//	file->WriteByte(0x00);
//	file->WriteByte(0x20);
	
	// sprites
	for (int i = 0x4C00; i < 0x5C00; i++)
	{
		u8 v = viewVicDisplay->debugInterface->GetByteFromRamC64(i);
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
	
	viewC64->ShowMessageSuccess("File has been saved");
	
	return true;
}

//
bool CViewC64VicEditor::ExportPNG(CSlrString *path)
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

	viewVicDisplay->RefreshScreenImageData(&(viewC64->viciiStateToShow), 255, 255);
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

	imageCrop = IMG_CropImageRGBA(viewVicDisplay->imageDataScreen, 0, 0, 320, 200);
	
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
				u8 v = viewVicDisplay->debugInterface->GetByteFromRamC64(addr);
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
				
				ConvertSpriteDataToImage(spriteData, imageDataSprite, paintColorD021, spriteColor, viewVicDisplay->debugInterface, 0);
			}
			else
			{
				ConvertColorSpriteDataToImage(spriteData, imageDataSprite,
											  paintColorD021, paintColorD025, paintColorD026, paintColorSprite,
											  viewVicDisplay->debugInterface, 0, 0);
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
	viewC64->ShowMessageSuccess(str);
	delete str;
	
	LOGM("CViewVicEditor::ExportPNG: file saved");
	
	return true;
}

//
bool CViewC64VicEditor::ExportSpritesData(CSlrString *path)
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
				u8 v = viewVicDisplay->debugInterface->GetByteFromRamC64(addr);
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

void CViewC64VicEditor::SaveScreenshotAsPNG()
{
	exportMode = VICEDITOR_EXPORT_PNG;
	this->OpenDialogExportFile();
}

bool CViewC64VicEditor::ExportVCE(CSlrString *path)
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
	
	CByteBuffer *serializeBuffer = new CByteBuffer();
	this->Serialize(serializeBuffer, true, true, true);
	
	// uncompressed size
	byteBuffer->PutU32(serializeBuffer->length);
	
	// compress
	uLong outBufferSize = compressBound(serializeBuffer->length);
	u8 *outBuffer = new u8[outBufferSize];
	
	int result = compress2(outBuffer, &outBufferSize, serializeBuffer->data, serializeBuffer->length, 9);
	
	if (result != Z_OK)
	{
		guiMain->UnlockMutex();

		LOGError("zlib error: %d", result);
		viewC64->ShowMessageError("zlib error");
		delete [] outBuffer;
		delete serializeBuffer;
		delete byteBuffer;
		delete file;
		return false;
	}
	
	u32 outSize = (u32)outBufferSize;
	
	LOGD("..original size=%d compressed=%d", serializeBuffer->length, outSize);
	
	//byteBuffer->putU32(outSize);
	byteBuffer->putBytes(outBuffer, outSize);
	
	//
	byteBuffer->storeToFileNoHeader(file);
	
	file->Close();
	
	delete [] outBuffer;
	delete serializeBuffer;
	delete byteBuffer;
	delete file;
	
	guiMain->UnlockMutex();
	
	CSlrString *str = path->GetFileNameComponentFromPath();
	str->Concatenate(" saved");
	viewC64->ShowMessageSuccess(str);
	delete str;
	
	LOGM("CViewVicEditor::ExportVCE: file saved");
	
	return true;
}

bool CViewC64VicEditor::ImportVCE(CSlrString *path)
{
	LOGD("CViewVicEditor::ImportVCE");
	
	char *cPath = path->GetStdASCII();
	CSlrFileFromOS *file = new CSlrFileFromOS(cPath);
	delete [] cPath;
	
	if (!file->Exists())
	{
		delete file;
		viewC64->ShowMessageError("File not found");
		return false;
	}
	
	guiMain->LockMutex();
	
	u32 magic = file->ReadUnsignedInt();
	if (magic != VIC_EDITOR_FILE_MAGIC)
	{
		guiMain->UnlockMutex();

		viewC64->ShowMessageError("This is not VCE file format");
		delete file;
		return false;
	}
	
	u32 fileVersion = file->ReadUnsignedInt();
	if (fileVersion > VIC_EDITOR_FILE_VERSION)
	{
		guiMain->UnlockMutex();

		char *buf = SYS_GetCharBuf();
		sprintf(buf, "File v%d higher than supported %d.", fileVersion, VIC_EDITOR_FILE_VERSION);
		viewC64->ShowMessageError(buf);
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
	
	CByteBuffer *serializeBuffer = new CByteBuffer(uncompressedData, uncompressedSize);
	this->Deserialize(serializeBuffer, fileVersion);
	
	delete serializeBuffer;
	
	delete file;
	
	UpdateReferenceLayers();
	
	
	guiMain->UnlockMutex();
	
	CSlrString *str = path->GetFileNameComponentFromPath();
	str->Concatenate(" loaded");
	viewC64->ShowMessageSuccess(str);
	delete str;

	return true;
}

// context menu
bool CViewC64VicEditor::HasContextMenuItems()
{
	if (isShowingContextMenu)
		return true;
	
	if (IsInsideView(guiMain->mousePosX, guiMain->mousePosY))
	{
		return false;
	}
	return true;
}

void CViewC64VicEditor::RenderContextMenuItems()
{
	isShowingContextMenu = true;
	
	if (!IsVisible())
	{
		if (ImGui::MenuItem("Show VIC Editor"))
		{
			this->SetVisible(true);
		}
		ImGui::Separator();
	}
	
	if (ImGui::MenuItem("Create New Picture"))
	{
		CreateNewPicture();
	}
	if (ImGui::MenuItem("Clear Screen"))
	{
		ClearScreen();
	}
	ImGui::Separator();
		
	if (ImGui::MenuItem("Import from file", kbsVicEditorOpenFile->cstr))
	{
		this->OpenDialogImportFile();
	}
	
	if (ImGui::MenuItem("Save VCE", kbsVicEditorSaveVCE->cstr))
	{
		this->OpenDialogSaveVCE();
	}

	if (ImGui::MenuItem("Export to file", kbsVicEditorExportFile->cstr))
	{
		this->OpenDialogExportFile();
	}

	if (ImGui::MenuItem("Save as PNG", viewC64->mainMenuBar->kbsSaveScreenImageAsPNG->cstr))
	{
		this->SaveScreenshotAsPNG();
	}
	
	recentlyOpened->RenderImGuiMenu("Recent##CViewC64VicEditor");
	
	ImGui::Separator();
	if (ImGui::MenuItem("Undo", kbsVicEditorDoUndo->cstr))
	{
		kbsVicEditorDoUndo->Run();
	}
	if (ImGui::MenuItem("Redo", kbsVicEditorDoRedo->cstr))
	{
		kbsVicEditorDoRedo->Run();
	}
	
	ImGui::Separator();
	ImGui::MenuItem("C64 Charset", kbsVicEditorToggleWindowCharset->cstr, &viewC64->viewC64Charset->visible);
	ImGui::MenuItem("C64 Sprite", kbsVicEditorToggleWindowSprite->cstr, &viewC64->viewC64Sprite->visible);
	ImGui::MenuItem("C64 Palette", kbsVicEditorToggleWindowPalette->cstr, &viewC64->viewC64Palette->visible);
	ImGui::MenuItem("C64 Layers", kbsVicEditorToggleWindowLayers->cstr, &viewC64->viewVicEditorLayers->visible);

	ImGui::Separator();

	// add layout parameters
	bool colorChangeBlocksPaint = !c64SettingsVicEditorForceReplaceColor;
	if (ImGui::MenuItem("Block paint on char color change", NULL, &colorChangeBlocksPaint))
	{
		c64SettingsVicEditorForceReplaceColor = !colorChangeBlocksPaint;
		C64DebuggerStoreSettings();
	}
	
	bool showSpritesFrames = viewVicDisplay->showSpritesFrames;
	if (ImGui::MenuItem("Show sprites frames", kbsVicEditorToggleSpriteFrames->cstr, &showSpritesFrames))
	{
		SetSpritesFramesVisible(showSpritesFrames);
	}
	
	if (ImGui::BeginMenu("Image grid"))
	{
		if (ImGui::MenuItem("Show grid lines", kbsVicEditorShowGrid->cstr, &viewVicDisplay->showGridLines))
		{
		}
		
		if (ImGui::MenuItem("Automatic grid lines", NULL, &viewVicDisplay->gridLinesAutomatic))
		{
			viewC64->config->SetBool("VicDisplayAutomaticGridLines", &viewVicDisplay->gridLinesAutomatic);
		}
		if (ImGui::SliderFloat("Grid lines zoom level", &viewVicDisplay->gridLinesShowZoomLevel, 0.1f, 10.0f))
		{
			viewC64->config->SetFloat("VicDisplayAutomaticGridLinesShowZoomLevel", &viewVicDisplay->gridLinesShowZoomLevel);
		}
		if (ImGui::SliderFloat("Pixel values zoom level", &viewVicDisplay->gridLinesShowValuesZoomLevel, 0.1f, 40.0f))
		{
			viewC64->config->SetFloat("VicDisplayAutomaticValuesShowZoomLevel", &viewVicDisplay->gridLinesShowValuesZoomLevel);
		}
		ImGui::Separator();
		char *buf = SYS_GetCharBuf();
		sprintf(buf, "Current zoom level: %5.2f", viewVicDisplay->scale);
		ImGui::MenuItem(buf, NULL, false, false);
		SYS_ReleaseCharBuf(buf);
		ImGui::EndMenu();
	}

	ImGui::Separator();

	if (ImGui::MenuItem("Switch palette colors", kbsVicEditorSwitchPaletteColors->cstr))
	{
		kbsVicEditorSwitchPaletteColors->Run();
	}
	if (ImGui::MenuItem("Brush Rectangle +", kbsVicEditorRectangleBrushSizePlus->cstr))
	{
		kbsVicEditorRectangleBrushSizePlus->Run();
	}
	if (ImGui::MenuItem("Brush Rectangle -", kbsVicEditorRectangleBrushSizeMinus->cstr))
	{
		kbsVicEditorRectangleBrushSizeMinus->Run();
	}

	if (ImGui::MenuItem("Brush Circle +", kbsVicEditorCircleBrushSizePlus->cstr))
	{
		kbsVicEditorCircleBrushSizePlus->Run();
	}
	if (ImGui::MenuItem("Brush Circle -", kbsVicEditorCircleBrushSizeMinus->cstr))
	{
		kbsVicEditorCircleBrushSizeMinus->Run();
	}
}

