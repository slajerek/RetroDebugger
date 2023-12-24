#ifndef _CViewFileBrowser_h_
#define _CViewFileBrowser_h_

#include "CGuiView.h"
#include "CRecentlyOpenedFiles.h"
#include <iostream>
#include <filesystem>
#include <set>
#include <vector>
#include <list>
#include <map>

namespace fs = std::filesystem;

class CViewFileBrowserCallback
{
public:
	virtual void ViewFileBrowserCallbackOpenFile(fs::path path) {};
};

struct SetPathSorter
{
	bool operator () (const fs::directory_entry& lhs, const fs::directory_entry& rhs) const
	{
		if (lhs.path().filename().string() == "..")
		{
			return true;
		}
		if (rhs.path().filename().string() == "..")
		{
			return false;
		}
		if (lhs.is_directory() && !rhs.is_directory())
		{
			return true;
		}
		if (!lhs.is_directory() && rhs.is_directory())
		{
			return false;
		}
		return Utf8StringToLowercase(lhs.path().filename().string()) < Utf8StringToLowercase(rhs.path().filename().string());
	}
};

class CViewFileBrowser : public CGuiView, public CRecentlyOpenedFilesCallback
{
public:
	CViewFileBrowser(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY,
					 set<string> allowedExtensions, CViewFileBrowserCallback *callback);
	virtual ~CViewFileBrowser();

	virtual void RenderImGui();

	virtual bool HasContextMenuItems();
	virtual void RenderContextMenuItems();

	virtual void ActivateView();
	virtual void DeactivateView();
	
	virtual void Init(CViewFileBrowserCallback *callback);
	virtual void SetPath(const char *newPath);
	virtual void SetPath(fs::path newPath);
	
	fs::path currentPath;
	set<fs::directory_entry, SetPathSorter> pathEntries;
	vector<char *> pathEntriesFilenames;
	set<fs::directory_entry, SetPathSorter>::iterator selectedEntry;
	
	bool showAllFileExtensions;
	set<string> allowedExtensions;
	virtual void SetAllowedExtensions(set<string> allowedExtensions);
	
	list<int> previousItemIndexes;
	list<float> previousScrollYs;
	
	ImGuiKey previousKey;
	
	bool updateScrollY;
	float targetScrollY;
	
	CRecentlyOpenedFiles *favourities;
	bool pathExistsInFavourities;
	virtual void RecentlyOpenedFilesCallbackSelectedMenuItem(CSlrString *filePath);

	//
	CViewFileBrowserCallback *callback;
};

#endif //_CViewFileBrowser_h_
