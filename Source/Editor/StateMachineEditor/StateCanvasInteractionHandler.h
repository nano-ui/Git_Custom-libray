#pragma once

#include <cstdint>
#include <string>
#include <imgui.h>
#include <imgui_node_editor.h>

class StateGraphDataManager;
class StateGraphPaletteWindow;
class GraphData;

class StateCanvasInteractionHandler
{
public:
	StateCanvasInteractionHandler() = default;
	~StateCanvasInteractionHandler() = default;

	//コンテキストメニューの受付とノード追加・変換処理
	void HandleContextMenu(
		StateGraphDataManager* data_manager,	//データ管理インスタンスへのポインタ
		GraphData* current_graph,				//グラフデータへの参照ポインタ
		uint32_t current_graph_id				//表示中の階層ID
	);

	//パレットウィンドウに保留されている追加要求ノードをキャンパスに配置
	void HandlePendingPaletteNode(
		StateGraphDataManager* data_manager,		//データ管理インスタンスへのポインタ
		StateGraphPaletteWindow* palette_window,	//パレットウィンドウへのポインタ
		GraphData*& current_graph,					//編集中のグラフデータへの参照ポインタ
		uint32_t current_graph_id					//表示中の階層ID
	);

	//ノード及び「リンクの削除要求処理
	void HandleDeletion(
		StateGraphDataManager* data_manager,	//データ管理インスタンスへのポインタ
		GraphData* current_graph,				//表示中のグラフデータへの参照ポインタ
		uint32_t current_graph_id				//表示中の階層ID
	);

	//パレットからのドラッグ&ドロップ受け取り処理
	void HandleDragAndDrop(
		StateGraphDataManager* data_manager,	//データ管理インスタンスへのポインタ
		GraphData*& current_graph,				//編集中のグラフデータへの参照ポインタ
		uint32_t current_graph_id				//表示中の階層ID
	);

private:
	//ノードの削除処理
	void DeleteNode(
		StateGraphDataManager* data_manager,	//データ管理インスタンスへのポインタ
		GraphData* current_graph,				//編集中のグラフデータへの参照ポインタ
		uint32_t current_graph_id				//表示中の階層ID
	);

	//リンクの削除処理
	void DeleteLink(
		StateGraphDataManager* data_manger,		//データ管理インスタンスへのポインタ
		GraphData* current_graph,				//編集中のグラフデータへの参照ポインタ
		uint32_t current_graph_id				//表示中の階層ID
	);
};

