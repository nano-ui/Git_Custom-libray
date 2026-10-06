#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <memory>

class GraphNode;

//ノードを繋ぐ線の情報
struct GraphLink
{
	uint32_t id;			//線の固有ID
	uint32_t start_pin_id;	//接続元の出力ピンID
	uint32_t end_pin_id;	//接続先の入力ピンID
};

//階層の情報
struct GraphData
{
	std::vector<std::unique_ptr<GraphNode>> nodes;	//ノード群
	uint32_t id;					//グラフのID
	std::string name;				//階層名
	std::vector<GraphLink> links;	//階層に存在する接続線群
};

class GraphDataManager
{
public:
	//コンストラクタ
	GraphDataManager();

	//デストラクタ
	virtual ~GraphDataManager() = default;

	//ID発行処理
	uint32_t FetchAndIncrementId();

	//接続判定処理
	bool CheckCanConnect(uint32_t graph_id, uint32_t start_pin_id, uint32_t end_pin_id);

	//ノードの追加
	bool AddNode(uint32_t graph_id, std::unique_ptr<GraphNode> graph_node);

	//ノードの削除
	bool DeleteNode(uint32_t graph_id, uint32_t node_id);

	//リンクの生成
	bool CreateLink(uint32_t graph_id, uint32_t start_pin_id, uint32_t end_pin_id);

	//リンクの削除
	void DeleteLink(uint32_t graph_id, uint32_t target_link_id);

	//指定されたノードIDが所属する階層のIDを検索して取得
	uint32_t GetGraphIdFromNodeId(uint32_t node_id);

	//指定されたピンIDが所属している親ノードのIDを逆引き取得
	uint32_t GetNodeIdFromPinId(uint32_t graph_id, uint32_t pin_id);

	//グラフ情報の取得
	const GraphData* GetGraphData(uint32_t graph_id)const;

protected:
	//グラフ情報を検索
	GraphData* FindGraphData(uint32_t graph_id);

	//グラフ情報を検索
	const GraphData* FindGraphData(uint32_t graph_id) const;

protected:
	std::vector<GraphData> layer_datas;
	uint32_t next_id;
};

