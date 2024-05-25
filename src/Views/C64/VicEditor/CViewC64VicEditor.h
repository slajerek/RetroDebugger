#ifndef _CViewVicEditor_h_
#define _CViewVicEditor_h_

#include "CGuiView.h"
#include "CGlobalOSWindowChangedCallback.h"

#include "CViewC64Palette.h"
#include "CSlrKeyboardShortcuts.h"
#include "ViceWrapper.h"
#include "CSystemFileDialogCallback.h"
#include "CRecentlyOpenedFiles.h"
#include <list>
#include <vector>
#include <map>

class CSlrFont;
class CSlrMutex;

class CDebugInterfaceC64;

class CViewC64VicDisplay;
class CViewC64VicControl;

class CViewC64VicEditorLayers;
class CViewC64Charset;
class CViewC64Sprite;
class CViewC64Palette;

class CViewC64VicEditorCreateNewPicture;

class CVicEditorBrush;
class CVicEditorLayer;
class CVicEditorLayerC64Screen;
class CVicEditorLayerC64Canvas;
class CVicEditorLayerC64Sprites;
class CVicEditorLayerVirtualSprites;
class CVicEditorLayerUnrestrictedBitmap;
class CVicEditorLayerImage;

class CViewC64VicEditor : public CGuiView, public CViewC64PaletteCallback, public CSlrKeyboardShortcutCallback, CGlobalOSWindowChangedCallback, CSystemFileDialogCallback, CRecentlyOpenedFilesCallback
{
public:
	///
	CDebugInterfaceC64 *debugInterface;
	
	CViewC64VicDisplay *viewVicDisplay;
	CViewC64VicControl *viewVicControl;
	void UpdateDisplayRasterPos();
	void MoveDisplayDiff(float diffX, float diffY);
	void MoveDisplayToScreenPos(float px, float py);
	void UpdateDisplayFrame();
	void CheckDisplayBoundaries(float *px, float *py);
	void ZoomDisplay(float newScale);
	void ZoomDisplay(float newScale, float anchorX, float anchorY);
	bool isKeyboardMovingDisplay;
	
	CSlrFont *font;
	float fontScale;
	float fontHeight;
	
	// painting
	void RunC64EndlessLoop();
	void CreateNewPicture();
	void ClearScreen();
	bool PaintUsingLeftMouseButton(float x, float y);
	bool PaintUsingRightMouseButton(float x, float y);
	void ShowPaintMessage(u8 result);
	u8 PaintBrushLine(CVicEditorBrush *brush, int rx0, int ry0, int rx1, int ry1, u8 colorSource);
	void PaintBrushLineWithMessage(CVicEditorBrush *brush, int rx1, int ry1, int rx2, int ry2, u8 colorSource);
	u8 PaintBrush(CVicEditorBrush *brush, int rx, int ry, u8 colorSource);
	void PaintBrushWithMessage(CVicEditorBrush *brush, int rx, int ry, u8 colorSource);
	u8 PaintPixel(int rx, int ry, u8 colorSource);
	u8 PaintPixelColor(bool forceColorReplace, int rx, int ry, u8 color, int selectedChar);
	u8 PaintPixelColor(int rx, int ry, u8 color);
	bool GetColorAtRasterPos(int rx, int ry, u8 *color);
	bool IsColorReplace();
	bool isPainting;
	int prevRx;
	int prevRy;
	float prevMousePosX;
	float prevMousePosY;
	
	//
	CViewC64VicEditorCreateNewPicture *viewCreateNewPicture;
	
	//
	CVicEditorBrush *currentBrush = NULL;
	int brushSize;
	
	CViewC64VicEditorLayers *viewLayers;
	CViewC64Charset *viewCharset;
	CViewC64Sprite *viewSprite;
	CViewC64Palette *viewPalette;
	void SetHelperViews(CViewC64VicControl *viewVicControl, CViewC64VicEditorLayers *viewLayers, CViewC64Charset *viewCharset, CViewC64Palette *viewPalette, CViewC64Sprite *viewSprite);
	
	//
	void InitPaintGridColors();
	
	//
	virtual void PaletteColorChanged(u8 colorSource, u8 newColorValue);
	
	//
	CSlrMutex *mutex;
	void LockMutex();
	void UnlockMutex();
	
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
	void UpdateReferenceLayers();
	
	//
	void SetSpritesFramesVisible(bool showSpriteFrames);
	
	// undo
	std::list<CByteBuffer *> poolList;
	std::list<CByteBuffer *> undoList;
	std::list<CByteBuffer *> redoList;
	void DebugPrintUndo(char *header);
	void StoreUndo();
	void DoUndo();
	void DoRedo();
	void Serialize(CByteBuffer *byteBuffer, bool storeVicRegisters, bool storeC64Memory, bool storeVicDisplayControlState);
	void Deserialize(CByteBuffer *byteBuffer, int version);
	
	// import/export
	std::list<CSlrString *> importFileExtensions;
	std::list<CSlrString *> exportHiresBitmapFileExtensions;
	std::list<CSlrString *> exportMultiBitmapFileExtensions;
	std::list<CSlrString *> exportHiresTextFileExtensions;
	std::list<CSlrString *> exportHyperBitmapFileExtensions;
	std::list<CSlrString *> exportVCEFileExtensions;
	std::list<CSlrString *> exportPNGFileExtensions;
	u8 exportMode;
	void OpenDialogImportFile();
	void OpenDialogExportFile();
	void OpenDialogSaveVCE();
	u8 exportFileDialogMode;
	u8 GetExportModeFromVicState(vicii_cycle_state_t *viciiState);
	virtual void SystemDialogFileOpenSelected(CSlrString *path);
	virtual void SystemDialogFileOpenCancelled();
	virtual void SystemDialogFileSaveSelected(CSlrString *path);
	virtual void SystemDialogFileSaveCancelled();
	virtual void RecentlyOpenedFilesCallbackSelectedMenuItem(CSlrString *filePath);
	void ImportImage(CSlrString *filePath);
	void ImportImage(CSlrString *filePath, bool onlyVicEditorFormats);
	CRecentlyOpenedFiles *recentlyOpened;
	
	bool ImportImage(CImageData *image);
	bool ImportVCE(CSlrString *path);
	bool ImportPNG(CSlrString *path);
	bool ImportKoala(CSlrString *path, bool showMessage);
	bool ImportKoala(CSlrString *path, u16 bitmapAddress, u16 screenAddress, u16 colorRamAddress, u8 *colorD020, u8 *colorD021);
	bool ImportDoodle(CSlrString *path);
	bool ImportArtStudio(CSlrString *path);
	bool ExportVCE(CSlrString *path);
	bool ExportKoala(CSlrString *path);
	bool ExportArtStudio(CSlrString *path);
	bool ExportRawText(CSlrString *path);
	bool ExportHyper(CSlrString *path);
	void ExportScreen(CSlrString *path);
	bool ExportCharset(CSlrString *path);
	bool ExportSpritesData(CSlrString *path);
	void SaveScreenshotAsPNG();
	bool ExportPNG(CSlrString *path);
	
	// helpers
	void EnsureCorrectScreenAndBitmapAddr();
	void SetVicMode(bool isBitmapMode, bool isMultiColor, bool isExtendedBackground);
	void SetVicModeRegsOnly(bool isBitmapMode, bool isMultiColor, bool isExtendedBackground);
	void SetVicAddresses(int vbank, int screenAddr, int charsetAddr, int bitmapAddr);
	
	// keyboard shortcuts
	CSlrKeyboardShortcut *kbsVicEditorCreateNewPicture;
	CSlrKeyboardShortcut *kbsVicEditorPreviewScale;
	CSlrKeyboardShortcut *kbsVicEditorShowCursor;
	CSlrKeyboardShortcut *kbsVicEditorShowGrid;
	CSlrKeyboardShortcut *kbsVicEditorDoUndo;
	CSlrKeyboardShortcut *kbsVicEditorDoRedo;
	CSlrKeyboardShortcut *kbsVicEditorOpenFile;
	CSlrKeyboardShortcut *kbsVicEditorExportFile;
	CSlrKeyboardShortcut *kbsVicEditorSaveVCE;
	//	CSlrKeyboardShortcut *kbsVicEditorLeaveEditor;
	CSlrKeyboardShortcut *kbsVicEditorClearScreen;
	CSlrKeyboardShortcut *kbsVicEditorRectangleBrushSizePlus;
	CSlrKeyboardShortcut *kbsVicEditorRectangleBrushSizeMinus;
	CSlrKeyboardShortcut *kbsVicEditorCircleBrushSizePlus;
	CSlrKeyboardShortcut *kbsVicEditorCircleBrushSizeMinus;
	CSlrKeyboardShortcut *kbsVicEditorToggleAllWindows;
	
	CSlrKeyboardShortcut *kbsVicEditorToggleWindowPreview;
	CSlrKeyboardShortcut *kbsVicEditorToggleWindowPalette;
	CSlrKeyboardShortcut *kbsVicEditorToggleWindowLayers;
	CSlrKeyboardShortcut *kbsVicEditorToggleWindowCharset;
	CSlrKeyboardShortcut *kbsVicEditorToggleWindowSprite;
	CSlrKeyboardShortcut *kbsVicEditorToggleSpriteFrames;
	CSlrKeyboardShortcut *kbsVicEditorToggleTopBar;
	CSlrKeyboardShortcut *kbsVicEditorToggleToolBox;
	CSlrKeyboardShortcut *kbsVicEditorSwitchPaletteColors;
	
	CSlrKeyboardShortcut *kbsVicEditorSelectNextLayer;
	
	virtual bool ProcessKeyboardShortcut(u32 zone, u8 actionType, CSlrKeyboardShortcut *keyboardShortcut);
	
	CViewC64VicEditor(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY);
	virtual ~CViewC64VicEditor();
	
	float prevDisplayPosX, prevDisplayPosY;
	virtual void SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY);
	
	virtual void Render();
	virtual void RenderImGui();
	virtual void DoLogic();
	
	virtual bool DoTap(float x, float y);
	virtual bool DoMove(float x, float y, float distX, float distY, float diffX, float diffY);
	
	virtual bool DoRightClick(float x, float y);
	virtual bool DoRightClickMove(float x, float y, float distX, float distY, float diffX, float diffY);
	
	virtual bool FinishMove(float x, float y, float distX, float distY, float accelerationX, float accelerationY);
	virtual bool DoNotTouchedMove(float x, float y);
	
	virtual bool DoFinishRightClick(float x, float y);
	virtual bool DoFinishTap(float x, float y);
	
	virtual bool DoDoubleTap(float x, float y);
	virtual bool DoFinishDoubleTap(float posX, float posY);
	
	virtual bool InitZoom();
	virtual bool DoZoomBy(float x, float y, float zoomValue, float difference);
	
	// multi touch
	virtual bool DoMultiTap(COneTouchData *touch, float x, float y);
	virtual bool DoMultiMove(COneTouchData *touch, float x, float y);
	virtual bool DoMultiFinishTap(COneTouchData *touch, float x, float y);
	
	virtual bool KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyDownRepeat(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyPressed(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);	// repeats
	
	virtual bool KeyDownOnMouseHover(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyUpOnMouseHover(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyDownGlobal(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);		// fired always, even on not focused and not visible
	virtual bool KeyUpGlobal(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	
	virtual bool DoScrollWheel(float deltaX, float deltaY);
	
	virtual bool HasContextMenuItems();
	virtual void RenderContextMenuItems();
	
	virtual void ActivateView();
	virtual void DeactivateView();
	
	
	// TODO: move me to CGuiView
	virtual bool KeyboardShortcut(CSlrKeyboardShortcut *shortcut);
	
	virtual void SerializeLayout(CByteBuffer *byteBuffer);
	virtual bool DeserializeLayout(CByteBuffer *byteBuffer, int version);
	float serializedDisplayX;
	float serializedDisplayY;
};

#endif

