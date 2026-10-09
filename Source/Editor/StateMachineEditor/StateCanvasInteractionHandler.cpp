#include "StateCanvasInteractionHandler.h"
#include "Editor\StateMachineEditor\StateGraphDataManager.h"
#include "StateGraphPaletteWindow.h"

#include <cstdio>

namespace ed = ax::NodeEditor;

//コンテキストメニューの受付とノード追加・変換処理
void StateCanvasInteractionHandler::HandleContextMenu(
	StateGraphDataManager* data_manager,	//データ管理インスタンスへのポインタ
	GraphData* current_graph,				//グラフデータへの参照ポインタ
	uint32_t current_graph_id)				//表示中の階層ID
{
	//-----------------------------
	//ポインタの健全性チェック
	//-----------------------------
	if (!data_manager || !current_graph)
	{
		printf("Error: StateCanvasInteractionHandler::HandleContextMenu - data_manager または current_graph が nullptr です。\n");
		return;
	}

	bool trigger_add_node = false;			//通常ノード追加フラグ
	bool trigger_add_subgraph = false;		//サブグラフ追加フラグ
	bool trigger_convert_subgraph = false;	//サブグラフ変換フラグ

	static ImVec2 popup_click_pos = ImVec2(0.0f, 0.0f);	//クリックしたキャンパス上の座標
	static ed::NodeId context_node_id = 0;				//クリック対象のノードID

	//-------------------------------------------------------
	//NodeEditorのサスペンドとコンテキストメニューの描画
	//-------------------------------------------------------
	ed::Suspend();

	//キャンパス背景のクリック判定
	if (ed::ShowBackgroundContextMenu())
	{
		ImGui::OpenPopup("Create New Node Context Menu");
		popup_click_pos = ed::ScreenToCanvas(ImGui::GetMousePos());
	}

	//ノード上の右クリック判定
	if (ed::ShowNodeContextMenu(&context_node_id))
	{
		ed::SelectNode(context_node_id, true);
		ImGui::OpenPopup("Node Context Menu");
	}

	//背景用ポップアップメニューの要素描画
	if (ImGui::BeginPopup("Create New Node Context Menu"))
	{
		//階層属性に応じたコンテキストメニューの切り替え
		if (current_graph->layer_type == LayerType::BehaviorTree)
		{
			if (ImGui::MenuItem(u8"ルートノードの追加"))
			{
				data_manager->AddBehaviorNode(
					current_graph,
					static_cast<float>(popup_click_pos.x),
					static_cast<float>(popup_click_pos.y),
					BehaviorCategory::Root);

				const uint32_t new_node_id = current_graph->nodes.back().id;
				ed::SetNodePosition(new_node_id, popup_click_pos);
			}
			if (ImGui::MenuItem(u8"優先順位ノードの追加"))
			{
				data_manager->AddBehaviorNode(
					current_graph,
					static_cast<float>(popup_click_pos.x),
					static_cast<float>(popup_click_pos.y),
					BehaviorCategory::Composite,
					CompositeNodeType::Select);

				const uint32_t new_node_id = current_graph->nodes.back().id;
				ed::SetNodePosition(new_node_id, popup_click_pos);
			}
			if (ImGui::MenuItem(u8"重み抽選ノードの追加"))
			{
				data_manager->AddBehaviorNode(
					current_graph,
					static_cast<float>(popup_click_pos.x),
					static_cast<float>(popup_click_pos.y),
					BehaviorCategory::Composite,
					CompositeNodeType::Weight);

				const uint32_t new_node_id = current_graph->nodes.back().id;
				ed::SetNodePosition(new_node_id, popup_click_pos);
			}
			if (ImGui::MenuItem(u8"アクションノードの追加"))
			{
				data_manager->AddBehaviorNode(
					current_graph,
					static_cast<float>(popup_click_pos.x),
					static_cast<float>(popup_click_pos.y),
					BehaviorCategory::Action);

				const uint32_t new_node_id = current_graph->nodes.back().id;
				ed::SetNodePosition(new_node_id, popup_click_pos);
			}
		}
		else
		{
			//「ステート追加」項目が選択されたか判定
			if (ImGui::MenuItem(u8"ステート追加"))
			{
				trigger_add_node = true;
			}
			//「サブグラフ追加」項目が選択されたか判定
			if (ImGui::MenuItem(u8"サブグラフ追加"))
			{
				trigger_add_subgraph = true;
			}
		}
		ImGui::EndPopup();
	}

	//ノード用ポップアップメニューの要素描画
	if (ImGui::BeginPopup("Node Context Menu"))
	{
		//「サブグラフへ変換」項目が選択されたか判定
		if (ImGui::MenuItem(u8"サブグラフへ変換"))
		{
			trigger_convert_subgraph = true;
		}
		ImGui::EndPopup();
	}

	ed::Resume();

	//-----------------------------------------------
	//ユーザー操作要求に応じたデータ追加・変換処理
	//-----------------------------------------------
	//通常ノード追加要求処理
	if (trigger_add_node)
	{
		data_manager->AddNode(
			current_graph,
			static_cast<float>(popup_click_pos.x),
			static_cast<float>(popup_click_pos.y)
		);
		const uint32_t new_state_id = current_graph->nodes.back().id;	//追加されたノードの固有ID
		ed::SetNodePosition(new_state_id, popup_click_pos);
	}

	//サブグラフノード追加要求処理
	if (trigger_add_subgraph)
	{
		data_manager->AddSubGrapNode(
			current_graph_id,
			static_cast<float>(popup_click_pos.x),
			static_cast<float>(popup_click_pos.y)
		);

		//サブグラフ追加により階層配列が再確保された可能性があるためポインタを再取得
		for (size_t g_idx = 0; g_idx < data_manager->GetLayerDatas().size(); g_idx++)
		{
			//一致する階層IDを判定
			if (data_manager->GetLayerDatas()[g_idx].id == current_graph_id)
			{
				current_graph = &data_manager->GetLayerDatas()[g_idx];
				break;
			}
		}
		const uint32_t new_node_id = current_graph->nodes.back().id;	//追加されたサブグラフの固有ID
		ed::SetNodePosition(new_node_id, popup_click_pos);
	}

	//サブグラフへの変換処理
	if (trigger_convert_subgraph)
	{
		const uint32_t raw_node_id = static_cast<uint32_t>(context_node_id.Get());	//対象のノードID
		data_manager->ConvertToSubGraph(current_graph_id, raw_node_id);
	}
}

//パレットウィンドウに保留されている追加要求ノードをキャンパスに配置
void StateCanvasInteractionHandler::HandlePendingPaletteNode(
	StateGraphDataManager* data_manager,		//データ管理インスタンスへのポインタ
	StateGraphPaletteWindow* palette_window,	//パレットウィンドウへのポインタ
	GraphData*& current_graph,					//編集中のグラフデータへの参照ポインタ
	uint32_t current_graph_id)					//表示中の階層ID
{
	//-----------------------------
	//ポインタの健全性チェック
	//-----------------------------
	if (!data_manager || !palette_window || !current_graph)
	{
		printf("Error: StateCanvasInteractionHandler::HandlePendingPaletteNode - 渡されたポインタのいずれかが nullptr です。\n");
		return;
	}

	//パレット側で追加要求があるか確認
	if (palette_window->HasPendingAddNode())
	{
		const ImVec2 center_pos = ImGui::GetMainViewport()->GetCenter();	//メインビューポートの中心座標
		const ImVec2 canvas_pos = ed::ScreenToCanvas(center_pos);			//キャンバス座標

		//追加対象がサブグラフか判定
		if (palette_window->IsPendingSubGraph())
		{
			data_manager->AddSubGrapNode(
				current_graph_id,
				static_cast<float>(canvas_pos.x),
				static_cast<float>(canvas_pos.y),
				palette_window->GetPendingNodeName()
			);

			//配列再確保対策として現在のグラフポインタを再取得
			for (size_t g_idx = 0; g_idx < data_manager->GetLayerDatas().size(); g_idx++)
			{
				//一致する階層IDを判定
				if (data_manager->GetLayerDatas()[g_idx].id == current_graph_id)
				{
					current_graph = &data_manager->GetLayerDatas()[g_idx];
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
		const uint32_t new_state_id = current_graph->nodes.back().id;	//新しいノードID
		ed::SetNodePosition(new_state_id, canvas_pos);
		palette_window->ClearPendingNode();
	}
}

//ノード及びリンクの削除要求処理
void StateCanvasInteractionHandler::HandleDeletion(
	StateGraphDataManager* data_manager,	//データ管理インスタンスへのポインタ
	GraphData* current_graph,				//表示中のグラフデータへの参照ポインタ
	uint32_t current_graph_id)				//表示中の階層ID
{
	//-----------------------------
	//ポインタの健全性チェック
	//-----------------------------
	if (!data_manager || !current_graph)
	{
		printf("Error: StateCanvasInteractionHandler::HandleDeletion - data_manager または current_graph が nullptr です。\n");
		return;
	}
	//-------------------------------
	//NodeEditorの削除クエリ受付
	//-------------------------------
	//削除クエリ受付スコープ開始
	if (ed::BeginDelete())
	{
		DeleteNode(data_manager, current_graph, current_graph_id);
		DeleteLink(data_manager, current_graph, current_graph_id);
	}
	ed::EndDelete();
}

//パレットからのドラッグ&ドロップ受け取り処理
void StateCanvasInteractionHandler::HandleDragAndDrop(
	StateGraphDataManager* data_manager,	//データ管理インスタンスへのポインタ
	GraphData*& current_graph,				//編集中のグラフデータへの参照ポインタ
	uint32_t current_graph_id)				//表示中の階層ID
{
	//-----------------------------
	//ポインタの健全性チェック
	//-----------------------------
	if (!data_manager || !current_graph)
	{
		printf("Error: StateCanvasInteractionHandler::HandleDragAndDrop - data_manager または current_graph が nullptr です。\n");
		return;
	}

	//---------------------------------------
	//ImGuiドラッグ&ドロップターゲット処理
	//---------------------------------------
	//ドロップ可能領域であるか判定
	if (ImGui::BeginDragDropTarget())
	{
		//通常ステートのドロップ受け取り
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_PAYLOAD_NORMAL"))
		{
			const char* dropped_node_name = static_cast<const char*>(payload->Data);		//ドロップされたステート名
			const ImVec2 drag_mouse_screen_pos = ImGui::GetMousePos();						//マウスの画面座標
			const ImVec2 drop_mouse_canves_pos = ed::ScreenToCanvas(drag_mouse_screen_pos);	//キャンパス座標

			data_manager->AddNode(
				current_graph,
				static_cast<float>(drop_mouse_canves_pos.x),
				static_cast<float>(drop_mouse_canves_pos.y),
				dropped_node_name
			);

			const uint32_t new_node_id = current_graph->nodes.back().id;	//新規生成ノードID
			ed::SetNodePosition(new_node_id, drop_mouse_canves_pos);

			printf("StateCanvasInteractionHandler: 通常ステート「%s」をドラッグ＆ドロップで配置しました。\n", dropped_node_name);
		}

		//サブグラフのドロップ受け取り
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_PAYLOAD_SUB"))
		{
			const char* dropped_sub_name = static_cast<const char*>(payload->Data);			//ドロップされたサブステート名
			const ImVec2 drag_mouse_screen_pos = ImGui::GetMousePos();						//マウスの画面座標
			const ImVec2 drop_mouse_canves_pos = ed::ScreenToCanvas(drag_mouse_screen_pos);	//キャンパス座標

			data_manager->AddSubGrapNode(
				current_graph_id,
				static_cast<float>(drop_mouse_canves_pos.x),
				static_cast<float>(drop_mouse_canves_pos.y),
				dropped_sub_name
			);

			//配列再確保対策として現在のグラフポインタを再取得
			for (size_t g_idx = 0; g_idx < data_manager->GetLayerDatas().size(); g_idx++)
			{
				//一致する階層IDを判定
				if (data_manager->GetLayerDatas()[g_idx].id == current_graph_id)
				{
					current_graph = &data_manager->GetLayerDatas()[g_idx];
					break;
				}
			}

			const uint32_t new_node_id = current_graph->nodes.back().id;	//新規生成ノードID
			ed::SetNodePosition(new_node_id, drop_mouse_canves_pos);

			printf("StateCanvasInteractionHandler: サブグラフ「%s」をドラッグ＆ドロップで完全複製配置しました。\n", dropped_sub_name);
		}
		ImGui::BeginDragDropTarget();
	}
}

//ノードの削除処理
void StateCanvasInteractionHandler::DeleteNode(
	StateGraphDataManager* data_manager,	//データ管理インスタンスへのポインタ
	GraphData* current_graph,				//編集中のグラフデータへの参照ポインタ
	uint32_t current_graph_id)				//表示中の階層ID
{
	ed::NodeId delete_node_id;	//削除クエリ対象ノードID

	//削除クエリ対象が存在する間ループ
	while (ed::QueryDeletedNode(&delete_node_id))
	{
		//削除アクションの確定判定
		if (ed::AcceptDeletedItem())
		{
			const uint32_t target_id = static_cast<uint32_t>(delete_node_id.Get());	//uint32_t型にキャストした削除対象ノードID
			data_manager->DeleteNode(current_graph_id, target_id);
		}
	}
}

//リンクの削除処理
void StateCanvasInteractionHandler::DeleteLink(
	StateGraphDataManager* data_manger,	//データ管理インスタンスへのポインタ
	GraphData* current_graph,			//編集中のグラフデータへの参照ポインタ
	uint32_t current_graph_id)			//表示中の階層ID
{
	ed::LinkId delete_link_id;	//削除クエリ対象リンクID

	//削除クエリ対象が存在する間ループ
	while (ed::QueryDeletedLink(&delete_link_id))
	{
		//削除アクションの確定判定
		if (ed::AcceptDeletedItem())
		{
			const uint32_t target_id = static_cast<uint32_t>(delete_link_id.Get());	//uint32_t型にキャストした削除対象リンクID
			data_manger->DeleteLink(current_graph_id, target_id);
		}
	}
}
