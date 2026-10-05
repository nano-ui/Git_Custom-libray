#pragma once

#include <cstdint>

class StateGraphDataManager;
struct GraphData;
struct GraphLink;

class StateLinkConnectionHandler
{
public:
	//コンストラクタ
	StateLinkConnectionHandler() = default;

	//デストラクタ
	~StateLinkConnectionHandler() = default;

	//接続先の作成クエリを検知してデータに追加
	void HandleLinkCreation(
		StateGraphDataManager* data_manager,	//データ管理インスタンス
		GraphData* current_graph,				//編集中のグラフデータ
		uint32_t current_graph_id				//表示中の階層ID
	);

private:
	//ピン同士が接続ルールに準拠しているか判定
	bool CanConnect(
		StateGraphDataManager* data_manager,	//データ管理インスタンス
		uint32_t graph_id,						//階層ID
		uint32_t start_pin_id,					//接続元ピンID
		uint32_t end_pin_id						//接続先ピンID
	);

	//リンク作成完了時にピンから所属ノードを特定・検証
	void OnLinkCreated(
		const GraphData* current_graph,			//編集中のグラフデータ
		const GraphLink& new_link				//新規作成リンクデータ
	);
};

