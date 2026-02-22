#ifndef _CVIEWSTACK_H_
#define _CVIEWSTACK_H_

#include "SYS_Defs.h"
#include "CGuiView.h"
#include "CStackAnnotation.h"

class CSlrFont;
class CDebugInterface;
class CDebugDataAdapter;
class CViewDisassembly;

#define STACK_VIEW_MAX_HIT_REGIONS 64

struct CStackAddrHitRegion
{
	float x, y, w, h;
	u16 addr;
};

class CViewStack : public CGuiView
{
public:
	CViewStack(const char *name, float posX, float posY, float posZ,
			   float sizeX, float sizeY,
			   CDebugInterface *debugInterface,
			   CStackAnnotationData *stackAnnotation,
			   CDebugDataAdapter *dataAdapter);

	virtual void RenderImGui();
	virtual void Render();
	virtual void DoLogic();
	virtual bool DoTap(float x, float y);

	virtual void SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY);
	virtual void LayoutParameterChanged(CLayoutParameter *layoutParameter);

	CDebugInterface *debugInterface;
	CStackAnnotationData *stackAnnotation;
	CDebugDataAdapter *dataAdapter;
	CViewDisassembly *viewDisassembly;

	CSlrFont *fontBytes;
	float fontSize;
	bool hasManualFontSize;

	// SP reader callback: set by owner to provide current SP register value
	typedef u8 (*ReadSPFunc)(void *context);
	ReadSPFunc readSPFunc;
	void *readSPContext;

	u8 currentSP;

	// Hit regions for origin PC addresses (populated during Render)
	CStackAddrHitRegion addrHitRegions[STACK_VIEW_MAX_HIT_REGIONS];
	int numAddrHitRegions;

	// Get the entry type name as a short string
	const char *GetEntryTypeName(u8 entryType);

	virtual void Serialize(CByteBuffer *byteBuffer);
	virtual void Deserialize(CByteBuffer *byteBuffer);
};

#endif
