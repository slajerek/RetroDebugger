#include "CViewSourceCode.h"
#include "CSourceRendering.h"
#include "CDebugSourceTrace.h"
#include "CSyntaxLexer.h"
#include "CViewDisassembly.h"
#include "CDebugInterface.h"
#include "CDebugSymbols.h"
#include "CDebugSymbolsSegment.h"
#include "CDebugBreakpointsAddr.h"
#include "CDebugDatabase.h"
#include "CDebugSourceBreakpoint.h"
#include "CSourceStepController.h"
#include "CByteBuffer.h"
#include "DBG_Log.h"
#include "imgui.h"
#include <algorithm>
#include <cstring>

// the lexer's look-back for a line's start state (an open block comment)
static const int kLexLookBack = 200;

CViewSourceCode::CViewSourceCode(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY,
								 CDebugInterface *debugInterface, CDebugSymbols *debugSymbols,
								 CViewDisassembly *viewDisassembly)
: CGuiView(name, posX, posY, posZ, sizeX, sizeY)
{
	imGuiNoWindowPadding = false;
	imGuiNoScrollbar = true;

	this->viewDisassembly = viewDisassembly;
	this->debugSymbols = debugSymbols;
	this->debugInterface = debugInterface;
	stepper = NULL;
	bankAware = true;

	follow = SourceViewFollow::Pc;
	showLineNumbers = true;

	regionId = -1;
	hasFile = false;
	fileId = 0;
	pcLine = 0;
	pcCandidates = 0;
	selectedLine = 0;
	scrollToLine = 0;
	lastFollowAddress = -1;
	sourceRootInput[0] = 0;

	if (viewDisassembly)
		viewDisassembly->viewSourceCode = this;
}

CViewSourceCode::~CViewSourceCode()
{
}

const CDebugSourceFile *CViewSourceCode::CurrentFile() const
{
	if (!database || !hasFile)
		return NULL;
	return database->GetFile(fileId);
}

void CViewSourceCode::ShowLine(u32 fileId, i32 line)
{
	this->hasFile = true;
	this->fileId = fileId;
	this->selectedLine = line;
	this->scrollToLine = line;
}

bool CViewSourceCode::ShowAddress(int address)
{
	std::shared_ptr<const CDebugDatabase> db = debugSymbols->debugDatabase;
	if (!db || !debugSymbols->currentSegment || debugSymbols->currentSegment->debugRegionId < 0)
		return false;
	const CDebugSourceMapping *mapping = ResolveAt(db.get(), (u32)debugSymbols->currentSegment->debugRegionId, (u16)address);
	if (mapping == NULL)
		return false;
	follow = SourceViewFollow::Off;
	ShowLine(mapping->fileId, mapping->lineStart);
	return true;
}

const CDebugSourceMapping *CViewSourceCode::ResolveAt(const CDebugDatabase *database, u32 regionId, u16 address) const
{
	if (bankAware)
		return debugInterface->ResolveSourceAt(database, regionId, address);
	return database ? database->Resolve(regionId, address) : NULL;
}

// The PC line (always, for the arrow) and what the follow mode shows.
void CViewSourceCode::UpdatePosition()
{
	pcLine = 0;
	pcCandidates = 0;
	pcInline.clear();
	if (!database || regionId < 0)
		return;

	int pc = cpuPc ? cpuPc() : debugInterface->GetCpuPC();
	const CDebugSourceMapping *pcMapping = (pc >= 0) ? ResolveAt(database.get(), (u32)regionId, (u16)pc) : NULL;
	if (pcMapping)
	{
		// RD-017: "in twice() inlined at main.c:24"
		const std::vector<CDebugInlineSite> &chain = database->InlineChain(pcMapping);
		if (!chain.empty())
		{
			const CDebugSourceFile *callerFile = database->GetFile(chain[0].fileId);
			pcInline = "in " + (chain[0].function.empty() ? std::string("inlined code") : chain[0].function + "()")
				+ " inlined at " + (callerFile ? callerFile->fileName : std::string("?")) + ":" + std::to_string(chain[0].line);
		}
		std::vector<const CDebugSourceMapping *> candidates;
		database->ResolveCandidates((u32)regionId, (u32)pc, candidates);
		pcCandidates = (int)candidates.size();
	}

	int followAddress = -1;
	if (follow == SourceViewFollow::Pc)
		followAddress = pc;
	else if (follow == SourceViewFollow::Cursor && viewDisassembly)
		followAddress = viewDisassembly->cursorAddress;

	if (followAddress >= 0 && followAddress != lastFollowAddress)
	{
		const CDebugSourceMapping *mapping = ResolveAt(database.get(), (u32)regionId, (u16)followAddress);
		if (mapping)
		{
			bool moved = !hasFile || mapping->fileId != fileId || mapping->lineStart != selectedLine;
			hasFile = true;
			fileId = mapping->fileId;
			if (moved)
			{
				selectedLine = mapping->lineStart;
				scrollToLine = mapping->lineStart;
			}
		}
	}
	lastFollowAddress = followAddress;

	if (!hasFile && database->NumFiles() > 0)
	{
		hasFile = true;
		fileId = database->Files().begin()->first;
	}
	if (pcMapping && hasFile && pcMapping->fileId == fileId)
		pcLine = pcMapping->lineStart;
}

bool CViewSourceCode::LineHasCode(i32 line) const
{
	std::vector<const CDebugSourceMapping *> mappings;
	database->ResolveSource(fileId, line, mappings);
	for (const CDebugSourceMapping *m : mappings)
	{
		if ((int)m->regionId == regionId)
			return true;
	}
	return false;
}

std::vector<u32> CViewSourceCode::AddressesOfLine(i32 line)
{
	std::vector<u32> addresses;
	std::shared_ptr<const CDebugDatabase> db = debugSymbols->debugDatabase;
	if (!db || !hasFile || !debugSymbols->currentSegment)
		return addresses;
	int region = debugSymbols->currentSegment->debugRegionId;
	std::vector<const CDebugSourceMapping *> mappings;
	db->ResolveSource(fileId, line, mappings);
	for (const CDebugSourceMapping *m : mappings)
	{
		if ((int)m->regionId != region)
			continue;
		// the start of each range in effect; a continuation range of the same
		// line adds nothing new to jump to
		bool continuation = false;
		for (const CDebugSourceMapping *other : mappings)
		{
			if (other != m && other->regionId == m->regionId && other->end == m->start)
				continuation = true;
		}
		if (!continuation && std::find(addresses.begin(), addresses.end(), m->start) == addresses.end())
			addresses.push_back(m->start);
	}
	std::sort(addresses.begin(), addresses.end());
	return addresses;
}

void CViewSourceCode::NavigateToLine(i32 line)
{
	selectedLine = line;
	navigateAddresses = AddressesOfLine(line);
	if (navigateAddresses.size() == 1 && viewDisassembly)
	{
		viewDisassembly->ScrollToAddress((int)navigateAddresses[0]);
		navigateAddresses.clear();
	}
	else if (navigateAddresses.size() > 1)
	{
		ImGui::OpenPopup("##sourceNavigate");
	}
}

// matches the way DebugResolveSourceLine finds a file: recorded path, loaded
// path, then the file name
static bool SourceBreakpointIsInFile(const CDebugSourceBreakpoint *breakpoint, const CDebugSourceFile *file)
{
	if (breakpoint->path == file->originalPath || (!file->loadedPath.empty() && breakpoint->path == file->loadedPath))
		return true;
	return SourceDocumentFileName(breakpoint->path) == file->fileName;
}

int CViewSourceCode::LineBreakpointState(const CDebugSourceFile *file, i32 line) const
{
	int state = 0;
	for (const CDebugSourceBreakpoint *breakpoint : debugSymbols->sourceBreakpoints)
	{
		if (!SourceBreakpointIsInFile(breakpoint, file))
			continue;
		if (breakpoint->IsResolved() && breakpoint->resolvedLine == line)
			return 1;
		if (!breakpoint->IsResolved() && breakpoint->line == line)
			state = 2;
	}
	if (state == 0 && debugSymbols->currentSegment && debugSymbols->currentSegment->breakpointsPC)
	{
		std::vector<const CDebugSourceMapping *> mappings;
		database->ResolveSource(fileId, line, mappings);
		for (const CDebugSourceMapping *m : mappings)
		{
			if ((int)m->regionId == regionId && debugSymbols->currentSegment->breakpointsPC->GetBreakpoint((int)m->start))
				return 1;
		}
	}
	return state;
}

void CViewSourceCode::ToggleBreakpointAtLine(i32 line)
{
	std::shared_ptr<const CDebugDatabase> db = debugSymbols->debugDatabase;
	const CDebugSourceFile *file = (db && hasFile) ? db->GetFile(fileId) : NULL;
	if (file == NULL)
		return;

	debugInterface->LockMutex();
	// a source breakpoint on the line (or one that slid onto it) goes
	CDebugSourceBreakpoint *existing = NULL;
	for (CDebugSourceBreakpoint *breakpoint : debugSymbols->sourceBreakpoints)
	{
		if (SourceBreakpointIsInFile(breakpoint, file)
			&& ((breakpoint->IsResolved() && breakpoint->resolvedLine == line) || breakpoint->line == line))
		{
			existing = breakpoint;
			break;
		}
	}
	if (existing)
	{
		debugSymbols->RemoveSourceBreakpoint(existing);
	}
	else
	{
		// address breakpoints the user set on the line's code go
		bool removed = false;
		CDebugSymbolsSegment *segment = debugSymbols->currentSegment;
		if (segment && segment->breakpointsPC)
		{
			for (u32 address : AddressesOfLine(line))
			{
				if (segment->breakpointsPC->GetBreakpoint((int)address))
				{
					segment->breakpointsPC->DeleteBreakpoint((int)address);
					removed = true;
				}
			}
		}
		if (!removed)
			debugSymbols->AddSourceBreakpoint(file->originalPath.c_str(), line);
	}
	debugInterface->UnlockMutex();
	debugSymbols->UpdateRenderBreakpoints();
}

void CViewSourceCode::RenderHeader(const CDebugSourceFile *file)
{
	// file
	ImGui::SetNextItemWidth(ImGui::GetFontSize() * 12.0f);
	if (ImGui::BeginCombo("##sourceFile", file ? file->fileName.c_str() : "(no file)"))
	{
		for (auto &it : database->Files())
		{
			bool selected = hasFile && it.first == fileId;
			std::string label = it.second->fileName + "##" + std::to_string(it.first);
			if (ImGui::Selectable(label.c_str(), selected))
			{
				hasFile = true;
				fileId = it.first;
				follow = SourceViewFollow::Off;
				scrollToLine = 1;
			}
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("%s", it.second->originalPath.c_str());
		}
		ImGui::EndCombo();
	}

	// follow mode
	ImGui::SameLine();
	static const char *followNames[] = { "Follow PC", "Follow cursor", "Follow off" };
	ImGui::SetNextItemWidth(ImGui::GetFontSize() * 7.5f);
	int followIndex = (int)follow;
	if (ImGui::Combo("##sourceFollow", &followIndex, followNames, 3))
	{
		follow = (SourceViewFollow)followIndex;
		lastFollowAddress = -1;
	}

	// PC position and the last step, on the same row
	if (!pcInline.empty())
	{
		ImGui::SameLine();
		ImGui::TextColored(kColorLineNumber, "PC %s", pcInline.c_str());
	}
	if (pcCandidates > 1)
	{
		ImGui::SameLine();
		ImGui::TextColored(kColorWarning, "PC: ambiguous (%d)", pcCandidates);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("%d source ranges cover the PC; showing the one in effect", pcCandidates);
	}
	CSourceStepResult step = (stepper ? stepper : debugInterface->sourceStepper)->GetLastResult();
	if (step.reason != SourceStepStopReason::None)
	{
		ImGui::SameLine();
		bool abnormal = step.reason == SourceStepStopReason::BudgetExceeded || step.reason == SourceStepStopReason::NoSource;
		if (step.mapped)
			ImGui::TextColored(abnormal ? kColorWarning : kColorLineNumber, "Step %s: %s at %s:%d",
							   SourceStepKindName(step.kind), SourceStepStopReasonName(step.reason),
							   step.fileName.c_str(), step.line);
		else
			ImGui::TextColored(abnormal ? kColorWarning : kColorLineNumber, "Step %s: %s at $%04X",
							   SourceStepKindName(step.kind), SourceStepStopReasonName(step.reason), step.pc);
	}

	for (const std::string &warning : debugSymbols->debugInfoWarnings)
		ImGui::TextColored(kColorWarning, "Stale debug info: %s", warning.c_str());
}

void CViewSourceCode::RenderMissing(const CDebugSourceFile *file)
{
	ImGui::TextColored(kColorWarning, "Source not found: %s", file->originalPath.c_str());
	ImGui::TextUnformatted("Looked for it at:");
	for (const std::string &candidate : documents.Candidates(file->originalPath, file->loadedPath))
		ImGui::BulletText("%s", candidate.c_str());

	ImGui::TextUnformatted("Add a folder to look in by file name:");
	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - ImGui::GetFontSize() * 5.0f);
	ImGui::InputText("##sourceRoot", sourceRootInput, sizeof(sourceRootInput));
	ImGui::SameLine();
	if (ImGui::Button("Add folder") && sourceRootInput[0])
	{
		documents.sourceRoots.push_back(sourceRootInput);
		sourceRootInput[0] = 0;
	}
}

void CViewSourceCode::RenderLineText(const std::string &text, const CSyntaxLexer *lexer, int *state)
{
	SourceRenderLineText(text, lexer, state);
}

void CViewSourceCode::RenderDocument(const CDebugSourceFile *file, const CSourceDocument *document)
{
	const CSyntaxLexer *lexer = SyntaxLexerForPath(file->originalPath, database->producer);
	float lineHeight = ImGui::GetTextLineHeightWithSpacing();
	float gutterWidth = ImGui::GetFontSize() * 1.6f;
	float numberWidth = showLineNumbers ? ImGui::CalcTextSize("00000").x : 0.0f;

	ImGui::BeginChild("##sourceText", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

	if (scrollToLine > 0)
	{
		float target = (scrollToLine - 1) * lineHeight - ImGui::GetWindowHeight() * 0.4f;
		ImGui::SetScrollY(std::max(0.0f, target));
		scrollToLine = 0;
	}

	ImDrawList *drawList = ImGui::GetWindowDrawList();
	ImGuiListClipper clipper;
	clipper.Begin(document->NumLines(), lineHeight);
	while (clipper.Step())
	{
		// the lexer state at the first visible line, from a short look-back
		int state = 0;
		std::vector<SyntaxSpan> scratch;
		if (lexer)
		{
			for (int line = std::max(1, clipper.DisplayStart + 1 - kLexLookBack); line <= clipper.DisplayStart; line++)
				lexer->LexLine(document->GetLine(line), &state, scratch);
		}

		for (int index = clipper.DisplayStart; index < clipper.DisplayEnd; index++)
		{
			i32 line = index + 1;
			ImGui::PushID(line);
			ImVec2 rowStart = ImGui::GetCursorScreenPos();
			float rowWidth = ImGui::GetWindowWidth() + ImGui::GetScrollX();

			if (line == pcLine)
				drawList->AddRectFilled(rowStart, ImVec2(rowStart.x + rowWidth, rowStart.y + lineHeight), kColorPcLine);
			else if (line == selectedLine)
				drawList->AddRectFilled(rowStart, ImVec2(rowStart.x + rowWidth, rowStart.y + lineHeight), kColorSelectedLine);

			// gutter: breakpoint / code dot, PC arrow; a click toggles a breakpoint
			if (ImGui::InvisibleButton("##gutter", ImVec2(gutterWidth, lineHeight)))
				ToggleBreakpointAtLine(line);
			float cy = rowStart.y + lineHeight * 0.5f;
			float radius = ImGui::GetFontSize() * 0.28f;
			int bpState = LineBreakpointState(file, line);
			if (bpState == 1)
				drawList->AddCircleFilled(ImVec2(rowStart.x + radius + 2.0f, cy), radius, kColorBreakpoint);
			else if (bpState == 2)
				drawList->AddCircle(ImVec2(rowStart.x + radius + 2.0f, cy), radius, kColorBreakpoint, 0, 1.5f);
			else if (LineHasCode(line))
				drawList->AddCircleFilled(ImVec2(rowStart.x + radius + 2.0f, cy), radius * 0.35f, kColorCodeDot);
			if (line == pcLine)
			{
				float ax = rowStart.x + gutterWidth - radius * 2.2f;
				drawList->AddTriangleFilled(ImVec2(ax, cy - radius), ImVec2(ax, cy + radius),
											ImVec2(ax + radius * 1.6f, cy), kColorPc);
			}

			ImGui::SameLine(0.0f, 0.0f);
			if (showLineNumbers)
			{
				char number[16];
				snprintf(number, sizeof(number), "%5d ", line);
				ImGui::TextColored(kColorLineNumber, "%s", number);
				ImGui::SameLine(0.0f, 0.0f);
			}
			(void)numberWidth;

			// the text; a click selects, a double-click shows the code
			ImVec2 textStart = ImGui::GetCursorScreenPos();
			RenderLineText(document->GetLine(line), lexer, &state);
			// as wide as the text or the visible rest of the row, never wider:
			// a wider item would widen the content and scroll the view sideways
			float textWidth = ImGui::GetItemRectMax().x - textStart.x;
			float visibleWidth = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x - textStart.x;
			ImGui::SetCursorScreenPos(textStart);
			ImGui::InvisibleButton("##text", ImVec2(std::max(1.0f, std::max(textWidth, visibleWidth)), lineHeight));
			if (ImGui::IsItemClicked())
				selectedLine = line;
			if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				NavigateToLine(line);

			ImGui::PopID();
		}
	}
	clipper.End();

	if (ImGui::BeginPopup("##sourceNavigate"))
	{
		ImGui::TextUnformatted("Line has code at:");
		for (u32 address : navigateAddresses)
		{
			char label[16];
			snprintf(label, sizeof(label), "$%04X", address);
			if (ImGui::Selectable(label) && viewDisassembly)
				viewDisassembly->ScrollToAddress((int)address);
		}
		ImGui::EndPopup();
	}

	ImGui::EndChild();
}

void CViewSourceCode::RenderImGui()
{
	PreRenderImGui();

	// Hold the snapshot for the whole frame: a reload swaps in a new one.
	database = debugSymbols->debugDatabase;
	regionId = debugSymbols->currentSegment ? debugSymbols->currentSegment->debugRegionId : -1;

	if (!database)
	{
		ImGui::TextDisabled("No debug info loaded");
		for (const std::string &warning : debugSymbols->debugInfoWarnings)
			ImGui::TextColored(kColorWarning, "%s", warning.c_str());
		PostRenderImGui();
		return;
	}

	if (hasFile && database->GetFile(fileId) == NULL)
		hasFile = false;	// a reload without that file

	UpdatePosition();
	const CDebugSourceFile *file = CurrentFile();

	debugInterface->LockMutex();
	RenderHeader(file);
	RenderSourceTrace();
	if (file)
	{
		std::shared_ptr<const CSourceDocument> document = documents.Open(file->originalPath, file->loadedPath);
		if (!document && file->HasText() && file->loadedPath.empty())
			document = OpenFileText(file);		// a source with no file: assembled in memory (RD-043)
		if (document)
			RenderDocument(file, document.get());
		else
			RenderMissing(file);
	}
	debugInterface->UnlockMutex();

	database.reset();
	PostRenderImGui();
}

std::shared_ptr<const CSourceDocument> CViewSourceCode::OpenFileText(const CDebugSourceFile *file)
{
	// the file object is new with every database: its text's version
	std::string key = "memory:" + file->originalPath;
	if (!documents.HasText(key, file))
	{
		std::string text;
		for (size_t i = 1; i < file->lines.size(); i++)
		{
			char *line = file->lines[i]->GetStdASCII();
			text += line;
			text += "\n";
			delete [] line;
		}
		return documents.OpenText(key, file, text);
	}
	return documents.OpenText(key, file, std::string());
}

bool CViewSourceCode::HasContextMenuItems()
{
	return true;
}

void CViewSourceCode::RenderContextMenuItems()
{
	CGuiView::RenderContextMenuItems();
	ImGui::MenuItem("Show line numbers", NULL, &showLineNumbers);
	if (ImGui::MenuItem("Follow PC", NULL, follow == SourceViewFollow::Pc))
		follow = SourceViewFollow::Pc;
	if (ImGui::MenuItem("Follow disassembly cursor", NULL, follow == SourceViewFollow::Cursor))
		follow = SourceViewFollow::Cursor;
	if (ImGui::MenuItem("Follow off", NULL, follow == SourceViewFollow::Off))
		follow = SourceViewFollow::Off;
	// RD-045: the main CPU's executed lines, live or from a replayed trace
	if (!cpuPc)
	{
		ImGui::Separator();
		bool recording = debugInterface->IsRecordingSourceTrace();
		if (ImGui::MenuItem("Record executed lines", NULL, recording))
			debugInterface->SetSourceTraceRecording(!recording);
	}
}

// RD-045: the latest executed source lines, newest first; a click shows one
void CViewSourceCode::RenderSourceTrace()
{
	if (cpuPc || !debugInterface->IsRecordingSourceTrace() || debugInterface->sourceTrace == NULL)
		return;
	std::vector<CDebugSourceTraceEntry> entries = debugInterface->sourceTrace->GetEntries(kSourceTraceShown);
	if (!ImGui::CollapsingHeader("Executed lines##sourceTrace", ImGuiTreeNodeFlags_DefaultOpen))
		return;
	float height = ImGui::GetTextLineHeightWithSpacing() * (float)(kSourceTraceShown / 2);
	ImGui::BeginChild("##sourceTraceList", ImVec2(0, height), true);
	for (auto it = entries.rbegin(); it != entries.rend(); ++it)
	{
		char label[160];
		if (it->mapped)
			snprintf(label, sizeof(label), "%s:%d  $%04X  x%llu##%p", it->fileName.c_str(), it->line, it->pc,
					 (unsigned long long)it->instructions, (void *)&(*it));
		else
			snprintf(label, sizeof(label), "(no source)  $%04X  x%llu##%p", it->pc, (unsigned long long)it->instructions, (void *)&(*it));
		if (ImGui::Selectable(label) && it->mapped)
		{
			follow = SourceViewFollow::Off;
			ShowLine(it->fileId, it->line);
		}
	}
	ImGui::EndChild();
}

// Layout
void CViewSourceCode::Serialize(CByteBuffer *byteBuffer)
{
}

void CViewSourceCode::Deserialize(CByteBuffer *byteBuffer)
{
}
