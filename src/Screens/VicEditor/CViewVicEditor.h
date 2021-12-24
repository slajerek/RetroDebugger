#ifndef _VIEW_VICEDITOR_
#define _VIEW_VICEDITOR_

#include "CGuiViewWindowsManager.h"
#include "CGuiButton.h"
#include "CGuiViewMenu.h"
#include "SYS_FileSystem.h"
#include "CViewC64VicDisplay.h"
#include "CViewC64Charset.h"
#include "CViewC64Palette.h"
#include "CGlobalOSWindowChangedCallback.h"
#include "CViewVicEditorDisplayPreview.h"
#include "CGuiViewToolBox.h"
#include "DebuggerDefs.h"
#include <list>

class CSlrKeyboardShortcut;
class CViewC64MenuItem;
class CVicEditorLayer;
class CViewC64Sprite;
class CViewVicEditorLayers;
class CViewVicEditorCreateNewPicture;
class CViewToolBox;

class CVicEditorLayerC64Screen;
class CVicEditorLayerC64Canvas;
class CVicEditorLayerC64Sprites;
class CVicEditorLayerVirtualSprites;
class CVicEditorLayerUnrestrictedBitmap;
class CVicEditorLayerImage;

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

class CViewVicEditor : public CGuiViewWindowsManager, CGuiButtonCallback, CGuiViewMenuCallback, CViewC64PaletteCallback, CSystemFileDialogCallback, CGlobalOSWindowChangedCallback, CGuiViewToolBoxCallback
{
public:
	CViewVicEditor(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY);
	virtual ~CViewVicEditor();
	
	virtual void Render();
	virtual void Render(float posX, float posY);
	//virtual void Render(float posX, float posY, float sizeX, float sizeY);
	virtual void DoLogic();
	
	virtual bool DoTap(float x, float y);
	virtual bool DoFinishTap(float x, float y);
	
	virtual bool DoDoubleTap(float x, float y);
	virtual bool DoFinishDoubleTap(float posX, float posY);
	
	virtual bool DoRightClick(float x, float y);
	virtual bool DoRightClickMove(float x, float y, float distX, float distY, float diffX, float diffY);
	virtual bool FinishRightClickMove(float x, float y, float distX, float distY, float accelerationX, float accelerationY);

	virtual bool DoMove(float x, float y, float distX, float distY, float diffX, float diffY);
	virtual bool FinishMove(float x, float y, float distX, float distY, float accelerationX, float accelerationY);
	
	virtual bool InitZoom();
	virtual bool DoZoomBy(float x, float y, float zoomValue, float difference);
	
	virtual bool DoNotTouchedMove(float x, float y);

	// multi touch
	virtual bool DoMultiTap(COneTouchData *touch, float x, float y);
	virtual bool DoMultiMove(COneTouchData *touch, float x, float y);
	virtual bool DoMultiFinishTap(COneTouchData *touch, float x, float y);
	
	// TODO: remove me, FinishTouches does not exist anymore
	virtual void FinishTouches();
	
	virtual bool KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyPressed(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);	// repeats
	
	virtual void ActivateView();
	virtual void DeactivateView();
	
	virtual bool DoScrollWheel(float deltaX, float deltaY);

	CSlrFont *font;
	float fontScale;
	float fontHeight;
	float tr;
	float tg;
	float tb;
	
	CSlrString *strHeader;
	void SwitchToVicEditor();
	
	float mainDisplayPosX;
	float mainDisplayPosY;
	float mainDisplaySizeX;
	float mainDisplaySizeY;
	float mainDisplayPosEndX;
	float mainDisplayPosEndY;

	//
	CViewC64VicDisplay *viewVicDisplayMain;
	CViewVicEditorDisplayPreview *viewVicDisplaySmall;
	CViewC64Palette *viewPalette;
	CViewC64Charset *viewCharset;
	CViewC64Sprite *viewSprite;
	CViewVicEditorLayers *viewLayers;
	CViewVicEditorCreateNewPicture *viewCreateNewPicture;
	CGuiViewToolBox *viewTopBar;
	CGuiViewToolBox *viewToolBox;
	
	// layers
	std::list<CVicEditorLayer *> layers;
	CVicEditorLayerC64Screen *layerC64Screen;
	CVicEditorLayerC64Canvas *layerC64Canvas;
	CVicEditorLayerC64Sprites *layerC64Sprites;
	CVicEditorLayerVirtualSprites *layerVirtualSprites;
	CVicEditorLayerUnrestrictedBitmap *layerUnrestrictedBitmap;
	CVicEditorLayerImage *layerReferenceImage;

	CVicEditorLayer *selectedLayer;
	void SelectLayer(CVicEditorLayer *layer);
	
	bool IsColorReplace();
	
	// temp buffer for showing tools preview
	CVicEditorLayerUnrestrictedBitmap *layerToolPreview;
	bool toolIsActive;
	
	//
	void InitAddresses();
	
	void UpdateDisplayFrame();
	
	void UpdateReferenceLayers();
	
	void MoveDisplayDiff(float diffX, float diffY);
	void MoveDisplayToScreenPos(float px, float py);
	void MoveDisplayToPreviewScreenPos(float x, float y);
	void ZoomDisplay(float newScale);
	
	bool isKeyboardMovingDisplay;
	
	bool isPainting;
	
	float prevMousePosX;
	float prevMousePosY;
	
	int prevRx;
	int prevRy;
	
	bool isMovingPreviewFrame;
	bool isPaintingOnPreviewFrame;
	
	void ShowPaintMessage(u8 result);
	
	u8 PaintBrushLine(CImageData *brush, int rx0, int ry0, int rx1, int ry1, u8 colorSource);
	void PaintBrushLineWithMessage(CImageData *brush, int rx1, int ry1, int rx2, int ry2, u8 colorSource);
	u8 PaintBrush(CImageData *brush, int rx, int ry, u8 colorSource);
	void PaintBrushWithMessage(CImageData *brush, int rx, int ry, u8 colorSource);
	u8 PaintPixel(int rx, int ry, u8 colorSource);
	u8 PaintPixelColor(bool forceColorReplace, int rx, int ry, u8 color, int selectedChar);
	u8 PaintPixelColor(int rx, int ry, u8 color);

	
	bool GetColorAtRasterPos(int rx, int ry, u8 *color);
	
	bool inPresentationScreen;
	bool prevVisiblePreview;
	bool prevVisiblePalette;
	bool prevVisibleCharset;
	bool prevVisibleSprite;
	bool prevVisibleLayers;
	bool prevVisibleToolBox;
	bool prevVisibleTopBar;
	
	///
	
	void SetShowDisplayBorderType(u8 newBorderType);
	
	//
	void UpdateSmallDisplayScale();
	void ResetSmallDisplayScale(double newRealScale);
	int resetDisplayScaleIndex;
	
	CImageData *currentBrush;
	
	int brushSize;
	CImageData *CreateBrushCircle(int newSize);
	CImageData *CreateBrushRectangle(int newSize);
	
	//
	void EnsureCorrectScreenAndBitmapAddr();
	
	// undo
	void DebugPrintUndo(char *header);
	std::list<CByteBuffer *> poolList;
	std::list<CByteBuffer *> undoList;
	std::list<CByteBuffer *> redoList;
	void StoreUndo();
	void DoUndo();
	void DoRedo();
	void Serialise(CByteBuffer *byteBuffer, bool storeVicRegisters, bool storeC64Memory, bool storeVicDisplayControlState);
	void Deserialise(CByteBuffer *byteBuffer, int version);
	
	// import / export
	std::list<CSlrString *> importFileExtensions;
	std::list<CSlrString *> exportHiresBitmapFileExtensions;
	std::list<CSlrString *> exportMultiBitmapFileExtensions;
	std::list<CSlrString *> exportHiresTextFileExtensions;
	std::list<CSlrString *> exportHyperBitmapFileExtensions;
	std::list<CSlrString *> exportVCEFileExtensions;
	std::list<CSlrString *> exportPNGFileExtensions;
	
	u8 exportMode;
	
	u8 GetExportModeFromVicState(vicii_cycle_state_t *viciiState);

	void ExportScreen(CSlrString *path);
	
	void OpenDialogImportFile();

	virtual void SystemDialogFileOpenSelected(CSlrString *path);
	virtual void SystemDialogFileOpenCancelled();
	virtual void SystemDialogFileSaveSelected(CSlrString *path);
	virtual void SystemDialogFileSaveCancelled();

	void SetVicMode(bool isBitmapMode, bool isMultiColor, bool isExtendedBackground);
	void SetVicModeRegsOnly(bool isBitmapMode, bool isMultiColor, bool isExtendedBackground);
	void SetVicAddresses(int vbank, int screenAddr, int charsetAddr, int bitmapAddr);

	bool ImportVCE(CSlrString *path);
	bool ImportPNG(CSlrString *path);
	bool ImportKoala(CSlrString *path, bool showMessage);
	bool ImportKoala(CSlrString *path, u16 bitmapAddress, u16 screenAddress, u16 colorRamAddress, u8 *colorD020, u8 *colorD021);
	bool ImportDoodle(CSlrString *path);
	bool ImportArtStudio(CSlrString *path);
	
	void OpenDialogExportFile();
	void OpenDialogSaveVCE();
	u8 exportFileDialogMode;
	
	bool ExportVCE(CSlrString *path);
	bool ExportKoala(CSlrString *path);
	bool ExportArtStudio(CSlrString *path);
	bool ExportRawText(CSlrString *path);
	bool ExportHyper(CSlrString *path);

	bool ExportCharset(CSlrString *path);
	bool ExportSpritesData(CSlrString *path);
	
	void SaveScreenshotAsPNG();

	bool ExportPNG(CSlrString *path);
	
	// callback from palette on change color, TODO: change this to proper callback
	virtual void PaletteColorChanged(u8 colorSource, u8 newColorValue);
	
	// callback when OS window size is changed
	virtual void GlobalOSWindowChangedCallback();

	//
	std::vector<CGuiView *> traversalOfViews;
	bool CanSelectView(CGuiView *view);
	void MoveFocusToNextView();
	void MoveFocusToPrevView();

	void InitPaintGridColors();
	void InitPaintGridShowZoomLevel();

	bool backupRenderDataWithColors;
	
	//
	void ClearScreen();
	
	void RunC64EndlessLoop();
	
	// icons topbar
	CSlrImage *imgClear;
	CSlrImage *imgExport;
	CSlrImage *imgImport;
	CSlrImage *imgNew;
	CSlrImage *imgOpen;
	CSlrImage *imgSave;
	CSlrImage *imgSettings;

	CSlrImage *imgToolBrushCircle;
	CSlrImage *imgToolBrushSquare;
	CSlrImage *imgToolCircle;
	CSlrImage *imgToolDither;
	CSlrImage *imgToolFill;
	CSlrImage *imgToolLine;
	CSlrImage *imgToolPen;
	CSlrImage *imgToolRectangle;
	CSlrImage *imgToolAlwaysOnTopOn;
	CSlrImage *imgToolAlwaysOnTopOff;
	CGuiButton *btnToolAlwaysOnTop;

	void SetSpritesFramesVisible(bool showSpriteFrames);
	void SetTopBarVisible(bool isTopBarVisible);
	
	virtual void ToolBoxIconPressed(CSlrImage *imgIcon);

	void CreateNewPicture();
	
	/// tools
	virtual void ToolPaintPixel(int px, int py, u8 colorSource);
	virtual void ToolPaintBrush(int px, int py, u8 colorSource);
	virtual void ToolPaintPixelOnDraft(int px, int py, u8 colorSource);
	virtual void ToolPaintBrushOnDraft(int px, int py, u8 colorSource);

	virtual void ToolClearDraftImage();
	virtual void ToolMakeDraftActive();
	virtual void ToolMakeDraftNotActive();

	///
//	CSlrImage *imgMock;
	void RunDebug();
};


#endif //_VIEW_VICEDITOR_
