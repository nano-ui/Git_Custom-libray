#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <memory>
#include <DirectXMath.h>

struct GraphNode;

//判定ノードの種類
enum class ConditionNodeType
{
	NormalCompare,	//通常の比較
	Random,			//ランダム判定
	Distance,		//距離判定
	Ratio,			//割合判定
	InputCheck,		//入力判定
	AnimationEnd,	//アニメーション終了判定
};

//遷移条件の編集・保持
struct GraphTransitionCondition
{
	ConditionNodeType type = ConditionNodeType::NormalCompare;	//判定の種類
	uint32_t hash_key = 0;			//対象のキー
	float reference_value = 0.0f;	//基準値
	int compare_operator = 0;		//比較演算子識別番号
	float param_second = 0.0f;		//第2引数パラメータ
	uint32_t secondary_hash = 0;	//比較対象のハッシュキー
	DirectX::XMFLOAT3 vector_reference_value = { 0.0f, 0.0f, 0.0f }; // XMFLOAT3用の比較基準値
};

//ノードを繋ぐ線の情報
struct GraphLink
{
	uint32_t id;			//線の固有ID
	uint32_t start_pin_id;	//接続元の出力ピンID
	uint32_t end_pin_id;	//接続先の入力ピンID
	std::vector<GraphTransitionCondition> conditions;	//遷移条件リスト
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

	//サブグラフの追加
	uint32_t CreateSubGraph(const std::string& name);

	//指定されたノードIDが所属する階層のIDを検索して取得
	uint32_t GetGraphIdFromNodeId(uint32_t node_id);

	//指定されたピンIDが所属している親ノードのIDを逆引き取得
	uint32_t GetNodeIdFromPinId(uint32_t graph_id, uint32_t pin_id);

	//指定されたノードIDを出発基とする全てのリンクのポインタを取得
	std::vector<GraphLink*> GetLinkesFromNode(uint32_t graph_id, uint32_t node_id);

	//グラフ情報の取得
	const GraphData* GetGraphData(uint32_t graph_id)const;

protected:
	//グラフ情報を検索
	GraphData* FindGraphData(uint32_t graph_id);

	//グラフ情報を検索
	const GraphData* FindGraphData(uint32_t graph_id) const;

	//階層構造を上に辿って循環参照
	bool IsAncestorGraph(uint32_t target_graph_id, uint32_t candidate_graph_id);

protected:
	std::vector<GraphData> graph_datas;
	uint32_t next_id;
};

