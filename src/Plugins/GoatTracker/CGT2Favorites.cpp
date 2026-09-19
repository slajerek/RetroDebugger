#include "CGT2Favorites.h"
#include "hjson.h"
#include "SYS_FileSystem.h"
#include <fstream>
#include <sstream>
#include <cstring>

extern "C" {
#include "gcommon.h"
#include "gsong.h"
}

// gPathToSettings is declared in the platform SYS_FileSystem.h but we
// include that above via SYS_FileSystem.h, so we access it directly.

CGT2Favorites::CGT2Favorites() {}

std::string CGT2Favorites::FilePath() const
{
	std::string base = gPathToSettings ? gPathToSettings : "";
	if (!base.empty() && base.back() != '/' && base.back() != '\\')
		base += "/";
	return base + "gt2/favorites.hjson";
}

// ---------------------------------------------------------------------------
// Save
// ---------------------------------------------------------------------------

void CGT2Favorites::Save()
{
	// Ensure gt2/ subfolder exists.
	std::string base = gPathToSettings ? gPathToSettings : "";
	if (!base.empty() && base.back() != '/' && base.back() != '\\')
		base += "/";
	std::string dir = base + "gt2";
	SYS_CreateFolder(dir.c_str());

	std::string path = FilePath();

	// Build the Hjson array.
	Hjson::Value root(Hjson::Type::Vector);

	for (size_t ei = 0; ei < entries.size(); ei++)
	{
		const GT2FavoriteEntry &e = entries[ei];
		const INSTRPACKAGE &p = e.package;

		Hjson::Value entry(Hjson::Type::Map);
		entry["name"] = e.displayName;

		// INSTR fields
		entry["ad"]         = (int)p.instr.ad;
		entry["sr"]         = (int)p.instr.sr;
		entry["vibdelay"]   = (int)p.instr.vibdelay;
		entry["gatetimer"]  = (int)p.instr.gatetimer;
		entry["firstwave"]  = (int)p.instr.firstwave;
		entry["instrname"]  = std::string(p.instr.name,
		                          strnlen(p.instr.name, MAX_INSTRNAMELEN));

		// ptr[4] (1-based source pointers captured at snapshot time)
		Hjson::Value ptrArr(Hjson::Type::Vector);
		for (int c = 0; c < MAX_TABLES; c++)
			ptrArr.push_back(Hjson::Value((int)p.origptr[c]));
		entry["ptr"] = ptrArr;

		// Per-table: length + ltab + rtab slices
		for (int c = 0; c < MAX_TABLES; c++)
		{
			char lenKey[32], lKey[32], rKey[32];
			snprintf(lenKey, sizeof(lenKey), "tablen%d", c);
			snprintf(lKey,   sizeof(lKey),   "ltab%d",   c);
			snprintf(rKey,   sizeof(rKey),   "rtab%d",   c);

			int len = p.tablen[c];
			entry[lenKey] = len;

			Hjson::Value lArr(Hjson::Type::Vector);
			Hjson::Value rArr(Hjson::Type::Vector);
			for (int i = 0; i < len; i++)
			{
				lArr.push_back(Hjson::Value((int)(unsigned char)p.ltab[c][i]));
				rArr.push_back(Hjson::Value((int)(unsigned char)p.rtab[c][i]));
			}
			entry[lKey] = lArr;
			entry[rKey] = rArr;
		}

		root.push_back(entry);
	}

	// Marshal to string and write to file.
	std::string text = Hjson::Marshal(root);

	FILE *fp = fopen(path.c_str(), "wb");
	if (fp)
	{
		fwrite(text.c_str(), 1, text.size(), fp);
		fclose(fp);
	}
}

// ---------------------------------------------------------------------------
// Load
// ---------------------------------------------------------------------------

void CGT2Favorites::Load()
{
	entries.clear();

	std::string path = FilePath();

	// Read the file; tolerate missing file by returning with empty entries.
	FILE *fp = fopen(path.c_str(), "rb");
	if (!fp)
		return;

	fseek(fp, 0, SEEK_END);
	long sz = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	if (sz <= 0)
	{
		fclose(fp);
		return;
	}

	std::string text(sz, '\0');
	fread(&text[0], 1, sz, fp);
	fclose(fp);

	Hjson::Value root;
	try
	{
		root = Hjson::Unmarshal(text);
	}
	catch (...)
	{
		return;
	}

	if (root.type() != Hjson::Type::Vector)
		return;

	for (size_t ei = 0; ei < root.size(); ei++)
	{
		Hjson::Value entry = root[(int)ei];
		if (entry.type() != Hjson::Type::Map)
			continue;

		GT2FavoriteEntry e;
		memset(&e.package, 0, sizeof(INSTRPACKAGE));

		// displayName
		Hjson::Value displayNameValue = entry["name"];
		if (displayNameValue.type() == Hjson::Type::String)
			e.displayName = static_cast<const std::string&>(displayNameValue);

		// INSTR fields
		if (entry["ad"].defined())        e.package.instr.ad        = (unsigned char)(int)entry["ad"];
		if (entry["sr"].defined())        e.package.instr.sr        = (unsigned char)(int)entry["sr"];
		if (entry["vibdelay"].defined())  e.package.instr.vibdelay  = (unsigned char)(int)entry["vibdelay"];
		if (entry["gatetimer"].defined()) e.package.instr.gatetimer = (unsigned char)(int)entry["gatetimer"];
		if (entry["firstwave"].defined()) e.package.instr.firstwave = (unsigned char)(int)entry["firstwave"];

		Hjson::Value instrumentNameValue = entry["instrname"];
		if (instrumentNameValue.type() == Hjson::Type::String)
		{
			std::string n = static_cast<const std::string&>(instrumentNameValue);
			strncpy(e.package.instr.name, n.c_str(), MAX_INSTRNAMELEN - 1);
			e.package.instr.name[MAX_INSTRNAMELEN - 1] = '\0';
		}

		// ptr[4]
		if (entry["ptr"].type() == Hjson::Type::Vector)
		{
			Hjson::Value ptrArr = entry["ptr"];
			for (int c = 0; c < MAX_TABLES && c < (int)ptrArr.size(); c++)
				e.package.origptr[c] = (unsigned char)(int)ptrArr[c];
		}
		// Also set instr.ptr from origptr so the instrument points correctly
		// when applied without tables (zero-len tables keep ptr=0 via apply logic).
		for (int c = 0; c < MAX_TABLES; c++)
			e.package.instr.ptr[c] = e.package.origptr[c];

		// Per-table slices
		for (int c = 0; c < MAX_TABLES; c++)
		{
			char lenKey[32], lKey[32], rKey[32];
			snprintf(lenKey, sizeof(lenKey), "tablen%d", c);
			snprintf(lKey,   sizeof(lKey),   "ltab%d",   c);
			snprintf(rKey,   sizeof(rKey),   "rtab%d",   c);

			int len = 0;
			if (entry[lenKey].defined())
				len = (int)entry[lenKey];
			if (len < 0) len = 0;
			if (len > MAX_TABLELEN) len = MAX_TABLELEN;
			e.package.tablen[c] = len;

			if (len > 0 && entry[lKey].type() == Hjson::Type::Vector
			            && entry[rKey].type() == Hjson::Type::Vector)
			{
				Hjson::Value lArr = entry[lKey];
				Hjson::Value rArr = entry[rKey];
				for (int i = 0; i < len; i++)
				{
					if (i < (int)lArr.size())
						e.package.ltab[c][i] = (unsigned char)(int)lArr[i];
					if (i < (int)rArr.size())
						e.package.rtab[c][i] = (unsigned char)(int)rArr[i];
				}
			}
		}

		e.package.valid = 1;
		entries.push_back(e);
	}
}

// ---------------------------------------------------------------------------
// AddFromInstrument / RemoveAt / ApplyTo
// ---------------------------------------------------------------------------

void CGT2Favorites::AddFromInstrument(int einum)
{
	GT2FavoriteEntry e;
	instrpackage_capture(einum, &e.package);

	// Trim the name (GT2 names are padded with spaces or nulls).
	const char *raw = ginstr[einum].name;
	size_t len = strnlen(raw, MAX_INSTRNAMELEN);
	// Trim trailing spaces.
	while (len > 0 && raw[len - 1] == ' ')
		len--;
	e.displayName = std::string(raw, len);
	if (e.displayName.empty())
		e.displayName = "Instrument";

	entries.push_back(e);
	Save();
}

void CGT2Favorites::RemoveAt(int index)
{
	if (index < 0 || index >= (int)entries.size())
		return;
	entries.erase(entries.begin() + index);
	Save();
}

void CGT2Favorites::ApplyTo(int einum, int index)
{
	if (index < 0 || index >= (int)entries.size())
		return;
	instrpackage_apply(einum, &entries[index].package);
}
