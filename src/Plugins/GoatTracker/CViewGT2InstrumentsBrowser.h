#ifndef _CViewGT2InstrumentsBrowser_H_
#define _CViewGT2InstrumentsBrowser_H_

#include "SYS_Defs.h"
#include "CGuiView.h"
#include "CRecentlyOpenedFiles.h"
#include <string>
#include <vector>

class CGT2FontAtlas;

// ImGui drag&drop payload carrying one absolute *.ins path, NUL-terminated.
// Dropped onto CViewGT2InstrumentList (loads into the row under the cursor)
// or CViewGT2Instrument (loads into the currently edited instrument).
#define GT2_INSTRUMENT_FILE_PAYLOAD "GT2_INS_FILE"

// A file browser dedicated to GoatTracker instruments. The current folder is
// shown as a clickable breadcrumb; below it the folders and *.ins files of
// that folder; at the bottom a search box that walks the tree recursively.
// Clicking an instrument loads it into the currently selected instrument.
// Loading is undoable — gsong.c loadinstrument() records a step on the
// editor's one shared undo history (CGT2UndoHistory.h) — so Undo/Redo here,
// the tables view, the toolbar and Ctrl+Z anywhere all walk the same timeline.
class CViewGT2InstrumentsBrowser : public CGuiView, public CRecentlyOpenedFilesCallback
{
public:
	CViewGT2InstrumentsBrowser(const char *name, float posX, float posY, float posZ,
							   float sizeX, float sizeY, CGT2FontAtlas *fontAtlas);
	virtual ~CViewGT2InstrumentsBrowser();

	virtual void RenderImGui();
	virtual bool KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyDownRepeat(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	// A file dragged in from the OS: navigate to its folder and, when it is an
	// instrument, load it into the currently selected instrument.
	virtual bool DoDropFile(char *filePath);

	CGT2FontAtlas *fontAtlas;

	// Navigation. SetFolder() ignores paths that do not resolve to a folder,
	// so a stale remembered path cannot leave the view stranded.
	void SetFolder(const char *path);
	const char *GetFolder() const { return currentPath.c_str(); }
	void GoToParent();
	void Refresh();

	// Preview the loaded instrument with the note last auditioned in the
	// editor (gt2LastPreviewNote). Persisted in gt2Config.
	bool previewOnClick;

	// The label a search hit gets: `fullPath` relative to `base`, tolerating a
	// base that does or does not end with a separator. Static so a test can
	// exercise both shapes without SetFolder() normalising one of them away.
	static std::string RelativeDisplayName(const std::string &fullPath,
										   const std::string &base);

	// True when `name` ends in a case-insensitive ".ins".
	static bool IsInstrumentFile(const char *name);

	// Favourite folders. The same mechanism CViewFileBrowser uses -- the
	// engine's recently-opened-files list under a settings file of its own --
	// so a folder added here cannot collide with File/Recents or with the GT2
	// song recents. Selecting one navigates there.
	CRecentlyOpenedFiles *favouriteFolders;
	virtual void RecentlyOpenedFilesCallbackSelectedMenuItem(CSlrString *filePath);
	bool IsFolderInFavourites(const char *path);
	void AddFolderToFavourites(const char *path);
	void RemoveFolderFromFavourites(const char *path);

	// The view's own context menu (title bar / empty area) carries the same
	// items as the right-click menu in the listing.
	virtual bool HasContextMenuItems();
	virtual void RenderContextMenuItems();

	// True on the frames the listing's right-click menu is drawn. A test
	// cannot look for it by window name: ImGui calls popup windows
	// "##Popup_<id>" and keeps the label out of the name.
	bool contextMenuOpen;

	// Testing hooks — same code paths the mouse drives.
	int GetEntryCount();
	// The label the listing draws for entry `index`, or NULL if out of range.
	// In search results this is the path relative to the folder searched, which
	// is the part that got mangled when the folder ended with a separator.
	const char *GetEntryDisplayName(int index);
	bool IsSearchActive() const { return !activeSearch.empty(); }
	void SetSearch(const char *text);
	// Loads entry `index` of the listing currently on screen. Returns false
	// for a folder or an out-of-range index.
	bool LoadEntry(int index);
	// Enters entry `index` when it is a folder. Returns false otherwise.
	bool EnterEntry(int index);

private:
	struct Entry
	{
		std::string name;      // display label
		std::string fullPath;
		bool isDir;
	};

	std::string currentPath;
	std::vector<Entry> entries;

	char searchBuffer[128];
	std::string activeSearch;
	std::vector<Entry> searchResults;
	bool searchTruncated;

	// Set while the listing must be rebuilt before the next draw. Navigation
	// happens mid-frame, so the rebuild is deferred rather than done inline.
	bool listingDirty;
	// After navigating, scroll the breadcrumb to its right end -- the folder
	// you are in is the useful half of a long path, not the root.
	bool breadcrumbScrollPending;

	// What the listing's right-click menu is about: the row that was clicked,
	// or the current folder when the click landed on empty space. Captured at
	// click time because the listing can be rebuilt before the menu is used.
	std::string contextPath;
	bool contextIsDir;
	bool contextOpenPending;

	std::vector<Entry> *ActiveList();
	void RebuildListing();
	void RebuildSearch();
	void RenderBreadcrumb(float uiScale);
	void RenderToolbar(float uiScale);
	void RenderList(float uiScale);
	void RenderSearchBar(float uiScale);
	// Body of both context menus. `path` is the item the menu is about;
	// favourites always act on a folder, so a file's parent is used.
	void RenderContextMenuBody(const std::string &path, bool isDir);
	// Call right after a row's widget: remembers it if it was right-clicked.
	void MarkContextTarget(const Entry &entry);
};

#endif
