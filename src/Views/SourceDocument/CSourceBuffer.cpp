#include "CSourceBuffer.h"
#include "CSourceDocument.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <system_error>

CSourceBuffer::CSourceBuffer(const std::string &path)
: path(path)
{
}

bool CSourceBuffer::ReadDiskTime(i64 *time) const
{
	std::error_code error;
	std::filesystem::file_time_type t = std::filesystem::last_write_time(SourcePath(path), error);
	if (error)
		return false;
	*time = (i64)t.time_since_epoch().count();
	return true;
}

bool CSourceBuffer::Load()
{
	std::ifstream in(SourcePath(path), std::ios::in | std::ios::binary);
	if (!in)
		return false;
	std::stringstream content;
	content << in.rdbuf();
	text = content.str();
	savedText = text;
	undoStack.clear();
	redoStack.clear();
	conflicting = false;
	diskExists = ReadDiskTime(&diskTime);
	return true;
}

bool CSourceBuffer::Save(std::string *error)
{
	namespace fs = std::filesystem;
	fs::path target = SourcePath(path);
	fs::path temporary = target;
	temporary += ".rdsave";
	{
		std::ofstream out(temporary, std::ios::out | std::ios::binary | std::ios::trunc);
		out << text;
		out.flush();
		if (!out)
		{
			if (error)
				*error = "cannot write " + SourcePathString(temporary);
			std::error_code ignored;
			fs::remove(temporary, ignored);
			return false;
		}
	}
	std::error_code renameError;
	fs::rename(temporary, target, renameError);
	if (renameError)
	{
		if (error)
			*error = "cannot replace " + path + ": " + renameError.message();
		std::error_code ignored;
		fs::remove(temporary, ignored);
		return false;
	}
	savedText = text;
	conflicting = false;
	diskExists = ReadDiskTime(&diskTime);
	return true;
}

void CSourceBuffer::SetText(const std::string &newText, u64 nowMs)
{
	if (newText == text)
		return;
	if (undoStack.empty() || nowMs - lastEditMs > kUndoBurstMs)
	{
		undoStack.push_back(text);
		if (undoStack.size() > kUndoLimit)
			undoStack.erase(undoStack.begin());
	}
	redoStack.clear();
	text = newText;
	lastEditMs = nowMs;
}

bool CSourceBuffer::Undo()
{
	if (undoStack.empty())
		return false;
	redoStack.push_back(text);
	text = undoStack.back();
	undoStack.pop_back();
	lastEditMs = 0;		// the next edit starts a new step
	return true;
}

bool CSourceBuffer::Redo()
{
	if (redoStack.empty())
		return false;
	undoStack.push_back(text);
	text = redoStack.back();
	redoStack.pop_back();
	lastEditMs = 0;
	return true;
}

CSourceBuffer::DiskState CSourceBuffer::CheckDisk()
{
	i64 time = 0;
	bool exists = ReadDiskTime(&time);
	if (!exists)
		return diskExists ? DiskState::Missing : DiskState::Unchanged;
	if (diskExists && time == diskTime)
		return DiskState::Unchanged;
	if (IsDirty())
	{
		conflicting = true;
		diskTime = time;
		diskExists = true;
		return DiskState::Conflict;
	}
	if (!Load())
		return DiskState::Missing;
	return DiskState::Reloaded;
}

bool CSourceBuffer::Revert()
{
	return Load();
}

int CSourceBuffer::NumLines() const
{
	if (text.empty())
		return 0;
	int lines = (int)std::count(text.begin(), text.end(), '\n');
	if (text.back() != '\n')
		lines++;
	return lines;
}

std::string CSourceBuffer::GetLine(int line) const
{
	if (line <= 0)
		return "";
	size_t start = 0;
	for (int i = 1; i < line; i++)
	{
		start = text.find('\n', start);
		if (start == std::string::npos)
			return "";
		start++;
	}
	size_t end = text.find('\n', start);
	std::string s = text.substr(start, end == std::string::npos ? std::string::npos : end - start);
	if (!s.empty() && s.back() == '\r')
		s.pop_back();
	return s;
}

long CSourceBuffer::Find(const std::string &needle, long from, bool caseSensitive) const
{
	if (needle.empty() || text.empty())
		return -1;
	auto equal = [caseSensitive](char a, char b) {
		return caseSensitive ? a == b : std::tolower((unsigned char)a) == std::tolower((unsigned char)b);
	};
	if (from < 0 || from >= (long)text.size())
		from = 0;
	auto search = [&](long begin, long end) -> long {
		auto it = std::search(text.begin() + begin, text.begin() + end, needle.begin(), needle.end(), equal);
		return it == text.begin() + end ? -1 : (long)(it - text.begin());
	};
	long found = search(from, (long)text.size());
	if (found < 0 && from > 0)
		found = search(0, std::min((long)text.size(), from + (long)needle.size() - 1));
	return found;
}

int CSourceBuffer::LineOfOffset(long offset) const
{
	if (offset < 0)
		return 0;
	return 1 + (int)std::count(text.begin(), text.begin() + std::min((long)text.size(), offset), '\n');
}
