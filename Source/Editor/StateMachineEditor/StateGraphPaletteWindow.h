#pragma once

#include <string>
#include <vector>
#include <cstdint>

class StateGraphDataManager;
struct GraphData;

class StateGraphPaletteWindow
{
public:
	//コンストラクタ
	StateGraphPaletteWindow() = default;

	//デストラクタ
	~StateGraphPaletteWindow() = default;

	//パレットウィンドウの全体描画
	void DrawPalette(StateGraphDataManager* data_manager, GraphData* current_graph, uint32_t& out_focus_node_id);

	//追加予約されたノードがあるか取得
	bool HasPendingAddNode()const { return !pending_add_palette_node_name.empty(); }

	//追加予約されたノード名を取得
	std::string GetPendingNodeName()const { return pending_add_palette_node_name; }

	//追加予約がサブグラフか取得
	bool IsPendingSubGraph()const { return pending_add_is_sub_graph; }

	//追加予約のクリア
	void ClearPendingNode() { pending_add_palette_node_name = ""; pending_add_is_sub_graph = false; }

private:
	//階層ノードタブを描画
	void DrawHierarchyNodeList(GraphData* current_graph, uint32_t& out_focus_node_id);

	//パレット切り替えフィルターボタン描画
	void DrawPaletterFilterButtons();

	//通常ステートのパレット項目描画
	void DrawNormalStatePalette(StateGraphDataManager* data_manager, float button_offset_x);

	//サブグラフのパレット項目描画
	void DrawSubGraphPalette(StateGraphDataManager* data_manager, float button_offset_x);

	//通常ノード名を全データから取得
	std::vector<std::string> GetExistingNormalStateNames(StateGraphDataManager* data_manager);

	//サブグラフ名を全データから取得
	std::vector<std::string> GetExistingSubGraphNames(StateGraphDataManager* data_manager);

private:
	//パレットの表示切り替え状態
	enum class PaletteFilter
	{
		ALL,		//すべて描画
		SubGraph,	//サブグラフのみ描画
		Normal		//通常ステートのみ描画
	};

private:
	PaletteFilter current_filter = PaletteFilter::ALL;	//現在のパレットフィルター
	std::string pending_add_palette_node_name = "";		//パレットから追加予約されたステート名
	bool pending_add_is_sub_graph = false;				//サブグラフかのフラグ
};

