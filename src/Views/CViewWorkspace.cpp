#include "CViewWorkspace.h"
#include "CSourceBuffer.h"
#include "CSourceDocument.h"
#include "CSourceRendering.h"
#include "CSyntaxLexer.h"
#include "CWorkspaceSession.h"
#include "CDebugInterface.h"
#include "CDebugSymbols.h"
#include "CDebugSymbolsSegment.h"
#include "CDebugDatabase.h"
#include "CDebugSourceBreakpoint.h"
#include "CGuiMain.h"
#include "CSlrString.h"
#include "SYS_FileSystem.h"
#include "SYS_Funct.h"
#include "SYS_Main.h"
#include "imgui.h"
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <functional>

CViewWorkspace::CViewWorkspace(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY)
: CGuiView(name, posX, posY, posZ, sizeX, sizeY)
{
	imGuiNoWindowPadding = false;
	imGuiNoScrollbar = true;
	filter[0] = 0;
	findText[0] = 0;
	findCaseSensitive = false;
	findOffset = 0;
	showFind = false;
	selectedTask = 0;
	handledBuildGeneration = 0;
	lastWatchMs = 0;
	consentTask = -1;
	pendingCloseTab = -1;
	editTab = -1;
	scrollConsole = false;
}

CViewWorkspace::~CViewWorkspace()
{
}

bool CViewWorkspace::OpenFolder(const std::string &path)
{
	std::string error;
	CWorkspace opened;
	if (!opened.Open(path, &error))
	{
		lastBuildResult = error;
		return false;
	}
	CloseFolder();
	workspace = opened;
	selectedTask = 0;
	return true;
}

void CViewWorkspace::CloseFolder()
{
	build.Cancel();
	build.Wait();
	workspace.Close();
	listings.clear();
	filter[0] = 0;
}

CSourceBuffer *CViewWorkspace::OpenFile(const std::string &path, int line)
{
	namespace fs = std::filesystem;
	std::error_code ec;
	std::string canonical = SourcePathString(fs::weakly_canonical(SourcePath(path), ec));
	if (ec)
		canonical = path;
	for (int i = 0; i < (int)tabs.size(); i++)
	{
		if (tabs[i].buffer->path == canonical)
		{
			SelectTab(i);
			if (line > 0)
			{
				tabs[i].scrollToLine = line;
				tabs[i].selectedLine = line;
			}
			return tabs[i].buffer.get();
		}
	}
	Tab tab;
	tab.buffer.reset(new CSourceBuffer(canonical));
	if (!tab.buffer->Load())
		return NULL;
	tab.scrollToLine = line;
	tab.selectedLine = line;
	tabs.push_back(std::move(tab));
	SelectTab((int)tabs.size() - 1);
	return tabs.back().buffer.get();
}

CSourceBuffer *CViewWorkspace::GetActiveBuffer()
{
	return (activeTab >= 0 && activeTab < (int)tabs.size()) ? tabs[activeTab].buffer.get() : NULL;
}

void CViewWorkspace::SelectTab(int index)
{
	if (index < 0 || index >= (int)tabs.size())
		return;
	activeTab = index;
	editTab = -1;		// the editor widget takes the new tab's text
}

bool CViewWorkspace::CloseTab(int index, bool discard)
{
	if (index < 0 || index >= (int)tabs.size())
		return false;
	if (tabs[index].buffer->IsDirty() && !discard)
		return false;
	tabs.erase(tabs.begin() + index);
	if (activeTab >= (int)tabs.size())
		activeTab = (int)tabs.size() - 1;
	editTab = -1;
	return true;
}

bool CViewWorkspace::SaveTab(int index, std::string *error)
{
	if (index < 0 || index >= (int)tabs.size())
		return false;
	bool saved = tabs[index].buffer->Save(error);
	tabs[index].notice = saved ? "" : (error ? *error : std::string("save failed"));
	return saved;
}

void CViewWorkspace::PollFiles()
{
	for (int i = 0; i < (int)tabs.size(); i++)
	{
		Tab &tab = tabs[i];
		switch (tab.buffer->CheckDisk())
		{
			case CSourceBuffer::DiskState::Reloaded:
				tab.notice = "Reloaded: the file changed on disk";
				if (i == editTab)
					editTab = -1;
				break;
			case CSourceBuffer::DiskState::Conflict:
				tab.notice = "The file changed on disk; saving overwrites it, Revert takes it";
				break;
			case CSourceBuffer::DiskState::Missing:
				tab.notice = "The file is gone from disk; Save writes it again";
				break;
			case CSourceBuffer::DiskState::Unchanged:
				break;
		}
	}
	listings.clear();		// the explorer shows new and deleted files
}

CDebugInterface *CViewWorkspace::GetMachine()
{
	return CWorkspaceSession::Machine(workspace.machine);
}

bool CViewWorkspace::StartBuild(int task, bool consent)
{
	if (task < 0 || task >= (int)workspace.buildTasks.size() || build.IsRunning())
		return false;
	if (!CWorkspaceTrust::IsTrusted(workspace))
	{
		if (!consent)
		{
			consentTask = task;
			return false;
		}
		CWorkspaceTrust::SetTrusted(workspace, true);
	}
	// unsaved edits would not be what the build reads
	for (int i = 0; i < (int)tabs.size(); i++)
		if (tabs[i].buffer->IsDirty() && workspace.Contains(tabs[i].buffer->path))
			SaveTab(i);
	lastBuildResult = "Building " + workspace.buildTasks[task].name + "...";
	scrollConsole = true;
	build.Start(workspace.buildTasks[task], workspace.Variables());
	return true;
}

void CViewWorkspace::PollBuild()
{
	CBuildTaskRun::State state = build.GetState();
	if (state == CBuildTaskRun::State::Idle || state == CBuildTaskRun::State::Running
		|| build.generation == handledBuildGeneration)
		return;
	handledBuildGeneration = build.generation;
	if (state == CBuildTaskRun::State::Succeeded)
	{
		std::string error;
		bool load = build.expanded.afterBuild != CBuildTaskConfig::AfterBuild::None;
		if (!load || CWorkspaceSession::LoadBuild(build, &error))
			lastBuildResult = "Build succeeded" + std::string(load ? ", loaded" : "");
		else
			lastBuildResult = "Build succeeded, load failed: " + error;
	}
	else
	{
		CWorkspaceSession::MarkStale(build);
		lastBuildResult = (state == CBuildTaskRun::State::Cancelled ? "Build cancelled" : "Build failed: " + build.GetFailure())
			+ " -- the loaded session is from an earlier build";
	}
}

bool CViewWorkspace::OpenDiagnostic(const CBuildDiagnostic &diagnostic)
{
	if (diagnostic.path.empty())
		return false;
	return OpenFile(diagnostic.path, diagnostic.line) != NULL;
}

void CViewWorkspace::SystemDialogPickFolderSelected(CSlrString *path)
{
	char *ascii = path->GetStdASCII();
	OpenFolder(ascii);
	delete [] ascii;
}

void CViewWorkspace::SystemDialogPickFolderCancelled()
{
}

// --- rendering ---

const std::vector<CWorkspaceEntry> &CViewWorkspace::Listing(const std::string &directory)
{
	auto it = listings.find(directory);
	if (it == listings.end())
		it = listings.emplace(directory, workspace.List(directory)).first;
	return it->second;
}

void CViewWorkspace::RenderDirectory(const std::string &directory)
{
	for (const CWorkspaceEntry &entry : Listing(directory))
	{
		ImGui::PushID(entry.path.c_str());
		if (entry.isDirectory)
		{
			if (ImGui::TreeNodeEx(entry.name.c_str(), ImGuiTreeNodeFlags_SpanAvailWidth))
			{
				RenderDirectory(entry.path);
				ImGui::TreePop();
			}
		}
		else
		{
			bool open = false;
			for (Tab &tab : tabs)
				open = open || tab.buffer->path == entry.path;
			ImGui::TreeNodeEx(entry.name.c_str(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen
							  | ImGuiTreeNodeFlags_SpanAvailWidth | (open ? ImGuiTreeNodeFlags_Selected : 0));
			if (ImGui::IsItemClicked())
				OpenFile(entry.path);
		}
		ImGui::PopID();
	}
}

void CViewWorkspace::RenderExplorer()
{
	ImGui::SetNextItemWidth(-1.0f);
	ImGui::InputTextWithHint("##projectFilter", "Find file", filter, sizeof(filter));
	ImGui::BeginChild("##projectFiles", ImVec2(0, 0), false);
	if (filter[0])
	{
		for (const CWorkspaceEntry &entry : workspace.FindFiles(filter, 200))
		{
			std::string label = workspace.RelativePath(entry.path) + "##" + entry.path;
			if (ImGui::Selectable(label.c_str()))
				OpenFile(entry.path);
		}
	}
	else
		RenderDirectory(workspace.root);
	ImGui::EndChild();
}

void CViewWorkspace::RenderToolbar()
{
	if (ImGui::Button("Open Folder..."))
		SYS_DialogPickFolder(this, NULL);
	if (!workspace.IsOpen())
		return;
	ImGui::SameLine();
	if (ImGui::Button("Refresh"))
	{
		listings.clear();
		workspace.LoadConfig();
	}
	ImGui::SameLine();
	ImGui::TextDisabled("%s", workspace.root.c_str());
	if (!workspace.configError.empty())
		ImGui::TextColored(kColorWarning, "%s", workspace.configError.c_str());

	if (!workspace.buildTasks.empty())
	{
		if (selectedTask >= (int)workspace.buildTasks.size())
			selectedTask = 0;
		ImGui::SetNextItemWidth(ImGui::GetFontSize() * 10.0f);
		if (ImGui::BeginCombo("##buildTask", workspace.buildTasks[selectedTask].name.c_str()))
		{
			for (int i = 0; i < (int)workspace.buildTasks.size(); i++)
			{
				std::string label = workspace.buildTasks[i].name + "##" + std::to_string(i);
				if (ImGui::Selectable(label.c_str(), i == selectedTask))
					selectedTask = i;
			}
			ImGui::EndCombo();
		}
		ImGui::SameLine();
		if (build.IsRunning())
		{
			if (ImGui::Button("Cancel"))
				build.Cancel();
		}
		else if (ImGui::Button("Build"))
			StartBuild(selectedTask, false);
		ImGui::SameLine();
	}
	else
	{
		ImGui::TextDisabled("No build tasks (%s)", CWorkspace::kConfigFileName);
		ImGui::SameLine();
	}
	ImGui::TextUnformatted(lastBuildResult.c_str());
}

int CViewWorkspace::PcLineFor(const CSourceBuffer *buffer)
{
	CDebugInterface *di = GetMachine();
	if (di == NULL || di->symbols == NULL || !di->symbols->currentSegment)
		return 0;
	std::shared_ptr<const CDebugDatabase> database = di->symbols->debugDatabase;
	int regionId = di->symbols->currentSegment->debugRegionId;
	if (!database || regionId < 0)
		return 0;
	const CDebugSourceMapping *mapping = di->ResolveSourceAt(database.get(), (u32)regionId, (u16)di->GetCpuPC());
	const CDebugSourceFile *file = mapping ? database->GetFile(mapping->fileId) : NULL;
	if (file == NULL)
		return 0;
	std::string name = SourceDocumentFileName(buffer->path);
	bool same = file->loadedPath == buffer->path || file->originalPath == buffer->path || file->fileName == name;
	return same ? mapping->lineStart : 0;
}

void CViewWorkspace::FindNext(Tab &tab)
{
	long found = tab.buffer->Find(findText, findOffset, findCaseSensitive);
	if (found < 0)
	{
		tab.notice = std::string("Not found: ") + findText;
		return;
	}
	tab.notice.clear();
	findOffset = found + 1;
	int line = tab.buffer->LineOfOffset(found);
	tab.selectedLine = line;
	tab.scrollToLine = line;
}

void CViewWorkspace::RenderView(Tab &tab)
{
	CSourceBuffer *buffer = tab.buffer.get();
	const CSyntaxLexer *lexer = SyntaxLexerForPath(buffer->path, "");
	CDebugInterface *di = GetMachine();
	CDebugSymbols *symbols = di ? di->symbols : NULL;
	int pcLine = PcLineFor(buffer);
	float lineHeight = ImGui::GetTextLineHeightWithSpacing();
	float gutterWidth = ImGui::GetFontSize() * 1.6f;

	ImGui::BeginChild("##projectText", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
	if (tab.scrollToLine > 0)
	{
		ImGui::SetScrollY(std::max(0.0f, (tab.scrollToLine - 1) * lineHeight - ImGui::GetWindowHeight() * 0.4f));
		tab.scrollToLine = 0;
	}
	// the lines once per frame: the buffer is one string
	std::vector<std::string> lines;
	const std::string &text = buffer->GetText();
	size_t pos = 0;
	while (pos <= text.size())
	{
		size_t end = text.find('\n', pos);
		std::string line = text.substr(pos, end == std::string::npos ? std::string::npos : end - pos);
		if (!line.empty() && line.back() == '\r')
			line.pop_back();
		if (end == std::string::npos && line.empty() && pos > 0)
			break;
		lines.push_back(line);
		if (end == std::string::npos)
			break;
		pos = end + 1;
	}
	ImDrawList *drawList = ImGui::GetWindowDrawList();
	int state = 0;		// the whole file is lexed from the top: files here are small
	for (int index = 0; index < (int)lines.size(); index++)
	{
		int line = index + 1;
		ImGui::PushID(line);
		ImVec2 rowStart = ImGui::GetCursorScreenPos();
		float rowWidth = ImGui::GetWindowWidth() + ImGui::GetScrollX();
		if (line == pcLine)
			drawList->AddRectFilled(rowStart, ImVec2(rowStart.x + rowWidth, rowStart.y + lineHeight), kColorPcLine);
		else if (line == tab.selectedLine)
			drawList->AddRectFilled(rowStart, ImVec2(rowStart.x + rowWidth, rowStart.y + lineHeight), kColorSelectedLine);

		// gutter: a click toggles a source breakpoint of the workspace's machine
		if (ImGui::InvisibleButton("##gutter", ImVec2(gutterWidth, lineHeight)) && symbols)
		{
			CDebugSourceBreakpoint *existing = symbols->FindSourceBreakpoint(buffer->path.c_str(), line);
			if (existing)
				symbols->RemoveSourceBreakpoint(existing);
			else
				symbols->AddSourceBreakpoint(buffer->path.c_str(), line);
			symbols->UpdateRenderBreakpoints();
		}
		float cy = rowStart.y + lineHeight * 0.5f;
		float radius = ImGui::GetFontSize() * 0.28f;
		CDebugSourceBreakpoint *breakpoint = symbols ? symbols->FindSourceBreakpoint(buffer->path.c_str(), line) : NULL;
		if (breakpoint)
		{
			if (breakpoint->IsResolved())
				drawList->AddCircleFilled(ImVec2(rowStart.x + radius + 2.0f, cy), radius, kColorBreakpoint);
			else
				drawList->AddCircle(ImVec2(rowStart.x + radius + 2.0f, cy), radius, kColorBreakpoint, 0, 1.5f);
		}
		if (line == pcLine)
		{
			float ax = rowStart.x + gutterWidth - radius * 2.2f;
			drawList->AddTriangleFilled(ImVec2(ax, cy - radius), ImVec2(ax, cy + radius), ImVec2(ax + radius * 1.6f, cy), kColorPc);
		}
		ImGui::SameLine(0.0f, 0.0f);
		ImGui::TextColored(kColorLineNumber, "%5d ", line);
		ImGui::SameLine(0.0f, 0.0f);
		ImVec2 textStart = ImGui::GetCursorScreenPos();
		SourceRenderLineText(lines[index], lexer, &state);
		float textWidth = ImGui::GetItemRectMax().x - textStart.x;
		float visibleWidth = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x - textStart.x;
		ImGui::SetCursorScreenPos(textStart);
		ImGui::InvisibleButton("##text", ImVec2(std::max(1.0f, std::max(textWidth, visibleWidth)), lineHeight));
		if (ImGui::IsItemClicked())
			tab.selectedLine = line;
		ImGui::PopID();
	}
	ImGui::EndChild();
}

static int EditCallback(ImGuiInputTextCallbackData *data)
{
	if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
	{
		std::string *text = (std::string *)data->UserData;
		text->resize((size_t)data->BufTextLen);
		data->Buf = (char *)text->c_str();
	}
	return 0;
}

void CViewWorkspace::RenderEditor(Tab &tab)
{
	CSourceBuffer *buffer = tab.buffer.get();
	int index = (int)(&tab - &tabs[0]);
	if (editTab != index)
	{
		editText = buffer->GetText();
		editTab = index;
	}
	ImGuiIO &io = ImGui::GetIO();
	bool undo = false, redo = false;
	if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && (io.KeyCtrl || io.KeySuper))
	{
		undo = ImGui::IsKeyPressed(ImGuiKey_Z) && !io.KeyShift;
		redo = ImGui::IsKeyPressed(ImGuiKey_Y) || (ImGui::IsKeyPressed(ImGuiKey_Z) && io.KeyShift);
	}
	if ((undo && buffer->Undo()) || (redo && buffer->Redo()))
	{
		editText = buffer->GetText();
		ImGui::ClearActiveID();		// the widget takes the new text
	}
	ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput | ImGuiInputTextFlags_NoUndoRedo | ImGuiInputTextFlags_CallbackResize;
	if (ImGui::InputTextMultiline("##projectEdit", (char *)editText.c_str(), editText.capacity() + 1, ImVec2(-1.0f, -1.0f),
								  flags, EditCallback, &editText))
		buffer->SetText(editText, SYS_GetCurrentTimeInMillis());
}

void CViewWorkspace::RenderTabs()
{
	if (!ImGui::BeginTabBar("##projectTabs", ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_AutoSelectNewTabs
							| ImGuiTabBarFlags_FittingPolicyScroll))
		return;
	int requested = activeTab;
	for (int i = 0; i < (int)tabs.size(); i++)
	{
		Tab &tab = tabs[i];
		// the ID from the path's hash: a path's '/' would split an ImGui ID path
		char id[32];
		snprintf(id, sizeof(id), "###tab%zx", std::hash<std::string>()(tab.buffer->path));
		std::string label = SourceDocumentFileName(tab.buffer->path) + (tab.buffer->IsDirty() ? " *" : "") + id;
		bool open = true;
		ImGuiTabItemFlags flags = (i == requested && requested != lastRenderedTab) ? ImGuiTabItemFlags_SetSelected : 0;
		if (ImGui::BeginTabItem(label.c_str(), &open, flags))
		{
			if (activeTab != i && requested == lastRenderedTab)
				SelectTab(i);
			// tab toolbar
			if (ImGui::Checkbox("Edit", &tab.editing))
				editTab = -1;
			ImGui::SameLine();
			ImGui::BeginDisabled(!tab.buffer->IsDirty() && !tab.buffer->IsConflicting());
			if (ImGui::Button("Save"))
				SaveTab(i);
			ImGui::EndDisabled();
			ImGui::SameLine();
			ImGui::BeginDisabled(!tab.buffer->CanUndo());
			if (ImGui::Button("Undo") && tab.buffer->Undo())
				editTab = -1;
			ImGui::EndDisabled();
			ImGui::SameLine();
			ImGui::BeginDisabled(!tab.buffer->CanRedo());
			if (ImGui::Button("Redo") && tab.buffer->Redo())
				editTab = -1;
			ImGui::EndDisabled();
			ImGui::SameLine();
			if (ImGui::Button("Revert"))
			{
				tab.buffer->Revert();
				tab.notice.clear();
				editTab = -1;
			}
			ImGui::SameLine();
			if (ImGui::Button("Find"))
				showFind = !showFind;
			if (!tab.notice.empty())
				ImGui::TextColored(kColorWarning, "%s", tab.notice.c_str());
			if (showFind)
			{
				ImGui::SetNextItemWidth(ImGui::GetFontSize() * 14.0f);
				if (ImGui::InputText("##find", findText, sizeof(findText), ImGuiInputTextFlags_EnterReturnsTrue))
					FindNext(tab);
				ImGui::SameLine();
				if (ImGui::Button("Next"))
					FindNext(tab);
				ImGui::SameLine();
				ImGui::Checkbox("Aa", &findCaseSensitive);
			}
			if (tab.editing)
				RenderEditor(tab);
			else
				RenderView(tab);
			ImGui::EndTabItem();
		}
		if (!open)
		{
			if (tab.buffer->IsDirty())
				pendingCloseTab = i;
			else
			{
				CloseTab(i, false);
				break;
			}
		}
	}
	lastRenderedTab = activeTab;
	ImGui::EndTabBar();
}

void CViewWorkspace::RenderConsole()
{
	std::vector<std::string> output = build.GetOutput();
	std::vector<CBuildDiagnostic> diagnostics = build.GetDiagnostics();
	if (output.empty())
		return;
	char header[96];
	snprintf(header, sizeof(header), "Build output (%d diagnostics)###projectConsole", (int)diagnostics.size());
	if (!ImGui::CollapsingHeader(header, ImGuiTreeNodeFlags_DefaultOpen))
		return;
	ImGui::BeginChild("##projectConsoleLines", ImVec2(0, ImGui::GetTextLineHeightWithSpacing() * 8.0f), true,
					  ImGuiWindowFlags_HorizontalScrollbar);
	std::map<int, const CBuildDiagnostic *> byLine;
	for (const CBuildDiagnostic &d : diagnostics)
		byLine[d.outputLine] = &d;
	for (int i = 0; i < (int)output.size(); i++)
	{
		auto it = byLine.find(i);
		if (it == byLine.end())
		{
			ImGui::TextUnformatted(output[i].c_str());
			continue;
		}
		const CBuildDiagnostic *d = it->second;
		ImVec4 color = d->severity == CBuildDiagnostic::Severity::Error ? ImVec4(0.95f, 0.45f, 0.45f, 1.0f)
			: d->severity == CBuildDiagnostic::Severity::Warning ? kColorWarning : kColorLineNumber;
		ImGui::PushStyleColor(ImGuiCol_Text, color);
		std::string label = output[i] + "##diag" + std::to_string(i);
		if (ImGui::Selectable(label.c_str()))
			OpenDiagnostic(*d);
		ImGui::PopStyleColor();
		if (ImGui::IsItemHovered() && d->path.empty())
			ImGui::SetTooltip("%s is not a file here", d->file.c_str());
	}
	if (scrollConsole)
	{
		ImGui::SetScrollHereY(1.0f);
		scrollConsole = build.IsRunning();
	}
	ImGui::EndChild();
}

void CViewWorkspace::RenderPopups()
{
	if (consentTask >= 0)
		ImGui::OpenPopup("Run build commands?##projectConsent");
	if (ImGui::BeginPopupModal("Run build commands?##projectConsent", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::TextUnformatted("This project's build runs these commands:");
		if (consentTask >= 0 && consentTask < (int)workspace.buildTasks.size())
		{
			for (const std::vector<std::string> &argv : workspace.buildTasks[consentTask].steps)
			{
				std::string command;
				for (const std::string &arg : argv)
					command += (command.empty() ? "" : " ") + arg;
				ImGui::BulletText("%s", command.c_str());
			}
		}
		ImGui::TextDisabled("They run as you. Allow them for %s?", workspace.root.c_str());
		if (ImGui::Button("Allow and build"))
		{
			int task = consentTask;
			consentTask = -1;
			ImGui::CloseCurrentPopup();
			StartBuild(task, true);
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel"))
		{
			consentTask = -1;
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	if (pendingCloseTab >= 0)
		ImGui::OpenPopup("Discard changes?##projectClose");
	if (ImGui::BeginPopupModal("Discard changes?##projectClose", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		std::string name = pendingCloseTab < (int)tabs.size() ? SourceDocumentFileName(tabs[pendingCloseTab].buffer->path) : "";
		ImGui::Text("%s has unsaved changes.", name.c_str());
		if (ImGui::Button("Save and close"))
		{
			if (SaveTab(pendingCloseTab))
				CloseTab(pendingCloseTab, false);
			pendingCloseTab = -1;
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Discard"))
		{
			CloseTab(pendingCloseTab, true);
			pendingCloseTab = -1;
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel"))
		{
			pendingCloseTab = -1;
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

void CViewWorkspace::RenderImGui()
{
	PreRenderImGui();

	u64 now = SYS_GetCurrentTimeInMillis();
	if (now - lastWatchMs >= kWatchIntervalMs)
	{
		lastWatchMs = now;
		PollFiles();
	}
	PollBuild();

	RenderToolbar();
	if (workspace.IsOpen() || !tabs.empty())
	{
		float explorerWidth = workspace.IsOpen() ? ImGui::GetFontSize() * 14.0f : 0.0f;
		if (workspace.IsOpen())
		{
			ImGui::BeginChild("##projectExplorer", ImVec2(explorerWidth, -ImGui::GetFrameHeightWithSpacing() * 0.0f), true);
			RenderExplorer();
			ImGui::EndChild();
			ImGui::SameLine();
		}
		ImGui::BeginGroup();
		float consoleHeight = build.GetOutput().empty() ? 0.0f : ImGui::GetTextLineHeightWithSpacing() * 10.0f;
		ImGui::BeginChild("##projectDocuments", ImVec2(0, -consoleHeight), false);
		RenderTabs();
		ImGui::EndChild();
		RenderConsole();
		ImGui::EndGroup();
	}
	else
		ImGui::TextDisabled("Open a folder: its files, tabs and build tasks (%s)", CWorkspace::kConfigFileName);

	RenderPopups();
	PostRenderImGui();
}
