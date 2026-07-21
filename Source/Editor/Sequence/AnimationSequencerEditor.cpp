#include "AnimationSequencerEditor.h"
#include "ModelPreviewScene.h"
#include "Engine\Graphics\Graphics.h"
#include "Editor\FileDialogHelper.h"
#include "Editor\EditorMediator.h"

#include <windows.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <algorithm>

static constexpr float MIN_KEYFRAME_INTERVAL = 0.02f;

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

	//プレビューモデル側からロード中のアニメーション総時間を取得
	float duration = EditorMediator::Instance().GetModelAnimationDuration();

	//アニメーション総時間の変更（新しいアニメーションのロード完了）を検知
	if (duration > 0.0f && animation_duration != duration)
	{
		animation_duration = duration;
		InitializerTimeMap();
		EditorMediator::Instance().SetModelAnimationPlaying(false);
	}

	if (animation_duration <= 0.0f)return;

	//アニメーションがロード済み、かつシーケンサが再生中の場合のみシーケンサ時間を進める
	if (animation_duration > 0.0f && is_playing)
	{
		//再生速度を考慮したデルタ経過時間をシーケンサ時間へ加算
		current_time += elapsed_time * playback_speed;

		//タイムラインの終端に達した場合のループ/停止処理
		if (current_time > animation_duration)
		{
			if (is_loop)
			{
				current_time = fmodf(current_time, animation_duration);
			}
			else
			{
				current_time = animation_duration;
				is_playing = false;
			}
		}
		//シーケンサの時間をもとに、タイムリマップ補間されたモデル再生時間を取得
		float remapped_time = GetRemappedTime(current_time);

		//取得した時間をプレビューウィンドウ内の3Dモデルに直接強制適用
		EditorMediator::Instance().SetModelAnimationTime(remapped_time);
	}
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

	// タイムラインウィンドウの描画
	if (ImGui::Begin(u8"タイムライン", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) 
	{
		// プレビュー中のモデルからアニメーション総時間を取得
		animation_duration = EditorMediator::Instance().GetModelAnimationDuration();

		// アニメーションが存在しない場合は操作を制限するセーフガード
		bool is_disabled = (animation_duration <= 0.0f);
		if (is_disabled)
		{
			ImGui::BeginDisabled();
		}

		// 【固定エリア：上部コントロール＆シークバー】
		ImGui::BeginChild("TimelineControl", ImVec2(0, 65), false, ImGuiWindowFlags_NoScrollbar);
		{
			// 再生 / 一時停止ボタン
			if (ImGui::Button(is_playing ? u8"一時停止" : u8"再生"))
			{
				is_playing = !is_playing;
				EditorMediator::Instance().SetModelAnimationPlaying(is_playing);
			}

			ImGui::SameLine();

			ImGui::SetNextItemWidth(-1.0f);

			char progress_label[64];
			sprintf_s(progress_label, "%.2f s / %.2f s", current_time, animation_duration);

			// マウスで引っ張って時間を変えられるシークバースライダー
			if (ImGui::SliderFloat("##TimelineSeek", &current_time, 0.0f, animation_duration, progress_label))
			{
				float remapped_time = GetRemappedTime(current_time);
				EditorMediator::Instance().SetModelAnimationTime(remapped_time);
			}
		}
		ImGui::EndChild(); // 固定コントロールエリアの終了

		// コントロール部分と下部スクロールコンテンツの境界線
		ImGui::Separator();

		// ここから下の領域はマウスでスクロールしても、上記のコントロール部分は一番上に固定表示
		ImGui::BeginChild("TimelineDetails", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
		{
			DrawTimelineTracks();
		}
		ImGui::EndChild(); // 可動エリアの終了

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

//キーフレームの初期化
void AnimationSequencerEditor::InitializerTimeMap()
{
	time_map_keyframes.clear();

	if (animation_duration <= 0.0f)return;

	//アニメーション開始時のキーフレーム
	TimeMapKeyframe start_kf = { 0.0f,0.0f };
	time_map_keyframes.push_back(start_kf);

	//アニメーション終了時のキーフレーム
	TimeMapKeyframe end_kf = { animation_duration,animation_duration };
	time_map_keyframes.push_back(end_kf);
}

//モデル時間を線形補間して算出
float AnimationSequencerEditor::GetRemappedTime(float seq_time) const
{
	if (time_map_keyframes.empty())
	{
		OutputDebugStringA("[Sequencer Error] GetRemappedTime: Keyframes array is empty!\n");
		return 0.0f;
	}

	//キーフレームが1点しかない場合はその点の対応値を返す
	if (time_map_keyframes.size() == 1)return time_map_keyframes[0].model_time;

	//指定時間が最初のキーフレームより前の場合は、最前点の値を返す
	if (seq_time <= time_map_keyframes.front().sequencer_time)return time_map_keyframes.front().model_time;

	//指定時間が最後のキーフレームより後の場合は、最後端の値を返す
	if (seq_time >= time_map_keyframes.back().sequencer_time)return time_map_keyframes.back().model_time;

	//適切なキーフレーム区間（二点の間）を線形探索
	for (size_t i = 0; i < time_map_keyframes.size()-1; i++)
	{
		const TimeMapKeyframe& kf0 = time_map_keyframes[i];
		const TimeMapKeyframe& kf1 = time_map_keyframes[i + 1];

		//探索対象のシーケンサ時間がこの二つのキーフレームの間に存在するか確認
		if (seq_time >= kf0.sequencer_time && seq_time <= kf1.sequencer_time)
		{
			//区間内の総時間幅を計算
			float duration = kf1.sequencer_time - kf0.sequencer_time;

			//キーフレーム同士が極端に重なっている場合のゼロ除算防止
			if (duration <= 0.00001f)return kf0.model_time;

			//2つのキーフレーム間における進行比率を算出
			float rate = (seq_time - kf0.sequencer_time) / duration;

			//線形補間（Lerp）を用いて、モデルが再生すべき正確な時間を計算して返す
			return kf0.model_time + rate * (kf1.model_time - kf0.model_time);
		}
	}
	return 0.0f;
}

//タイムライン詳細トラックを描画し、ドラッグなどのマウス操作を行う
void AnimationSequencerEditor::DrawTimelineTracks()
{
	//アニメーションの長さが未設定、またはデータがおかしい場合は描画をスキップ
	if (animation_duration <= 0.0f || time_map_keyframes.empty())
	{
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), u8"※アニメーションを読み込んでください。");
		return;
	}

	ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), u8"タイムリマップ（縦：モデル時間 / 横：シーケンス時間）");
	ImGui::Spacing();

	//カスタム描画用の領域幅と座標情報を算出
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
	ImVec2 canvas_size = ImGui::GetContentRegionAvail();

	//トラック描画エリアの高さ
	float track_height = 120.0f;
	if (canvas_size.y < track_height)
	{
		canvas_size.y = track_height;
	}
	canvas_size.y = track_height;

	//外枠の背景枠を描画
	ImVec2 canvas_end = ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y);
	draw_list->AddRectFilled(canvas_pos, canvas_end, IM_COL32(40, 40, 45, 255));
	draw_list->AddRect(canvas_pos, canvas_end, IM_COL32(100, 100, 110, 255));

	float padding_x = 15.0f;
	float usable_width = canvas_size.x - (padding_x * 2.0f);

	//マウス入力イベント
	ImGuiIO& io = ImGui::GetIO();
	ImVec2 mouse_pos = io.MousePos;

	//【当たり判定と入力処理の管理】
	if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))selected_keyframe_index = -1;

	//各キーフレームをループして、当たり判定および描画
	for (int i = 0; i < static_cast<int>(time_map_keyframes.size()); i++)
	{
		TimeMapKeyframe& kf = time_map_keyframes[i];

		//制御点のピクセル座標を算出
		float t_rate_x = kf.sequencer_time / animation_duration;
		float t_rate_y = kf.model_time / animation_duration;

		float kf_x = canvas_pos.x + padding_x + (t_rate_x * usable_width);
		float kf_y = canvas_pos.y + canvas_size.y - padding_x - (t_rate_y * (canvas_size.y - (padding_x * 2.0f)));
		ImVec2 pt(kf_x, kf_y);

		//制御点（丸）とマウスの距離を測って、当たり判定チェック
		float dx = mouse_pos.x - pt.x;
		float dy = mouse_pos.y - pt.y;
		float dist_sq = (dx * dx) + (dy * dy);
		bool is_hovered = (dist_sq < 8.0f * 8.0f);
		ImU32 dot_color = is_hovered ? IM_COL32(255, 220, 0, 255) : IM_COL32(0, 190, 255, 255);

		//【ドラッグ操作による時間調整】
		if (is_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))selected_keyframe_index = i;

		if (selected_keyframe_index == i && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
		{
			//ドラッグによるマウスの移動先から新しい時間を逆算
			float relative_mouse_x = mouse_pos.x - (canvas_pos.x + padding_x);
			float relative_mouse_y = (canvas_pos.y + canvas_size.y - padding_x) - mouse_pos.y;
			float new_seq_time = (relative_mouse_x / usable_width) * animation_duration;
			float new_model_time = (relative_mouse_y / (canvas_size.y - (padding_x * 2.0f))) * animation_duration;

			//時間が範囲外にはみ出ないようにクランプ制限
			if (new_seq_time < 0.0f)new_seq_time = 0.0f;
			if (new_seq_time > animation_duration)new_seq_time = animation_duration;
			if (new_model_time < 0.0f)new_model_time = 0.0f;
			if (new_model_time > animation_duration)new_model_time = animation_duration;

			//前後のキーフレームを追い越さないように移動可能限界を設
			float min_seq = (i > 0) ? time_map_keyframes[i - 1].sequencer_time + MIN_KEYFRAME_INTERVAL : 0.0f;
			float max_seq = (i < static_cast<int>(time_map_keyframes.size()) - 1) ? time_map_keyframes[i + 1].sequencer_time - MIN_KEYFRAME_INTERVAL : animation_duration;

			//最初と最後のキーフレームは、補間関係維持のために「シーケンサ時間(横軸)」の移動はロック
			if (i == 0)kf.sequencer_time = 0.0f;
			else if (i == static_cast<int>(time_map_keyframes.size()) - 1)kf.sequencer_time = animation_duration;
			else kf.sequencer_time = (std::max)(min_seq, (std::min)(new_seq_time, max_seq));

			//縦軸（モデルの実際の再生時間）は、制限なくドラッグ移動
			kf.model_time = new_model_time;

			//モデル側に変更された時間を即時通知して、3Dビューをドラッグに追従
			float current_remapped = GetRemappedTime(current_time);
			EditorMediator::Instance().SetModelAnimationTime(current_remapped);
		}
		//【右クリックによるキーフレーム削除】
		if (is_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
		{
			// 最初と最後のフレームはタイムリマップの基準点となるため、削除不可
			if (i > 0 && i < static_cast<int>(time_map_keyframes.size()) - 1)
			{
				time_map_keyframes.erase(time_map_keyframes.begin() + i);
				selected_keyframe_index = -1;

				// 警告ログをデバッグ出力
				OutputDebugStringA("[Sequencer] Keyframe deleted via right-click.\n");
				break; //イテレータの破綻を防ぐためループを抜る
			}
		}
		//制御点（丸）を描画
		draw_list->AddCircleFilled(pt, 6.0f, dot_color);
		draw_list->AddCircle(pt, 7.0f, IM_COL32(0, 0, 0, 200), 0, 1.5f);

		//時間表示のテキストをオーバーレイ
		char kf_text[32];
		sprintf_s(kf_text, "S:%.1fs\nM:%.1fs", kf.sequencer_time, kf.model_time);
		draw_list->AddText(ImVec2(pt.x - 20.0f, pt.y - 35.0f), IM_COL32(220, 220, 220, 255), kf_text);
	}

	//【制御点を繋ぐ折れ線の描画】
	for (size_t i = 0; i < time_map_keyframes.size() - 1; ++i)
	{
		const auto& kf0 = time_map_keyframes[i];
		const auto& kf1 = time_map_keyframes[i + 1];

		float x0 = canvas_pos.x + padding_x + ((kf0.sequencer_time / animation_duration) * usable_width);
		float y0 = canvas_pos.y + canvas_size.y - padding_x - ((kf0.model_time / animation_duration) * (canvas_size.y - (padding_x * 2.0f)));

		float x1 = canvas_pos.x + padding_x + ((kf1.sequencer_time / animation_duration) * usable_width);
		float y1 = canvas_pos.y + canvas_size.y - padding_x - ((kf1.model_time / animation_duration) * (canvas_size.y - (padding_x * 2.0f)));

		draw_list->AddLine(ImVec2(x0, y0), ImVec2(x1, y1), IM_COL32(0, 220, 100, 255), 2.0f);
	}
	//【ダブルクリックによるキーフレーム追加】
	if (ImGui::IsMouseHoveringRect(canvas_pos, canvas_end) && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
	{
		float relative_click_x = mouse_pos.x - (canvas_pos.x + padding_x);
		float relative_click_y = (canvas_pos.y + canvas_size.y - padding_x) - mouse_pos.y;

		float click_seq_time = (relative_click_x / usable_width) * animation_duration;
		float click_model_time = (relative_click_y / (canvas_size.y - (padding_x * 2.0f))) * animation_duration;

		if (click_seq_time > 0.0f && click_seq_time < animation_duration)
		{
			// 新規キーフレームを追加
			TimeMapKeyframe new_kf = { click_seq_time, click_model_time };
			time_map_keyframes.push_back(new_kf);

			// 追加後にシーケンサ時間の昇順になるように配列を強制的にソート
			std::sort(time_map_keyframes.begin(), time_map_keyframes.end(), CompareKeyframes);

			// デバッグログを出力
			char added_log[256];
			sprintf_s(added_log, "[Sequencer] Added Keyframe via Double-Click at SequencerTime: %.2fs -> ModelTime: %.2fs\n", click_seq_time, click_model_time);
			OutputDebugStringA(added_log);
		}
	}

	// 描画領域を次に進めるための ImGui カーソル進め処理
	ImGui::Dummy(canvas_size);
}

//キーフレームをシーケンサ時間の昇順でソート
bool AnimationSequencerEditor::CompareKeyframes(const TimeMapKeyframe& a, const TimeMapKeyframe& b)
{
	return a.sequencer_time < b.sequencer_time;
}
