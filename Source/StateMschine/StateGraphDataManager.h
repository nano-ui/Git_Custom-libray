#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <unordered_map>
#include <../Serialization/json.hpp>

//ピンの種類
enum class PinKind
{
	Input,	//入力
	Output	//出力
};

//ノードの端子の情報
struct GraphPin
{
	uint32_t id;		//固有ID
	std::string name;	//ピン名
	PinKind kind;		//ピンの種類
	uint32_t node_id;	//所属しているノードID
};

//ステート情報
struct GraphNode
{
	uint32_t id;					//ノードの固有ID
	std::string name;				//ステート名
	float position_x;				//X座標
	float position_y;				//Y座標
	std::vector<GraphPin> inputs;	//入力ピンのリスト
	std::vector<GraphPin> outputs;	//出力ピンのリスト
	bool is_sub_graph;				//階層型ステートマシンのグラフ
	uint32_t sub_graph_id;			//下位階層のグラフID
	uintptr_t state_runtime_address = 0;	//ステートのメモリ番地
};

//判定ノードの種類
enum class ConditionNodeType
{
	NormalCompare,	//通常の比較
	Random,			//ランダム判定
	Distance,		//距離判定
	Ratio,			//割合判定
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
	uint32_t id;					//グラフのID
	std::string name;				//階層名
	std::vector<GraphNode> nodes;	//階層に存在するノード群
	std::vector<GraphLink> links;	//階層に存在する接続線群
};

class StateGraphDataManager
{
public:
	//コンストラクタ
	StateGraphDataManager();

	//デストラクタ
	~StateGraphDataManager() = default;

	//ファイルに保存
	void SaveToFile(const std::string& file_path);

	//ファイル読み込み
	bool LoadFromFile(const std::string& file_path);

	//ノードの生成
	void AddNode(GraphData* current_graph, float click_x, float click_y, const std::string& node_name = u8"新規ステート");

	//サブグラフノードの生成
	void AddSubGrapNode(uint32_t graph_id, float click_x, float click_y, const std::string& name = u8"新規サブグラフ");

	//既存のノードをサブグラフに変換
	void ConvertToSubGraph(uint32_t graph_id, uint32_t node_id);

	//下位階層データを生成してIDを返す
	uint32_t CreateNewSubGraph(const std::string& name);

	//階層が空の場合に初期ノードを構築
	void CheckAndInitDefaultNode(uint32_t graph_id);

	//ピンIDを受け取り、接続ルールに準拠しているか判定
	bool CheckCanConnect(uint32_t graph_id, uint32_t start_pin_id, uint32_t end_pin_id);

	//ノード削除
	void DeleteNode(uint32_t graph_id, uint32_t target_node_id);

	//リンクの削除
	void DeleteLink(uint32_t graph_id, uint32_t target_link_id);

	//遷移条件を追加
	void AddConditionToLink(uint32_t graph_id, uint32_t link_id);

	//遷移条件を削除
	void DeleteConditionFromLink(uint32_t graph_id, uint32_t link_id, size_t condition_index);

	//全ての階層リストを取得
	std::vector<GraphData>& GetLayerDatas() { return layer_datas; }

	//指定されたピンIDが所属している親ノードのIDを逆引き取得
	uint32_t GetNodeIdFromPinId(uint32_t graph_id, uint32_t pin_id);

	//IDカウンターの参照と更新
	uint32_t FetchAndIncrementId() { return next_id++; }

private:
	std::vector<GraphData> layer_datas;	//全ての階層データのリスト
	uint32_t next_id;					//全ての要素の割り当てIDカウンター
};

