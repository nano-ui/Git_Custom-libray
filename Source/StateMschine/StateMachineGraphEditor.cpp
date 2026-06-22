#define IMGUI_DEFINE_MATH_OPERATORS

#include "StateMachineGraphEditor.h"
#include "../StateMschine/StateBlackboard.h"
#include "../StateMschine/StateGraphDataManager.h"
#include "../StateMschine/TransitionConditionEditor.h"

#include <imgui_node_editor_internal.h>
#include <cassert>

namespace ed = ax::NodeEditor;

static uint32_t g_pending_focus_node_id = 0;

//コンストラクタ
StateMachineGraphEditor::StateMachineGraphEditor()
{
	//ライブラリ初期化
	ed::Config config; // 設定データ
	config.SettingsFile = "Data/Json/NodeEditor_State.json";
	editor_context.reset(ed::CreateEditor(&config));

	//初期グラフ生成
	const uint32_t root_id = 0; // ルート階層の固定ID
	current_graph_id = root_id;

	data_manager = std::make_unique<StateGraphDataManager>();
	conditon_editor = std::make_unique<TransitionConditionEditor>();

	pending_add_palette_node_name = "";
	pending_add_is_sub_graph = false;
}

//デストラクタ
StateMachineGraphEditor::~StateMachineGraphEditor() = default;

//エディタ描画
void StateMachineGraphEditor::DrawEditor(StateBlackboard* blackboard)
{
	// アクティブグラフ検索
	GraphData* current_graph = nullptr;	// 現在の階層情報

	// 全ての階層データから現在のグラフIDに一致するものを探す
	for (size_t i = 0; i < data_manager->GetLayerDatas().size(); i++)
	{
		if (data_manager->GetLayerDatas()[i].id == current_graph_id)
		{
			current_graph = &data_manager->GetLayerDatas()[i];
			break;
		}
	}

	// ループの外側で未発見チェック
	if (!current_graph)
	{
		assert(false && "StateMachineGraphEditor: 指定されたグラフIDが見つかりません。");
		return;
	}

	//ブラックボードとリンクの条件式から現在アクティブなノードを完全自動特定
	if (current_active_node_id == 0 && !current_graph->nodes.empty())
	{
		current_active_node_id = current_graph->nodes.front().id;
	}

	if (blackboard)
	{
		// 階層内のすべてのリンクを走査
		for (size_t i = 0; i < current_graph->links.size(); i++)
		{
			const GraphLink& link = current_graph->links[i]; // 精査対象のリンク

			// リンクの開始ピンが、現在アクティブなノードの出力ピンであるか逆引き判定
			uint32_t src_node_id = data_manager->GetNodeIdFromPinId(current_graph_id, link.start_pin_id); // リンク元のノードID

			if (src_node_id == current_active_node_id)
			{
				bool is_all_conditions_met = !link.conditions.empty(); // 条件が1つ以上あり、すべて満たされているかフラグ

				// リンク内に登録されているすべての条件式（Health <= 0 など）をループ評価
				for (size_t c = 0; c < link.conditions.size(); c++)
				{
					const GraphTransitionCondition& graph_cond = link.conditions[c]; // 編集データ側の条件

					// 実行時判定用の構造体へ安全にデータをテンポラリ変換マッピング
					TransitionCondition runtime_cond; // 実行時判定用の構造体インスタンス
					runtime_cond.type = graph_cond.type; // 判定タイプのコピー
					runtime_cond.hash_key = graph_cond.hash_key; // 変数ハッシュのコピー
					runtime_cond.reference_value = graph_cond.reference_value; // 基準値のコピー
					runtime_cond.compart_op = static_cast<CompareOperator>(graph_cond.compare_operator); // 演算子のキャストコピー
					runtime_cond.param_second = graph_cond.param_second; // 第2引数パラメータのコピー
					runtime_cond.secondary_hash = graph_cond.secondary_hash; // 対象ハッシュのコピー

					// ブラックボードの実数値を元に、条件が満たされているか自律判定
					if (!runtime_cond.IsJudgment(*blackboard))
					{
						is_all_conditions_met = false; // 1つでも満たしていなければ不成立
						break;
					}
				}

				// すべての遷移条件が完全に満たされた場合、自動的にアクティブノードを切り替えます！
				if (is_all_conditions_met)
				{
					uint32_t dst_node_id = data_manager->GetNodeIdFromPinId(current_graph_id, link.end_pin_id); // 遷移先のノードID

					previous_active_node_id = current_active_node_id; // 前フレームのIDを退避
					current_active_node_id = dst_node_id;             // 新しいアクティブノードIDを確定登録

					auto_flowing_link_id = link.id;     // 流光エフェクトを走らせる対象のリンクIDをロック
					const float flow_duration = 0.5f;    // アニメーション表示時間（0.5秒） 
					auto_flow_timer = flow_duration;     // 残り時間タイマーをリセット
					break;
				}
			}
		}
	}

	// アニメーションタイマーが作動している場合は毎フレーム自動で減算 
	if (auto_flow_timer > 0.0f)
	{
		auto_flow_timer -= ImGui::GetIO().DeltaTime; // デルタタイム分引き算
	}

	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);

	bool trigger_add_node = false;			// ノード追加の実行トリガー
	bool trigger_add_subgraph = false;		// サブグラフ追加の実行トリガー
	bool trigger_convert_subgraph = false;	// サブグラフ変換の実行トリガー

	// 画面描画
	ImGui::Begin(u8"ステートマシンエディタ");

	// 階層ナビゲーションを描画
	if (DrawHeaderNavigation())
	{
		ImGui::End();
		return;
	}

	static float dynamic_left_width = 290.0f;	// マウスで変更可能な左サイドバー横幅
	static float dynamic_right_width = 300.0f;	// マウスで変更可能な右サイドバー横幅

	const float min_pane_width = 100.0f;		// 各ペインの最小横幅制限 
	const float min_pane_height = 100.0f;		// 各ペインの最小縦幅制限 
	const float separator_line_width = 6.0f;	// セパレーターの見かけの掴み幅 

	float total_available_width = ImGui::GetContentRegionAvail().x;	// ウィンドウ全体の有効横幅

	float canvas_width = total_available_width - dynamic_left_width - dynamic_right_width - (separator_line_width * 2.0f);	// キャンバスの動的横幅

	// ウィンドウが小さくなりすぎた場合に中央が潰れないよう安全ガード
	if (canvas_width < min_pane_width)
	{
		canvas_width = min_pane_width;
	}

	float canvas_height = ImGui::GetContentRegionAvail().y;	// ウィンドウ全体の有効縦幅
	if (canvas_height < min_pane_height)
	{
		canvas_height = min_pane_height;
	}

	// 左ペイン・ステート一覧の描画
	ImGui::BeginChild("LeftSidebarZone##Child", ImVec2(dynamic_left_width, canvas_height), true);
	DrawStateListWindow(current_graph);
	ImGui::EndChild();

	ImGui::SameLine();

	// 見えない透明なボタンを配置して、マウスでドラッグ可能にする
	ImGui::Button("##LeftSplitter", ImVec2(separator_line_width, canvas_height));
	if (ImGui::IsItemActive())
	{
		dynamic_left_width += ImGui::GetIO().MouseDelta.x;
		if (dynamic_left_width < min_pane_width) dynamic_left_width = min_pane_width;
	}

	ImGui::SameLine();

	// 中央ペイン・ノードキャンバスの描画
	ImGui::BeginChild("CenterCanvasZone##Child", ImVec2(canvas_width, canvas_height), false);

	ed::SetCurrentEditor(editor_context.get());
	ed::Begin("Node Canvas");

	// ファーストフレームの初期化
	if (current_graph_id == 0 && current_graph->nodes.empty())
	{
		data_manager->CheckAndInitDefaultNode(current_graph_id);

		uint32_t default_node_id = current_graph->nodes.front().id; // 待機ノードID
		ed::SetNodePosition(default_node_id, ImVec2(100.0f, 100.0f));
	}

	// 階層内のすべてのノードを描画
	for (size_t i = 0; i < current_graph->nodes.size(); i++)
	{
		const GraphNode& node = current_graph->nodes[i]; // 現在描画対象となっているノードデータの参照

		int pushed_style_count = 0;	//復元のためにこのノードでPushしたスタイルカラーの総数をカウント

		bool is_active_now = (node.id == current_active_node_id); // アクティブ状態フラグ
		if (is_active_now)
		{
			const ImVec4 gold_glow_color = ImVec4(0.0f, 1.0f, 0.3f, 1.0f); // ライムグリーン
			ed::PushStyleColor(ed::StyleColor_NodeBorder, gold_glow_color); // ノードの枠線を発光色に上書き
			pushed_style_count++; // プッシュ数をインクリメント
		}

		// サブグラフノード（下位階層持ち）であるか判定
		if (node.is_sub_graph)
		{
			const ImVec4 sub_bg_color = ImVec4(0.1f, 0.2f, 0.4f, 0.85f); // サブグラフ用の深みのある背景色
			const ImVec4 sub_sel_color = ImVec4(0.3f, 0.6f, 1.0f, 1.0f); // サブグラフ選択時の境界線カラー

			ed::PushStyleColor(ed::StyleColor_NodeBg, sub_bg_color); // ノードの背景色を変更
			ed::PushStyleColor(ed::StyleColor_SelNodeBorder, sub_sel_color); // 選択時の枠線色を変更
			pushed_style_count += 2; // 2つのスタイルが追加されたことを記録
		}
		ed::BeginNode(node.id); // ノードの描画スコープを開始

		ImGui::Text("%s", node.name.c_str()); // 上部にステート名を表示
		ImGui::Spacing(); // 縦方向に少し隙間を空ける

		// 入力ピン群を左側にまとめるためのグループ配置を開始
		ImGui::BeginGroup();
		for (size_t in_idx = 0; in_idx < node.inputs.size(); in_idx++)
		{
			const GraphPin& pin = node.inputs[in_idx]; // 入力ピンデータの参照
			ed::BeginPin(pin.id, ed::PinKind::Input); // ピンの磁着範囲スコープ開始
			ImGui::Text("->%s", pin.name.c_str()); // 入力ピン名を描画
			ed::EndPin(); // ピンのスコープ終了
		}
		ImGui::EndGroup(); // 入力グループを終了

		ImGui::SameLine(); // 横並びにするため改行を抑制
		const float middle_spacer_width = 40.0f; // 入出力ピンの間に挟む空間の横幅定数
		ImGui::Dummy(ImVec2(middle_spacer_width, 0.0f)); // 中央に固定の余白を生成
		ImGui::SameLine(); // 横並びを継続

		// 出力ピン群を右側にまとめるためのグループ配置を開始
		ImGui::BeginGroup();
		for (size_t out_idx = 0; out_idx < node.outputs.size(); out_idx++)
		{
			const GraphPin& pin = node.outputs[out_idx]; // 出力ピンデータの参照
			ed::BeginPin(pin.id, ed::PinKind::Output); // ピンの磁着範囲スコープ開始
			ImGui::Text("%s ->", pin.name.c_str()); // 出力ピン名を描画
			ed::EndPin(); // ピンのスコープ終了
		}
		ImGui::EndGroup(); // 出力グループを終了

		ed::EndNode(); // 統合されたノードの描画スコープを終了
		// ループ内でプッシュされた数だけ正確にポップを回してスタックのズレを完全に防ぐ
		for (int color_idx = 0; color_idx < pushed_style_count; color_idx++)
		{
			ed::PopStyleColor(); // スタイルカラーを1つポップして復元
		}
	}

	// 接続線の描画
	for (size_t i = 0; i < current_graph->links.size(); i++)
	{
		const GraphLink& link = current_graph->links[i]; // リンク参照
		ed::Link(link.id, link.start_pin_id, link.end_pin_id);
		if (link.id == auto_flowing_link_id && auto_flow_timer > 0.0f)
		{
			ed::Flow(link.id);
		}
	}

	// 接続線の作成判定
	if (ed::BeginCreate())
	{
		CreateNewLink(current_graph);
	}
	ed::EndCreate();

	// サスペンドする前に、右クリック用の静的変数を確実に定義しておく
	static ImVec2 popup_click_pos = ImVec2(0.0f, 0.0f);	// クリック位置
	static ed::NodeId context_node_id = 0;				// ポップアップを呼んだノードのID

	ed::Suspend();	// エディタの描画を一時停止

	// 背景がクリックされたか
	if (ed::ShowBackgroundContextMenu())
	{
		ImGui::OpenPopup("Create New Node Context Menu");
		popup_click_pos = ed::ScreenToCanvas(ImGui::GetMousePos());
	}

	// 選択されたノードの上で右クリックされたか判定
	if (ed::ShowNodeContextMenu(&context_node_id))
	{
		ed::SelectNode(context_node_id, true);
		ImGui::OpenPopup("Node Context Menu");
	}

	// 背景右クリックポップアップの描画
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

	// ノード右クリックポップアップの描画
	if (ImGui::BeginPopup("Node Context Menu"))
	{
		if (ImGui::MenuItem(u8"サブグラフへ変換"))
		{
			trigger_convert_subgraph = true;
		}
		ImGui::EndPopup();
	}

	ed::Resume(); // エディタの描画を再開

	//パレットから予約されたノードの追加
	if (!pending_add_palette_node_name.empty())
	{
		//画面の中心からキャンバスの座標を逆算してポップ位置を設定
		ImVec2 center_pos = ImGui::GetMainViewport()->GetCenter();
		ImVec2 canvas_pos = ed::ScreenToCanvas(center_pos);

		if (pending_add_is_sub_graph)
		{
			data_manager->AddSubGrapNode(current_graph_id, canvas_pos.x, canvas_pos.y, pending_add_palette_node_name);
		
			for (size_t i = 0; i < data_manager->GetLayerDatas().size(); i++)
			{
				current_graph = &data_manager->GetLayerDatas()[i];
				break;
			}
		}
		else
		{
			data_manager->AddNode(current_graph, canvas_pos.x, canvas_pos.y, pending_add_palette_node_name);
		}

		uint32_t new_state_id = current_graph->nodes.back().id;
		ed::SetNodePosition(new_state_id, canvas_pos);

		pending_add_palette_node_name = "";
	}

	//安全な区間でのデータ操作実行 
	if (trigger_add_node)
	{
		data_manager->AddNode(current_graph, popup_click_pos.x, popup_click_pos.y);
		uint32_t new_state_id = current_graph->nodes.back().id; // 追加ノードID
		ed::SetNodePosition(new_state_id, popup_click_pos);
	}
	if (trigger_add_subgraph)
	{
		data_manager->AddSubGrapNode(current_graph_id, popup_click_pos.x, popup_click_pos.y);

		//current_graph ポインタを最新の正しいアドレスへ再取得
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

	// 削除操作の確定受付
	if (ed::BeginDelete())
	{
		DeleteNode(current_graph);
		DeleteLink(current_graph);
	}
	ed::EndDelete();

	// ダブルクリックの階層移動チェック
	CheckNavigateToSubGraph(current_graph);

	ed::End(); // キャンバス描画終了

	//キャンバス領域へのドラッグ＆ドロップ受け入れ
	if (ImGui::BeginDragDropTarget())
	{
		//通常ステートがドロップされた瞬間の判定
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_PAYLOAD_NORMAL"))
		{
			const char* dropped_node_name = static_cast<const char*>(payload->Data);	//ノード名
			ImVec2 drap_mouse_screen_pos = ImGui::GetMousePos();	//離した瞬間のマウス座標
			ImVec2 drop_mouse_canvas_pos = ed::ScreenToCanvas(drap_mouse_screen_pos);	//キャンバス座標

			data_manager->AddNode(current_graph, drop_mouse_canvas_pos.x, drop_mouse_canvas_pos.y, dropped_node_name);
		
			uint32_t new_node_id = current_graph->nodes.back().id;	//新しいノードID
			ed::SetNodePosition(new_node_id, drop_mouse_canvas_pos);

			printf("StateMachineGraphEditor: 通常ステート「%s」をドラッグ＆ドロップで配置しました。\n", dropped_node_name);
		}

		//サブグラフがドロップされた瞬間の判定
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_PAYLOAD_SUB"))
		{
			const char* dropped_sub_name = static_cast<const char*>(payload->Data);	//ノード名
			ImVec2 drap_mouse_screen_pos = ImGui::GetMousePos();	//離した瞬間のマウス座標
			ImVec2 drop_mouse_canvas_pos = ed::ScreenToCanvas(drap_mouse_screen_pos);	//キャンバス座標

			data_manager->AddSubGrapNode(current_graph_id, drop_mouse_canvas_pos.x, drop_mouse_canvas_pos.y, dropped_sub_name);

			//ポインタのアドレスを再取得
			for (size_t i = 0; i < data_manager->GetLayerDatas().size(); i++)
			{
				if (data_manager->GetLayerDatas()[i].id == current_graph_id)
				{
					current_graph = &data_manager->GetLayerDatas()[i];
					break;
				}
			}

			uint32_t new_node_id = current_graph->nodes.back().id;	//新しいノードID
			ed::SetNodePosition(new_node_id, drop_mouse_canvas_pos);

			printf("StateMachineGraphEditor: サブグラフ「%s」をドラッグ＆ドロップで完全複製配置しました。\n", dropped_sub_name);
		}
		ImGui::EndDragDropTarget();
	}

	if (g_pending_focus_node_id != 0)
	{
		uint32_t focus_target_id = g_pending_focus_node_id;	//対象IDのローカル退避
		ed::SelectNode(focus_target_id, false);
		ed::NavigateToSelection(false, 0.5f);

		g_pending_focus_node_id = 0;
		printf("StateMachineGraphEditor: 安全なタイミングでノード ID:%d へのカメラフォーカスを実行しました。\n", focus_target_id);
	}

	ImGui::EndChild();

	ImGui::SameLine();

	// 右側ドラッグセパレーター
	ImGui::Button("##RightSplitter", ImVec2(separator_line_width, canvas_height));
	if (ImGui::IsItemActive())
	{
		dynamic_right_width -= ImGui::GetIO().MouseDelta.x;
		if (dynamic_right_width < min_pane_width) dynamic_right_width = min_pane_width;
	}

	ImGui::SameLine();

	//右ペイン・詳細プロパティの描画
	ImGui::BeginChild("RightSidebarZone##Child", ImVec2(dynamic_right_width, canvas_height), true);
	DrawPropertyWindow(current_graph, blackboard);
	ImGui::EndChild();

	ed::SetCurrentEditor(nullptr);
	ImGui::End();
}

//サブグラフへの階層移動を検知・処理
void StateMachineGraphEditor::CheckNavigateToSubGraph(GraphData* current_graph)
{
	//グラフデータが渡されているか確認
	if (!current_graph)
	{
		printf("Error: StateMachineGraphEditor::CheckNavigateToSubGraph - current_graph が nullptr です。\n");
		return;
	}

	ed::NodeId double_clicked_node_id = ed::GetDoubleClickedNode();	//ダブルクリックされたノードID

	//ノードがダブルクリックされたか確認
	if (double_clicked_node_id)
	{
		uint32_t clicked_id = static_cast<uint32_t>(double_clicked_node_id.Get());	//ダブルクリックされたID

		//現在の階層にいる全ノードから、該当するノードを検索
		for (size_t i = 0; i < current_graph->nodes.size(); i++)
		{
			const GraphNode& node = current_graph->nodes[i];	//比較対象のノード

			//ダブルクリックされたノードIDを一致するか確認
			if (node.id == clicked_id)
			{
				//サブグラフの場合のみ中に入る
				if (node.is_sub_graph)
				{
					current_graph_id = node.sub_graph_id;
					printf("StateMachineGraphEditor: サブグラフ「%s」の内部に入ります。階層ID: %d へ切り替えました。\n",
						node.name.c_str(), current_graph_id);
				}
				break;
			}
		}
	}
}

//階層ナビゲーションを描画
bool StateMachineGraphEditor::DrawHeaderNavigation()
{
	//ナビゲーションバーの背景色の描画
	ImVec2 window_pos = ImGui::GetWindowPos(); // 描画位置
	float title_bar_height = ImGui::GetFrameHeight(); // タイトルバーの高さ
	ImVec2 bar_pos = ImVec2(window_pos.x, window_pos.y + title_bar_height); // バーの座標
	ImVec2 bar_size = ImVec2(ImGui::GetWindowWidth(), 35.0f); // バーのサイズ

	ImDrawList* draw_list = ImGui::GetWindowDrawList(); // 描画リスト取得
	draw_list->AddRectFilled(bar_pos, ImVec2(bar_pos.x + bar_size.x, bar_pos.y + bar_size.y), IM_COL32(35, 35, 35, 255));

	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
	ImGui::SetWindowFontScale(1.2f);
	ImGui::SetCursorScreenPos(ImVec2(bar_pos.x + 15.0f, bar_pos.y + 8.0f));

	ImGui::Text(u8"現在の階層ID：%d", current_graph_id);
	ImGui::SameLine();

	uint32_t target_navigate_id = current_graph_id;	//IDを一時保存

	//ルート階層にいる場合はルートの文字だけ描画
	if (ImGui::Selectable(u8" / ルート", current_graph_id == 0, ImGuiSelectableFlags_None, ImGui::CalcTextSize(u8" / ルート")))
	{
		target_navigate_id = 0;																		// 階層IDをルートへ変更
		printf("StateMachineGraphEditor: ナビゲーションバーの文字クリックによりルート階層へ復帰しました。\n");
	}

	if (current_graph_id != 0)
	{
		ImGui::SameLine();

		std::vector<uint32_t> breadcurmbs;		//経路IDを保存するリスト
		uint32_t trace_id = current_graph_id;	//探索用の現在のID

		//ルートに到達するまで親を逆引き探索
		while (trace_id != 0)
		{
			breadcurmbs.push_back(trace_id);
			uint32_t parent_id = 0;			//親のID
			bool found_parent = false;		//親が見つかったかのフラグ

			//全ての階層データを走査して親を探す
			for (size_t g = 0; g < data_manager->GetLayerDatas().size(); g++)
			{
				for (size_t n = 0; n < data_manager->GetLayerDatas()[g].nodes.size(); n++)
				{
					//ノードがサブグラフであり、行き先が探索用のIDを一致するか確認
					if (data_manager->GetLayerDatas()[g].nodes[n].is_sub_graph && data_manager->GetLayerDatas()[g].nodes[n].sub_graph_id == trace_id)
					{
						parent_id = data_manager->GetLayerDatas()[g].id;
						found_parent = true;
						break;
					}
				}

				//親が見つかったか確認
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
				printf("Warning: StateMachineGraphEditor - 階層ID %d の親が見つかりませんでした。\n", trace_id);
				break;
			}
		}

		//末尾から描画
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

			//中間階層の要素をすべて描画してクリック可能にする
			if (ImGui::Selectable(selectable_label.c_str(), path_id == current_graph_id, ImGuiSelectableFlags_None, text_size))
			{
				target_navigate_id = path_id;
				printf("StateMachineGraphEditor: ナビゲーションパスにより階層ID: %d へ移動しました。\n", target_navigate_id);
			}
		}
	}
	bool is_navigated = (current_graph_id != target_navigate_id); // 移動検知フラグ
	current_graph_id = target_navigate_id;

	ImGui::PopStyleColor();
	ImGui::SetWindowFontScale(1.0f);

	ImGui::SetCursorPos(ImVec2(0.0f, title_bar_height + bar_size.y + 5.0f));
	ImGui::Dummy(ImVec2(0.0f, 1.0f));

	return is_navigated;
}

//ステート一覧リストを描画して、その位置に移動
void StateMachineGraphEditor::DrawStateListWindow(GraphData* current_graph)
{
	// グラフデータが渡されているか確認
	if (!current_graph)
	{
		printf("Error: StateMachineGraphEditor::DrawStateListWindow - current_graph が nullptr です。\n"); // エラーデバッグ出力 [cite: 2026-06-11]
		return;
	}

	ImGui::Spacing();

	// 左ペインをタブで分割し、機能を行き来できるようにする
	if (ImGui::BeginTabBar("LeftSidebarTabBar"))
	{
		// 現在の階層ステート一覧
		if (ImGui::BeginTabItem(u8"階層ノード"))
		{
			DrawHierarchyNodeList(current_graph);
			ImGui::EndTabItem();
		}

		// 全てのステートからのポップ追加
		if (ImGui::BeginTabItem(u8"ステート追加"))
		{
			ImGui::Spacing();

			DrawPaletterFilterButtons();

			const float list_box_height = ImGui::GetContentRegionAvail().y;	// 残りの縦幅全てを使う 
			ImGui::BeginChild("PeletteListChild", ImVec2(0.0f, list_box_height), true);

			const float button_offset_x = 65.0f;	// 右端からボタンを引き算するオフセット幅 

			DrawNormalStatePalette(button_offset_x);

			DrawSubGraphPalette(button_offset_x);

			ImGui::EndChild();
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
}

//階層ノードタブを描画
void StateMachineGraphEditor::DrawHierarchyNodeList(GraphData* current_graph)
{
	ImGui::Spacing();
	const float list_box_height = ImGui::GetContentRegionAvail().y;	// 残りの縦幅全てを使う 

	ImGui::BeginChild("LeftStateListChild", ImVec2(0.0f, list_box_height), true);

	// 現在の階層内の全ノードを走査してリストアップ
	for (size_t i = 0; i < current_graph->nodes.size(); i++)
	{
		const GraphNode& node = current_graph->nodes[i]; // ノード情報 
		ImGui::Text("ID：%d[%s]", node.id, node.name.c_str());
		ImGui::SameLine(ImGui::GetWindowWidth() - 115.0f);
		std::string button_label = u8"フォーカス##" + std::to_string(node.id); // ボタンラベル 

		// ボタンがクリックされたか判定
		if (ImGui::Button(button_label.c_str()))
		{
			g_pending_focus_node_id = node.id; // フォーカス要求を安全に予約
			printf("StateMachineGraphEditor: ノード ID:%d (%s) へのフォーカスを予約しました。\n",
				node.id, node.name.c_str());
		}
	}
	ImGui::EndChild();
}

//パレットの切り替えフィルターボタン描画
void StateMachineGraphEditor::DrawPaletterFilterButtons()
{
	PaletteFilter next_filter = current_filter; // 次フレームから適用するフィルター状態 

	const ImVec4 active_color = ImVec4(0.2f, 0.6f, 0.4f, 1.0f); // 選択中のハイライト緑色（マジックナンバー回避） 

	// 「すべて表示」ボタンの処理
	if (current_filter == PaletteFilter::ALL)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, active_color); // 選択中は緑色にハイライト
	}
	if (ImGui::Button(u8"全て適用"))
	{
		next_filter = PaletteFilter::ALL; // その場では変えず、予約のみ
	}
	if (current_filter == PaletteFilter::ALL)
	{
		ImGui::PopStyleColor(); // 最初の状態に基づいてPop
	}

	ImGui::SameLine();

	// 「サブグラフのみ」ボタンの処理
	if (current_filter == PaletteFilter::SubGraph)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, active_color); // 選択中は緑色にハイライト
	}
	if (ImGui::Button(u8"サブグラフのみ適用"))
	{
		next_filter = PaletteFilter::SubGraph; //  その場では変えず、予約のみ
	}
	if (current_filter == PaletteFilter::SubGraph)
	{
		ImGui::PopStyleColor(); // 最初の状態に基づいてPop
	}

	ImGui::SameLine();

	// 「サブグラフ以外（通常）」ボタンの処理
	if (current_filter == PaletteFilter::Normal)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, active_color); // 選択中は緑色にハイライト
	}
	if (ImGui::Button(u8"サブグラフ以外適用"))
	{
		next_filter = PaletteFilter::Normal; //  その場では変えず、予約のみ
	}
	if (current_filter == PaletteFilter::Normal)
	{
		ImGui::PopStyleColor(); // 最初の状態に基づいてPop
	}

	//  すべてのボタンのPush/Popが安全に終了した「ここ」で、初めて状態を確定反映します！
	current_filter = next_filter;

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();
}

//通常ステートのパレット項目描画
void StateMachineGraphEditor::DrawNormalStatePalette(float button_offset_x)
{
	// 通常ステートの描画判定
	if (current_filter == PaletteFilter::ALL || current_filter == PaletteFilter::Normal)
	{
		ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), u8"▼ 通常ステート");
		ImGui::Separator();

		std::vector<std::string> normal_names = GetExistingNormalStateNames(); // 通常ステートリスト 

		for (size_t i = 0; i < normal_names.size(); i++)
		{
			ImGui::Text("・%s", normal_names[i].c_str());

			//直前に描画したTextアイテムをマウスで掴んで引っ張れるように設定
			if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
			{
				ImGui::Text(u8"移動中：%s", normal_names[i].c_str());
				size_t payload_size = normal_names[i].size() + 1;	//ヌル終端文字を含めた送信バイトサイズ
				ImGui::SetDragDropPayload("DND_PAYLOAD_NORMAL", normal_names[i].c_str(), payload_size);
				ImGui::EndDragDropSource();
			}

			ImGui::SameLine(ImGui::GetWindowWidth() - button_offset_x);
			std::string add_btn_label = u8"追加##Normal" + std::to_string(i); // 通常用固有IDラベル 

			if (ImGui::Button(add_btn_label.c_str()))
			{
				pending_add_palette_node_name = normal_names[i];
				pending_add_is_sub_graph = false; // 通常ステート属性として予約
				printf("StateMachineGraphEditor: パレットから通常「%s」の追加を予約しました。\n", pending_add_palette_node_name.c_str());
			}
			ImGui::Separator();
		}
		ImGui::Spacing();
	}
}

//サブグラフのパレット項目を描画
void StateMachineGraphEditor::DrawSubGraphPalette(float button_offset_x)
{
	// サブグラフの描画判定
	if (current_filter == PaletteFilter::ALL || current_filter == PaletteFilter::SubGraph)
	{
		ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), u8"▼ サブステート"); // 色とテキストを見やすく変更
		ImGui::Separator();

		std::vector<std::string> sub_graph_names = GetExistingSubGraphNames(); // サブステートリスト 

		for (size_t i = 0; i < sub_graph_names.size(); i++)
		{
			ImGui::Text("・%s", sub_graph_names[i].c_str());

			//直前に描画したTextアイテムをマウスで掴んで引っ張れるように設定
			if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
			{
				ImGui::Text(u8"移動中：%s", sub_graph_names[i].c_str());
				size_t payload_size = sub_graph_names[i].size() + 1;	//ヌル終端文字を含めた送信バイトサイズ
				ImGui::SetDragDropPayload("DND_PAYLOAD_SUB", sub_graph_names[i].c_str(), payload_size);
				ImGui::EndDragDropSource();
			}

			ImGui::SameLine(ImGui::GetWindowWidth() - button_offset_x);

			std::string add_btn_label = u8"追加##Sub" + std::to_string(i); // サブステート用固有IDラベル 

			if (ImGui::Button(add_btn_label.c_str()))
			{
				pending_add_palette_node_name = sub_graph_names[i];
				pending_add_is_sub_graph = true;
				printf("StateMachineGraphEditor: パレットから「%s」サブグラフの追加を予約しました。\n", pending_add_palette_node_name.c_str());
			}
			ImGui::Separator();
		}
		ImGui::Spacing();
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

//ノードの詳細情報を描画
void StateMachineGraphEditor::DrawPropertyWindow(GraphData* current_graph, StateBlackboard* blackboard)
{
	//グラフデータが渡されているか確認
	if (!current_graph)
	{
		printf("Error: StateMachineGraphEditor::DrawPropertyWindow - current_graph が nullptr です。\n");
		return;
	}

	ImGui::Text(u8"【ステートプロパティ】");
	ImGui::Spacing();

	const int max_count = 1;	//取得要求の最大ノード数
	ed::NodeId selected_nodes[max_count];	//ノードのID格納コンテナ
	int select_count = ed::GetSelectedNodes(selected_nodes, max_count);	//取得数

	ed::LinkId selected_links[max_count];	//選択中のリンクID配列
	int select_link_count = ed::GetSelectedLinks(selected_links, max_count); // 取得数

	//選択されているノードがあるか確認
	if (select_count > 0)
	{
		uint32_t selected_node_id = static_cast<uint32_t>(selected_nodes[0].Get()); // キャストID
		DrawNodeProperty(current_graph, selected_node_id);
	}
	else if (select_link_count > 0)
	{
		uint32_t selected_link_id = static_cast<uint32_t>(selected_links[0].Get()); // キャストID
		DrawLinkProperty(current_graph, selected_link_id, blackboard);
	}
	else
	{
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), u8"キャンバス上の要素を\n選択すると詳細が表示されます");
	}
}

//ノード選択時の詳細プロパティ描画
void StateMachineGraphEditor::DrawNodeProperty(GraphData* current_graph, uint32_t node_id)
{
	GraphNode* target_node = nullptr;	//編集対象ノード

	//階層データ内から該当ノードを検索
	for (size_t i = 0; i < current_graph->nodes.size(); i++)
	{
		if (current_graph->nodes[i].id == node_id)
		{
			target_node = &current_graph->nodes[i];
			break;
		}
	}

	if (!target_node)
	{
		printf("Warning: 選択されたノードID: %d がデータ内に見つかりません。\n", node_id);
		return;
	}

	ImGui::Text(u8"【ステート設定(ID：%d)】", target_node->id);
	ImGui::Spacing();

	const size_t name_buffer_size = 128;	//バッファサイズ
	char name_input_buffer[name_buffer_size] = {};	//入力バッファ

	strcpy_s(name_input_buffer, name_buffer_size, target_node->name.c_str());

	ImGui::SetNextItemWidth(-1.0f);
	if (ImGui::InputText(u8"##StateNameInput", name_input_buffer, name_buffer_size))
	{
		target_node->name = name_input_buffer;

		//サブグラフ名との同期
		if (target_node->is_sub_graph)
		{
			for (size_t i = 0; i < data_manager->GetLayerDatas().size(); i++)
			{
				if (data_manager->GetLayerDatas()[i].id == target_node->sub_graph_id)
				{
					data_manager->GetLayerDatas()[i].name = target_node->name;
					break;
				}
			}
		}
	}

	//削除ボタン
	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	const ImVec4 red_button_color = ImVec4(0.6f, 0.2f, 0.2f, 1.0f); // 削除ボタン用の赤色
	ImGui::PushStyleColor(ImGuiCol_Button, red_button_color);

	//プロパティウィンドウ内に配置する削除実行ボタン
	if (ImGui::Button(u8"ステートを削除する", ImVec2(-1.0f, 30.0f)))
	{
		uint32_t remove_node_id = target_node->id;	//削除対象の確定ID
		ed::DeleteNode(remove_node_id);
		printf("StateMachineGraphEditor: プロパティ画面からノード ID:%d (%s) の削除要求を発行しました。\n",
			remove_node_id, target_node->name.c_str());
	}
	ImGui::PopStyleColor();
}

//リンク選択時の詳細プロパティ描画
void StateMachineGraphEditor::DrawLinkProperty(GraphData* current_graph, uint32_t link_id, StateBlackboard* blackboard)
{
	GraphLink* target_link = nullptr;	//編集対象リンク

	//階層データ内から該当リンクを検索
	for (size_t i = 0; i < current_graph->links.size(); i++)
	{
		if (current_graph->links[i].id == link_id)
		{
			target_link = &current_graph->links[i];
			break;
		}
	}

	if (!target_link)
	{
		printf("Warning: 選択されたリンクID: %d がデータ内に見つかりません。\n", link_id);
		return;
	}

	//遷移条件プロパティのUI描画
	ImGui::Text(u8"遷移線設定(ID：%d)", target_link->id);
	ImGui::Spacing();

	conditon_editor->DrawConditonSettings(data_manager.get(), blackboard, current_graph_id, target_link);

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.2f, 1.0f), u8"※ここにブラックボード変数を用いた");
	ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.2f, 1.0f), u8"  条件式(==, !=, >, <)の設定項目が");
	ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.2f, 1.0f), u8"  並ぶようになります。");

	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	const ImVec4 red_button_color = ImVec4(0.6f, 0.2f, 0.2f, 1.0f); // 削除ボタン用の赤色
	ImGui::PushStyleColor(ImGuiCol_Button, red_button_color);

	//プロパティウィンドウ内に配置するリンクの削除実行ボタン
	if (ImGui::Button(u8"遷移線を削除する", ImVec2(-1.0f, 30.0f)))
	{
		uint32_t remove_link_id = target_link->id; // 削除対象となる確定リンクID
		ed::DeleteLink(remove_link_id);
		printf("StateMachineGraphEditor: プロパティ画面からリンク ID:%d の削除要求を発行しました。\n", remove_link_id);
	}
	ImGui::PopStyleColor();
}

//接続線の作成を検知してデータに追加
void StateMachineGraphEditor::CreateNewLink(GraphData* current_graph)
{
	//グラフデータが渡されているか確認
	if (!current_graph)
	{
		printf("Error: StateMachineGraphEditor::CreateNewLink - current_graph が nullptr です。\n");
		return;
	}

	ed::PinId start_pin_id;	//接続元のピン
	ed::PinId end_pin_id;	//接続先のピン

	//接続線の作成要求が来ているか確認
	if (ed::QueryNewLink(&start_pin_id, &end_pin_id))
	{
		uint32_t start_id = static_cast<uint32_t>(start_pin_id.Get());	//接続元のID
		uint32_t end_id = static_cast<uint32_t>(end_pin_id.Get());		//接続先のID

		//ルールチェック関数を呼び出し、接続可能か判定
		if (data_manager->CheckCanConnect(current_graph_id, start_id, end_id))
		{
			const ImVec4 success_color = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // 成功色 

			// 4：マウスの判定を独自に行わず、ライブラリの公式な完了判定に委ねることでループバグを防ぐ
			if (ed::AcceptNewItem(success_color, 2.0f))
			{
				GraphLink new_link;	//新しい接続情報
				new_link.id = data_manager->FetchAndIncrementId();
				new_link.start_pin_id = static_cast<uint32_t>(start_pin_id.Get());
				new_link.end_pin_id = static_cast<uint32_t>(end_pin_id.Get());
				current_graph->links.push_back(new_link);

				OnLinkCreated(current_graph, new_link);

				printf("StateMachineGraphEditor: リンクを作成しました。ID: %d, 出力ピン: %d -> 入力ピン: %d\n",
					new_link.id, new_link.start_pin_id, new_link.end_pin_id);
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
	//グラフポインタの安全性を確認
	if (!current_graph)
	{
		return;
	}

	uint32_t source_node_id = 0;	//遷移元のID
	uint32_t target_node_id = 0;	//遷移先のID

	//現在の階層にいるすべてのノードを検索し、ピンの所属先を特定
	for (size_t i = 0; i < current_graph->nodes.size(); i++)
	{
		const GraphNode& node = current_graph->nodes[i];	//検索対象のノード

		//出力ピンのリストから、リンクの開始ピンIDを検索
		for (size_t p = 0; p < node.outputs.size(); p++)
		{
			//ピンIDが一致しているか確認
			if (node.outputs[p].id == new_link.start_pin_id)
			{
				source_node_id = node.id;
				break;
			}
		}

		//入力ピンのリストから、リンクの終了ピンIDを検索
		for (size_t p = 0; p < node.inputs.size(); p++)
		{
			//ピンIDが一致したか確認
			if (node.inputs[p].id == new_link.end_pin_id)
			{
				target_node_id = node.id;
				break;
			}
		}
	}

	//意図しない挙動を検出
	if (source_node_id == 0 || target_node_id == 0)
	{
		printf("Error: OnLinkCreated - 接続されたピンに対応するノードが見つかりませんでした。\n");
		return;
	}

	//どのステートからどのステートへ矢印が生成されたかをログ出力
	printf("StateMachineGraphEditor: 遷移関係を構築しました。[ステートID:%d] ==(遷移)==> [ステートID:%d]\n",
		source_node_id, target_node_id);

}

//実在するノード名の一覧を全データから動的に集計して取得
std::vector<std::string> StateMachineGraphEditor::GetExistingStateNames()
{
	std::vector<std::string> unique_names;	//返却用の一覧コンテナ

	//全ての階層情報を巡回して名前を収集
	for (size_t g = 0; g < data_manager->GetLayerDatas().size(); g++)
	{
		const GraphData& graph = data_manager->GetLayerDatas()[g];	//対象階層

		//階層にいるすべてのノードを走査
		for (size_t n = 0; n < graph.nodes.size(); n++)
		{
			const std::string& node_name = graph.nodes[n].name;	//ノード名
			bool is_duplicate = false;	//収集済みかのフラグ

			for (size_t i = 0; i < unique_names.size(); i++)
			{
				if (unique_names[i] == node_name)
				{
					is_duplicate = true;
					break;
				}
			}

			if (!is_duplicate)
			{
				unique_names.push_back(node_name);
			}
		}
	}
	return unique_names;
}

//通常ノード名を全データから取得
std::vector<std::string> StateMachineGraphEditor::GetExistingNormalStateNames()
{
	std::vector<std::string> unique_names;	//通常ステート用コンテナ

	//全ての階層情報を巡回して通常ノード名を収集
	for (size_t g = 0; g < data_manager->GetLayerDatas().size(); g++)
	{
		const GraphData& graph = data_manager->GetLayerDatas()[g];	//対象の階層

		//階層内のすべてのノードを巡回
		for (size_t n = 0; n < graph.nodes.size(); n++)
		{
			//通常ステートのみを抽出
			if (!graph.nodes[n].is_sub_graph)
			{
				const std::string& node_name = graph.nodes[n].name;	//ノード名
				bool is_duplicate = false;	//重複管理グラフ

				//コンテナを巡回
				for (size_t i = 0; i < unique_names.size(); i++)
				{
					//名前が一致しているか確認
					if (unique_names[i] == node_name)
					{
						is_duplicate = true;
						break;
					}
				}

				//重複していないか確認
				if (!is_duplicate)
				{
					unique_names.push_back(node_name);
				}
			}
		}
	}
	return unique_names;
}

//サブグラフ名を全データから取得
std::vector<std::string> StateMachineGraphEditor::GetExistingSubGraphNames()
{
	std::vector<std::string> unique_names;	//サブグラフ用コンテナ

	//全ての階層情報を巡回してサブグラフノード名を収集
	for (size_t g = 0; g < data_manager->GetLayerDatas().size(); g++)
	{
		const GraphData& graph = data_manager->GetLayerDatas()[g];	//対象の階層

		//階層内のすべてのノードを巡回
		for (size_t n = 0; n < graph.nodes.size(); n++)
		{
			//サブグラフステートのみを抽出
			if (graph.nodes[n].is_sub_graph)
			{
				const std::string& node_name = graph.nodes[n].name;	//ノード名
				bool is_duplicate = false;	//重複管理グラフ

				//コンテナを巡回
				for (size_t i = 0; i < unique_names.size(); i++)
				{
					//名前が一致しているか確認
					if (unique_names[i] == node_name)
					{
						is_duplicate = true;
						break;
					}
				}

				//重複していないか確認
				if (!is_duplicate)
				{
					unique_names.push_back(node_name);
				}
			}
		}
	}
	return unique_names;
}

//カスタムデリータ
void StateMachineGraphEditor::EditorContexDeleter::operator()(ax::NodeEditor::EditorContext* context) const noexcept
{
	if (context)
	{
		ed::DestroyEditor(context);
	}
}