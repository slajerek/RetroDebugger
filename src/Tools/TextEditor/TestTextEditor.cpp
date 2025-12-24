#include "SYS_Defs.h"
#include "DBG_Log.h"
#include "TextEditor.h"
#include <fstream>
#include <streambuf>

TextEditor *editor = NULL;

void TEST_Editor()
{
	return;
	
	LOGM("TEST_Editor");

	editor = new TextEditor();
	auto lang = TextEditor::LanguageDefinition::CPlusPlus();
	
	editor->SetLanguageDefinition(lang);
	
	TextEditor::ErrorMarkers markers;
		markers.insert(std::make_pair<int, std::string>(6, "Example error here:\nInclude file not found: \"TextEditor.h\""));
		markers.insert(std::make_pair<int, std::string>(41, "Another example error"));
		editor->SetErrorMarkers(markers);
	
	static const char* fileToEdit = "/Users/mars/develop/c64d/src/Tools/TextEditor/TextEditor.cpp";
	
	{
			std::ifstream t(fileToEdit);
			if (t.good())
			{
				std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
				editor->SetText(str);
			}
		}
	
	
	LOGM("TEST_Editor DONE");
}

void TEST_Editor_Render()
{
	auto cpos = editor->GetCursorPosition();
	ImGui::Begin("Source code", nullptr, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_MenuBar);
			ImGui::SetWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
	ImGui::Text("%6d/%-6d %6d lines  | %s | %s | %s", cpos.mLine + 1, cpos.mColumn + 1,
				editor->GetTotalLines(),
				editor->IsOverwrite() ? "Ovr" : "Ins",
				editor->CanUndo() ? "*" : " ",
				editor->GetLanguageDefinition().mName.c_str());

	editor->Render("TextEditor");
	ImGui::End();
	

}

