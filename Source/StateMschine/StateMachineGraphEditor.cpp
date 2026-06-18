#define IMGUI_DEFINE_MATH_OPERATORS

#include "StateMachineGraphEditor.h"
#include "../StateMschine/StateBlackboard.h"

#include <imgui_node_editor_internal.h>
#include <cassert>

namespace ed = ax::NodeEditor;

//コンストラクタ
StateMachineGraphEditor::StateMachineGraphEditor()
{
	//ライブラリ初期化
	ed::Config config;
	config.SettingsFile = "Data/Json/NodeEditor_State.json";
	editor_context.reset(ed::CreateEditor());

	//初期グラフ生成
	const uint32_t root_id = 0;
	current_graph_id = root_id;

	GraphData root_graph;
	root_graph.id = root_id;
	root_graph.name = "ルート";

	layer_datas.push_back(root_graph);

	next_id = 100;
}

//エディタ描画
void StateMachineGraphEditor::DrawEditor(StateBlackboard* blackboard)
{
	// アクティブグラフ検索
	GraphData* current_graph = nullptr;	// 現在の階層情報

	// 全ての階層データから現在のグラフIDに一致するものを探す
	for (size_t i = 0; i < layer_datas.size(); i++)
	{
		if (layer_datas[i].id == current_graph_id)
		{
			current_graph = &layer_datas[i];
			break;
		}
	}

	// ループの外側で未発見チェック
	if (!current_graph)
	{
		assert(false && "StateMachineGraphEditor: 指定されたグラフIDが見つかりません。");
		return;
	}

	// 無限生成バグを防ぐため、毎フレームの冒頭で必ずフラグを false に初期化する
	bool trigger_add_node = false;			// ノード追加の実行トリガー
	bool trigger_add_subgraph = false;		// サブグラフ追加の実行トリガー
	bool trigger_convert_subgraph = false;	// サブグラフ変換の実行トリガー

	// 画面描画
	ImGui::Begin(u8"ステートマシンエディタ");

	// 階層ナビゲーションを描画
	DrawHeaderNavigation();


	ed::SetCurrentEditor(editor_context.get());
	ed::Begin("Node Canvas");

	// ファーストフレームの初期化
	if (current_graph_id == 0 && current_graph->nodes.empty())
	{
		GraphNode test_node;
		test_node.id = next_id++;
		test_node.name = u8"待機状態";
		test_node.position_x = 100.0f;
		test_node.position_y = 100.0f;
		test_node.is_sub_graph = false;
		test_node.sub_graph_id = 0;

		// 入力ピン設定
		GraphPin test_input;
		test_input.id = next_id++;
		test_input.name = u8"入力";
		test_input.kind = PinKind::Input;
		test_input.node_id = test_node.id;
		test_node.inputs.push_back(test_input);

		// 出力ピン設定
		GraphPin test_output;
		test_output.id = next_id++;
		test_output.name = u8"出力";
		test_output.kind = PinKind::Output;
		test_output.node_id = test_node.id;
		test_node.outputs.push_back(test_output);

		current_graph->nodes.push_back(test_node);

		//IDが100番台になったことで、古いキャッシュに邪魔されず座標が確実に適用されます
		ed::SetNodePosition(test_node.id, ImVec2(100.0f, 100.0f));
	}

	// 階層内のすべてのノードを描画
	for (size_t i = 0; i < current_graph->nodes.size(); i++)
	{
		const GraphNode& node = current_graph->nodes[i];

		// サブグラフノードの場合は色を変更するスタイルをプッシュ
		int pushed_style_count = 0;	// スタイル変更を適用した数
		if (node.is_sub_graph)
		{
			ed::PushStyleColor(ed::StyleColor_NodeBg, ImVec4(0.1f, 0.2f, 0.4f, 0.85f));
			ed::PushStyleColor(ed::StyleColor_SelNodeBorder, ImVec4(0.3f, 0.6f, 1.0f, 1.0f));
			pushed_style_count = 2;
		}

		ed::BeginNode(node.id);
		ImGui::Text("%s", node.name.c_str());
		ImGui::Spacing();

		// 入力ピンのグループ配置
		ImGui::BeginGroup();
		for (size_t in_idx = 0; in_idx < node.inputs.size(); in_idx++)
		{
			const GraphPin& pin = node.inputs[in_idx];
			ed::BeginPin(pin.id, ed::PinKind::Input);
			ImGui::Text("->%s", pin.name.c_str());
			ed::EndPin();
		}
		ImGui::EndGroup();

		ImGui::SameLine();
		ImGui::Dummy(ImVec2(40.0f, 0.0f));
		ImGui::SameLine();

		// 出力ピンのグループ配置
		ImGui::BeginGroup();
		for (size_t out_idx = 0; out_idx < node.outputs.size(); out_idx++)
		{
			const GraphPin& pin = node.outputs[out_idx];
			ed::BeginPin(pin.id, ed::PinKind::Output);
			ImGui::Text("%s ->", pin.name.c_str());
			ed::EndPin();
		}
		ImGui::EndGroup();

		ed::EndNode();

		// スタイルカラーの復元
		for (int color_idx = 0; color_idx < pushed_style_count; color_idx++)
		{
			ed::PopStyleColor();
		}
	}

	// 接続線の描画
	for (size_t i = 0; i < current_graph->links.size(); i++)
	{
		const GraphLink& link = current_graph->links[i];
		ed::Link(link.id, link.start_pin_id, link.end_pin_id);
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

	//安全な区間でのデータ操作実行 --
	if (trigger_add_node)
	{
		AddNode(current_graph, popup_click_pos);
	}
	if (trigger_add_subgraph)
	{
		AddSubGrapNode(current_graph, popup_click_pos);
	}
	if (trigger_convert_subgraph)
	{
		uint32_t raw_node_id = static_cast<uint32_t>(context_node_id.Get());
		ConvertToSubGraph(current_graph, raw_node_id);
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

	DrawStateListWindow(current_graph);

	// プロパティウィンドウ描画
	DrawPropertyWindow(current_graph);

	ed::SetCurrentEditor(nullptr);
	ImGui::End();
}

//ノードの生成
void StateMachineGraphEditor::AddNode(GraphData* current_graph, const ImVec2& click_pos)
{
	//グラフデータが渡されているか確認
	if (!current_graph)
	{
		printf("Error: StateMachineGraphEditor::AddNewNode - current_graph が nullptr です。\n");
		return;
	}

	GraphNode new_node;	//新しいノード情報

	//ノードのパラメータ設定
	new_node.id = next_id++;
	new_node.name = u8"新規ステート";
	new_node.position_x = click_pos.x;
	new_node.position_y = click_pos.y;
	new_node.is_sub_graph = false;
	new_node.sub_graph_id = 0;
	
	//新しい入力ピンのデータ作成
	GraphPin new_input;		//新しい入力ピン
	new_input.id = next_id++;
	new_input.name = u8"入力";
	new_input.kind = PinKind::Input;
	new_input.node_id = new_node.id;
	new_node.inputs.push_back(new_input);

	//新しい出力ピンのデータ作成
	GraphPin new_output;	//新しい出力ピン
	new_output.id = next_id++;
	new_output.name = u8"出力";
	new_output.kind = PinKind::Output;
	new_output.node_id = new_node.id;
	new_node.outputs.push_back(new_output);

	current_graph->nodes.push_back(new_node);
	ed::SetNodePosition(new_node.id, click_pos);	//ノードの初期座標を設定
	printf("StateMachineGraphEditor: ノードを追加しました。ID: %d, 入力ピンID: %d, 出力ピンID: %d\n",
		new_node.id, new_input.id, new_output.id);
}

//サブグラフノードの生成
void StateMachineGraphEditor::AddSubGrapNode(GraphData* current_graph, const ImVec2& click_pos)
{
	//グラフに情報が渡されているか確認
	if (!current_graph)
	{
		printf("Error: StateMachineGraphEditor::AddSubGraphNode - current_graph が nullptr です。\n");
		return;
	}

	std::string sub_graph_name = u8"新規サブグラフ";	//サブグラフの名前
	uint32_t real_sub_graph_id = CreateNewSubGraph(sub_graph_name);

	//layer_datas が引っ越した可能性を考慮し、現在の表示階層ポインタをs再取得
	for (size_t i = 0; i < layer_datas.size(); i++)
	{
		if (layer_datas[i].id == current_graph_id)
		{
			current_graph = &layer_datas[i];
			break;
		}
	}

	GraphNode new_node;	//新しいノード情報

	//パラメータ設定
	new_node.id = next_id++;
	new_node.name = sub_graph_name;
	new_node.position_x = click_pos.x;
	new_node.position_y = click_pos.y;
	new_node.is_sub_graph = true;
	uint32_t assigned_sub_graph_id = real_sub_graph_id;
	new_node.sub_graph_id = assigned_sub_graph_id;

	//サブグラフ用の入力ピン
	GraphPin new_input;	//新しい入力ピン情報
	new_input.id = next_id++;
	new_input.name = u8"入力";
	new_input.kind = PinKind::Input;
	new_input.node_id = new_node.id;
	new_node.inputs.push_back(new_input);
	
	//サブグラフ用の出力ピン
	GraphPin new_output;	//新しい出力ピン情報
	new_output.id = next_id++;
	new_output.name = u8"出力";
	new_output.kind = PinKind::Output;
	new_output.node_id = new_node.id;
	new_node.outputs.push_back(new_output);

	//現在のグラフにノードを追加
	current_graph->nodes.push_back(new_node);
	ed::SetNodePosition(new_node.id, click_pos);

	if (assigned_sub_graph_id != real_sub_graph_id)
	{
		printf("Warning: サブグラフIDの予測が一致しませんでした。データを確認してください。\n");
	}

	printf("StateMachineGraphEditor: サブグラフノードを追加しました。ID: %d, 下位階層ID: %d\n",
		new_node.id, new_node.sub_graph_id);
}

//階層データを作成してIDを返す
uint32_t StateMachineGraphEditor::CreateNewSubGraph(const std::string& name)
{
	GraphData new_graph;	//新しい階層情報

	//パラメータ設定
	new_graph.id = next_id++;
	new_graph.name = name;
	layer_datas.push_back(new_graph);

	printf("StateMachineGraphEditor: 新しい階層データを作成しました。階層ID: %d, 名前: %s\n", new_graph.id, name.c_str());

	return new_graph.id;
}

//既存のノードをサブグラフに変換
void StateMachineGraphEditor::ConvertToSubGraph(GraphData* currnet_graph, uint32_t node_id)
{
	//グラフデータが渡されているか確認
	if (!currnet_graph)
	{
		printf("Error: StateMachineGraphEditor::ConvertToSubGraph - current_graph が nullptr です。\n");
		return;
	}

	//現在のグラフ内の全ノードを走査して対象のノードを検索
	for (size_t i = 0; i < currnet_graph->nodes.size(); i++)
	{
		GraphNode& target_node = currnet_graph->nodes[i];	//対象のノード
		
		//変換対象のノードIDを一致するか確認
		if (target_node.id == node_id)
		{
			//サブグラフ化されていないか確認
			if (target_node.is_sub_graph)
			{
				printf("StateMachineGraphEditor: ノード ID:%d は既にサブグラフです。\n", node_id);
				return;
			}

			//ノードのフラグ書き換えと階層生成
			target_node.is_sub_graph = true;
			target_node.sub_graph_id = CreateNewSubGraph(target_node.name);

			target_node.name += u8" サブステート";

			printf("StateMachineGraphEditor: ノード「%s」(ID:%d) をサブグラフ(階層ID:%d)へ変換しました。\n",
				target_node.name.c_str(), target_node.id, target_node.sub_graph_id);

			break;
		}
	}
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
void StateMachineGraphEditor::DrawHeaderNavigation()
{
	//ナビゲーションバーの背景色の描画
	ImVec2 window_pos = ImGui::GetWindowPos();
	float title_bar_height = ImGui::GetCursorPos().y;
	ImVec2 bar_pos = ImVec2(window_pos.x, window_pos.y + title_bar_height);
	ImVec2 bar_size = ImVec2(ImGui::GetWindowWidth(), 35.0f);

	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	draw_list->AddRectFilled(bar_pos, ImVec2(bar_pos.x + bar_size.x, bar_pos.y + bar_size.y), IM_COL32(35, 35, 35, 255));

	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
	ImGui::SetWindowFontScale(1.2f);
	ImGui::SetCursorScreenPos(ImVec2(bar_pos.x + 15.0f, bar_pos.y + 8.0f));

	ImGui::Text(u8"現在の階層ID：%d", current_graph_id);
	ImGui::SameLine();

	uint32_t target_navigate_id = current_graph_id;	//IDを一時保存

	//ルート改装にいる場合はルートの文字だけ描画
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
			for (size_t g = 0; g < layer_datas.size(); g++)
			{
				for (size_t n = 0; n < layer_datas[g].nodes.size(); n++)
				{
					//ノードがサブグラフであり、行き先が探索用のIDを一致するか確認
					if (layer_datas[g].nodes[n].is_sub_graph && layer_datas[g].nodes[n].sub_graph_id == trace_id)
					{
						parent_id = layer_datas[g].id;
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
			for (size_t g = 0; g < layer_datas.size(); g++)
			{
				if (layer_datas[g].id == path_id)
				{
					path_name = layer_datas[g].name;
					break;
				}
			}

			std::string display_text = " / " + path_name;	//表示するテキスト名
			ImVec2 text_size = ImGui::CalcTextSize(display_text.c_str());	//文字の大きさ

			ImGui::SameLine();

			//中間階層の要素をすべて描画してクリック可能にする
			if (ImGui::Selectable(display_text.c_str(), path_id == current_graph_id, ImGuiSelectableFlags_None, text_size))
			{
				target_navigate_id = path_id;
				printf("StateMachineGraphEditor: ナビゲーションパスにより階層ID: %d へ移動しました。\n", target_navigate_id);
			}
		}
	}
	current_graph_id = target_navigate_id;

	ImGui::PopStyleColor();
	ImGui::SetWindowFontScale(1.0f);

	ImGui::SetCursorPos(ImVec2(0.0f, title_bar_height + bar_size.y + 5.0f));
}

//ステート一覧リストを描画して、その位置に移動
void StateMachineGraphEditor::DrawStateListWindow(GraphData* current_graph)
{
	//グラフデータが渡されているか確認
	if (!current_graph)
	{
		printf("Error: StateMachineGraphEditor::DrawStateListInProperty - current_graph が nullptr です。\n");
		return;
	}

	ImGui::Begin(u8"【ステート一覧】");

	ImGui::Spacing();
	ImGui::Text(u8"【現在の階層のステート一覧】");
	ImGui::Spacing();

	//レイアウト用の小窓を作成してステートをリストに描画
	if (ImGui::BeginChild("LeftStateListChild"), ImVec2(0.0f, 200.0f), true)
	{
		//現在の階層内の全ノードを走査してリストアップ
		for (size_t i = 0; i < current_graph->nodes.size(); i++)
		{
			const GraphNode& node = current_graph->nodes[i];
			ImGui::Text("ID：%d[%s]", node.id, node.name.c_str());
			ImGui::SameLine(ImGui::GetWindowWidth() - 120.0f);
			std::string button_label = u8"フォーカス##" + std::to_string(node.id);

			//ボタンがクリックされたか判定
			if (ImGui::Button(button_label.c_str()))
			{
				ed::SelectNode(node.id, false);
				ed::NavigateToSelection(false, 0.5f);

				printf("StateMachineGraphEditor: ノード ID:%d (%s) へカメラを強制ジャンプしました。\n",
					node.id, node.name.c_str());
			}
		}
		ImGui::EndChild();
	}
	ImGui::End();
}

//ノードの削除
void StateMachineGraphEditor::DeleteNode(GraphData* current_graph)
{
	//グラフデータが渡されているか確認
	if (!current_graph)
	{
		printf("Error: StateMachineGraphEditor::DeleteSelectedObjects - current_graph が nullptr です。\n");
		return;
	}

	ed::NodeId delete_node_id;	//削除対象のID格納先

	//削除要求されているノードをループで取得
	while (ed::QueryDeletedNode(&delete_node_id))
	{
		//削除してよいかの最終確認済みか判定
		if (ed::AcceptDeletedItem())
		{
			uint32_t target_id = static_cast<uint32_t>(delete_node_id.Get());	//削除対象のID
			bool is_removed = false;	//一致するノードを階層データから検索して削除するフラグ

			//現在のグラフ内の全ノードを走査
			for (auto it = current_graph->nodes.begin(); it != current_graph->nodes.end();)
			{
				//削除対象のIDと一致するか確認
				if (it->id == target_id)
				{
					std::string debug_name = it->name;	//デバッグ出力用の名前
					it = current_graph->nodes.erase(it);
					printf("StateMachineGraphEditor: ノードを削除しました。ID: %d, 名前: %s\n", target_id, debug_name.c_str());
					is_removed = true;
					break;
				}
				else
				{
					it++;
				}
			}

			//エディタ側で消したのにデータリストに見つからなかった場合
			if (!is_removed)
			{
				printf("Warning: 削除要求されたノードID: %d が layer_datas 内に見つかりませんでした。\n", target_id);
			}
		}
	}

}

//接続線の削除
void StateMachineGraphEditor::DeleteLink(GraphData* current_graph)
{
	//グラフデータが渡されているか確認
	if (!current_graph)
	{
		printf("Error: StateMachineGraphEditor::DeleteLink - current_graph が nullptr です。\n");
		return;
	}

	ed::LinkId delete_link_id;	//削除対象のリンクID

	//削除対象として要求されているリンクをループで取得
	while (ed::QueryDeletedLink(&delete_link_id))
	{
		//削除承認が出たか判定
		if (ed::AcceptDeletedItem())
		{
			uint32_t target_id = static_cast<uint32_t>(delete_link_id.Get());	//取得したID

			//一致するリンクを検索して走査
			for (auto it = current_graph->links.begin(); it != current_graph->links.end(); ) 
			{
				//削除対象のIDと一致するか確認
				if (it->id == target_id)
				{
					it = current_graph->links.erase(it);
					printf("StateMachineGraphEditor: リンクを削除しました。ID: %d\n", target_id);
					break;
				}
				else
				{
					it++;
				}
			}
		}
	}
}

//ノードの詳細情報を描画
void StateMachineGraphEditor::DrawPropertyWindow(GraphData* current_graph)
{
	//グラフデータが渡されているか確認
	if (!current_graph)
	{
		printf("Error: StateMachineGraphEditor::DrawPropertyWindow - current_graph が nullptr です。\n");
		return;
	}

	ImGui::Begin(u8"ステートプロパティ");

	const int max_count = 1;	//取得要求の最大ノード数
	ed::NodeId selected_nodes[max_count];	//ノードのID格納コンテナ
	int select_count = ed::GetSelectedNodes(selected_nodes, max_count);	//取得数

	//選択されているノードがあるか確認
	if (select_count <= 0)
	{
		ImGui::Text(u8"ノードを選択すると詳細が表示されます");
		ImGui::End();
		return;
	}

	uint32_t selected_id = static_cast<uint32_t>(selected_nodes[0].Get());	//取得したノードID
	GraphNode* target_node = nullptr;	//選択されたIDのノードデータコンテナ

	//現在のグラフ内の全ノードを走査
	for (size_t i = 0; i < current_graph->nodes.size(); i++)
	{
		//ノードIDが選択されたIDを検索
		if (current_graph->nodes[i].id == selected_id)
		{
			target_node = &current_graph->nodes[i];
			break;
		}
	}

	//一致するノードが見つからなかったか判定
	if (!target_node)
	{
		printf("Warning: 選択されたノードID: %d がデータ内に見つかりません。\n", selected_id);
		ImGui::End();
		return;
	}

	const size_t name_buffer_size = 128;	//バッファサイズ
	char name_input_buffer[name_buffer_size] = {};	//ノードの名前
	strcpy_s(name_input_buffer, name_buffer_size, target_node->name.c_str());

	if (ImGui::InputText(u8"ステート名", name_input_buffer, name_buffer_size))
	{
		target_node->name = name_input_buffer;
	}
	ImGui::End();
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
		if (CheckCanConnect(current_graph, start_id, end_id))
		{
			ed::RejectNewItem(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), 2.0f);

			//マウスを離して接続を確定したか判定
			if (ed::AcceptNewItem())
			{
				GraphLink new_link;	//新しい接続情報
				new_link.id = next_id++;
				new_link.start_pin_id = static_cast<uint32_t>(start_pin_id.Get());
				new_link.end_pin_id = static_cast<uint32_t>(end_pin_id.Get());
				current_graph->links.push_back(new_link);

				printf("StateMachineGraphEditor: リンクを作成しました。ID: %d, 出力ピン: %d -> 入力ピン: %d\n",
					new_link.id, new_link.start_pin_id, new_link.end_pin_id);
			}

		}
		else
		{
			ed::RejectNewItem(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), 2.0f);
		}
	}
}

//リンクの接続ルールを判定
bool StateMachineGraphEditor::CheckCanConnect(GraphData* current_graph, uint32_t start_id, uint32_t end_id)
{
	//念のためポインタの安全性を確認
	if (!current_graph)
	{
		return false;
	}

	const GraphPin* start_pin = nullptr;	//接続元のピン
	const GraphPin* end_pin = nullptr;		//接続先のピン

	//現在の全ノードを走査して対象のピンデータを検索
	for (size_t i = 0; i < current_graph->nodes.size(); i++)
	{
		const GraphNode& node = current_graph->nodes[i];

		//入力ピンから検索
		for (size_t p = 0; p < node.inputs.size(); p++)
		{
			if (node.inputs[p].id == start_id) start_pin = &node.inputs[p];
			if (node.inputs[p].id == end_id) end_pin = &node.inputs[p];
		}
		
		//出力ピンから検索
		for (size_t p = 0; p < node.outputs.size(); p++)
		{
			if (node.outputs[p].id == start_id) start_pin = &node.outputs[p];
			if (node.outputs[p].id == end_id) end_pin = &node.outputs[p];
		}
	}
	//両方のピンがデータ内に存在するか安全性のチェック
	if (!start_pin || !end_pin)
	{
		return false;
	}

	//同じノード内のピン同士の接続は禁止
	if (start_pin->node_id == end_pin->node_id)
	{
		return false;
	}

	//同じ種類のピン同士（入力から入力、出力から出力）の接続は禁止
	if (start_pin->kind == end_pin->kind)
	{
		return false;
	}

	//入力ピンからドラッグして出力ピンへ繋ぐ逆方向の操作を禁止
	if (start_pin->kind == PinKind::Input && end_pin->kind == PinKind::Output)
	{
		return false;
	}

	return true;
}

//カスタムデリータ
void StateMachineGraphEditor::EditorContexDeleter::operator()(ax::NodeEditor::EditorContext* context) const noexcept
{
	if (context)
	{
		ed::DestroyEditor(context);
	}
}
