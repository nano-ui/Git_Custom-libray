#include "AnimationSequencerEditor.h"
#include "ModelPreviewScene.h"
#include "Engine\Graphics\Graphics.h"
#include "Editor\FileDialogHelper.h"

#include <windows.h>
#include <imgui.h>
#include <imgui_internal.h>

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
	active_scene->Initialize();
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
	// 画面最上部のメインメニューバーを描画
	if (ImGui::BeginMainMenuBar())
	{
		// ファイルメニューの展開判定
		if (ImGui::BeginMenu(u8"ファイル"))
		{
			// モデル読み込み項目の選択判定
			if (ImGui::MenuItem(u8"モデルを開く"))
			{
				const PathResult path_result = FileDialogHelper::OpenGenericFileDialog();

				// 取得した相対パスが有効かつプレビュー画面が存在するか判定
				if (!path_result.relative_path.empty() && preview_scene)
				{
					// 構造体が自動計算した相対パスをそのままモデル読み込みに渡す
					preview_scene->LoadModel(path_result.relative_path);
				}
			}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}

	// シーケンサ専用のドッキングスペースIDを定義
	const ImGuiID dockspace_id = ImGui::GetID("SequencerDockSpace");

	// 左端サイドバーの幅（80px）を避けるため、メインビューポートの右側領域にドッキングスペースを配置
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImVec2 dock_pos = viewport->Pos;
	dock_pos.x += 80.0f;
	ImVec2 dock_size = viewport->Size;
	dock_size.x -= 80.0f;

	ImGui::SetNextWindowPos(dock_pos);
	ImGui::SetNextWindowSize(dock_size);
	ImGui::SetNextWindowViewport(viewport->ID);

	// ドッキングスペースの土台ウィンドウを表示
	const ImGuiWindowFlags host_window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;

	ImGui::Begin("SequencerHostWindow", nullptr, host_window_flags);
	ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

	// 初回起動時のみレイアウトを4分割に強制構築する判定
	static bool is_first_frame = true;
	if (is_first_frame)
	{
		is_first_frame = false;

		// 既存のレイアウト構造を一度リセット
		ImGui::DockBuilderRemoveNode(dockspace_id);
		ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockspace_id, dock_size);

		ImGuiID dock_main_id = dockspace_id;
		ImGuiID dock_left_id;
		ImGuiID dock_down_id;

		// 画面全体を上下に分割（下部にタイムライン用領域を確保。全体の約30%の比率）
		dock_main_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.30f, &dock_down_id, &dock_main_id);

		// 残った上部エリアを左右に分割（左側にコントロール・リスト用領域を確保。幅約25%の比率）
		ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.25f, &dock_left_id, &dock_main_id);

		ImGuiID dock_left_top_id;
		ImGuiID dock_left_bottom_id;

		// 左側エリアをさらに上下に均等分割（上がボタン、下がアニメーション一覧）
		ImGui::DockBuilderSplitNode(dock_left_id, ImGuiDir_Up, 0.50f, &dock_left_top_id, &dock_left_bottom_id);

		// 各ノードIDに対して、対応するウィンドウ名を紐付け（ドッキング）
		ImGui::DockBuilderDockWindow(u8"操作パネル", dock_left_top_id);
		ImGui::DockBuilderDockWindow(u8"アニメーション一覧", dock_left_bottom_id);
		ImGui::DockBuilderDockWindow(u8"タイムライン", dock_down_id);
		ImGui::DockBuilderDockWindow("Animation Preview", dock_main_id);

		// 構築したレイアウトを確定
		ImGui::DockBuilderFinish(dockspace_id);
	}
	ImGui::End();

	// 操作パネルウィンドウの描画
	if (ImGui::Begin(u8"操作パネル"))
	{
		ImGui::Text(u8"いろんなボタン配置");
	}
	ImGui::End();

	// アニメーション一覧ウィンドウの描画
	if (ImGui::Begin(u8"アニメーション一覧"))
	{
		ImGui::Text(u8"アニメーション一覧");
	}
	ImGui::End();

	// タイムラインウィンドウの描画
	if (ImGui::Begin(u8"タイムライン"))
	{
		ImGui::Text(u8"アニメーションのイベント設定バーやアニメーションカーブなどを表示");
	}
	ImGui::End();

	// アクティブシーンが有効か判定
	if (active_scene)
	{
		active_scene->RenderGui();
	}
}