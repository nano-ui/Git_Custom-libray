#include "AnimationSequencerEditor.h"
#include "ModelPreviewScene.h"
#include "Engine\Graphics\Graphics.h"
#include "Editor\FileDialogHelper.h"
#include "Editor\EditorMediator.h"

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
		ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), u8"シーケンス制御");
		ImGui::Separator();
		ImGui::Spacing();

		// 【ImGui描画】再生速度の編集用スライダー
		if (ImGui::SliderFloat(u8"再生速度", &playback_speed, 0.0f, 3.0f, "%.2f x"))
		{
			// 速度がスライダーで変更されたら、即座に仲介役を介してプレビューウィンドウへ同期
			EditorMediator::Instance().SetModelAnimationSpeed(playback_speed);
		}

		ImGui::Spacing();

		// ループ設定の切り替え
		if (ImGui::Checkbox(u8"ループ再生", &is_loop))
		{
			// 必要に応じてプレビュー側と同期（今回はモデル側が個別にis_loopを持っていますが同期させておく）
		}
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
		// プレビュー中のモデルからアニメーション総時間を取得
		animation_duration = EditorMediator::Instance().GetModelAnimationDuration();

		// アニメーションが存在しない場合は操作を制限するセーフガード
		bool is_disabled = (animation_duration <= 0.0f);
		if (is_disabled)
		{
			ImGui::BeginDisabled();
		}

		// 再生 / 一時停止ボタン
		if (ImGui::Button(is_playing ? u8"一時停止" : u8"再生"))
		{
			is_playing = !is_playing;
			EditorMediator::Instance().SetModelAnimationPlaying(is_playing);
		}

		ImGui::SameLine();

		if (!ImGui::IsAnyItemActive() && is_playing)
		{
			current_time = EditorMediator::Instance().GetModelAnimationCurrentTime();
		}

		ImGui::SetNextItemWidth(-1.0f);

		char progress_label[64];
		sprintf_s(progress_label, "%.2f s / %.2f s", current_time, animation_duration);

		// マウスで引っ張って時間を変えられるシークバースライダー
		if (ImGui::SliderFloat("##TimelineSeek", &current_time, 0.0f, animation_duration, progress_label))
		{
			// マウスでドラッグ中のみ、変更された時間（current_time）を仲介役を介してモデルに即座にシーク通知する
			EditorMediator::Instance().SetModelAnimationTime(current_time);
		}

		if (is_disabled)
		{
			ImGui::EndDisabled();
			ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), u8"※アニメーションデータがロードされていません。");
		}
	}
	ImGui::End();

	// アクティブシーンが有効か判定
	if (active_scene)
	{
		active_scene->RenderGui();
	}
}