#ifndef _CVIEWC64SOURCECODE_H_
#define _CVIEWC64SOURCECODE_H_

// The Source Code view (RD-003/RD-004/RD-005, spec #10): a read-only
// document view of the sources the loaded debug info names, per CPU symbols.
//  - the document comes from CSourceDocumentRegistry, not from the database,
//    so a missing file shows its recorded path and where it was looked for
//    (never stale text), and an edited file is read again;
//  - syntax colours from CSyntaxLexer (C, 6502 dialects);
//  - gutter: breakpoint (filled: bound, hollow: a source breakpoint that
//    resolves to nothing), current PC arrow, a dot on lines that have code;
//    a click toggles a source breakpoint;
//  - follow: the PC (default), the disassembly cursor, or off; double-click
//    a line to show its code in the disassembly (a popup when it has several
//    ranges); the disassembly's "Show source" comes back here;
//  - header: file, follow mode, the last step's stop reason, stale debug
//    info warnings.

#include "CGuiView.h"
#include "CSourceDocument.h"
#include <memory>
#include <string>
#include <vector>

class CDebugInterface;
class CDebugSymbols;
class CDebugDatabase;
class CDebugSourceFile;
class CViewDisassembly;
class CSyntaxLexer;

enum class SourceViewFollow : u8
{
	Pc,
	Cursor,
	Off
};

class CViewSourceCode : public CGuiView
{
public:
	CViewSourceCode(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY,
					CDebugInterface *debugInterface, CDebugSymbols *debugSymbols,
					CViewDisassembly *viewDisassembly);
	virtual ~CViewSourceCode();

	virtual void RenderImGui();
	virtual bool HasContextMenuItems();
	virtual void RenderContextMenuItems();

	CViewDisassembly *viewDisassembly;
	CDebugSymbols *debugSymbols;
	CDebugInterface *debugInterface;

	CSourceDocumentRegistry documents;
	SourceViewFollow follow;
	bool showLineNumbers;

	// shows the source line of ADDRESS (the disassembly's "Show source"):
	// follow off, the line scrolled into view; false when ADDRESS has no line
	bool ShowAddress(int address);
	// shows LINE of the database file FILEID
	void ShowLine(u32 fileId, i32 line);

	// the current position, for the UI tests and remote
	bool HasFile() const { return hasFile; }
	u32 GetFileId() const { return fileId; }
	i32 GetPcLine() const { return pcLine; }
	i32 GetSelectedLine() const { return selectedLine; }
	// the document of a database file's own text, for a source with no file
	// on disk (code the embedded assembler assembled, RD-043)
	std::shared_ptr<const CSourceDocument> OpenFileText(const CDebugSourceFile *file);

	// a click on line LINE's gutter: toggles a source breakpoint there (or
	// removes the address breakpoints a user set on its code)
	void ToggleBreakpointAtLine(i32 line);
	// a double-click on LINE: its code in the disassembly; the addresses
	// LINE maps to in the current region, empty when none
	std::vector<u32> AddressesOfLine(i32 line);
	void NavigateToLine(i32 line);

	// Layout
	virtual void Serialize(CByteBuffer *byteBuffer);
	virtual void Deserialize(CByteBuffer *byteBuffer);

private:
	std::shared_ptr<const CDebugDatabase> database;		// held for the frame
	int regionId;

	bool hasFile;
	u32 fileId;
	i32 pcLine;				// 0 when the PC is not in this file
	int pcCandidates;		// mappings covering the PC (more than one: ambiguous)
	i32 selectedLine;
	i32 scrollToLine;		// 0: no request
	int lastFollowAddress;

	std::vector<u32> navigateAddresses;	// the popup's choices
	char sourceRootInput[512];

	void UpdatePosition();
	const CDebugSourceFile *CurrentFile() const;
	void RenderHeader(const CDebugSourceFile *file);
	void RenderMissing(const CDebugSourceFile *file);
	void RenderDocument(const CDebugSourceFile *file, const CSourceDocument *document);
	void RenderLineText(const std::string &text, const CSyntaxLexer *lexer, int *state);

	// line states for the gutter
	bool LineHasCode(i32 line) const;
	int LineBreakpointState(const CDebugSourceFile *file, i32 line) const;	// 0 none, 1 bound, 2 unresolved
};

#endif //_CVIEWC64SOURCECODE_H_
