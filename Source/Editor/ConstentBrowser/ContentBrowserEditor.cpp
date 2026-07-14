#include "ContentBrowserEditor.h"

#include <windows.h>
#include <imgui.h>

//コンストラクタ
ContentBrowserEditor::ContentBrowserEditor()
{
}

//デストラクタ
ContentBrowserEditor::~ContentBrowserEditor()
{
}

//初期化処理
void ContentBrowserEditor::Initialize()
{
	root_path = "Data";
	current_path = root_path;
	
	if (!std::filesystem::exists(root_path))
	{
		OutputDebugStringA("[Error] ContentBrowserEditor: 'Data' directory not found!\n");
	}
}

//Gui描画
void ContentBrowserEditor::RenderGui()
{
	const float default_window_width = 500.0f;	//ウィンドウ幅
	const float default_window_height = 400.0f;	//ウィンドウの高さ
	ImGui::SetNextWindowSize(ImVec2(default_window_width, default_window_height), ImGuiCond_FirstUseEver);

	if (ImGui::Begin(u8"コンテンツブラウザ"))
	{
		const float left_panel_width = 200.0f;	//フォルダツリーの横幅
		const float panel_child_id = 1;			//パネル識別ID

		ImGui::BeginChild("FolderTreeChild", ImVec2(left_panel_width, 0.0f), true);
		DrawFolderTree(root_path);
		ImGui::EndChild();

		ImGui::SameLine();

		ImGui::BeginChild("FolderContentsChild", ImVec2(0.0f, 0.0f), true);
		DrawFolderContents(current_path);
		ImGui::EndChild();
	}
	ImGui::End();
}

//フォルダ階層を表示
void ContentBrowserEditor::DrawFolderTree(const std::filesystem::path& path)
{
	try
	{
		for (const auto& entry : std::filesystem::directory_iterator(path))
		{
			if (entry.is_directory())
			{
				std::string folder_name = entry.path().filename().string();	//フォルダ名
				ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;	//挙動制御フラグ

				if (current_path == entry.path())
				{
					node_flags |= ImGuiTreeNodeFlags_Selected;
				}

				if (ImGui::TreeNodeEx(folder_name.c_str(), node_flags))
				{
					if (ImGui::IsItemClicked())
					{
						current_path = entry.path();
						selected_path = entry.path();
					}
					DrawFolderTree(entry.path());
					ImGui::TreePop();
				}
				else
				{
					if (ImGui::IsItemClicked())
					{
						current_path = entry.path();
						selected_path = entry.path();
					}
				}
			}
		}
	}
	catch (const std::filesystem::filesystem_error& error)
	{
		OutputDebugStringA("[Error] ContentBrowserEditor: Failed to iterate directory in tree view.\n");
	}
}

//フォルダ内容を表示
void ContentBrowserEditor::DrawFolderContents(const std::filesystem::path& path)
{
	if (!std::filesystem::exists(path))
	{
		return;
	}

	try
	{
		for (const auto& entry : std::filesystem::directory_iterator(path))
		{
			std::string name = entry.path().filename().string();	//ファイル名

			if (entry.is_directory())
			{
				std::string folder_label = "[Folder]" + name;	//フォルダ名

				if (ImGui::Selectable(folder_label.c_str(), current_path == entry.path(), ImGuiSelectableFlags_AllowDoubleClick))
				{
					if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
					{
						current_path = entry.path();
					}
				}
			}
			else
			{
				std::string file_label = "[File]" + name;	//ファイルラベル
				ImGui::TextUnformatted(file_label.c_str());
			}
		}
	}
	catch (const std::filesystem::filesystem_error& error)
	{
		OutputDebugStringA("[Error] ContentBrowserEditor: Failed to iterate directory contents.\n");
	}
}
