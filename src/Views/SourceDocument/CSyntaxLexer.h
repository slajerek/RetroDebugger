#ifndef _CSyntaxLexer_h_
#define _CSyntaxLexer_h_

// Syntax highlighting for the Source Code view (RD-004): token categories
// only, the colours come from the view's theme table. One line at a time;
// STATE carries what a line leaves open (a block comment) into the next, so
// the view lexes the visible lines plus a short look-back, never the file.

#include "SYS_Defs.h"
#include <string>
#include <vector>

enum class SyntaxCategory : u8
{
	Text,			// identifiers and anything not classified
	Keyword,
	Type,
	Number,
	String,			// strings and character literals
	Comment,
	Preprocessor,	// C #lines, KickAssembler #import/#if
	Operator,
	Label,			// assembler label definitions
	Mnemonic,		// 6502 instructions, illegal opcodes included
	Directive		// assembler directives: .byte, .macro, *=
};

const char *SyntaxCategoryName(SyntaxCategory category);

struct SyntaxSpan
{
	int start;		// byte offset in the line
	int length;
	SyntaxCategory category;
};

class CSyntaxLexer
{
public:
	virtual ~CSyntaxLexer() {}
	virtual const char *GetName() const = 0;
	// Spans in order, whitespace left out. STATE: 0 at the start of a file.
	virtual void LexLine(const std::string &line, int *state, std::vector<SyntaxSpan> &out) const = 0;
};

class CSyntaxLexerC : public CSyntaxLexer
{
public:
	virtual const char *GetName() const override { return "C"; }
	virtual void LexLine(const std::string &line, int *state, std::vector<SyntaxSpan> &out) const override;
};

enum class Syntax6502Dialect : u8
{
	KickAss,		// ; // /* */ comments, .directives, #preprocessor, !local labels
	Tass64,			// ; comments, .directives
	Ca65			// ; comments, .directives, @local labels, labels end with ':'
};

class CSyntaxLexer6502 : public CSyntaxLexer
{
public:
	explicit CSyntaxLexer6502(Syntax6502Dialect dialect) : dialect(dialect) {}
	virtual const char *GetName() const override;
	virtual void LexLine(const std::string &line, int *state, std::vector<SyntaxSpan> &out) const override;

	Syntax6502Dialect dialect;
};

// The lexer for a source file: C for .c/.h, 6502 assembly for .asm/.s/.inc/
// .a65/.tas/.65s (the dialect from the producer of the debug info, else the
// extension); NULL for anything else (plain text). Static instances.
const CSyntaxLexer *SyntaxLexerForPath(const std::string &path, const std::string &producer);

#endif
