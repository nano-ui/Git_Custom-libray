#include "AnimationSequencerEditor.h"
#include "ModelPreviewScene.h"
#include "Engine\Graphics\Graphics.h"
#include "Editor\FileDialogHelper.h"

#include <windows.h>
#include <imgui.h>

//コンストラクタ
AnimationSequencerEditor::AnimationSequencerEditor()
{
}

//デストラクタ
AnimationSequencerEditor::~AnimationSequencerEditor() = default;

//初期化処理
void AnimationSequencerEditor::Initialize()
{
	preview_scene = std::make_shared<ModelPreviewScene>();
	active_scene = preview_scene;
}

//更新処理
void AnimationSequencerEditor::Update(float elapsed_time)
{
	active_scene->Update(elapsed_time);
}

//描画処理
void AnimationSequencerEditor::Render(ID3D11DeviceContext* immediate_context)
{
	active_scene->Render(immediate_context);
}

//ImGui描画処理
void AnimationSequencerEditor::RenderGui()
{
	//メニューバーの描画
	if (ImGui::BeginMenuBar())
	{
		//ファイルメニューの展開判定
		if (ImGui::BeginMenu(u8"ファイル"))
		{
			//モデル読み込み項目の選択判定
			if (ImGui::MenuItem(u8"モデルを開く"))
			{
				const std::string selected_path = FileDialogHelper::OpenFileDialog();
				if (!selected_path.empty() && preview_scene)
				{
					preview_scene->LoadModel(selected_path);
				}
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}
	}

	const ImGuiID dockspace_id = ImGui::GetID("SequencerDockSpace");
	ImGui::DockSpaceOverViewport(dockspace_id, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

	if (ImGui::Begin(u8"操作パネル"))
	{
		ImGui::Text(u8"いろんなボタン配置");
	}
	ImGui::End();

	if (ImGui::Begin(u8"アニメーション一覧"))
	{
		ImGui::Text(u8"アニメーション一覧");
	}
	ImGui::End();

	if (ImGui::Begin("タイムライン"))
	{
		ImGui::Text("アニメーションのイベント設定バーやアニメーションカーブなどを表示");
	}
	ImGui::End();


	active_scene->RenderGui();
}