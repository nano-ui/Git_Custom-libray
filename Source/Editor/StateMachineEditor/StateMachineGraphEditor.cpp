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
#include "Gameplay\Components\Editor\StateMachineComponent.h"
#include "Editor\AssetLoader.h"
#include "Editor\PathHelper.h"

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
	state_machine_component = std::make_unique<StateMachineComponent>();
	editor_dummy_blackboard = std::make_unique<StateBlackboard>();
	blackboard_inspector = std::make_unique<StateBlackboardInspectorWindow>();
	state_node_renderer = std::make_unique<StateNodeRenderer>();
	state_link_renderer = std::make_unique<StateLinkRenderer>();
	state_graph_navigator = std::make_unique<StateGraphNavigator>();
	canvas_interaction_handler = std::make_unique<StateCanvasInteractionHandler>();

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
	if (runtime_active_node_id != UINT32_MAX && previous_active_node_id != 0 && previous_active_node_id != runtime_active_node_id)
	{
		flow_src_node_id = previous_active_node_id;
		flow_dst_node_id = runtime_active_node_id;
		constexpr float default_flow_duration = 0.35f; // 強調表示時間（秒）
		flow_effect_timer = default_flow_duration;     // 実機遷移時にもタイマーをセット
		has_flow_requsted = true;

		// デバッグ出力で遷移検知とノードIDを確認
		printf("StateMachineGraphEditor: 実機遷移を検知しました。ノードID: %u -> %u (タイマー: %.2f秒)\n",
			flow_src_node_id, flow_dst_node_id, flow_effect_timer);

		// リアルタイム追尾機能が有効であるかを判定
		if (is_tracking_active_node)
		{
			g_pending_focus_node_id = runtime_active_node_id;
		}
	}

	// 擬似シミュレーションがONになっている場合、条件評価を行ってアクティブノードを自動更新
	if (is_simulation_active)
	{
		if (blackboard_inspector)blackboard_inspector->SyncBlackboardVariablesFromGraph(current_graph, active_blackboard);
		UpdateSimulationMode(active_blackboard, current_graph, current_active_node_id);
	}

	SyncActiveNodeAnimation(current_graph, current_active_node_id);

	//// ゲーム側の実行ノードIDが前フレームから変化した瞬間を直接検知
	//if (runtime_active_node_id != UINT32_MAX && previous_active_node_id != 0 && previous_active_node_id != runtime_active_node_id)
	//{
	//	flow_src_node_id = previous_active_node_id;
	//	flow_dst_node_id = runtime_active_node_id;
	//	has_flow_requsted = true;
	//	//printf("StateMachineGraphEditor: 純粋なステート遷移を検知しました。ノードID: %d -> %d\n", flow_src_node_id, flow_dst_node_id);

	//	// リアルタイム追尾機能が有効であるかを判定
	//	if (is_tracking_active_node)
	//	{
	//		g_pending_focus_node_id = runtime_active_node_id;
	//	}
	//}

	// 有効な実行中IDが届いている場合のみ、次フレーム用の比較元として保存
	if (runtime_active_node_id != UINT32_MAX)
	{
		previous_active_node_id = runtime_active_node_id;
	}

	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);

	if (DrawTopMenuBar(active_blackboard))
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

//上部メニューとナビゲーション
bool StateMachineGraphEditor::DrawTopMenuBar(StateBlackboard* blackboard)
{
	ImGui::Begin(u8"ステートマシンエディタ");

	const ImVec4 save_btn_color = ImVec4(0.2f, 0.5f, 0.2f, 1.0f);
	ImGui::PushStyleColor(ImGuiCol_Button, save_btn_color);

	const float upper_btn_width = 200.0f;
	const float upper_btn_height = 25.0f;

	if (ImGui::Button(u8"保存", ImVec2(upper_btn_width, upper_btn_height)))
	{
		std::string save_target_path = current_loaded_file_path; // 保存先パス

		//現在の保存先パスが空で、紐付けモデルが存在する場合に自動パスを構築
		if (save_target_path.empty() && !data_manager->GetTargetModelPath().empty())
		{
			std::filesystem::path path_obj(data_manager->GetTargetModelPath());
			std::string model_name = path_obj.stem().string(); //拡張子なしのモデル名
			const std::string suffix_name = "_StateMachine";    //接尾辞定数

			save_target_path = PathHelper::GenerateJsonFilePath(model_name, suffix_name);
		}

		//それでもパスが決まらない場合（モデル未設定時）はファイルダイアログを表示
		if (save_target_path.empty())
		{
			save_target_path = FileDialogHelper::SaveFileDialog();
		}

		//有効な保存先パスが確定したか判定
		if (!save_target_path.empty())
		{
			data_manager->SaveToFile(save_target_path);
			current_loaded_file_path = save_target_path;
			SaveEditorCondig();
			printf("StateMachineGraphEditor: ファイル「%s」へ保存を完了しました。\n", current_loaded_file_path.c_str());
		}
		else
		{
			// 意図しない挙動（保存先パス未指定）が発生した場合のデバッグ出力
			printf("Error: StateMachineGraphEditor - 保存先のパスが指定されなかったため保存をキャンセルしました。\n");
		}
	}
	ImGui::PopStyleColor();

	ImGui::SameLine();

	const ImVec4 load_btn_color = ImVec4(0.2f, 0.4f, 0.6f, 1.0f);
	ImGui::PushStyleColor(ImGuiCol_Button, load_btn_color);

	if (ImGui::Button(u8"モデル読み込み", ImVec2(upper_btn_width, upper_btn_height)))
	{
		PathResult path_result = FileDialogHelper::OpenGenericFileDialog(); 
		if (!path_result.absolute_path.empty())
		{
			if (asset_loader->LoadModelAnimations(path_result.relative_path))
			{
				data_manager->SetTargetModelPath(path_result.relative_path); 
					std::filesystem::path path_obj(path_result.relative_path); 
					std::string model_name = path_obj.stem().string();

				//モデル名と接尾辞から対応するJSONパスを自動構築
				const std::string suffix_name = "_StateMachine"; // ファイル接尾辞の定数化
				std::string auto_json_path = PathHelper::GenerateJsonFilePath(model_name, suffix_name); 

					//生成されたパスが有効か検証
					if (!auto_json_path.empty())
					{
						current_loaded_file_path = auto_json_path;
						data_manager->LoadFromFile(current_loaded_file_path);
						printf("StateMachineGraphEditor: モデル「%s」のJsonパス「%s」を自動構築して読み込みました。\n",
								model_name.c_str(), current_loaded_file_path.c_str());
					}
					else
					{
						printf("Error: StateMachineGraphEditor - PathHelperでのパス生成に失敗しました。\n");
					}

				target_model_hash = StateBlackboard::CalculateHash(model_name); 
					EditorMediator::Instance().OnModelDubleClied(path_result.relative_path); 
					printf("StateMachineGraphEditor: モデル読み込み完了: %s\n", path_result.relative_path.c_str()); 
					TriggerHotReload(); 
			}
		}
	}
	ImGui::PopStyleColor();
	ImGui::SameLine();

	//モデル読み込みの成否を判定
	if (!asset_loader->GetLoadedModelPath().empty())
	{
		ImGui::SameLine();
		ImGui::Text(u8" 紐付けモデル: %s", asset_loader->GetLoadedModelPath().c_str());
	}

	ImGui::SameLine();

	//追尾設定チェックボックスが変更されたかを判定
	if (ImGui::Checkbox(u8"追尾", &is_tracking_active_node))
	{
		printf("StateMachineGraphEditor: 追尾モードが %s に切り替わりました。\n", is_tracking_active_node ? "ON" : "OFF");
	}

	ImGui::SameLine();

	if (ImGui::Checkbox(u8"シミュレーション", &is_simulation_active))
	{
		last_synced_node_id = UINT32_MAX;
		// シミュレーション開始時に最新のグラフ構造を保存・リロードして初期化
		if (is_simulation_active && state_machine_component)
		{
			if (!current_loaded_file_path.empty())
			{
				data_manager->SaveToFile(current_loaded_file_path);
				state_machine_component->SetStateMachinePath(current_loaded_file_path);
			}
			state_machine_component->RequestReload();
			state_machine_component->Initialize(blackboard);
		}
	}

	ImGui::Spacing();

	if (state_graph_navigator)
	{
		if (state_graph_navigator->DrawHeaderNavigation(data_manager.get(), current_graph_id))
		{
			ImGui::End();
			return true;
		}
	}
	else
	{
		printf("Error: DrawTopMenuBar - state_graph_navigator が nullptr です。\n");
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

	if (ed::BeginCreate())
	{
		CreateNewLink(current_graph);
	}
	ed::EndCreate();

	//-------------------------------------------------------------
	//ユーザー操作処理をハンドラーへ委譲
	//-------------------------------------------------------------
	if (canvas_interaction_handler)
	{
		// 右クリックコンテキストメニュー（ステート追加、サブグラフ追加・変換）の処理
		canvas_interaction_handler->HandleContextMenu(data_manager.get(), current_graph, current_graph_id);

		// パレットウィンドウ側で保留されている追加ノードの配置処理
		canvas_interaction_handler->HandlePendingPaletteNode(data_manager.get(), palette_window.get(), current_graph, current_graph_id);

		// ノードおよびリンクの削除クエリ処理
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

	//-------------------------------------------------------------
	//ドラッグ＆ドロップ受け取り処理をハンドラーへ委譲
	//-------------------------------------------------------------
	if (canvas_interaction_handler)
	{
		canvas_interaction_handler->HandleDragAndDrop(data_manager.get(), current_graph, current_graph_id);
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