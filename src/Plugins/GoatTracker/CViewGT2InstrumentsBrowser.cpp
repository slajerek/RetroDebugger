#include "CViewGT2InstrumentsBrowser.h"
#include "C64DebuggerPluginGoatTracker.h"
#include "CViewGT2Tables.h"
#include "CGT2FontAtlas.h"
#include "GT2ViewCommon.h"
#include "GT2RenderHelper.h"
#include "CConfigStorageHjson.h"
#include "IconsFontAwesome_c.h"
#include "SYS_FileSystem.h"
#include "CGuiMain.h"
#include "imgui.h"

extern "C" {
#include "gcommon.h"
#include "gsong.h"
#include "ginstr.h"
#include "ginstrops.h"
#include "goattrk2.h"
}

#include <algorithm>
#include <cstring>
#include <cctype>

// Walking an arbitrarily deep tree from a search box has to be bounded or a
// search started in "/" would freeze the UI. These caps are generous for an
// instrument collection and cheap enough to run synchronously on edit.
static const int kSearchMaxDepth = 8;
static const size_t kSearchMaxResults = 500;
static const size_t kSearchMaxFolders = 4000;

// currentPath is set two ways -- SetFolder() and the constructor's fallbacks --
// and only SetFolder used to strip a trailing separator. gCPathToDocuments ends
// with one, so a first run (no saved instrument folder) left currentPath
// slash-terminated and every search result lost its first character. Stripping
// lives here so both callers get it.
static void GT2StripTrailingSeparators(std::string &path);

static bool GT2PathIsSeparator(char c)
{
	// Accept both separators everywhere: a path can reach this view from a
	// config file or an OS drop written on another platform.
	return c == '/' || c == '\\';
}

static void GT2StripTrailingSeparators(std::string &path)
{
	// Never strip the root itself.
	while (path.size() > 1 && GT2PathIsSeparator(path[path.size() - 1]))
		path.erase(path.size() - 1);
}

static std::string GT2JoinPath(const std::string &folder, const std::string &name)
{
	if (folder.empty()) return name;
	if (GT2PathIsSeparator(folder[folder.size() - 1]))
		return folder + name;
	char sep[2] = { SYS_FILE_SYSTEM_PATH_SEPARATOR, 0 };
	return folder + sep + name;
}

// The folder a path lives in: a folder is its own, a file is its parent.
// Kept alongside GT2JoinPath so both halves of the path handling read together.
static std::string GT2FolderOf(const std::string &path, bool isDir)
{
	if (isDir || path.empty()) return path;
	for (size_t i = path.size(); i-- > 0; )
	{
		if (GT2PathIsSeparator(path[i]))
			return (i == 0) ? path.substr(0, 1) : path.substr(0, i);
	}
	return path;
}

static std::string GT2ToLower(const std::string &s)
{
	std::string r = s;
	for (size_t i = 0; i < r.size(); i++)
		r[i] = (char)tolower((unsigned char)r[i]);
	return r;
}

// Lists one folder. Passing NULL extensions returns every file, which is what
// the recursive search wants; the flat listing asks for "ins" and relies on
// SYS_GetFilesInFolder's case-insensitive match.
static std::vector<CFileItem *> *GT2ListFolder(const std::string &path, bool onlyInstruments)
{
	std::list<char *> extensions;
	char extIns[] = "ins";
	if (onlyInstruments)
		extensions.push_back(extIns);

	std::vector<char> pathBuf(path.begin(), path.end());
	pathBuf.push_back(0);

	return SYS_GetFilesInFolder(pathBuf.data(), onlyInstruments ? &extensions : NULL, true);
}

static void GT2FreeFolderListing(std::vector<CFileItem *> *files)
{
	if (files == NULL) return;
	while (!files->empty())
	{
		CFileItem *f = files->back();
		files->pop_back();
		delete f;
	}
	delete files;
}

bool CViewGT2InstrumentsBrowser::IsInstrumentFile(const char *name)
{
	if (name == NULL) return false;
	size_t len = strlen(name);
	if (len < 4) return false;
	const char *ext = name + len - 4;
	return ext[0] == '.'
		&& tolower((unsigned char)ext[1]) == 'i'
		&& tolower((unsigned char)ext[2]) == 'n'
		&& tolower((unsigned char)ext[3]) == 's';
}

CViewGT2InstrumentsBrowser::CViewGT2InstrumentsBrowser(const char *name, float posX, float posY, float posZ,
													   float sizeX, float sizeY, CGT2FontAtlas *fontAtlas)
: CGuiView(posX, posY, posZ, sizeX, sizeY)
{
	this->name = name;
	this->fontAtlas = fontAtlas;
	this->previewOnClick = false;
	this->searchBuffer[0] = 0;
	this->searchTruncated = false;
	this->listingDirty = true;
	this->breadcrumbScrollPending = true;
	this->contextIsDir = false;
	this->contextOpenPending = false;
	this->contextMenuOpen = false;

	// Own settings file, so these folders stay separate from File/Recents and
	// from the GT2 song recents. Same store CViewFileBrowser uses.
	this->favouriteFolders = new CRecentlyOpenedFiles(new CSlrString("favourites-gt2instruments"), this);

	if (pluginGoatTracker != NULL)
		this->currentPath = pluginGoatTracker->GetInstrumentFolder();
	if (this->currentPath.empty() && gCPathToDocuments != NULL)
		this->currentPath = gCPathToDocuments;
	// SetFolder() strips this; these two assignments bypass it, and
	// gCPathToDocuments ends with a separator.
	GT2StripTrailingSeparators(this->currentPath);
}

CViewGT2InstrumentsBrowser::~CViewGT2InstrumentsBrowser()
{
}

//
// Favourite folders
//

bool CViewGT2InstrumentsBrowser::IsFolderInFavourites(const char *path)
{
	if (favouriteFolders == NULL || path == NULL || path[0] == 0)
		return false;
	CSlrString *s = new CSlrString(path);
	bool exists = favouriteFolders->Exists(s);
	delete s;
	return exists;
}

void CViewGT2InstrumentsBrowser::AddFolderToFavourites(const char *path)
{
	if (favouriteFolders == NULL || path == NULL || path[0] == 0)
		return;
	CSlrString *s = new CSlrString(path);
	favouriteFolders->Add(s);
	delete s;
}

void CViewGT2InstrumentsBrowser::RemoveFolderFromFavourites(const char *path)
{
	if (favouriteFolders == NULL || path == NULL || path[0] == 0)
		return;
	CSlrString *s = new CSlrString(path);
	favouriteFolders->Remove(s);
	delete s;
}

void CViewGT2InstrumentsBrowser::RecentlyOpenedFilesCallbackSelectedMenuItem(CSlrString *filePath)
{
	// The favourites menu holds folders, so this is navigation, not a load.
	char *buf = filePath->GetStdASCII();
	SetFolder(buf);
	STRFREE(buf);
}

void CViewGT2InstrumentsBrowser::SetFolder(const char *path)
{
	if (path == NULL || path[0] == 0) return;

	std::string candidate = path;
	// So the breadcrumb does not grow an empty segment.
	GT2StripTrailingSeparators(candidate);

	if (!SYS_FileDirExists(candidate.c_str()))
		return;

	currentPath = candidate;
	listingDirty = true;
	breadcrumbScrollPending = true;
	if (!activeSearch.empty())
		RebuildSearch();

	if (pluginGoatTracker != NULL && pluginGoatTracker->gt2Config != NULL)
	{
		const char *c = currentPath.c_str();
		pluginGoatTracker->gt2Config->SetString("GoatTrackerInstrumentFolder", &c);
	}
}

void CViewGT2InstrumentsBrowser::GoToParent()
{
	if (currentPath.empty()) return;

	size_t cut = std::string::npos;
	for (size_t i = currentPath.size(); i-- > 0; )
	{
		if (GT2PathIsSeparator(currentPath[i])) { cut = i; break; }
	}
	if (cut == std::string::npos) return;

	// At the top level the parent is the root separator itself, not "".
	std::string parent = (cut == 0) ? currentPath.substr(0, 1) : currentPath.substr(0, cut);
	SetFolder(parent.c_str());
}

void CViewGT2InstrumentsBrowser::Refresh()
{
	listingDirty = true;
	if (!activeSearch.empty())
		RebuildSearch();
}

void CViewGT2InstrumentsBrowser::RebuildListing()
{
	entries.clear();
	listingDirty = false;
	if (currentPath.empty()) return;

	std::vector<CFileItem *> *files = GT2ListFolder(currentPath, true);
	if (files == NULL) return;

	for (size_t i = 0; i < files->size(); i++)
	{
		CFileItem *f = (*files)[i];
		if (f->name == NULL || f->name[0] == '.')   // skip dotfiles and dot-folders
			continue;
		Entry e;
		e.name = f->name;
		e.fullPath = f->fullPath != NULL ? f->fullPath : GT2JoinPath(currentPath, f->name);
		e.isDir = f->isDir;
		entries.push_back(e);
	}
	GT2FreeFolderListing(files);

	// Folders first, then names case-insensitively. The engine's own sort
	// does not guarantee the folders-first grouping this view wants.
	std::sort(entries.begin(), entries.end(), [](const Entry &a, const Entry &b) {
		if (a.isDir != b.isDir) return a.isDir;
		return GT2ToLower(a.name) < GT2ToLower(b.name);
	});
}

void CViewGT2InstrumentsBrowser::RebuildSearch()
{
	searchResults.clear();
	searchTruncated = false;
	if (activeSearch.empty() || currentPath.empty()) return;

	std::string needle = GT2ToLower(activeSearch);
	size_t foldersVisited = 0;

	// Breadth-limited DFS. Depth is tracked per queued folder so a deep tree
	// stops rather than recursing without bound; symlink loops are cut off by
	// the same limit.
	std::vector<std::pair<std::string, int> > pending;
	pending.push_back(std::make_pair(currentPath, 0));

	while (!pending.empty())
	{
		std::pair<std::string, int> cur = pending.back();
		pending.pop_back();

		if (++foldersVisited > kSearchMaxFolders) { searchTruncated = true; break; }

		std::vector<CFileItem *> *files = GT2ListFolder(cur.first, false);
		if (files == NULL) continue;

		for (size_t i = 0; i < files->size(); i++)
		{
			CFileItem *f = (*files)[i];
			if (f->name == NULL || f->name[0] == '.')
				continue;

			if (f->isDir)
			{
				if (cur.second + 1 <= kSearchMaxDepth)
					pending.push_back(std::make_pair(
						f->fullPath != NULL ? std::string(f->fullPath) : GT2JoinPath(cur.first, f->name),
						cur.second + 1));
				continue;
			}

			if (!IsInstrumentFile(f->name))
				continue;
			if (GT2ToLower(std::string(f->name)).find(needle) == std::string::npos)
				continue;

			if (searchResults.size() >= kSearchMaxResults) { searchTruncated = true; break; }

			Entry e;
			e.fullPath = f->fullPath != NULL ? f->fullPath : GT2JoinPath(cur.first, f->name);
			// Show where the hit lives, relative to the folder searched.
			//
			// Skip the separators rather than assuming exactly one. currentPath
			// may or may not end with one -- gCPathToDocuments does (see
			// SetFolder's fallback), GetInstrumentFolder() need not -- and
			// GT2JoinPath right above already accounts for both. Adding a fixed
			// +1 for "the slash" ate the first character of every filename
			// whenever the folder ended with a slash: searching a folder of
			// Instr01.ins / Instr02.ins listed them as nstr01.ins / nstr02.ins.
			e.name = RelativeDisplayName(e.fullPath, currentPath);
			e.isDir = false;
			searchResults.push_back(e);
		}
		GT2FreeFolderListing(files);

		if (searchTruncated) break;
	}

	std::sort(searchResults.begin(), searchResults.end(), [](const Entry &a, const Entry &b) {
		return GT2ToLower(a.name) < GT2ToLower(b.name);
	});
}

void CViewGT2InstrumentsBrowser::SetSearch(const char *text)
{
	strncpy(searchBuffer, text != NULL ? text : "", sizeof(searchBuffer) - 1);
	searchBuffer[sizeof(searchBuffer) - 1] = 0;
	activeSearch = searchBuffer;
	RebuildSearch();
}

std::vector<CViewGT2InstrumentsBrowser::Entry> *CViewGT2InstrumentsBrowser::ActiveList()
{
	if (!activeSearch.empty())
		return &searchResults;
	if (listingDirty)
		RebuildListing();
	return &entries;
}

int CViewGT2InstrumentsBrowser::GetEntryCount()
{
	return (int)ActiveList()->size();
}

std::string CViewGT2InstrumentsBrowser::RelativeDisplayName(const std::string &fullPath,
														   const std::string &base)
{
	// The label a search hit gets: its path relative to the folder searched.
	//
	// SKIP THE SEPARATORS, do not assume exactly one. This was
	// substr(base.size() + 1), where the +1 stood for "the slash" -- and base
	// does not always carry one. It ate the first character of every filename
	// whenever it did: a folder of Instr01.ins / Instr02.ins listed as
	// nstr01.ins / nstr02.ins. Reported 2026-09-09.
	//
	// currentPath is normalised now, so this is the second line of defence
	// rather than the fix -- which is why it is a static: it can be tested for
	// both shapes of base without going through SetFolder(), whose own
	// stripping would hide the case being tested.
	if (fullPath.size() <= base.size()
		|| base.empty()
		|| fullPath.compare(0, base.size(), base) != 0)
		return fullPath;

	size_t rel = base.size();
	while (rel < fullPath.size() && GT2PathIsSeparator(fullPath[rel]))
		rel++;
	return rel < fullPath.size() ? fullPath.substr(rel) : fullPath;
}

const char *CViewGT2InstrumentsBrowser::GetEntryDisplayName(int index)
{
	std::vector<Entry> *list = ActiveList();
	if (index < 0 || index >= (int)list->size()) return NULL;
	return (*list)[index].name.c_str();
}

bool CViewGT2InstrumentsBrowser::LoadEntry(int index)
{
	std::vector<Entry> *list = ActiveList();
	if (index < 0 || index >= (int)list->size()) return false;
	const Entry &e = (*list)[index];
	if (e.isDir) return false;
	if (pluginGoatTracker == NULL) return false;
	return pluginGoatTracker->LoadInstrumentFromFile(e.fullPath.c_str(), einum, previewOnClick);
}

bool CViewGT2InstrumentsBrowser::EnterEntry(int index)
{
	std::vector<Entry> *list = ActiveList();
	if (index < 0 || index >= (int)list->size()) return false;
	const Entry &e = (*list)[index];
	if (!e.isDir) return false;
	SetFolder(e.fullPath.c_str());
	return true;
}

void CViewGT2InstrumentsBrowser::RenderBreadcrumb(float uiScale)
{
	// A long path must not push the list off the window, so the breadcrumb
	// scrolls sideways on its own.
	float rowHeight = ImGui::GetFrameHeight() + ImGui::GetStyle().ScrollbarSize;
	ImGui::BeginChild("gt2InsBrowserPath", ImVec2(0, rowHeight), false,
					  ImGuiWindowFlags_HorizontalScrollbar);

	// Taken inside the child: the underline must be clipped to it, not drawn
	// on the parent window's list.
	ImDrawList *dl = ImGui::GetWindowDrawList();

	// Split the path into segments, remembering the prefix each one names.
	std::vector<std::pair<std::string, std::string> > segments;   // label, path
	{
		size_t i = 0;
		if (!currentPath.empty() && GT2PathIsSeparator(currentPath[0]))
		{
			segments.push_back(std::make_pair(std::string("/"), currentPath.substr(0, 1)));
			i = 1;
		}
		while (i < currentPath.size())
		{
			size_t end = i;
			while (end < currentPath.size() && !GT2PathIsSeparator(currentPath[end]))
				end++;
			if (end > i)
				segments.push_back(std::make_pair(currentPath.substr(i, end - i),
												  currentPath.substr(0, end)));
			i = end + 1;
		}
	}

	std::string navigateTo;
	for (size_t s = 0; s < segments.size(); s++)
	{
		if (s > 0)
		{
			ImGui::SameLine(0.0f, 0.0f);
			// The root segment is itself the separator -- emitting another
			// one after it would render "//home".
			bool previousWasRoot = (segments[s - 1].second.size() == 1
									&& GT2PathIsSeparator(segments[s - 1].second[0]));
			if (!previousWasRoot)
			{
				ImGui::TextDisabled("%c", SYS_FILE_SYSTEM_PATH_SEPARATOR);
				ImGui::SameLine(0.0f, 0.0f);
			}
		}

		ImGui::PushID((int)s);
		bool isLast = (s + 1 == segments.size());
		if (isLast)
			ImGui::TextUnformatted(segments[s].first.c_str());
		else
			ImGui::TextDisabled("%s", segments[s].first.c_str());

		if (ImGui::IsItemHovered())
		{
			// Underline the hovered folder and switch to the hand cursor, so
			// the breadcrumb reads as a row of links.
			ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
			ImVec2 mn = ImGui::GetItemRectMin();
			ImVec2 mx = ImGui::GetItemRectMax();
			dl->AddLine(ImVec2(mn.x, mx.y - 1.0f * uiScale), ImVec2(mx.x, mx.y - 1.0f * uiScale),
						ImGui::GetColorU32(ImGuiCol_Text), 1.0f * uiScale);
			ImGui::SetTooltip("%s", segments[s].second.c_str());
			if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
				navigateTo = segments[s].second;
		}
		ImGui::PopID();

		if (isLast && breadcrumbScrollPending)
		{
			ImGui::SetScrollHereX(1.0f);
			breadcrumbScrollPending = false;
		}
	}

	ImGui::EndChild();

	if (!navigateTo.empty())
		SetFolder(navigateTo.c_str());
}

void CViewGT2InstrumentsBrowser::RenderToolbar(float uiScale)
{
	CViewGT2Tables *tables = pluginGoatTracker ? pluginGoatTracker->viewTables : NULL;

	bool canUndo = tables && tables->CanUndoTableEdit();
	ImGui::BeginDisabled(!canUndo);
	if (ImGui::Button(ICON_FA_UNDO "##gt2InsBrowserUndo"))
		tables->UndoTableEdit();
	ImGui::EndDisabled();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
		ImGui::SetTooltip("Undo %s+Z -- steps back through the editor's changes, "
						  "so straight after a load it restores the instrument that was there before",
						  GT2_CmdKey());

	ImGui::SameLine();
	bool canRedo = tables && tables->CanRedoTableEdit();
	ImGui::BeginDisabled(!canRedo);
	if (ImGui::Button(ICON_FA_SHARE "##gt2InsBrowserRedo"))
		tables->RedoTableEdit();
	ImGui::EndDisabled();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
		ImGui::SetTooltip("Redo %s+Y", GT2_CmdKey());

	ImGui::SameLine();
	// ARROW_UP (U+f062), not LEVEL_UP (U+f148): the merged icon font is
	// FontAwesome 5 free solid, which has no level-up glyph -- it would draw a
	// "?" box. Every icon in this view is covered by the
	// ui/icon_font_glyphs_present test, which fails on a missing glyph instead
	// of letting one ship as a "?".
	if (ImGui::Button(ICON_FA_ARROW_UP "##gt2InsBrowserUp"))
		GoToParent();
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Parent folder");

	ImGui::SameLine();
	if (ImGui::Button(ICON_FA_REFRESH "##gt2InsBrowserRefresh"))
		Refresh();
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Rescan the folder");

	ImGui::SameLine();
	if (ImGui::Checkbox("Preview", &previewOnClick))
		PLUGIN_GoatTrackerSaveSettings();
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Play the loaded instrument with the note last used in the editor");

	// The target slot: clicking a file writes here.
	ImGui::SameLine();
	ImGui::TextDisabled("-> %02X %s", einum, einum >= 1 ? ginstr[einum].name : "");
}

void CViewGT2InstrumentsBrowser::MarkContextTarget(const Entry &entry)
{
	if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
	{
		contextPath = entry.fullPath;
		contextIsDir = entry.isDir;
		contextOpenPending = true;
	}
}

// Finder, Explorer, or whatever the desktop uses -- name the one the user has.
static const char *GT2FileManagerMenuLabel()
{
#if defined(MACOS) || defined(__APPLE__)
	return ICON_FA_FOLDER_OPEN "  Show in Finder";
#elif defined(WIN32) || defined(_WIN32)
	return ICON_FA_FOLDER_OPEN "  Show in Explorer";
#else
	return ICON_FA_FOLDER_OPEN "  Show in file manager";
#endif
}

void CViewGT2InstrumentsBrowser::RenderContextMenuBody(const std::string &path, bool isDir)
{
	// Favourites are folders; a file's menu offers its parent folder.
	std::string folder = GT2FolderOf(path, isDir);

	if (!path.empty())
	{
		size_t cut = path.find_last_of("/\\");
		const char *leaf = (cut == std::string::npos) ? path.c_str() : path.c_str() + cut + 1;
		ImGui::SeparatorText(leaf[0] != 0 ? leaf : path.c_str());

		if (ImGui::MenuItem(GT2FileManagerMenuLabel()))
			SYS_ShowFileInFileManager(path.c_str());
	}

	if (favouriteFolders != NULL && !favouriteFolders->listOfFiles.empty())
	{
		ImGui::SeparatorText("Favourite folders:");
		favouriteFolders->RenderImGuiMenu(NULL);
	}

	if (!folder.empty())
	{
		ImGui::Separator();
		if (IsFolderInFavourites(folder.c_str()))
		{
			// TIMES, not STAR_O: the outline star is FA4-only and is not in the
			// merged FA5 solid font -- it would draw as a missing-glyph box.
			if (ImGui::MenuItem(ICON_FA_TIMES "  Remove folder from favourites"))
				RemoveFolderFromFavourites(folder.c_str());
		}
		else
		{
			if (ImGui::MenuItem(ICON_FA_STAR "  Add folder to favourites"))
				AddFolderToFavourites(folder.c_str());
		}
	}
}

bool CViewGT2InstrumentsBrowser::HasContextMenuItems()
{
	return true;
}

void CViewGT2InstrumentsBrowser::RenderContextMenuItems()
{
	// The view's own menu is about the folder being listed -- there is no row
	// under the cursor when it opens from the title bar.
	RenderContextMenuBody(currentPath, true);
}

void CViewGT2InstrumentsBrowser::RenderList(float uiScale)
{
	std::vector<Entry> *list = ActiveList();

	float searchHeight = ImGui::GetFrameHeightWithSpacing();
	ImGui::BeginChild("gt2InsBrowserList", ImVec2(0, -searchHeight), true);

	int enterFolder = -1;
	int loadIndex = -1;

	// ".." only in the flat listing; in search results it would be meaningless.
	if (activeSearch.empty())
	{
		if (ImGui::Selectable(ICON_FA_ARROW_UP "  ..##gt2InsBrowserParentRow"))
			enterFolder = -2;
	}

	for (size_t i = 0; i < list->size(); i++)
	{
		const Entry &e = (*list)[i];
		ImGui::PushID((int)i);

		if (e.isDir)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.62f, 0.78f, 1.0f, 1.0f));
			if (ImGui::Selectable((std::string(ICON_FA_FOLDER "  ") + e.name).c_str()))
				enterFolder = (int)i;
			ImGui::PopStyleColor();
			MarkContextTarget(e);
		}
		else
		{
			// FILE (U+f15b), not FILE_O (U+f016) -- the outline variant is FA4
			// only and is not in the merged font.
			if (ImGui::Selectable((std::string(ICON_FA_FILE "  ") + e.name).c_str()))
				loadIndex = (int)i;

			MarkContextTarget(e);

			if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
			{
				ImGui::SetDragDropPayload(GT2_INSTRUMENT_FILE_PAYLOAD,
										  e.fullPath.c_str(), e.fullPath.size() + 1);
				ImGui::TextUnformatted(e.name.c_str());
				ImGui::TextDisabled("drop on an instrument slot");
				ImGui::EndDragDropSource();
			}
		}
		ImGui::PopID();
	}

	if (list->empty() && activeSearch.empty())
		ImGui::TextDisabled("No instruments in this folder.");
	if (list->empty() && !activeSearch.empty())
		ImGui::TextDisabled("Nothing matched \"%s\".", activeSearch.c_str());
	if (searchTruncated)
		ImGui::TextDisabled("(search stopped early -- too many files)");

	// A right click that hit no row is about the folder being listed.
	if (!contextOpenPending
		&& ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows)
		&& !ImGui::IsAnyItemHovered()
		&& ImGui::IsMouseReleased(ImGuiMouseButton_Right))
	{
		contextPath = currentPath;
		contextIsDir = true;
		contextOpenPending = true;
	}

	ImGui::EndChild();

	// Opened and drawn outside the child: the menu navigates, and navigating
	// rebuilds the listing the child is drawing.
	if (contextOpenPending)
	{
		contextOpenPending = false;
		ImGui::OpenPopup("gt2InsBrowserCtx");
	}
	// ImGui names a popup window "##Popup_<id>", so its own name carries no
	// trace of "gt2InsBrowserCtx" -- a test cannot find it by name. Report it
	// here instead, where the body is actually drawn.
	contextMenuOpen = false;
	if (ImGui::BeginPopup("gt2InsBrowserCtx"))
	{
		contextMenuOpen = true;
		RenderContextMenuBody(contextPath, contextIsDir);
		ImGui::EndPopup();
	}

	// Act after the child closed, so navigation cannot invalidate the list
	// the loop above is still walking.
	if (enterFolder == -2)
		GoToParent();
	else if (enterFolder >= 0)
		EnterEntry(enterFolder);
	else if (loadIndex >= 0)
		LoadEntry(loadIndex);
}

void CViewGT2InstrumentsBrowser::RenderSearchBar(float uiScale)
{
	ImGui::SetNextItemWidth(-1.0f);
	if (ImGui::InputTextWithHint("##gt2InsBrowserSearch",
								 ICON_FA_SEARCH "  search this folder and below...",
								 searchBuffer, sizeof(searchBuffer)))
	{
		activeSearch = searchBuffer;
		RebuildSearch();
	}
}

void CViewGT2InstrumentsBrowser::RenderImGui()
{
	PreRenderImGui();

	float uiScale = GT2EffectiveUIScale();
	ImGui::SetWindowFontScale(uiScale);
	ImGuiStyle &style = ImGui::GetStyle();
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
						ImVec2(style.FramePadding.x * uiScale, style.FramePadding.y * uiScale));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
						ImVec2(style.ItemSpacing.x * uiScale, style.ItemSpacing.y * uiScale));

	RenderBreadcrumb(uiScale);
	RenderToolbar(uiScale);
	RenderList(uiScale);
	RenderSearchBar(uiScale);

	ImGui::PopStyleVar(2);

	GT2_PropagateChildWindowFocus(this);
	PostRenderImGui();
}

bool CViewGT2InstrumentsBrowser::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	// Undo/redo drive the editor's one shared history (CGT2UndoHistory.h),
	// which is where an instrument load is recorded (gsong.c loadinstrument()).
	// Handled here so the shortcut works while the browser holds focus.
	if (!isShift && !isAlt && (isControl || isSuper))
	{
		CViewGT2Tables *tables = pluginGoatTracker ? pluginGoatTracker->viewTables : NULL;
		if (tables != NULL)
		{
			if (keyCode == 'z' || keyCode == 'Z' || keyCode == SDLK_Z)
			{
				tables->UndoTableEdit();
				return true;
			}
			if (keyCode == 'y' || keyCode == 'Y' || keyCode == SDLK_Y)
			{
				tables->RedoTableEdit();
				return true;
			}
		}
	}
	if (keyCode == MTKEY_BACKSPACE)
	{
		GoToParent();
		return true;
	}
	return GT2_HandleRenoiseOrForwardKeyDownToNative(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewGT2InstrumentsBrowser::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	GT2_ForwardKeyUp(keyCode);
	return true;
}

bool CViewGT2InstrumentsBrowser::KeyDownRepeat(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewGT2InstrumentsBrowser::DoDropFile(char *filePath)
{
	if (filePath == NULL) return false;

	if (SYS_FileDirExists(filePath))
	{
		SetFolder(filePath);
		return true;
	}
	if (!IsInstrumentFile(filePath))
		return false;

	if (pluginGoatTracker != NULL)
		pluginGoatTracker->LoadInstrumentFromFile(filePath, einum, previewOnClick);

	// Follow the dropped file, so the rest of its folder is one click away.
	CSlrString *slrPath = new CSlrString(filePath);
	CSlrString *folder = slrPath->GetFilePathWithoutFileNameComponentFromPath();
	char *cFolder = folder->GetStdASCII();
	SetFolder(cFolder);
	delete[] cFolder;
	delete folder;
	delete slrPath;
	return true;
}
