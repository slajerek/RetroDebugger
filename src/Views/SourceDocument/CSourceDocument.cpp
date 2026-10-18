#include "CSourceDocument.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <system_error>

static const std::string kEmptyLine;

std::filesystem::path SourcePath(const std::string &utf8)
{
	return std::filesystem::path(std::u8string(utf8.begin(), utf8.end()));
}

std::string SourcePathString(const std::filesystem::path &path)
{
	std::u8string s = path.u8string();
	return std::string(s.begin(), s.end());
}

static std::filesystem::path Utf8Path(const std::string &path)
{
	return SourcePath(path);
}

std::string SourceDocumentFileName(const std::string &path)
{
	size_t slash = path.find_last_of("/\\");
	return slash == std::string::npos ? path : path.substr(slash + 1);
}

static bool ModifiedTime(const std::string &path, i64 *time)
{
	std::error_code error;
	auto t = std::filesystem::last_write_time(Utf8Path(path), error);
	if (error)
		return false;
	*time = (i64)t.time_since_epoch().count();
	return true;
}

static void AppendExpanded(std::string &line, const std::string &raw)
{
	int column = 0;
	for (size_t i = 0; i < raw.size(); i++)
	{
		unsigned char c = (unsigned char)raw[i];
		if (c == '\t')
		{
			int spaces = CSourceDocument::kTabWidth - (column % CSourceDocument::kTabWidth);
			line.append(spaces, ' ');
			column += spaces;
			continue;
		}
		line.push_back((char)c);
		// a UTF-8 continuation byte does not start a column
		if ((c & 0xC0) != 0x80)
			column++;
	}
}

bool CSourceDocument::LoadFromText(const std::string &text)
{
	lines.clear();
	size_t begin = 0;
	if (text.size() >= 3 && (u8)text[0] == 0xEF && (u8)text[1] == 0xBB && (u8)text[2] == 0xBF)
		begin = 3;

	std::string raw;
	for (size_t i = begin; i < text.size(); i++)
	{
		char c = text[i];
		if (c == '\r' || c == '\n')
		{
			std::string line;
			AppendExpanded(line, raw);
			lines.push_back(line);
			raw.clear();
			if (c == '\r' && i + 1 < text.size() && text[i + 1] == '\n')
				i++;
			continue;
		}
		raw.push_back(c);
	}
	if (!raw.empty())
	{
		std::string line;
		AppendExpanded(line, raw);
		lines.push_back(line);
	}
	loaded = true;
	return true;
}

bool CSourceDocument::Load(const std::string &path)
{
	std::ifstream file(Utf8Path(path), std::ios::in | std::ios::binary);
	if (!file)
	{
		loaded = false;
		return false;
	}
	std::stringstream text;
	text << file.rdbuf();
	this->path = path;
	ModifiedTime(path, &modifiedTime);
	return LoadFromText(text.str());
}

const std::string &CSourceDocument::GetLine(int line) const
{
	if (line < 1 || line > (int)lines.size())
		return kEmptyLine;
	return lines[line - 1];
}

std::vector<std::string> CSourceDocumentRegistry::Candidates(const std::string &recordedPath, const std::string &hintPath) const
{
	std::vector<std::string> candidates;
	if (!hintPath.empty())
		candidates.push_back(hintPath);
	for (const auto &remap : pathRemaps)
	{
		if (!remap.first.empty() && recordedPath.compare(0, remap.first.size(), remap.first) == 0)
			candidates.push_back(remap.second + recordedPath.substr(remap.first.size()));
	}
	candidates.push_back(recordedPath);
	std::string fileName = SourceDocumentFileName(recordedPath);
	for (const std::string &root : sourceRoots)
	{
		if (root.empty())
			continue;
		char last = root[root.size() - 1];
		candidates.push_back((last == '/' || last == '\\') ? root + fileName : root + "/" + fileName);
	}
	return candidates;
}

std::shared_ptr<const CSourceDocument> CSourceDocumentRegistry::OpenText(const std::string &key, const void *textId,
																		 const std::string &text)
{
	auto it = texts.find(key);
	if (it != texts.end() && it->second.first == textId)
		return it->second.second;
	std::shared_ptr<CSourceDocument> document = std::make_shared<CSourceDocument>();
	document->LoadFromText(text);
	document->path = key;
	texts[key] = std::make_pair(textId, document);
	return document;
}

bool CSourceDocumentRegistry::HasText(const std::string &key, const void *textId) const
{
	auto it = texts.find(key);
	return it != texts.end() && it->second.first == textId;
}

std::shared_ptr<const CSourceDocument> CSourceDocumentRegistry::Open(const std::string &recordedPath, const std::string &hintPath)
{
	for (const std::string &candidate : Candidates(recordedPath, hintPath))
	{
		i64 time;
		if (!ModifiedTime(candidate, &time))
			continue;

		auto it = documents.find(candidate);
		if (it != documents.end() && it->second->modifiedTime == time)
			return it->second;

		std::shared_ptr<CSourceDocument> document = std::make_shared<CSourceDocument>();
		if (!document->Load(candidate))
			continue;
		documents[candidate] = document;
		return document;
	}
	return NULL;
}
