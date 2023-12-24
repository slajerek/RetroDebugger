#include "CViewFileBrowser.h"
#include "CGuiMain.h"
#include "CViewC64.h"
#include "CConfigStorageHjson.h"
#include <iostream>
#include <string>
#include <cctype>
#include "CSlrString.h"

using namespace ImGui;

// the code below is to allow me to test on macOS/linux
//#define WIN32
//std::vector<std::string> SYS_Win32GetAvailableDrivesPaths()
//{
//	std::vector<std::string> drives;
//	drives.push_back("C:\\");
//	drives.push_back("D:\\");
//	drives.push_back("/Users/mars/Downloads");
//	return drives;
//}


CViewFileBrowser::CViewFileBrowser(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY,
								   set<string> allowedExtensions, CViewFileBrowserCallback *callback)
: CGuiView(name, posX, posY, posZ, sizeX, sizeY)
{
	char *buf = SYS_GetCharBuf();
	sprintf(buf, "favourities-%08x.dat", GetHashCode32(name));
	
	// TODO: refactor CRecentlyOpenedFiles to use std::string and fs::path
	favourities = new CRecentlyOpenedFiles(new CSlrString(buf), this);

	currentPath = ".";
	
	SYS_ReleaseCharBuf(buf);

	pathExistsInFavourities = false;
	
	showAllFileExtensions = false;
	
	SetAllowedExtensions(allowedExtensions);
	Init(callback);
}

void CViewFileBrowser::Init(CViewFileBrowserCallback *callback)
{
	this->callback = callback;
	
	const char *defaultPath;
	
	char *buf = SYS_GetCharBuf();
	sprintf(buf, "CViewFileBrowser##%s", this->name);
	viewC64->config->GetString(buf, &defaultPath, gPathToDocuments);
	SYS_ReleaseCharBuf(buf);
	
	LOGD("CViewFileBrowser::Init: defaultPath=%s", defaultPath);
	SetPath(defaultPath);

	targetScrollY = 0;
	updateScrollY = false;
	
	previousKey = ImGuiKey_None;
}

void CViewFileBrowser::RenderImGui()
{
	PreRenderImGui();
	
	ImU32 mouseHoveredRowBgColor = ImGui::GetColorU32(ImVec4(0.7f, 0.3f, 0.3f, 0.65f));

	if (BeginTable("##CViewFileBrowser", 1, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Resizable))
	{
		TableSetupColumn("Name");
//		TableHeadersRow();

		ImGuiListClipper clipper;
		clipper.Begin((int)pathEntries.size());
		
		int firstRow = -1;
		int lastRow = 0;
		int currentRow = 0;
		
		bool itemSelected = false;

		while(clipper.Step())
		{
			currentRow = clipper.DisplayStart;
			
			auto it = pathEntries.begin();
			std::advance(it, currentRow);
			
			auto itFilename = pathEntriesFilenames.begin();
			std::advance(itFilename, currentRow);
						
			for ( ; currentRow < clipper.DisplayEnd; currentRow++, it++, itFilename++)
			{
				TableNextRow();
				TableNextColumn();

				fs::directory_entry entry = *it;
				
				bool isItemSelected = (selectedEntry == it);
				
				// we have a separate list of const char* filenames because this does not work correctly here: entry.path().filename().c_str();
				const char *entryName = *itFilename;
				
				ImGuiSelectableFlags selectableFlags = ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap;
				if (ImGui::Selectable(entryName, isItemSelected, selectableFlags, ImVec2(0, 0)))
				{
					selectedEntry = it;
					
//					if (oneClickOpensFile)
//					{
//						itemSelected = true;
//					}
				}
				
				if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
				{
					itemSelected = true;
				}

				if (IsItemVisible())
				{
					if (firstRow == -1)
						firstRow = currentRow;
					lastRow = currentRow;
				}
			}
		}
		
		if (updateScrollY)
		{
			SetScrollY(targetScrollY);
			updateScrollY = false;
		}
		
		int selectedItemIndex = (int)std::distance(pathEntries.begin(), selectedEntry);

		if (previousKey == ImGuiKey_DownArrow
			|| previousKey == ImGuiKey_UpArrow)
		{
			int numVisibleRows = lastRow - firstRow;
			LOGD("firstRow=%d lastRow=%d numVisibleRows=%d selectedItemIndex=%d", firstRow, lastRow, numVisibleRows, selectedItemIndex);

			if (lastRow <= selectedItemIndex)
			{
				// calc number of items to scroll
				int numRowsToScroll = selectedItemIndex - lastRow+1;
//				LOGD("numRowsToScroll=%d", numRowsToScroll);
				SetScrollY(GetScrollY() + clipper.ItemsHeight * numRowsToScroll);
			}
			else if (firstRow >= selectedItemIndex)
			{
				SetScrollY(clipper.ItemsHeight * selectedItemIndex);
			}

			previousKey = ImGuiKey_None;

		}
//
		// TODO: join two ifs below
		if (IsWindowFocused())
		{
			if (IsKeyPressed(GetKeyIndex(ImGuiKey_UpArrow))
				|| IsKeyPressed(GetKeyIndex(ImGuiKey_DownArrow)))
			{
				// navigation
				if (IsKeyPressed(GetKeyIndex(ImGuiKey_UpArrow)))
				{
					if (selectedEntry != pathEntries.begin())
					{
						selectedEntry = std::prev(selectedEntry);
						previousKey = ImGuiKey_UpArrow;
					}
				}

				if (IsKeyPressed(GetKeyIndex(ImGuiKey_DownArrow)))
				{
					auto newEntry = std::next(selectedEntry);
					if (newEntry != pathEntries.end())
					{
						selectedEntry = newEntry;
					}
					previousKey = ImGuiKey_DownArrow;
				}
				
				int selectedItemIndex = (int)std::distance(pathEntries.begin(), selectedEntry);
				int numVisibleRows = lastRow - firstRow;
				LOGD("firstRow=%d lastRow=%d numVisibleRows=%d selectedItemIndex=%d", firstRow, lastRow, numVisibleRows, selectedItemIndex);

				if (lastRow <= selectedItemIndex)
				{
					// calc number of items to scroll
					int numRowsToScroll = selectedItemIndex - lastRow+1;
	//				LOGD("numRowsToScroll=%d", numRowsToScroll);
					SetScrollY(GetScrollY() + clipper.ItemsHeight * numRowsToScroll);
				}
				else if (firstRow >= selectedItemIndex)
				{
					SetScrollY(clipper.ItemsHeight * selectedItemIndex);
				}
			}
			
			if (IsKeyPressed(ImGuiKey_Enter)
				|| IsKeyPressed(ImGuiKey_LeftArrow)
				|| IsKeyPressed(ImGuiKey_RightArrow)
				|| itemSelected)
			{
				fs::directory_entry entry = *selectedEntry;
				
				if (entry.is_regular_file())
				{
					if (callback)
					{
						callback->ViewFileBrowserCallbackOpenFile(entry.path());
					}
				}
				else
				{
					float previousScrollY = 0;
					int previousItemIndex = 0;

					if (entry.path().filename() == ".."
						|| IsKeyPressed(ImGuiKey_LeftArrow))
					{
						if (!previousScrollYs.empty())
						{
							previousScrollY = previousScrollYs.back();
							previousScrollYs.pop_back();
							
							previousItemIndex = previousItemIndexes.back();
							previousItemIndexes.pop_back();
						}
						targetScrollY = previousScrollY;
						updateScrollY = true;
						
						//
						SetPath(*(pathEntries.begin()));
						
						if (previousItemIndex < pathEntries.size())
						{
							selectedEntry = pathEntries.begin();
							std::advance(selectedEntry, previousItemIndex);
						}
					}
					else if (entry.is_directory())
					{
						previousScrollYs.push_back(GetScrollY());
						previousItemIndexes.push_back(selectedItemIndex);
						SetScrollY(0);
	//					SetScrollX(0);
						
						SetPath(*selectedEntry);
					}
					
				}
				

			}
		}	// IsWindowFocused

		EndTable();
	}
	
	

	PostRenderImGui();
}

void CViewFileBrowser::SetAllowedExtensions(set<string> allowedExtensions)
{
	this->allowedExtensions = allowedExtensions;
}

void CViewFileBrowser::SetPath(const char *newPath)
{
	LOGD("CViewFileBrowser::SetPath: newPath=%s", newPath);
	fs::path path = newPath;
	SetPath(path);
}

void CViewFileBrowser::SetPath(fs::path newPath)
{
	LOGD("CViewFileBrowser::SetPath: currentPath=%s newPath=%s", currentPath.string().c_str(), newPath.string().c_str());

	fs::path oldPath = currentPath;

	if (newPath.filename() == "..")
	{
		fs::path temp = fs::canonical(currentPath);
		currentPath = temp.parent_path();
	}
	else
	{
		currentPath = newPath;
	}

	// Microsoft's std::filename is implemented by literally fucking crazy people
	try
	{
		if (!fs::exists(currentPath))
		{
			LOGError("CViewFileBrowser::SetPath: path does not exist %s", currentPath.string().c_str());
			viewC64->ShowMessageError("Can't access path %s", newPath.string().c_str());
			currentPath = oldPath;
			
			if (!fs::exists(currentPath))
			{
				currentPath = ".";
			}
		}
	}
	catch (...)
	{
		LOGError("CViewFileBrowser::SetPath: exception");
		viewC64->ShowMessageError("Can't access path %s", newPath.string().c_str());
		currentPath = oldPath;

		if (!fs::exists(currentPath))
		{
			currentPath = ".";
		}
	}

	string strPath = currentPath.string();
	const char *cPath = strPath.c_str();
	LOGD("CViewFileBrowser::SetPath: currentPath is %s", cPath);
	
	char *buf = SYS_GetCharBuf();
	sprintf(buf, "CViewFileBrowser##%s", this->name);
	viewC64->config->SetString(buf, &cPath);
	SYS_ReleaseCharBuf(buf);

	// TODO: refactor CRecentlyOpenedFiles to use fs::path
	CSlrString *slrPath = new CSlrString(cPath);
	pathExistsInFavourities = favourities->Exists(slrPath);
	delete slrPath;
	
	
	//
	pathEntries.clear();
	
	while(!pathEntriesFilenames.empty())
	{
		char *buf = pathEntriesFilenames.back();
		pathEntriesFilenames.pop_back();
		STRFREE(buf);
	}
	
	fs::path path("..");
	fs::directory_entry entry(path);
	pathEntries.insert(entry);
	
	const std::filesystem::directory_options options = (
		std::filesystem::directory_options::follow_directory_symlink |
		std::filesystem::directory_options::skip_permission_denied
		);

	try
	{
		for (auto& entry : fs::directory_iterator(currentPath, std::filesystem::directory_options(options)))
		{
			try
			{
				if (!entry.is_regular_file() && !entry.is_directory())
					continue;

				if (showAllFileExtensions)
				{
					pathEntries.insert(entry);
					continue;
				}

				string fileName = entry.path().filename().string();

				if (entry.is_regular_file())
				{
					//			LOGD("allowedExtensions=%d", allowedExtensions.size());

					string fullFileExtension = entry.path().extension().string();
					if (!fullFileExtension.empty())
					{
						string fileExtension = fullFileExtension.substr(1, fullFileExtension.length() - 1);
						//				LOGD("fileExtension=%s", fileExtension.c_str());

						auto it = allowedExtensions.find(fileExtension);
						if (it != allowedExtensions.end())
						{
							if (fileName[0] != '.')
							{
								pathEntries.insert(entry);
							}
						}
					}
				}
				else if (entry.is_directory())
				{
					if ((fileName == "..")
						|| fileName[0] != '.')
					{
						pathEntries.insert(entry);
					}
				}
			}
			catch (...)
			{
				LOGError("CViewFileBrowser::SetPath: exception");
			}
		}
	}
	catch (...)
	{
		LOGError("CViewFileBrowser::SetPath: exception");
	}

	
	for (auto &entry : pathEntries)
	{
//		cout << entry.path().filename().c_str() << endl;
		
		char *buf = STRALLOC(entry.path().filename().string().c_str());
		pathEntriesFilenames.push_back(buf);
	}
	
	selectedEntry = pathEntries.begin();
	
//	LOGD("----");
}

bool CViewFileBrowser::HasContextMenuItems()
{
	return true;
}

void CViewFileBrowser::RenderContextMenuItems()
{
#if defined(WIN32)
	std::vector<std::string> drivesPaths = SYS_Win32GetAvailableDrivesPaths();
	ImGui::SeparatorText("Drives:");
	
	for (auto drivePath : drivesPaths)
	{
		if (ImGui::MenuItem(drivePath.c_str()))
		{
			SetPath(drivePath.c_str());
		}
	}
	ImGui::Separator();
#endif
	if (!favourities->listOfFiles.empty())
	{
		ImGui::SeparatorText("Favourities:");
		favourities->RenderImGuiMenu(NULL);
		ImGui::Separator();
	}
	
	if (!pathExistsInFavourities)
	{
		if (ImGui::MenuItem("Add to favourities"))
		{
			fs::path fPath = fs::canonical(currentPath);
			CSlrString *sPath = new CSlrString(fPath.string().c_str());
			sPath->DebugPrint("sPath=");
			favourities->Add(sPath);
			delete sPath;
		}
	}
	else
	{
		if (ImGui::MenuItem("Remove from favourities"))
		{
			fs::path fPath = fs::canonical(currentPath);
			CSlrString *sPath = new CSlrString(fPath.string().c_str());
			sPath->DebugPrint("sPath=");
			favourities->Remove(sPath);
			delete sPath;
			pathExistsInFavourities = false;
		}
	}
}

void CViewFileBrowser::RecentlyOpenedFilesCallbackSelectedMenuItem(CSlrString *filePath)
{
	char *buf = filePath->GetStdASCII();
	SetPath(buf);
	STRFREE(buf);
}

void CViewFileBrowser::ActivateView()
{
	LOGG("CViewFileBrowser::ActivateView()");
}

void CViewFileBrowser::DeactivateView()
{
	LOGG("CViewFileBrowser::DeactivateView()");
}

CViewFileBrowser::~CViewFileBrowser()
{
	while(!pathEntriesFilenames.empty())
	{
		char *buf = pathEntriesFilenames.back();
		pathEntriesFilenames.pop_back();
		STRFREE(buf);
	}
}

