#define IMGUI_DEFINE_MATH_OPERATORS

#include "StateMachineGraphEditor.h"
#include "Gameplay\StateMachine\StateBlackboard.h"
#include "Editor\StateMachineEditor\StateGraphDataManager.h"
#include "Gameplay/GameObjects/ObjectManager.h"
#include "Editor/FileDialogHelper.h"
#include "Editor/EditorMediator.h"
#include "StateGraphPaletteWindow.h"
#include "StateGraphPropertyWindow.h"
#include "StateGraphConfigManager.h"
#include "StateNodeRenderer.h"
#include "StateLinkRenderer.h"
#include "StateBlackboardInspectorWindow.h"
#include "StateGraphNavigator.h"
#include "StateCanvasInteractionHandler.h"
#include "StateLinkConnectionHandler.h"
#include "StateGraphCameraController.h"
#include "StateGraphToolbar.h"
#include "Gameplay\Components\Editor\StateMachineComponent.h"
#include "Editor\AssetLoader.h"
#include "Editor\PathHelper.h"

#include <imgui_node_editor_internal.h>
#include <cassert>
#include <fstream>
#include <iomanip>

namespace ed = ax::NodeEditor;

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
	state_machine_component = std::make_unique<StateMachineComponent>();
	editor_dummy_blackboard = std::make_unique<StateBlackboard>();
	blackboard_inspector = std::make_unique<StateBlackboardInspectorWindow>();
	state_node_renderer = std::make_unique<StateNodeRenderer>();
	state_link_renderer = std::make_unique<StateLinkRenderer>();
	state_graph_navigator = std::make_unique<StateGraphNavigator>();
	canvas_interaction_handler = std::make_unique<StateCanvasInteractionHandler>();
	link_connection_handler = std::make_unique<StateLinkConnectionHandler>();
	camera_controller = std::make_unique<StateGraphCameraController>();
	toolbar = std::make_unique<StateGraphToolbar>();

	target_model_hash = 0;

	config_manager->LoadEditorConfig();
	current_loaded_file_path = config_manager->GetCurrentLoadedFilePath();

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
		std::string target_path = data_manager->GetTargetModelPath();
		// 既存の関数でモデルとアニメーションリストをロード
		if (asset_loader->LoadModelAnimations(target_path))
		{
			std::filesystem::path path_obj(data_manager->GetTargetModelPath());
			std::string model_name = path_obj.stem().string(); //拡張子を除いたファイル名を抽出

			target_model_hash = StateBlackboard::CalculateHash(model_name);
			EditorMediator::Instance().OnModelDubleClied(target_path);
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

	StateBlackboard* active_blackboard = blackboard ? blackboard : editor_dummy_blackboard.get();

	GraphData* current_graph = nullptr; // 現在の階層情報

	for (size_t i = 0; i < data_manager->GetLayerDatas().size(); i++)
	{
		if (data_manager->GetLayerDatas()[i].id == current_graph_id)
		{
			current_graph = &data_manager->GetLayerDatas()[i];
			break;
		}
	}

	uint32_t& current_active_node_id = graph_active_nodes[current_graph_id]; // 階層固有のアクティブID

	// ゲーム側の実行ノードIDが前フレームから変化した瞬間を直接検知
	if (runtime_active_node_id != UINT32_MAX)
	{
		flow_dst_node_id = runtime_active_node_id;
		constexpr float default_flow_duration = 0.35f;
		flow_effect_timer = default_flow_duration;
		has_flow_requsted = true;

		// リアルタイム追尾機能が有効であるかを判定
		if (is_tracking_active_node && camera_controller)
		{
			// カメラコントローラーへフォーカス要求を発行
			camera_controller->RequestFocusNode(runtime_active_node_id);
		}
	}

	// 擬似シミュレーションがONになっている場合、条件評価を行ってアクティブノードを自動更新
	if (is_simulation_active)
	{
		if (blackboard_inspector)blackboard_inspector->SyncBlackboardVariablesFromGraph(current_graph, active_blackboard);
		UpdateSimulationMode(active_blackboard, current_graph, current_active_node_id);
	}

	SyncActiveNodeAnimation(current_graph, current_active_node_id);

	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);

	if (toolbar)
	{
		ToolbarContext toolbar_context = {
			data_manager.get(),
			config_manager.get(),
			asset_loader.get(),
			state_machine_component.get(),
			active_blackboard,
			state_graph_navigator.get(),
			current_loaded_file_path,
			current_graph_id,
			target_model_hash,
			is_tracking_active_node,
			is_simulation_active,
			last_synced_node_id
		};

		if (toolbar->DrawToolbar(toolbar_context)) 
		{
			return;
		}
	}
	else
	{
		printf("Error: DrawEditor - state_graph_toolbar が nullptr です。\n");
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

	DrawRightSidebar(current_graph, active_blackboard, dynamic_right_width, canvas_height);

	blackboard_inspector->DrawInspector(active_blackboard);
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
		if (state_machine_component)state_machine_component->SetStateMachinePath(current_loaded_file_path);
		
		config_manager->SetCurrentLoadedFilePath(current_loaded_file_path);
		config_manager->SaveEditorConfig(current_loaded_file_path);

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
		last_synced_node_id = UINT32_MAX;
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

//擬似シミュレーション更新
void StateMachineGraphEditor::UpdateSimulationMode(StateBlackboard* blackboard, GraphData* current_graph, uint32_t& current_active_node_id)
{
	//意図しない挙動の防止：ポインタの健全性チェック
	if (!state_machine_component || !current_graph)
	{
		printf("Error: StateMachineGraphEditor::UpdateSimulationMode - state_machine_component または current_graph が nullptr です。\n");
		return;
	}

	if (!blackboard)
	{
		printf("Error: StateMachineGraphEditor::UpdateSimulationMode - 有効な StateBlackboard が割り当てられていません。\n");
		return;
	}

	float delta_time = ImGui::GetIO().DeltaTime;
	uint32_t prev_node_id = state_machine_component->GetCurrentNodeId();

	state_machine_component->Update(delta_time, blackboard);

	uint32_t new_active_node_id = state_machine_component->GetCurrentNodeId();

	//ステート遷移が成立した場合
	if (new_active_node_id != UINT32_MAX && new_active_node_id != prev_node_id)
	{
		flow_src_node_id = prev_node_id;
		flow_dst_node_id = new_active_node_id;
		constexpr float default_flow_duration = 0.35f;
		flow_effect_timer = default_flow_duration;
		has_flow_requsted = true;
	}

	// 現在の確定アクティブノードIDを反映
	if (new_active_node_id != UINT32_MAX)
	{
		current_active_node_id = new_active_node_id;

		// 追尾モードが有効な場合、アクティブノードが所属する階層へ表示を自動切り替え
		if (is_tracking_active_node)
		{
			uint32_t target_graph_id = data_manager->GetGraphIdFromNodeId(new_active_node_id);
			if (target_graph_id != UINT32_MAX && target_graph_id != current_graph_id)
			{
				current_graph_id = target_graph_id;
				printf("StateMachineGraphEditor: サブステート追尾により表示階層を ID:%d へ自動切り替えしました。\n", current_graph_id);
			}
		}
	}
}

//アクティブノードのアニメーション同期
void StateMachineGraphEditor::SyncActiveNodeAnimation(GraphData* current_graph, uint32_t active_node_id)
{
	if (!current_graph || active_node_id == 0 || active_node_id == UINT32_MAX) return;

	// シミュレーション実行中は StateMachineComponent から直接再生中のアニメーションを取得する
	if (is_simulation_active && state_machine_component)
	{
		uint32_t current_sim_node_id = state_machine_component->GetCurrentNodeId();

		// 同一ノードでの連続再生命令を防ぐガード節
		if (current_sim_node_id == last_synced_node_id) return;

		last_synced_node_id = current_sim_node_id;

		std::string anim_name = state_machine_component->GetCurrentAnimationName();
		bool is_loop = state_machine_component->GetAnimationLoop();

		if (!anim_name.empty())
		{
			EditorMediator::Instance().PlayModelAnimation(anim_name, is_loop);
		}
		else
		{
			printf("Warning: StateMachineGraphEditor::SyncActiveNodeAnimation - ノード ID:%d のアニメーション名が空です。\n", current_sim_node_id);
		}
		return;
	}

	// 手動選択時（シミュレーション非実行時）の同期処理
	if (active_node_id == last_synced_node_id) return;

	const GraphNode* target_node = nullptr;
	for (size_t i = 0; i < current_graph->nodes.size(); i++)
	{
		if (current_graph->nodes[i].id == active_node_id)
		{
			target_node = &current_graph->nodes[i];
			break;
		}
	}

	if (!target_node) return;

	last_synced_node_id = active_node_id;

	if (!target_node->animation_name.empty())
	{
		EditorMediator::Instance().PlayModelAnimation(target_node->animation_name, target_node->is_loop);
	}
}

//左パレットとノードリスト
void StateMachineGraphEditor::DrawLeftSidebar(GraphData* current_graph, float width, float height)
{
	ImGui::BeginChild("LeftSidebarZone##Child", ImVec2(width, height), true);
	uint32_t focus_node_id = 0; //受け取り用のフォーカスID
	palette_window->DrawPalette(data_manager.get(), current_graph, focus_node_id);
	if (focus_node_id != 0 && camera_controller)
	{
		camera_controller->RequestFocusNode(focus_node_id);
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
		const GraphNode& node = current_graph->nodes[i];
		bool is_active_now = (node.id == graph_active_nodes[current_graph_id]);

		if (state_node_renderer)
		{
			state_node_renderer->DrawNode(node, is_active_now);
		}
		else
		{
			printf("Error: DrawCenterCanvas - node_renderer が nullptr です。\n");
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
		bool is_active_now = (node.id == graph_active_nodes[current_graph_id]);

		if (state_node_renderer)
		{
			state_node_renderer->DrawNode(node, is_active_now);
		}
		else
		{
			printf("Error: DrawCenterCanvas - state_node_renderer が nullptr です。\n");
		}
	}

	if (state_link_renderer)
	{
		state_link_renderer->DrawLinks(
			data_manager.get(),
			current_graph,
			flow_src_node_id,
			flow_dst_node_id,
			flow_effect_timer);
	}
	else
	{
		printf("Error: DrawCenterCanvas - state_link_renderer が nullptr です。\n");
	}

	if (link_connection_handler)
	{
		link_connection_handler->HandleLinkCreation(data_manager.get(), current_graph, current_graph_id);
	}
	else
	{
		printf("Error: DrawCenterCanvas - link_connection_handler が nullptr です。\n"); // 日本語エラーログ出力
	}
	//-------------------------------------------------------------
	//ユーザー操作処理をハンドラーへ委譲
	//-------------------------------------------------------------
	if (canvas_interaction_handler)
	{
		canvas_interaction_handler->HandleContextMenu(data_manager.get(), current_graph, current_graph_id);
		canvas_interaction_handler->HandlePendingPaletteNode(data_manager.get(), palette_window.get(), current_graph, current_graph_id);
		canvas_interaction_handler->HandleDeletion(data_manager.get(), current_graph, current_graph_id);
	}
	else
	{
		printf("Error: DrawCenterCanvas - canvas_interaction_handler が nullptr です。\n"); // 日本語エラーログ出力
	}

	if (state_graph_navigator)
	{
		state_graph_navigator->CheckNavigateToSubGraph(current_graph, current_graph_id);
	}
	else
	{
		printf("Error: DrawCenterCanvas - state_graph_navigator が nullptr です。\n");
	}

	ed::End();

	//----------------------------------
	//ドラッグ＆ドロップ受け取り処理
	//----------------------------------
	if (canvas_interaction_handler)
	{
		canvas_interaction_handler->HandleDragAndDrop(data_manager.get(), current_graph, current_graph_id);
	}

	//------------------------------
	//カメラフォーカスの更新処理
	//------------------------------
	if (camera_controller)
	{
		camera_controller->UpdateCameraFocus();
	}
	else
	{
		printf("Error: DrawCenterCanvas - camera_controller が nullptr です。\n"); 
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