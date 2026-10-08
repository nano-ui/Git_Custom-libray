#include "Editor\StateMachineEditor\Handlers\StateLinkConnectionHandler.h"
#include "Editor\StateMachineEditor\Data\StateGraphDataManager.h"
#include "Editor\StateMachineEditor\Nodes\StateGraphNode.h"

#include <imgui.h>
#include <imgui_node_editor.h>
#include <cstdio>

namespace ed = ax::NodeEditor;

//接続先の作成クエリを検知してデータに追加
void StateLinkConnectionHandler::HandleLinkCreation(
	StateGraphDataManager* data_manager,	//データ管理インスタンス
	GraphData* current_graph,				//編集中のグラフデータ
	uint32_t current_graph_id)				//表示中の階層ID
{
	//--------------------------
	//ポインタの健全性チェック
	//--------------------------
	//必要なポインタが存在するか判定
	if (!data_manager || !current_graph) 
	{
		printf("Error: StateLinkConnectionHandler::HandleLinkCreation - data_manager または current_graph が nullptr です。\n");
		return;
	}

	//-----------------------------------
	//NodeEditorのリンク生成スコープ
	//-----------------------------------
	//リンク生成受付スコープの開始判定
	if (ed::BeginCreate())
	{
		ed::PinId start_pin_id;	//接続元ピンID
		ed::PinId end_pin_id;	//接続先ピンID

		//新規リンク作成受付判定
		if (ed::QueryNewLink(&start_pin_id, &end_pin_id))
		{
			const uint32_t start_id = static_cast<uint32_t>(start_pin_id.Get());	//uint32_t型にキャストした接続元ピンID
			const uint32_t end_id = static_cast<uint32_t>(end_pin_id.Get());		//uint32_t型にキャストした接続先ピンID

			//接続ルールの正当性の確認
			if (CanConnect(data_manager, current_graph_id, start_id, end_id))
			{
				const ImVec4 success_color = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);	//成功時のガイド線の色
				constexpr float line_thickness = 2.0f;

				//接続を確定したか判定
				if (ed::AcceptNewItem(success_color, line_thickness))
				{
					GraphLink new_link;	//新しいリンク
					new_link.id = data_manager->FetchAndIncrementId();
					new_link.start_pin_id = start_id;
					new_link.end_pin_id = end_id;
					current_graph->links.push_back(new_link);
					OnLinkCreated(current_graph, new_link);
				}
			}
			else
			{
				const ImVec4 reject_color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);	//接続不可時のガイドライン線の色
				constexpr float reject_thickness = 2.0f;	//プレビュー線の太さ
				ed::RejectNewItem(reject_color, reject_thickness);
			}
		}
	}
	ed::EndCreate();
}

//ピン同士が接続ルールに準拠しているか判定
bool StateLinkConnectionHandler::CanConnect(
	StateGraphDataManager* data_manager,	//データ管理インスタンス
	uint32_t graph_id,						//階層ID
	uint32_t start_pin_id,					//接続元ピンID
	uint32_t end_pin_id)					//接続先ピンID
{
	const PinData* start_pin = nullptr;	//接続元のピンポインタ
	const PinData* end_pin = nullptr;		//接続先のピンポインタ
	const auto& layers = data_manager->GetGraphDatas();	//全階層データ

	//全階層を巡回
	for (size_t g_idx = 0; g_idx < layers.size(); g_idx++)
	{
		//現在の階層IDと一致するか判定
		if (layers[g_idx].id != graph_id)
		{
			continue;
		}

		//階層内の全ノードからピンを検索
		for (size_t n_idx = 0; n_idx < layers[g_idx].nodes.size(); n_idx++)
		{
			const GraphNode* node = layers[g_idx].nodes[n_idx].get();	//参照ノード
			
			//入力ピンから検索
			for (size_t p_idx = 0; p_idx < node->GetInputPins().size(); p_idx++)
			{
				PinData pin = node->GetInputPins()[p_idx];
				if (pin.pin_id == start_pin_id) start_pin = &pin;
				if (pin.pin_id == end_pin_id) end_pin = &pin;
			}

			//出力ピンから検索
			for (size_t p_idx = 0; p_idx < node.outputs.size(); p_idx++)
			{
				if (node.outputs[p_idx].id == start_pin_id)start_pin = &node.outputs[p_idx];
				if (node.outputs[p_idx].id == end_pin_id)end_pin = &node.outputs[p_idx];
			}
		}
		break;
	}
	
	//---------------------------------
	//接続ルールのバリデーション判定
	//---------------------------------
	//両方のピンが存在するか確認
	if (!start_pin || !end_pin)
	{
		return false;
	}

	//同一ノード内のピン同士か判定
	if (start_pin->node_id == end_pin->node_id)
	{
		return false;
	}

	//入力同士、または出力同士か判定
	if (start_pin->kind == end_pin->kind)
	{
		return false;
	}

	//逆方向接続か判定
	if (start_pin->kind == PinKind::Input && end_pin->kind == PinKind::Output)
	{
		return false;
	}
	
	return true;
}

//リンク作成完了時にピンから所属ノードを特定・検証
void StateLinkConnectionHandler::OnLinkCreated(
	const GraphData* current_graph,			//編集中のグラフデータ
	const GraphLink& new_link				//新規作成リンクデータ
)
{
	uint32_t source_node_id = 0;	//接続元ノードID
	uint32_t target_node_id = 0;	//接続先ノードID

	//-------------------------------------------------------
	//グラフ内の全ノードから対応するピンの親ノードを逆引き
	//-------------------------------------------------------
	//全ノードを巡回
	for (size_t n_idx = 0; n_idx < current_graph->nodes.size(); n_idx++)
	{
		const GraphNode& node = current_graph->nodes[n_idx];	//参照ノード

		//出力ピンを巡回
		for (size_t p_idx = 0; p_idx < node.outputs.size(); p_idx)
		{
			//開始ピンと一致するか判定
			if (node.outputs[p_idx].id == new_link.start_pin_id)
			{
				source_node_id = node.id;
				break;
			}
		}

		//入力ピンを巡回
		for (size_t n_idx = 0; n_idx < node.inputs.size(); n_idx++)
		{
			//終了ピンと一致するか判定
			if (node.inputs[n_idx].id == new_link.end_pin_id)
			{
				target_node_id = node.id;
				break;
			}
		}
	}
	//いずれの親ノードが見つからなかったか判定
	if (source_node_id == 0 || target_node_id == 0)
	{
		printf("Error: StateLinkConnectionHandler::OnLinkCreated - 接続されたピンに対応する親ノードが見つかりませんでした。\n");
		return;
	}
	printf("StateLinkConnectionHandler: リンクを正常に作成しました。ID: %u (ノード: %u -> %u)\n",
		new_link.id, source_node_id, target_node_id);
}
