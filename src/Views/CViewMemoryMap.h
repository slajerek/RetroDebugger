#ifndef __CViewMemoryMap__
#define __CViewMemoryMap__

#include "CGuiView.h"
#include "DebuggerDefs.h"
#include "SYS_Threading.h"

class CDebugInterface;
class CDataAdapter;
class CImageData;
class CSlrImage;
class CViewDataDump;
class CDebugInterface;
class CSlrFont;
class CViewMemoryMapCell;

class CViewMemoryMap : public CGuiView, CSlrThread
{
public:
	CViewMemoryMap(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY,
				   CDebugInterface *debugInterface, CDataAdapter *dataAdapter,
				   int imageWidth, int imageHeight, int ramSize, bool showCurrentExecutePC,
				   bool isFromDisk);
	~CViewMemoryMap();
	
	virtual void SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY);

	virtual bool KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyDownOnMouseHover(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyUpGlobal(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual void DoLogic();
	virtual void Render();
	virtual void RenderImGui();
	virtual bool HasContextMenuItems();
	virtual void RenderContextMenuItems();

	virtual bool DoTap(float x, float y);

	virtual bool DoScrollWheel(float deltaX, float deltaY);
	virtual bool InitZoom();
	virtual bool DoZoomBy(float x, float y, float zoomValue, float difference);
	virtual bool DoMove(float x, float y, float distX, float distY, float diffX, float diffY);
	virtual bool FinishMove(float x, float y, float distX, float distY, float accelerationX, float accelerationY);

	virtual bool DoRightClick(float x, float y);
	virtual bool DoRightClickMove(float x, float y, float distX, float distY, float diffX, float diffY);
	virtual bool FinishRightClickMove(float x, float y, float distX, float distY, float accelerationX, float accelerationY);

	virtual bool DoNotTouchedMove(float x, float y);

	CSlrFont *font;
	float fontScale;
	
	double updateMapNumberOfFps;
	bool updateMapIsAnimateEvents;
	
	volatile bool shouldRebindImage;
	
	// local copy of memory
	uint8 *memoryBuffer;
	
	float currentZoom;
	
	volatile bool cursorInside;
	float cursorX, cursorY;
	
	void ClearZoom();
	void ZoomMap(float zoom);
	void MoveMap(float diffX, float diffY);
	
	CDebugInterface *debugInterface;
	
	CViewMemoryMapCell **memoryCells;
	int ramSize;
	int imageWidth;
	int imageHeight;

	void UpdateWholeMap();
	
	void CellRead(int addr);
	void CellRead(int addr, int pc, int rasterX, int rasterY);
	void CellWrite(int addr, uint8 value);
	void CellWrite(int addr, uint8 value, int pc, int rasterX, int rasterY);
	void CellExecute(int addr, uint8 opcode);
	
	void CellsAnimationLogic(double targetFPS);
	void DriveROMCellsAnimationLogic();
	void UpdateMapColorsForCell(CViewMemoryMapCell *cell);
	
	CImageData *imageDataMemoryMap;
	CSlrImage *imageMemoryMap;
	
	bool showCurrentExecutePC;
	bool isFromDisk;
	
	int frameCounter;
	int nextScreenUpdateFrame;
	
	CViewDataDump *viewDataDump;
	void SetDataDumpView(CViewDataDump *viewDataDump);
	
	CDataAdapter *dataAdapter;
	void SetDataAdapter(CDataAdapter *newDataAdapter);

	bool IsExecuteCodeAddress(int address);
	void ClearExecuteMarkers();
	void ClearReadWriteMarkers();
	
	void UpdateTexturePosition(float newStartX, float newStartY, float newEndX, float newEndY);
	
	void HexDigitToBinary(uint8 hexDigit, char *buf);
	
	float textureStartX, textureStartY;
	float textureEndX, textureEndY;
	
	float renderTextureStartX, renderTextureStartY;
	float renderTextureEndX, renderTextureEndY;
	
	float mapPosX, mapPosY, mapSizeX, mapSizeY;
	float renderMapPosX, renderMapPosY, renderMapSizeX, renderMapSizeY;
	
	float cellSizeX, cellSizeY;
	float cellStartX, cellStartY;
	float cellEndX, cellEndY;
	int cellStartIndex;
	int numCellsInWidth;
	int numCellsInHeight;
	float currentFontDataScale;
	float textDataGapX;
	float textDataGapY;
	float currentFontAddrScale;
	float textAddrGapX;
	float textAddrGapY;
	float currentFontCodeScale;
	float textCodeCenterX;
	float textCodeGapY;
	float textCodeWidth;
	float textCodeWidth3;
	float textCodeWidth3h;
	float textCodeWidth6h;
	float textCodeWidth7h;
	float textCodeWidth8h;
	float textCodeWidth9h;
	float textCodeWidth10h;
	
	bool isBeingMoved;
	void UpdateMapPosition();
	
	// for double click
	long previousClickTime;
	int previousClickAddr;
	
	// move acceleration
	float accelerateX, accelerateY;
	
	bool isForcedMovingMap;
	float prevMousePosX;
	float prevMousePosY;

	// for cells animation
	virtual void ThreadRun(void *data);
	
	// Layout
	virtual void Serialize(CByteBuffer *byteBuffer);
	virtual void Deserialize(CByteBuffer *byteBuffer);
};



#endif
