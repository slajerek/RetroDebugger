#ifndef _CSourceDocument_h_
#define _CSourceDocument_h_

// Source documents for the Source Code view (RD-003, spec #10): the text of
// one source file as UTF-8 lines, independent of any debug database, plus a
// registry that finds a recorded path on this machine and reloads a file
// that changed on disk. Read-only for now; editing (RD-021) adds buffers on
// top of this, not a second model.

#include "SYS_Defs.h"
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

class CSourceDocument
{
public:
	// [0] is line 1. A UTF-8 BOM is dropped, LF, CRLF and lone CR end lines,
	// tabs expand to TABWIDTH columns.
	static const int kTabWidth = 4;

	std::string path;			// where the text was read from
	std::vector<std::string> lines;
	bool loaded = false;
	i64 modifiedTime = 0;		// of PATH when it was read, file clock ticks

	bool Load(const std::string &path);
	bool LoadFromText(const std::string &text);

	int NumLines() const { return (int)lines.size(); }
	// LINE is 1-based; empty for a line out of range
	const std::string &GetLine(int line) const;
};

// Path to document. A recorded path that does not exist is looked up (1)
// through each remap whose FROM prefixes it (FROM replaced by TO), (2) by its
// file name in each source root, in order. A document whose file changed
// since it was read is read again.
class CSourceDocumentRegistry
{
public:
	std::vector<std::pair<std::string, std::string>> pathRemaps;
	std::vector<std::string> sourceRoots;

	// the document for RECORDEDPATH, NULL when no candidate path exists;
	// HINTPATH (may be empty) is tried first, e.g. where a loader found it
	std::shared_ptr<const CSourceDocument> Open(const std::string &recordedPath, const std::string &hintPath);

	// every candidate Open would try, in order (the "locate" UI lists them)
	std::vector<std::string> Candidates(const std::string &recordedPath, const std::string &hintPath) const;

	// RD-043: a document from text in memory, for a source with no file (code
	// the embedded assembler assembled): one per KEY, built from TEXT again
	// only when TEXTID, which names that text's version, changes
	std::shared_ptr<const CSourceDocument> OpenText(const std::string &key, const void *textId, const std::string &text);
	bool HasText(const std::string &key, const void *textId) const;

	void Clear() { documents.clear(); texts.clear(); }

private:
	std::map<std::string, std::shared_ptr<CSourceDocument>> documents;	// by the path it was read from
	std::map<std::string, std::pair<const void *, std::shared_ptr<CSourceDocument>>> texts;
};

std::string SourceDocumentFileName(const std::string &path);

// UTF-8 strings and filesystem paths (u8path is deprecated in C++20, and
// u8string() returns std::u8string there)
std::filesystem::path SourcePath(const std::string &utf8);
std::string SourcePathString(const std::filesystem::path &path);

#endif
