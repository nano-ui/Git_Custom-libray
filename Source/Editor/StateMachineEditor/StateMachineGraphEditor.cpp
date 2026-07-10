#define IMGUI_DEFINE_MATH_OPERATORS

#include "StateMachineGraphEditor.h"
#include "Gameplay\StateMachine\StateBlackboard.h"
#include "../Gameplay/StateMachine/StateGraphDataManager.h"
#include "../Gameplay/GameObjects/ObjectManager.h"
#include "../Editor/FileDialogHelper.h"
#include "../Editor/EditorMediator.h"
#include "StateGraphPaletteWindow.h"
#include "StateGraphPropertyWindow.h"
#include "StateGraphSimulator.h"
#include "StateGraphConfigManager.h"
#include "Editor\AssetLoader.h"

#include <imgui_node_editor_internal.h>
#include <cassert>
#include <fstream>
#include <iomanip>

namespace ed = ax::NodeEditor;

static uint32_t g_pending_focus_node_id = 0;

//コンストラクタ
StateMachineGraphEditor::StateMachineGraphEditor()
{
	ed::Config config; // 設定データ
	config.SettingsFile = "Data/Json/NodeEditor_State.json";
	editor_context.reset(ed::CreateEditor(&config));

	const uint32_t root_id = 0; // ルート階層の固定ID
	current_graph_id = root_id;

	data_manager = std::make_unique<StateGraphDataManager>();

	palette_window = std::make_unique<StateGraphPaletteWindow>();
	property_window = std::make_unique<StateGraphPropertyWindow>();
	config_manager = std::make_unique<StateGraphConfigManager>();
	asset_loader = std::make_unique<AssetLoader>();

	target_model_hash = 0;

	LoadEditorCondig();

	bool is_success = false; //成功判定フラグ

	if (!current_loaded_file_path.empty())
	{
		is_success = data_manager->LoadFromFile(current_loaded_file_path);
	}
	else
	{
		current_loaded_file_path = "Data/Json/NodeEditor_State.json";
		if (!data_manager->LoadFromFile(current_loaded_file_path))
		{
			data_manager->CheckAndInitDefaultNode(current_graph_id);
		}
	}

	//グラフの読み込みに成功し、データにモデルパスが記録されているか判定
	if (is_success && !data_manager->GetTargetModelPath().empty())
	{
		asset_loader->LoadModelAnimations(data_manager->GetTargetModelPath());
	}

	// 起動時の自動復元に成功し、かつモデルパスがデータに存在するか判定
	if (is_success && !data_manager->GetTargetModelPath().empty())
	{
		// 既存の関数でモデルとアニメーションリストをロード
		if (asset_loader->LoadModelAnimations(data_manager->GetTargetModelPath()))
		{
			std::filesystem::path path_obj(data_manager->GetTargetModelPath());
			std::string model_name = path_obj.stem().string(); //拡張子を除いたファイル名を抽出

			target_model_hash = StateBlackboard::CalculateHash(model_name);
		}
	}

	TriggerHotReload();

	EditorMediator::Instance().RegisterStateMachineGraphEditor(this);
}

//デストラクタ
StateMachineGraphEditor::~StateMachineGraphEditor() = default;

//エディタ描画
void StateMachineGraphEditor::DrawEditor(StateBlackboard* blackboard)
{
	UpdateRuntimeTracking();

	GraphData* current_graph = nullptr; // 現在の階層情報

	for (size_t i = 0; i < data_manager->GetLayerDatas().size(); i++)
	{
		if (data_manager->GetLayerDatas()[i].id == current_graph_id)
		{
			current_graph = &data_manager->GetLayerDatas()[i];
			break;
		}
	}

	if (!current_graph)
	{
		assert(false && "StateMachineGraphEditor: 指定されたグラフIDが見つかりません。");
		return;
	}

	uint32_t& current_active_node_id = graph_active_nodes[current_graph_id]; // 階層固有のアクティブID

	if (runtime_active_node_id != UINT32_MAX)
	{
		current_active_node_id = runtime_active_node_id;
	}
	else
	{
		if (current_active_node_id == 0 && !current_graph->nodes.empty())
		{
			current_active_node_id = current_graph->nodes.front().id;
		}
	}

	// ゲーム側の実行ノードIDが前フレームから変化した瞬間を直接検知する条件
	if (runtime_active_node_id != UINT32_MAX && previous_active_node_id != 0 && previous_active_node_id != runtime_active_node_id)
	{
		flow_src_node_id = previous_active_node_id;
		flow_dst_node_id = runtime_active_node_id;
		has_flow_requsted = true;
		//printf("StateMachineGraphEditor: 純粋なステート遷移を検知しました。ノードID: %d -> %d\n", flow_src_node_id, flow_dst_node_id);

		// リアルタイム追尾機能が有効であるかを判定する条件
		if (is_tracking_active_node)
		{
			g_pending_focus_node_id = runtime_active_node_id;
		}
	}

	// 有効な実行中IDが届いている場合のみ、次フレーム用の比較元として保存する条件
	if (runtime_active_node_id != UINT32_MAX)
	{
		previous_active_node_id = runtime_active_node_id;
	}

	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);

	if (DrawTopMenuBar())
	{
		return;
	}

	const float pane_top_margin_y = 10.0f; // 上部マージン
	ImGui::Dummy(ImVec2(0.0f, pane_top_margin_y));

	static float dynamic_left_width = 470.0f; // マウス変更可能な左サイドバー横幅
	static float dynamic_right_width = 500.0f; // マウス変更可能な右サイドバー横幅

	const float min_pane_width = 100.0f; // 各ペインの最小横幅制限
	const float min_pane_height = 100.0f; // 各ペインの最小縦幅制限
	const float separator_line_width = 6.0f; // セパレーターの掴み幅

	float total_available_width = ImGui::GetContentRegionAvail().x; // 全体の有効横幅

	float canvas_width = total_available_width - dynamic_left_width - dynamic_right_width - (separator_line_width * 2.0f); // キャンバス横幅

	if (canvas_width < min_pane_width)
	{
		canvas_width = min_pane_width;
	}

	float canvas_height = ImGui::GetContentRegionAvail().y; // 全体の有効縦幅
	if (canvas_height < min_pane_height)
	{
		canvas_height = min_pane_height;
	}

	DrawLeftSidebar(current_graph, dynamic_left_width, canvas_height);

	ImGui::SameLine();

	ImGui::Button("##LeftSplitter", ImVec2(separator_line_width, canvas_height));
	if (ImGui::IsItemActive())
	{
		dynamic_left_width += ImGui::GetIO().MouseDelta.x;
		if (dynamic_left_width < min_pane_width) dynamic_left_width = min_pane_width;
	}

	ImGui::SameLine();

	DrawCenterCanvas(current_graph, canvas_width, canvas_height);

	ImGui::SameLine();

	ImGui::Button("##RightSplitter", ImVec2(separator_line_width, canvas_height));
	if (ImGui::IsItemActive())
	{
		dynamic_right_width -= ImGui::GetIO().MouseDelta.x;
		if (dynamic_right_width < min_pane_width) dynamic_right_width = min_pane_width;
	}

	ImGui::SameLine();

	DrawRightSidebar(current_graph, blackboard, dynamic_right_width, canvas_height);
}

//ファイルパスのグラフ情報をリロード
bool StateMachineGraphEditor::LoadGraphFromFile(const std::string& file_path)
{
	//パスが空文字か判定
	if (file_path.empty())
	{
		printf("[Warning] StateMachineGraphEditor::LoadGraphFromFile - 渡されたパスが空です。\n");
		return false;
	}

	bool load_result = data_manager->LoadFromFile(file_path);	//読み込みフラグ

	//読み込みの成否を判定
	if (load_result)
	{
		const uint32_t reset_root_id = 0;
		current_graph_id = reset_root_id;
		current_loaded_file_path = file_path;
		SaveEditorCondig();

		//読み込んだデータにモデルパスが記録されているか判定
		if (!data_manager->GetTargetModelPath().empty())
		{
			//モデルからアニメーションの読込が成功したか判定
			if (asset_loader->LoadModelAnimations(data_manager->GetTargetModelPath()))
			{
				std::filesystem::path path_obj(data_manager->GetTargetModelPath());
				std::string model_name = path_obj.stem().string();	//拡張子を除いたファイル名
				target_model_hash = StateBlackboard::CalculateHash(model_name);
			}
		}
		else
		{
			asset_loader->LoadModelAnimations("");
			target_model_hash = 0;
		}

		TriggerHotReload();
		last_tracked_runtime_node_id = UINT32_MAX;
		printf("StateMachineGraphEditor: 「%s」から正常読込したため階層をリセットしました。\n", file_path.c_str());
		return true;
	}
	else
	{
		printf("[Error] StateMachineGraphEditor::LoadGraphFromFile - ファイルの読込に失敗しました。ファイルが破損しているか、パスが不正です。対象パス: %s\n", file_path.c_str());
	}
	return false;
}

//追従ロジックとタイマー更新
void StateMachineGraphEditor::UpdateRuntimeTracking()
{
	//追尾機能が有効かつ実行中のアクティブノードIDが有効かを判定
	if (is_tracking_active_node && runtime_active_node_id != UINT32_MAX)
	{
		//実行中のアクティブノードIDが前フレームから変化したかを判定
		if (runtime_active_node_id != last_tracked_runtime_node_id)
		{
			uint32_t target_graph_id = data_manager->GetGraphIdFromNodeId(runtime_active_node_id);	//所属している階層IDを逆引き

			//所属階層が現在の表示階層と異なっているかを判定
			if (target_graph_id != UINT32_MAX && target_graph_id != current_graph_id)
			{
				current_graph_id = target_graph_id;
			}
			//printf("StateMachineGraphEditor: 追尾機能により表示階層を自動切り替えしました。階層ID: %d\n", current_graph_id);
			last_tracked_runtime_node_id = runtime_active_node_id;
		}
	}

	//エフェクトのタイマーが動いているかを判定
	if (flow_effect_timer > 0.0f)
	{
		float delta_time = ImGui::GetIO().DeltaTime;
		flow_effect_timer -= delta_time;

		//タイマーが負の値になったかを判定
		if (flow_effect_timer < 0.0f)
		{
			flow_effect_timer = 0.0f;
		}
	}
}

//上部メニューとナビゲーション
bool StateMachineGraphEditor::DrawTopMenuBar()
{
	ImGui::Begin(u8"ステートマシンエディタ");

	const ImVec4 save_btn_color = ImVec4(0.2f, 0.5f, 0.2f, 1.0f);
	ImGui::PushStyleColor(ImGuiCol_Button, save_btn_color);

	const float upper_btn_width = 200.0f;
	const float upper_btn_height = 25.0f;

	if (ImGui::Button(u8"グラフデータを保存", ImVec2(upper_btn_width, upper_btn_height)))
	{
		std::string selected_save_path = FileDialogHelper::SaveFileDialog();
		if (!selected_save_path.empty())
		{
			data_manager->SaveToFile(selected_save_path);
			current_loaded_file_path = selected_save_path;
			SaveEditorCondig();
		}
	}
	ImGui::PopStyleColor();

	ImGui::SameLine();

	const ImVec4 load_btn_color = ImVec4(0.2f, 0.4f, 0.6f, 1.0f);
	ImGui::PushStyleColor(ImGuiCol_Button, load_btn_color);

	if (ImGui::Button(u8"グラフデータを読込", ImVec2(upper_btn_width, upper_btn_height)))
	{
		std::string selected_load_path = FileDialogHelper::OpenFileDialog();
		if (!selected_load_path.empty())
		{
			LoadGraphFromFile(selected_load_path);
		}
	}
	ImGui::PopStyleColor();
	ImGui::SameLine();

	//モデル選択ボタンが押されたか判定
	if (ImGui::Button(u8"モデル選択"))
	{
		PathResult path_result = FileDialogHelper::OpenGenericFileDialog();

		//ファイルが選択されたか判定
		if (!path_result.absolute_path.empty())
		{
			//モデルのロードとアニメーション名抽出
			if (asset_loader->LoadModelAnimations(path_result.relative_path))
			{
				data_manager->SetTargetModelPath(path_result.relative_path);
				std::filesystem::path path_obj(path_result.relative_path);
				std::string model_name = path_obj.stem().string(); // 拡張子を除いたファイル名を抽出
				target_model_hash = StateBlackboard::CalculateHash(model_name);
				printf("StateMachineGraphEditor: モデル「%s」からアニメーションリストを取得しました。\n", path_result.relative_path.c_str());
				TriggerHotReload();
			}
		}
	}

	//モデル読み込みの成否を判定
	if (!asset_loader->GetLoadedModelPath().empty())
	{
		ImGui::SameLine();
		ImGui::Text(u8" 紐付けモデル: %s", asset_loader->GetLoadedModelPath().c_str());
	}

	ImGui::SameLine();

	//追尾設定チェックボックスが変更されたかを判定
	if (ImGui::Checkbox(u8"実行中ノードを追尾", &is_tracking_active_node))
	{
		printf("StateMachineGraphEditor: 追尾モードが %s に切り替わりました。\n", is_tracking_active_node ? "ON" : "OFF");
	}

	ImGui::SameLine();

	//ズーム補正チェックボックスが変更されたかを判定
	if (ImGui::Checkbox(u8"ズーム自動補正", &is_zoom_correction_enabled))
	{
		printf("StateMachineGraphEditor: ズーム自動補正が %s に切り替わりました。\n", is_zoom_correction_enabled ? "ON" : "OFF");
	}

	ImGui::SameLine();

	ImGui::SetNextItemWidth(100.0f);
	ImGui::SliderFloat(u8"フォーカス時間", &focus_duration_time, 0.0f, 1.0f, "%.2f");

	ImGui::SameLine();

	constexpr float min_margin_limit = 0.0f;	//最小制限
	constexpr float max_margin_limit = 200.0f;	//最大制限
	ImGui::SetNextItemWidth(120.0f);
	if (ImGui::SliderFloat(u8"判定マージン", &focus_margin, min_margin_limit, max_margin_limit, "%.0f px"))
	{

	}

	ImGui::SameLine();

	ImGui::Spacing();

	if (DrawHeaderNavigation())
	{
		ImGui::End();
		return true;
	}
	return false;
}

//左パレットとノードリスト
void StateMachineGraphEditor::DrawLeftSidebar(GraphData* current_graph, float width, float height)
{
	ImGui::BeginChild("LeftSidebarZone##Child", ImVec2(width, height), true);
	uint32_t focus_node_id = 0; //受け取り用のフォーカスID
	palette_window->DrawPalette(data_manager.get(), current_graph, focus_node_id);
	if (focus_node_id != 0)
	{
		g_pending_focus_node_id = focus_node_id;
	}
	ImGui::EndChild();
}

//メインのノードエディタキャンバス
void StateMachineGraphEditor::DrawCenterCanvas(GraphData* current_graph, float width, float height)
{
	bool trigger_add_node = false;         // ノード追加の実行トリガー用フラグ
	bool trigger_add_subgraph = false;     // サブグラフ追加の実行トリガー用フラグ
	bool trigger_convert_subgraph = false; // サブグラフ変換の実行トリガー用フラグ

	ImGui::BeginChild("CenterCanvasZone##Child", ImVec2(width, height), false);

	ed::SetCurrentEditor(editor_context.get());
	ed::Begin("Node Canvas");

	if (current_graph_id == 0 && current_graph->nodes.empty())
	{
		data_manager->CheckAndInitDefaultNode(current_graph_id);

		uint32_t default_node_id = current_graph->nodes.front().id; // 待機ノードID
		constexpr float default_init_pos_x = 100.0f; // 初期座標X
		constexpr float default_init_pos_y = 100.0f; // 初期座標Y
		ed::SetNodePosition(default_node_id, ImVec2(default_init_pos_x, default_init_pos_y));
	}

	for (size_t i = 0; i < current_graph->nodes.size(); i++)
	{
		const GraphNode& node = current_graph->nodes[i]; // 現在描画対象となっているノードデータの参照

		int pushed_style_count = 0; // スタイルカラーのプッシュ総数

		bool is_active_now = (node.id == graph_active_nodes[current_graph_id]); // アクティブ状態フラグ
		if (is_active_now)
		{
			const ImVec4 gold_glow_color = ImVec4(0.0f, 1.0f, 0.3f, 1.0f); // ライムグリーン
			ed::PushStyleColor(ed::StyleColor_NodeBorder, gold_glow_color);
			pushed_style_count++;
		}

		if (node.is_sub_graph)
		{
			const ImVec4 sub_bg_color = ImVec4(0.1f, 0.2f, 0.4f, 0.85f); // サブグラフ用背景色
			const ImVec4 sub_sel_color = ImVec4(0.3f, 0.6f, 1.0f, 1.0f); // サブグラフ選択枠色

			ed::PushStyleColor(ed::StyleColor_NodeBg, sub_bg_color);
			ed::PushStyleColor(ed::StyleColor_SelNodeBorder, sub_sel_color);
			pushed_style_count += 2;
		}
		ed::BeginNode(node.id);

		ImGui::Text("%s", node.name.c_str());
		ImGui::Spacing();

		ImGui::BeginGroup();
		for (size_t in_idx = 0; in_idx < node.inputs.size(); in_idx++)
		{
			const GraphPin& pin = node.inputs[in_idx]; // 入力ピンデータ
			ed::BeginPin(pin.id, ed::PinKind::Input);
			ImGui::Text("->%s", pin.name.c_str());
			ed::EndPin();
		}
		ImGui::EndGroup();

		ImGui::SameLine();
		const float middle_spacer_width = 40.0f; // 余白幅
		ImGui::Dummy(ImVec2(middle_spacer_width, 0.0f));
		ImGui::SameLine();

		ImGui::BeginGroup();
		for (size_t out_idx = 0; out_idx < node.outputs.size(); out_idx++)
		{
			const GraphPin& pin = node.outputs[out_idx]; // 出力ピンデータ
			ed::BeginPin(pin.id, ed::PinKind::Output);
			ImGui::Text("%s ->", pin.name.c_str());
			ed::EndPin();
		}
		ImGui::EndGroup();

		ed::EndNode();
		for (int color_idx = 0; color_idx < pushed_style_count; color_idx++)
		{
			ed::PopStyleColor();
		}
	}

	struct PinCacheData
	{
		uint32_t node_id; // 所属ノードID
		float color_r; // 線の赤
		float color_g; // 線の緑
		float color_b; // 線の青
	};

	std::unordered_map<uint32_t, PinCacheData> pin_cache_map; // キャッシュマップ

	for (size_t n = 0; n < current_graph->nodes.size(); n++)
	{
		const GraphNode& node = current_graph->nodes[n]; // ループ対象ノード
		PinCacheData cache; // 一時構造体
		cache.node_id = node.id;
		cache.color_r = node.link_color_r;
		cache.color_g = node.link_color_g;
		cache.color_b = node.link_color_b;

		for (size_t p = 0; p < node.inputs.size(); p++)
		{
			pin_cache_map[node.inputs[p].id] = cache;
		}

		for (size_t p = 0; p < node.outputs.size(); p++)
		{
			pin_cache_map[node.outputs[p].id] = cache;
		}
	}

	constexpr float FIXED_FLOW_COLOR_R = 0.2f;
	constexpr float FIXED_FLOW_COLOR_G = 0.6f;
	constexpr float FIXED_FLOW_COLOR_B = 0.4f;

	// 階層内の全リンクを巡回するループ処理
	for (size_t i = 0; i < current_graph->links.size(); i++)
	{
		const GraphLink& link = current_graph->links[i]; // リンク参照

		float r = 1.0f; // 通常時赤成分用変数
		float g = 1.0f; // 通常時緑成分用変数
		float b = 1.0f; // 通常時青成分用変数
		uint32_t src_node_id = 0; // 出発ノードID用変数

		auto start_it = pin_cache_map.find(link.start_pin_id); // 検索イテレーター

		// 開始ピンがハッシュマップ内に存在するかを判定する条件分岐
		if (start_it != pin_cache_map.end())
		{
			src_node_id = start_it->second.node_id;
			r = start_it->second.color_r;
			g = start_it->second.color_g;
			b = start_it->second.color_b;
		}

		bool is_last_transition_link = false; // 直近の遷移リンクであるかを保持するフラグ変数
		uint32_t dst_node_id = 0; // 接続先ノードID用変数
		auto end_it = pin_cache_map.find(link.end_pin_id); // 検索イテレーター

		// 終了ピンがハッシュマップ内に存在するかを判定する条件分岐
		if (end_it != pin_cache_map.end())
		{
			dst_node_id = end_it->second.node_id;
		}

		// このリンクが直近で遷移したノード間を結ぶものかを判定する条件分岐
		if (src_node_id == flow_src_node_id && dst_node_id == flow_dst_node_id)
		{
			is_last_transition_link = true;
		}

		// エディタ標準のキャッシュカラーを優先させるため、色は元の色のまま描画を実行
		ed::Link(link.id, link.start_pin_id, link.end_pin_id, ImVec4(r, g, b, 1.0f));

		// 直近の遷移経路として選ばれているリンクであるかを判定する条件分岐（常時エフェクト維持方式へ変更）
		if (is_last_transition_link)
		{
			// 画像から存在が確認できた StyleColor_Flow を用いて、パルスの光の色を上品な緑色に上書きする処理
			ed::PushStyleColor(ed::StyleColor_Flow, ImVec4(FIXED_FLOW_COLOR_R, FIXED_FLOW_COLOR_G, FIXED_FLOW_COLOR_B, 1.0f));

			// 次の遷移が起きるまで毎フレームエフェクトを流し続けるために、無条件でFlowを実行
			ed::Flow(link.id);

			// 上書きしたエフェクトの色を安全に復元するポップ処理
			ed::PopStyleColor();
		}
	}

	if (ed::BeginCreate())
	{
		CreateNewLink(current_graph);
	}
	ed::EndCreate();

	static ImVec2 popup_click_pos = ImVec2(0.0f, 0.0f); // クリック位置
	static ed::NodeId context_node_id = 0; // コンテキストノードID

	ed::Suspend();

	if (ed::ShowBackgroundContextMenu())
	{
		ImGui::OpenPopup("Create New Node Context Menu");
		popup_click_pos = ed::ScreenToCanvas(ImGui::GetMousePos());
	}

	if (ed::ShowNodeContextMenu(&context_node_id))
	{
		ed::SelectNode(context_node_id, true);
		ImGui::OpenPopup("Node Context Menu");
	}

	if (ImGui::BeginPopup("Create New Node Context Menu"))
	{
		if (ImGui::MenuItem(u8"ステート追加"))
		{
			trigger_add_node = true;
		}
		if (ImGui::MenuItem(u8"サブグラフ追加"))
		{
			trigger_add_subgraph = true;
		}
		ImGui::EndPopup();
	}

	if (ImGui::BeginPopup("Node Context Menu"))
	{
		if (ImGui::MenuItem(u8"サブグラフへ変換"))
		{
			trigger_convert_subgraph = true;
		}
		ImGui::EndPopup();
	}

	ed::Resume();

	if (palette_window->HasPendingAddNode())
	{
		ImVec2 center_pos = ImGui::GetMainViewport()->GetCenter(); // 画面中心
		ImVec2 canvas_pos = ed::ScreenToCanvas(center_pos); // キャンバス座標

		if (palette_window->IsPendingSubGraph())
		{
			data_manager->AddNode(
				current_graph,
				static_cast<float>(canvas_pos.x),
				static_cast<float>(canvas_pos.y),
				palette_window->GetPendingNodeName()
			);

			for (size_t i = 0; i < data_manager->GetLayerDatas().size(); i++)
			{
				if (data_manager->GetLayerDatas()[i].id == current_graph_id)
				{
					current_graph = &data_manager->GetLayerDatas()[i];
					break;
				}
			}
		}
		else
		{
			data_manager->AddNode(
				current_graph,
				static_cast<float>(canvas_pos.x),
				static_cast<float>(canvas_pos.y),
				palette_window->GetPendingNodeName()
			);
		}

		uint32_t new_state_id = current_graph->nodes.back().id; // 新しいノードID
		ed::SetNodePosition(new_state_id, canvas_pos);

		palette_window->ClearPendingNode();
	}

	if (trigger_add_node)
	{
		data_manager->AddNode(
			current_graph, 
			static_cast<float>(popup_click_pos.x),
			static_cast<float>(popup_click_pos.y));
		uint32_t new_state_id = current_graph->nodes.back().id; // 追加ノードID
		ed::SetNodePosition(new_state_id, popup_click_pos);
	}
	if (trigger_add_subgraph)
	{
		data_manager->AddSubGrapNode(
			current_graph_id,
			static_cast<float>(popup_click_pos.x),
			static_cast<float>(popup_click_pos.y));

		for (size_t i = 0; i < data_manager->GetLayerDatas().size(); i++)
		{
			if (data_manager->GetLayerDatas()[i].id == current_graph_id)
			{
				current_graph = &data_manager->GetLayerDatas()[i];
				break;
			}
		}
		uint32_t new_node_id = current_graph->nodes.back().id; // サブグラフノードID
		ed::SetNodePosition(new_node_id, popup_click_pos);
	}
	if (trigger_convert_subgraph)
	{
		uint32_t raw_node_id = static_cast<uint32_t>(context_node_id.Get()); // キャストID
		data_manager->ConvertToSubGraph(current_graph_id, raw_node_id);
	}

	if (ed::BeginDelete())
	{
		DeleteNode(current_graph);
		DeleteLink(current_graph);
	}
	ed::EndDelete();

	CheckNavigateToSubGraph(current_graph);

	ed::End();

	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_PAYLOAD_NORMAL"))
		{
			const char* dropped_node_name = static_cast<const char*>(payload->Data); // ノード名
			ImVec2 drap_mouse_screen_pos = ImGui::GetMousePos(); // マウス画面座標
			ImVec2 drop_mouse_canvas_pos = ed::ScreenToCanvas(drap_mouse_screen_pos); // キャンバス座標

			data_manager->AddNode(
				current_graph, 
				static_cast<float>(drop_mouse_canvas_pos.x), 
				static_cast<float>(drop_mouse_canvas_pos.y), 
				dropped_node_name);

			uint32_t new_node_id = current_graph->nodes.back().id; // 新しいノードID
			ed::SetNodePosition(new_node_id, drop_mouse_canvas_pos);

			printf("StateMachineGraphEditor: 通常ステート「%s」をドラッグ＆ドロップで配置しました。\n", dropped_node_name);
		}

		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_PAYLOAD_SUB"))
		{
			const char* dropped_sub_name = static_cast<const char*>(payload->Data); // ノード名
			ImVec2 drap_mouse_screen_pos = ImGui::GetMousePos(); // マウス画面座標
			ImVec2 drop_mouse_canvas_pos = ed::ScreenToCanvas(drap_mouse_screen_pos); // キャンバス座標

			data_manager->AddSubGrapNode(
				current_graph_id,
				static_cast<float>(drop_mouse_canvas_pos.x),
				static_cast<float>(drop_mouse_canvas_pos.y),
				dropped_sub_name);

			for (size_t i = 0; i < data_manager->GetLayerDatas().size(); i++)
			{
				if (data_manager->GetLayerDatas()[i].id == current_graph_id)
				{
					current_graph = &data_manager->GetLayerDatas()[i];
					break;
				}
			}

			uint32_t new_node_id = current_graph->nodes.back().id; // 新しいノードID
			ed::SetNodePosition(new_node_id, drop_mouse_canvas_pos);

			printf("StateMachineGraphEditor: サブグラフ「%s」をドラッグ＆ドロップで完全複製配置しました。\n", dropped_sub_name);
		}
		ImGui::EndDragDropTarget();
	}

	if (g_pending_focus_node_id != 0)
	{
		uint32_t focus_target_id = g_pending_focus_node_id; // 対象IDのローカル退避
		ed::SelectNode(focus_target_id, false);

		auto* internal_context = reinterpret_cast<ax::NodeEditor::Detail::EditorContext*>(ed::GetCurrentEditor()); // 内部コンテキスト
		bool is_node_in_screen = false; // 画面内存在判定フラグ

		if (internal_context)
		{
			ImRect view_rect = internal_context->GetViewRect(); // 表示領域矩形
			auto* internal_node = internal_context->FindNode(focus_target_id); // 内部ノード

			if (internal_node)
			{
				ImRect node_rect = internal_node->m_Bounds; // ノード領域矩形

				if ((view_rect.Min.x + focus_margin) <= node_rect.Min.x &&
					(view_rect.Max.x - focus_margin) >= node_rect.Max.x &&
					(view_rect.Min.y + focus_margin) <= node_rect.Min.y &&
					(view_rect.Max.y - focus_margin) >= node_rect.Max.y)
				{
					is_node_in_screen = true;
				}
			}
		}

		if (!is_node_in_screen)
		{
			ed::NavigateToSelection(is_zoom_correction_enabled, focus_duration_time);

			if (focus_duration_time > 0.0f)
			{
				printf("StateMachineGraphEditor: ノード ID:%d が画面外のためカメラフォーカスを実行しました。\n", focus_target_id);
			}
		}

		g_pending_focus_node_id = 0;
	}

	if (has_flow_requsted)
	{
		has_flow_requsted = false;
	}

	ImGui::EndChild();
}

//右プロパティウインドウ
void StateMachineGraphEditor::DrawRightSidebar(GraphData* current_graph, StateBlackboard* blackboard, float width, float height)
{
	ImGui::BeginChild("RightSidebarZone##Child", ImVec2(width, height), true);
	bool is_changed = property_window->DrawProperty(data_manager.get(), current_graph, blackboard, asset_loader->GetAnimationNames()); // プロパティ変更フラグ

	if (is_changed)
	{
		TriggerHotReload();
	}

	ImGui::EndChild();

	ed::SetCurrentEditor(nullptr);
	ImGui::End();
}

//サブグラフへの階層移動を検知・処理
void StateMachineGraphEditor::CheckNavigateToSubGraph(GraphData* current_graph)
{
	if (!current_graph)
	{
		printf("Error: StateMachineGraphEditor::CheckNavigateToSubGraph - current_graph が nullptr です。\n");
		return;
	}

	ed::NodeId double_clicked_node_id = ed::GetDoubleClickedNode();	//ダブルクリックされたノードID

	if (double_clicked_node_id)
	{
		uint32_t clicked_id = static_cast<uint32_t>(double_clicked_node_id.Get());	//ダブルクリックされたID

		for (size_t i = 0; i < current_graph->nodes.size(); i++)
		{
			const GraphNode& node = current_graph->nodes[i];	//比較対象のノード

			if (node.id == clicked_id)
			{
				if (node.is_sub_graph)
				{
					current_graph_id = node.sub_graph_id;
					//printf("StateMachineGraphEditor: サブグラフ「%s」の内部に入ります。階層ID: %d へ切り替えました。\n",
						//node.name.c_str(), current_graph_id);
				}
				break;
			}
		}
	}
}

//階層ナビゲーションを描画
bool StateMachineGraphEditor::DrawHeaderNavigation()
{
	ImVec2 window_pos = ImGui::GetWindowPos(); // 描画位置
	float title_bar_height = ImGui::GetFrameHeight(); // タイトルバーの高さ
	const float button_margin_y = 35.0f;
	ImVec2 bar_pos = ImVec2(window_pos.x, window_pos.y + title_bar_height + button_margin_y);
	const float bar_height_size = 35.0f;
	ImVec2 bar_size = ImVec2(ImGui::GetWindowWidth(), bar_height_size);

	ImDrawList* draw_list = ImGui::GetWindowDrawList(); // 描画リスト取得
	draw_list->AddRectFilled(bar_pos, ImVec2(bar_pos.x + bar_size.x, bar_pos.y + bar_size.y), IM_COL32(35, 35, 35, 255));

	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
	ImGui::SetWindowFontScale(1.2f);
	ImGui::SetCursorScreenPos(ImVec2(bar_pos.x + 15.0f, bar_pos.y + 8.0f));

	ImGui::Text(u8"現在の階層ID：%d", current_graph_id);
	ImGui::SameLine();

	uint32_t target_navigate_id = current_graph_id;	//IDを一時保存

	if (ImGui::Selectable(u8" / ルート", current_graph_id == 0, ImGuiSelectableFlags_None, ImGui::CalcTextSize(u8" / ルート")))
	{
		target_navigate_id = 0;																		// 階層IDをルートへ変更
		//printf("StateMachineGraphEditor: ナビゲーションバーの文字クリックによりルート階層へ復帰しました。\n");
	}

	if (current_graph_id != 0)
	{
		ImGui::SameLine();

		std::vector<uint32_t> breadcurmbs;		//経路IDを保存するリスト
		uint32_t trace_id = current_graph_id;	//探索用の現在のID

		while (trace_id != 0)
		{
			breadcurmbs.push_back(trace_id);
			uint32_t parent_id = 0;			//親のID
			bool found_parent = false;		//親が見つかったかのフラグ

			for (size_t g = 0; g < data_manager->GetLayerDatas().size(); g++)
			{
				for (size_t n = 0; n < data_manager->GetLayerDatas()[g].nodes.size(); n++)
				{
					if (data_manager->GetLayerDatas()[g].nodes[n].is_sub_graph && data_manager->GetLayerDatas()[g].nodes[n].sub_graph_id == trace_id)
					{
						parent_id = data_manager->GetLayerDatas()[g].id;
						found_parent = true;
						break;
					}
				}

				if (found_parent)
				{
					break;
				}
			}
			if (found_parent)
			{
				trace_id = parent_id;
			}
			else
			{
				//printf("Warning: StateMachineGraphEditor - 階層ID %d の親が見つかりませんでした。\n", trace_id);
				break;
			}
		}

		for (int i = static_cast<int>(breadcurmbs.size()) - 1; i >= 0; i--)
		{
			uint32_t path_id = breadcurmbs[i];	//描画する階層ID
			std::string path_name = "Unknown";	//階層名
			for (size_t g = 0; g < data_manager->GetLayerDatas().size(); g++)
			{
				if (data_manager->GetLayerDatas()[g].id == path_id)
				{
					path_name = data_manager->GetLayerDatas()[g].name;
					break;
				}
			}

			std::string display_text = " / " + path_name;	//表示するテキスト名
			std::string selectable_label = display_text + "##" + std::to_string(path_id);
			ImVec2 text_size = ImGui::CalcTextSize(display_text.c_str());	//文字の大きさ

			ImGui::SameLine();

			if (ImGui::Selectable(selectable_label.c_str(), path_id == current_graph_id, ImGuiSelectableFlags_None, text_size))
			{
				target_navigate_id = path_id;
				//printf("StateMachineGraphEditor: ナビゲーションパスにより階層ID: %d へ移動しました。\n", target_navigate_id);
			}
		}
	}
	bool is_navigated = (current_graph_id != target_navigate_id); // 移動検知フラグ
	current_graph_id = target_navigate_id;

	ImGui::PopStyleColor();
	ImGui::SetWindowFontScale(1.0f);

	ImGui::SameLine();
	const float offset_position_x = 400.0f;	//ボタンを右側に寄せるためのオフセット値
	ImGui::SetCursorScreenPos(ImVec2(bar_pos.x + offset_position_x, bar_pos.y + 5.0f));

	ImGui::SetWindowFontScale(1.0f);
	ImGui::SetCursorPos(ImVec2(0.0f, title_bar_height + bar_size.y + 5.0f));
	ImGui::Dummy(ImVec2(0.0f, 1.0f));

	return is_navigated;
}

//接続線の作成を検知してデータに追加
void StateMachineGraphEditor::CreateNewLink(GraphData* current_graph)
{
	if (!current_graph)
	{
		printf("Error: StateMachineGraphEditor::CreateNewLink - current_graph が nullptr です。\n");
		return;
	}

	ed::PinId start_pin_id;	//接続元のピン
	ed::PinId end_pin_id;	//接続先のピン

	if (ed::QueryNewLink(&start_pin_id, &end_pin_id))
	{
		uint32_t start_id = static_cast<uint32_t>(start_pin_id.Get());	//接続元のID
		uint32_t end_id = static_cast<uint32_t>(end_pin_id.Get());		//接続先のID

		if (data_manager->CheckCanConnect(current_graph_id, start_id, end_id))
		{
			const ImVec4 success_color = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // 成功色 

			if (ed::AcceptNewItem(success_color, 2.0f))
			{
				GraphLink new_link;	//新しい接続情報
				new_link.id = data_manager->FetchAndIncrementId();
				new_link.start_pin_id = static_cast<uint32_t>(start_pin_id.Get());
				new_link.end_pin_id = static_cast<uint32_t>(end_pin_id.Get());
				current_graph->links.push_back(new_link);

				OnLinkCreated(current_graph, new_link);

				//printf("StateMachineGraphEditor: リンクを作成しました。ID: %d, 出力ピン: %d -> 入力ピン: %d\n",
				//	new_link.id, new_link.start_pin_id, new_link.end_pin_id);
			}
		}
		else
		{
			const ImVec4 reject_color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // 失敗色 
			ed::RejectNewItem(reject_color, 2.0f);
		}
	}
}

//ノードの削除
void StateMachineGraphEditor::DeleteNode(GraphData* current_graph)
{
	ed::NodeId delete_node_id; // 対象ノードID
	while (ed::QueryDeletedNode(&delete_node_id))
	{
		if (ed::AcceptDeletedItem())
		{
			uint32_t target_id = static_cast<uint32_t>(delete_node_id.Get()); // キャストID
			data_manager->DeleteNode(current_graph_id, target_id);
		}
	}
}

//接続線の削除
void StateMachineGraphEditor::DeleteLink(GraphData* current_graph)
{
	ed::LinkId delete_link_id; // 対象リンクID
	while (ed::QueryDeletedLink(&delete_link_id))
	{
		if (ed::AcceptDeletedItem())
		{
			uint32_t target_id = static_cast<uint32_t>(delete_link_id.Get()); // キャストID
			data_manager->DeleteLink(current_graph_id, target_id);
		}
	}
}

//遷移条件を構築
void StateMachineGraphEditor::OnLinkCreated(GraphData* current_graph, const GraphLink& new_link)
{
	if (!current_graph)
	{
		return;
	}

	uint32_t source_node_id = 0;	//遷移元のID
	uint32_t target_node_id = 0;	//遷移先のID

	for (size_t i = 0; i < current_graph->nodes.size(); i++)
	{
		const GraphNode& node = current_graph->nodes[i];	//検索対象のノード

		for (size_t p = 0; p < node.outputs.size(); p++)
		{
			if (node.outputs[p].id == new_link.start_pin_id)
			{
				source_node_id = node.id;
				break;
			}
		}

		for (size_t p = 0; p < node.inputs.size(); p++)
		{
			if (node.inputs[p].id == new_link.end_pin_id)
			{
				target_node_id = node.id;
				break;
			}
		}
	}

	if (source_node_id == 0 || target_node_id == 0)
	{
		printf("Error: OnLinkCreated - 接続されたピンに対応するノードが見つかりませんでした。\n");
		return;
	}

	//printf("StateMachineGraphEditor: 遷移関係を構築しました。[ステートID:%d] ==(遷移)==> [ステートID:%d]\n",
	//	source_node_id, target_node_id);
}

//最後に使用したファイルパスを設定ファイルへ保存
void StateMachineGraphEditor::SaveEditorCondig()
{
	nlohmann::json config_json;
	config_json["LastOpenedFilePath"] = current_loaded_file_path;
	const std::string config_file_path = "Data/Json/StateEditorConfig.json";
	std::ofstream file_out(config_file_path);
	if (file_out.is_open())
	{
		const int indent_space_size = 4;
		file_out << std::setw(indent_space_size) << config_json << std::endl;
		//printf("StateMachineGraphEditor: 環境設定ファイルへ最後に開いたパスを記憶しました。\n");
	}
	else
	{
		printf("Error: SaveEditorConfig - 環境設定ファイル「%s」を開けませんでした。\n", config_file_path.c_str());
	}
}

//設定ファイルから最後に使用したファイルパスを読み込む
void StateMachineGraphEditor::LoadEditorCondig()
{
	const std::string config_file_path = "Data/Json/StateEditorConfig.json";
	std::ifstream file_in(config_file_path);

	if (!file_in.is_open())
	{
		printf("StateMachineGraphEditor: 環境設定ファイルがないため、初回デフォルト設定で起動します。\n");
		current_loaded_file_path = "";
		return;
	}
	nlohmann::json config_json;
	file_in >> config_json;

	if (config_json.find("LastOpenedFilePath") != config_json.end())
	{
		current_loaded_file_path = config_json["LastOpenedFilePath"].get<std::string>();
		printf("StateMachineGraphEditor: 前回の終了ファイルパス「%s」を自動検出しました。\n", current_loaded_file_path.c_str());
	}
	else
	{
		printf("Warning: LoadEditorConfig - 設定ファイルのキー構造が不正です。パスを初期化します。\n");
		current_loaded_file_path = "";
	}
}

//アニメーションマップを構築して送信
void StateMachineGraphEditor::TriggerHotReload()
{
	if (!current_loaded_file_path.empty())
	{
		data_manager->SaveToFile(current_loaded_file_path);
		EditorMediator::Instance().NotifyGraphChanged(current_loaded_file_path);
	}
}

//カスタムデリータ
void StateMachineGraphEditor::EditorContexDeleter::operator()(ax::NodeEditor::EditorContext* context) const noexcept
{
	if (context)
	{
		ed::DestroyEditor(context);
	}
}