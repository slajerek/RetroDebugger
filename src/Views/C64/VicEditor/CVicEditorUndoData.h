#ifndef _VICEDITORUNDODATA_H_
#define _VICEDITORUNDODATA_H_

#include "SYS_Defs.h"

class CViewC64VicEditor;
class CByteBuffer;

class CVicEditorUndoData
{
public:
	CVicEditorUndoData(CViewC64VicEditor *vicEditor);
	~CVicEditorUndoData();
	
	CViewC64VicEditor *vicEditor;
	
	virtual void StoreUndoData();
	virtual void RestoreUndoData();

	CByteBuffer *undoDataBuffer;
};

#endif
