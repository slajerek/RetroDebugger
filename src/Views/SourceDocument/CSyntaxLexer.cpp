#include "CSyntaxLexer.h"
#include <algorithm>
#include <cctype>
#include <cstring>

const char *SyntaxCategoryName(SyntaxCategory category)
{
	switch (category)
	{
		case SyntaxCategory::Text:			return "text";
		case SyntaxCategory::Keyword:		return "keyword";
		case SyntaxCategory::Type:			return "type";
		case SyntaxCategory::Number:		return "number";
		case SyntaxCategory::String:		return "string";
		case SyntaxCategory::Comment:		return "comment";
		case SyntaxCategory::Preprocessor:	return "preprocessor";
		case SyntaxCategory::Operator:		return "operator";
		case SyntaxCategory::Label:			return "label";
		case SyntaxCategory::Mnemonic:		return "mnemonic";
		case SyntaxCategory::Directive:		return "directive";
	}
	return "?";
}

enum { StateNormal = 0, StateBlockComment = 1 };

static bool IsIdentStart(char c) { return isalpha((unsigned char)c) || c == '_'; }
static bool IsIdentChar(char c) { return isalnum((unsigned char)c) || c == '_'; }

static bool InList(const char *const *list, const std::string &word)
{
	for (int i = 0; list[i]; i++)
	{
		if (word == list[i])
			return true;
	}
	return false;
}

static void Push(std::vector<SyntaxSpan> &out, int start, int end, SyntaxCategory category)
{
	if (end > start)
		out.push_back({ start, end - start, category });
}

// a quoted literal from I (the opening quote); returns the index past it,
// or the line end when it is not closed
static int SkipQuoted(const std::string &line, int i)
{
	char quote = line[i];
	int n = (int)line.size();
	for (i++; i < n; i++)
	{
		if (line[i] == '\\' && i + 1 < n)
		{
			i++;
			continue;
		}
		if (line[i] == quote)
			return i + 1;
	}
	return n;
}

// the end of a block comment opened before I, or -1 when the line ends first
static int FindBlockCommentEnd(const std::string &line, int i)
{
	size_t end = line.find("*/", i);
	return end == std::string::npos ? -1 : (int)end + 2;
}

// --- C ---

static const char *const kCKeywords[] = {
	"auto", "break", "case", "const", "continue", "default", "do", "else", "enum", "extern",
	"for", "goto", "if", "inline", "register", "restrict", "return", "sizeof", "static",
	"struct", "switch", "typedef", "union", "volatile", "while", "true", "false", "NULL",
	// cc65 and Oscar64 extensions
	"asm", "__asm", "__asm__", "__fastcall__", "__cdecl__", "__interrupt", "__hwinterrupt",
	"__zeropage", "__export", "__striped", "__native", "__noinline", "__forceinline",
	"__dynstack", "__noexit", "__reference", NULL
};

static const char *const kCTypes[] = {
	"void", "char", "short", "int", "long", "float", "double", "signed", "unsigned",
	"bool", "_Bool", "int8_t", "uint8_t", "int16_t", "uint16_t", "int32_t", "uint32_t",
	"size_t", "byte", "word", NULL
};

static int SkipCNumber(const std::string &line, int i)
{
	int n = (int)line.size();
	if (line[i] == '0' && i + 1 < n && (line[i + 1] == 'x' || line[i + 1] == 'X' || line[i + 1] == 'b' || line[i + 1] == 'B'))
		i += 2;
	while (i < n && (isalnum((unsigned char)line[i]) || line[i] == '.'))
		i++;
	return i;
}

void CSyntaxLexerC::LexLine(const std::string &line, int *state, std::vector<SyntaxSpan> &out) const
{
	out.clear();
	int n = (int)line.size();
	int i = 0;

	if (*state == StateBlockComment)
	{
		int end = FindBlockCommentEnd(line, 0);
		if (end < 0)
		{
			Push(out, 0, n, SyntaxCategory::Comment);
			return;
		}
		Push(out, 0, end, SyntaxCategory::Comment);
		*state = StateNormal;
		i = end;
	}

	// a preprocessor line: up to a comment
	int first = i;
	while (first < n && isspace((unsigned char)line[first]))
		first++;
	bool preprocessor = (first < n && line[first] == '#' && i == 0);

	while (i < n)
	{
		char c = line[i];
		if (isspace((unsigned char)c))
		{
			i++;
			continue;
		}
		if (c == '/' && i + 1 < n && line[i + 1] == '/')
		{
			Push(out, i, n, SyntaxCategory::Comment);
			return;
		}
		if (c == '/' && i + 1 < n && line[i + 1] == '*')
		{
			int end = FindBlockCommentEnd(line, i + 2);
			if (end < 0)
			{
				Push(out, i, n, SyntaxCategory::Comment);
				*state = StateBlockComment;
				return;
			}
			Push(out, i, end, SyntaxCategory::Comment);
			i = end;
			continue;
		}
		if (preprocessor)
		{
			// the directive and its arguments, as one span, up to a comment
			int end = i;
			while (end < n && !(line[end] == '/' && end + 1 < n && (line[end + 1] == '/' || line[end + 1] == '*')))
				end++;
			while (end > i && isspace((unsigned char)line[end - 1]))
				end--;
			Push(out, i, end, SyntaxCategory::Preprocessor);
			i = end;
			continue;
		}
		if (c == '"' || c == '\'')
		{
			int end = SkipQuoted(line, i);
			Push(out, i, end, SyntaxCategory::String);
			i = end;
			continue;
		}
		if (isdigit((unsigned char)c) || (c == '.' && i + 1 < n && isdigit((unsigned char)line[i + 1])))
		{
			int end = SkipCNumber(line, i);
			Push(out, i, end, SyntaxCategory::Number);
			i = end;
			continue;
		}
		if (IsIdentStart(c))
		{
			int end = i;
			while (end < n && IsIdentChar(line[end]))
				end++;
			std::string word = line.substr(i, end - i);
			SyntaxCategory category = InList(kCKeywords, word) ? SyntaxCategory::Keyword
				: InList(kCTypes, word) ? SyntaxCategory::Type : SyntaxCategory::Text;
			Push(out, i, end, category);
			i = end;
			continue;
		}
		// operators and punctuation, one character each
		Push(out, i, i + 1, SyntaxCategory::Operator);
		i++;
	}
}

// --- 6502 assembly ---

static const char *const k6502Mnemonics[] = {
	"adc", "and", "asl", "bcc", "bcs", "beq", "bit", "bmi", "bne", "bpl", "brk", "bvc", "bvs",
	"clc", "cld", "cli", "clv", "cmp", "cpx", "cpy", "dec", "dex", "dey", "eor", "inc", "inx",
	"iny", "jmp", "jsr", "lda", "ldx", "ldy", "lsr", "nop", "ora", "pha", "php", "pla", "plp",
	"rol", "ror", "rti", "rts", "sbc", "sec", "sed", "sei", "sta", "stx", "sty", "tax", "tay",
	"tsx", "txa", "txs", "tya",
	// NMOS illegal opcodes, the common names
	"lax", "sax", "dcp", "isc", "isb", "slo", "rla", "sre", "rra", "anc", "alr", "asr", "arr",
	"sbx", "axs", "las", "lae", "shy", "shx", "sha", "ahx", "tas", "shs", "xaa", "ane", "jam",
	"kil", "dop", "top", NULL
};

const char *CSyntaxLexer6502::GetName() const
{
	switch (dialect)
	{
		case Syntax6502Dialect::KickAss:	return "6502 (KickAssembler)";
		case Syntax6502Dialect::Tass64:		return "6502 (64tass)";
		case Syntax6502Dialect::Ca65:		return "6502 (ca65)";
	}
	return "6502";
}

static std::string Lower(const std::string &word)
{
	std::string lower = word;
	std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return (char)tolower(c); });
	return lower;
}

void CSyntaxLexer6502::LexLine(const std::string &line, int *state, std::vector<SyntaxSpan> &out) const
{
	out.clear();
	int n = (int)line.size();
	int i = 0;
	bool kickAss = (dialect == Syntax6502Dialect::KickAss);

	if (*state == StateBlockComment)
	{
		int end = FindBlockCommentEnd(line, 0);
		if (end < 0)
		{
			Push(out, 0, n, SyntaxCategory::Comment);
			return;
		}
		Push(out, 0, end, SyntaxCategory::Comment);
		*state = StateNormal;
		i = end;
	}

	bool sawStatement = false;		// a mnemonic or directive already on this line
	while (i < n)
	{
		char c = line[i];
		if (isspace((unsigned char)c))
		{
			i++;
			continue;
		}
		if (c == ';' || (kickAss && c == '/' && i + 1 < n && line[i + 1] == '/'))
		{
			Push(out, i, n, SyntaxCategory::Comment);
			return;
		}
		if (kickAss && c == '/' && i + 1 < n && line[i + 1] == '*')
		{
			int end = FindBlockCommentEnd(line, i + 2);
			if (end < 0)
			{
				Push(out, i, n, SyntaxCategory::Comment);
				*state = StateBlockComment;
				return;
			}
			Push(out, i, end, SyntaxCategory::Comment);
			i = end;
			continue;
		}
		if (c == '"' || (c == '\'' && i + 2 < n))
		{
			int end = SkipQuoted(line, i);
			Push(out, i, end, SyntaxCategory::String);
			i = end;
			continue;
		}
		// KickAssembler #import / #if; elsewhere '#' is the immediate operator
		if (kickAss && c == '#' && i + 1 < n && IsIdentStart(line[i + 1]) && !sawStatement)
		{
			int end = i + 1;
			while (end < n && IsIdentChar(line[end]))
				end++;
			Push(out, i, end, SyntaxCategory::Preprocessor);
			i = end;
			continue;
		}
		// *= origin, also written "* = $1000"
		if (c == '*' && !sawStatement)
		{
			int equals = i + 1;
			while (equals < n && line[equals] == ' ')
				equals++;
			if (equals < n && line[equals] == '=')
			{
				Push(out, i, equals + 1, SyntaxCategory::Directive);
				sawStatement = true;
				i = equals + 1;
				continue;
			}
		}
		if (c == '.' && i + 1 < n && IsIdentStart(line[i + 1]))
		{
			int end = i + 1;
			while (end < n && IsIdentChar(line[end]))
				end++;
			Push(out, i, end, SyntaxCategory::Directive);
			sawStatement = true;
			i = end;
			continue;
		}
		if (c == '$' || c == '%' || isdigit((unsigned char)c))
		{
			int end = i + 1;
			if (c == '$')
				while (end < n && isxdigit((unsigned char)line[end])) end++;
			else if (c == '%')
				while (end < n && (line[end] == '0' || line[end] == '1')) end++;
			else
				while (end < n && isalnum((unsigned char)line[end])) end++;
			if (end == i + 1 && !isdigit((unsigned char)c))
			{
				Push(out, i, end, SyntaxCategory::Operator);	// a lone $ or % (modulo)
			}
			else
			{
				Push(out, i, end, SyntaxCategory::Number);
			}
			i = end;
			continue;
		}
		// identifiers, with the local label prefixes of each dialect
		bool localPrefix = (c == '@' && dialect == Syntax6502Dialect::Ca65) || (c == '!' && kickAss && i + 1 < n && IsIdentChar(line[i + 1]));
		if (IsIdentStart(c) || localPrefix)
		{
			int end = i + 1;
			while (end < n && (IsIdentChar(line[end]) || (kickAss && line[end] == '.')))
				end++;
			if (kickAss && end < n && (line[end] == '+' || line[end] == '-') && localPrefix)
				end++;		// !loop+ / !loop-
			std::string word = line.substr(i, end - i);
			bool colon = (end < n && line[end] == ':');
			bool atStart = !sawStatement && out.empty();
			SyntaxCategory category = SyntaxCategory::Text;
			if (!sawStatement && word.size() == 3 && InList(k6502Mnemonics, Lower(word)) && !colon)
			{
				category = SyntaxCategory::Mnemonic;
				sawStatement = true;
			}
			else if (colon || (atStart && i == 0 && dialect != Syntax6502Dialect::Ca65))
			{
				// a label: "name:" anywhere before the statement, or a name in
				// the first column (KickAssembler, 64tass)
				category = SyntaxCategory::Label;
				if (colon)
					end++;
			}
			Push(out, i, end, category);
			i = end;
			continue;
		}
		Push(out, i, i + 1, SyntaxCategory::Operator);
		i++;
	}
}

// --- factory ---

static std::string Extension(const std::string &path)
{
	size_t slash = path.find_last_of("/\\");
	size_t dot = path.find_last_of('.');
	if (dot == std::string::npos || (slash != std::string::npos && dot < slash))
		return "";
	return Lower(path.substr(dot + 1));
}

const CSyntaxLexer *SyntaxLexerForPath(const std::string &path, const std::string &producer)
{
	static const CSyntaxLexerC lexerC;
	static const CSyntaxLexer6502 lexerKickAss(Syntax6502Dialect::KickAss);
	static const CSyntaxLexer6502 lexerTass64(Syntax6502Dialect::Tass64);
	static const CSyntaxLexer6502 lexerCa65(Syntax6502Dialect::Ca65);

	std::string ext = Extension(path);
	if (ext == "c" || ext == "h" || ext == "cpp" || ext == "hpp")
		return &lexerC;

	if (ext == "asm" || ext == "s" || ext == "inc" || ext == "a65" || ext == "tas" || ext == "65s" || ext == "a")
	{
		std::string lowerProducer = Lower(producer);
		if (lowerProducer.find("kick") != std::string::npos)
			return &lexerKickAss;
		if (lowerProducer.find("cc65") != std::string::npos || lowerProducer.find("ca65") != std::string::npos)
			return &lexerCa65;
		if (lowerProducer.find("64tass") != std::string::npos)
			return &lexerTass64;
		if (ext == "s" || ext == "inc")
			return &lexerCa65;
		if (ext == "a65" || ext == "tas")
			return &lexerTass64;
		return &lexerKickAss;
	}
	return NULL;
}
