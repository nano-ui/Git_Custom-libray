#include "Editor\StateMachineEditor\Data\StateGraphDataManager.h"
#include "Editor\StateMachineEditor\Nodes\StateGraphNode.h"
#include "Serialization\JsonSerializer.h"

#include <cstdio>
#include <fstream>
#include <iomanip>

//コンストラクタ
StateGraphDataManager::StateGraphDataManager()
{
}

//ファイルに保存
void StateGraphDataManager::SaveToFile(const std::string& file_path)
{
	nlohmann::json root_json;	//JSONのルートオブジェクト
	root_json["NextID"] = next_id; // IDカウンターの保存
	root_json["TargetModelPath"] = target_model_path;	//モデルパスを保存
	nlohmann::json layers_array = nlohmann::json::array();	//全階層情報を格納する配列オブジェクト

	//全階層情報を走査してパッキング
	for (size_t g = 0; g < graph_datas.size(); g++)
	{
		const GraphData& graph = graph_datas[g];	//現在の階層データ
		nlohmann::json graph_json;	//一つの階層情報を格納するJSONオブジェクト
		graph_json["GraphID"] = graph.id; // 階層IDの保存
		graph_json["GraphName"] = graph.name; // 階層名の保存

		nlohmann::json nodes_array = nlohmann::json::array();	//ノードデータを格納する一時配列

		//現在の階層のすべてのノードを走査
		for (size_t n = 0; n < graph.nodes.size(); n++)
		{
			GraphNode* node = graph.nodes[n].get();
			if (!node)
			{
				continue;
			}

			// JsonSerializer を介してノード自身の保存関数を利用
			JsonSerializer serializer;
			node->SetupSerializer(&serializer);

			nlohmann::json node_json;
			serializer.SaveToObject(node_json);

			nodes_array.push_back(node_json); // 階層用ノード配列へプッシュ
		}
		graph_json["Nodes"] = nodes_array;

		nlohmann::json links_array = nlohmann::json::array();	//接続線を格納する一時配列

		//現在の階層の全てのリンクを走査
		for (size_t l = 0; l < graph.links.size(); l++)
		{
			const GraphLink& link = graph.links[l];	//ループ対象のリンクデータ
			nlohmann::json link_json;	//単一リンク格納用のJSON

			link_json["ID"] = link.id; // リンク固有ID
			link_json["StartPinID"] = link.start_pin_id; // 開始ピンID
			link_json["EndPinID"] = link.end_pin_id; // 終了ピンID

			//リンクが抱える全ての遷移条件をパッキング
			nlohmann::json conds_array = nlohmann::json::array();	//条件式格納用の配列
			for (const auto& cond : link.conditions)
			{
				nlohmann::json cond_json;	//単一条件格納用のJSON
				cond_json["Type"] = static_cast<int>(cond.type); 
				cond_json["HashKey"] = cond.hash_key;
				cond_json["RefValue"] = cond.reference_value; 
				cond_json["CompOp"] = cond.compare_operator;
				cond_json["ParamSecond"] = cond.param_second; 
				cond_json["SecondaryHash"] = cond.secondary_hash;
				cond_json["VectorRefValue"] = { cond.vector_reference_value.x, cond.vector_reference_value.y, cond.vector_reference_value.z };
				conds_array.push_back(cond_json); 
			}
			link_json["Conditions"] = conds_array; 
			links_array.push_back(link_json);
		}
		graph_json["Links"] = links_array;
		layers_array.push_back(graph_json);
	}
	root_json["Layers"] = layers_array;

	//物理ファイルへの書き出し処理
	std::ofstream file_stream(file_path); // 保存用ファイルストリーム
	if (file_stream.is_open()) 
	{
		const int indent_space_count = 4; // インデント用のスペース幅定数
		file_stream << std::setw(indent_space_count) << root_json << std::endl;
		printf("StateGraphDataManager: グラフデータをファイル「%s」へ正常に保存しました。\n", file_path.c_str());
	}
	else
	{
		// 意図しない挙動（ファイルが開けない）が発生した場合のエラーログ
		printf("Error: SaveToFile - ファイル「%s」を開けませんでした。\n", file_path.c_str());
	}
}

//ファイル読み込み
bool StateGraphDataManager::LoadFromFile(const std::string& file_path)
{
	std::ifstream file_stream(file_path);	//ロード用ファイルストリーム

	//ファイルが存在しない、または開けないか判定
	if (!file_stream.is_open())
	{
		printf("Warning: LoadFromFile - 「%s」が存在しないため新規作成用の状態を維持します。\n", file_path.c_str());
		return false;
	}

	nlohmann::json root_json;	//パース用ルートオブジェクト
	file_stream >> root_json; // JSONの一括パース

	//データ破損チェック
	if (root_json.find("NextID") == root_json.end() || root_json.find("Layers") == root_json.end())
	{
		printf("Error: LoadFromFile - 「%s」のデータ構造が不正です。\n", file_path.c_str());
		return false;
	}

	graph_datas.clear();
	next_id = root_json["NextID"];

	//JSON内にモデルパスのキーが存在するか確認
	if (root_json.find("TargetModelPath") != root_json.end())
	{
		target_model_path = root_json["TargetModelPath"].get<std::string>();
	}
	else
	{
		target_model_path = "";
	}

	//階層情報の展開・復元
	for (const auto& graph_json : root_json["Layers"])
	{
		// Layers の中に Nodes キーが正しく書き込まれているか事前精査 
		if (graph_json.find("Nodes") == graph_json.end() || graph_json.find("Links") == graph_json.end())
		{
			printf("Error: LoadFromFile - 階層データのキー構造が壊れているため復元をスキップします。\n"); // 原因の可視化
			return false;
		}

		GraphData graph;	//復元先の階層インスタンス
		graph.id = graph_json["GraphID"];
		graph.name = graph_json["GraphName"];

		//ノード群の復元展開
		for (const auto& node_json : graph_json["Nodes"])
		{
			// StateGraphNode インスタンスを生成し、復元処理を実行
			std::unique_ptr<StateGraphNode> state_node = std::make_unique<StateGraphNode>();

			JsonSerializer serializer;
			state_node->SetupSerializer(&serializer);
			serializer.LoadFromObject(node_json);

			graph.nodes.push_back(std::move(state_node));
		}

		//接続線リンク及び遷移条件の復元展開
		for (const auto& link_json : graph_json["Links"])
		{
			GraphLink link;	//復元先のリンクインスタンス
			link.id = link_json["ID"];
			link.start_pin_id = link_json["StartPinID"];
			link.end_pin_id = link_json["EndPinID"];

			//各条件式の復元
			for (const auto& cond_json : link_json["Conditions"])
			{
				GraphTransitionCondition cond; // 条件式構造体 
				cond.type = static_cast<ConditionNodeType>(cond_json["Type"].get<int>());
				cond.hash_key = cond_json["HashKey"];
				cond.reference_value = cond_json["RefValue"];
				cond.compare_operator = cond_json["CompOp"];
				cond.param_second = cond_json["ParamSecond"];

				if (cond_json.find("SecondaryHash") != cond_json.end())
				{
					cond.secondary_hash = cond_json["SecondaryHash"];
				}

				if (cond_json.contains("VectorRefValue") && cond_json["VectorRefValue"].is_array() && cond_json["VectorRefValue"].size() == 3)
				{
					cond.vector_reference_value.x = cond_json["VectorRefValue"][0].get<float>();
					cond.vector_reference_value.y = cond_json["VectorRefValue"][1].get<float>();
					cond.vector_reference_value.z = cond_json["VectorRefValue"][2].get<float>();
				}

				link.conditions.push_back(cond);
			}
			graph.links.push_back(link);
		}
		graph_datas.push_back(std::move(graph)); 
	}
	printf("StateGraphDataManager: ファイル「%s」から全階層データを正常に復元ロードしました。\n", file_path.c_str());
	return true;
}

//ステートノード追加
uint32_t StateGraphDataManager::AddStateNode(uint32_t graph_id, DirectX::XMFLOAT2 click, const std::string name = u8"新規ステート")
{
	NodeBasicData basic_data = {};
	basic_data.id = FetchAndIncrementId();
	basic_data.name = name;
	basic_data.position = click;
	basic_data.is_sub_graph = false;
	basic_data.sub_graph_id = 0;
	basic_data.node_type = GraphNodeType::StateNode;

	std::unique_ptr<StateGraphNode> state_node = std::make_unique<StateGraphNode>();
}

//階層が空の場合に初期ノードを構築
void StateGraphDataManager::CheckAndInitDefaultNode(uint32_t graph_id)
{
	// 初期ノードの構築判定と生成
	for (size_t g = 0; g < graph_datas.size(); g++)
	{
		if (graph_datas[g].id != graph_id)
		{
			continue;
		}

		// 既にノードが存在する場合はスキップ
		if (!graph_datas[g].nodes.empty())
		{
			return;
		}

		// ノードの基本パラメータを構築
		NodeBasicData basic_data = {};
		basic_data.id = FetchAndIncrementId();
		basic_data.name = u8"待機状態";
		basic_data.position = { 100.0f, 100.0f };
		basic_data.is_sub_graph = false;
		basic_data.sub_graph_id = 0;
		basic_data.node_type = GraphNodeType::StateNode;

		// StateGraphNodeのインスタンスを生成
		std::unique_ptr<StateGraphNode> graph_node = std::make_unique<StateGraphNode>();
		// 基本データの設定とピンのセットアップ(仮想関数SetupPinsが実行される)
		graph_node->Initialize(basic_data);

		// 基底クラス管理のノード配列へ所有権を移動して追加
		graph_datas[g].nodes.push_back(std::move(graph_node));

		printf("StateGraphDataManager: 階層ID %d に初期ノード(待機状態)を作成しました。\n", graph_id);
		return;
	}
}

//遷移条件を追加
void StateGraphDataManager::AddConditionToLink(uint32_t graph_id, uint32_t link_id)
{
	//全ての階層情報を巡回して指定の階層を特定
	for (size_t g = 0; g < graph_datas.size(); g++)
	{
		//階層IDが一致しているか確認
		if (graph_datas[g].id == graph_id)
		{
			//階層内の全ての接続線を走査
			for (size_t l = 0; l < graph_datas[g].links.size(); l++)
			{
				//リンクIDが対象と一致したか判定
				if (graph_datas[g].links[l].id == link_id)
				{
					GraphTransitionCondition new_condition;	//新しい条件情報
					new_condition.hash_key = 0;
					new_condition.reference_value = 0.0f;
					new_condition.compare_operator = 0;

					graph_datas[g].links[l].conditions.push_back(new_condition);
					printf("StateGraphDataManager: 階層ID %d のリンクID %d に新しい遷移条件を追加しました。\n", graph_id, link_id);
					return;
				}
			}
		}
	}
	printf("Error: AddConditionToLink - 指定された階層ID %d またはリンクID %d が見つかりませんでした。\n", graph_id, link_id);
}

//遷移条件を削除
void StateGraphDataManager::DeleteConditionFromLink(uint32_t graph_id, uint32_t link_id, size_t condition_index)
{
	//全ての階層情報を巡回して指定の階層を特定
	for (size_t g = 0; g < graph_datas.size(); g++)
	{
		//階層IDが一致しているか確認
		if (graph_datas[g].id == graph_id)
		{
			//階層内の全ての接続線を走査
			for (size_t l = 0; l < graph_datas[g].links.size(); l++)
			{
				//リンクIDが対象と一致したか判定
				if (graph_datas[g].links[l].id == link_id)
				{
					//配列の範囲外か判定
					if (condition_index >= graph_datas[g].links[l].conditions.size())
					{
						printf("Error: DeleteConditionFromLink - インデックス %zu が範囲外です。\n", condition_index); // デバッグ出力
						return;
					}
					auto target_iterator = graph_datas[g].links[l].conditions.begin() + condition_index;	//削除対象のイテレーター
					graph_datas[g].links[l].conditions.erase(target_iterator);
					printf("StateGraphDataManager: 階層ID %d のリンクID %d から条件インデックス %zu を削除しました。\n", graph_id, link_id, condition_index);
					return;
				}
			}
		}
	}
	printf("Error: DeleteConditionFromLink - 指定された階層ID %d またはリンクID %d が見つかりませんでした。\n", graph_id, link_id);
}

//サブグラフノードの生成
void StateGraphDataManager::AddSubGrapNode(uint32_t graph_id, float click_x, float click_y, const std::string& name)
{
	std::string sub_graph_name = name;
	const GraphData* src_graph = nullptr;

	for (size_t g = 0; g < graph_datas.size(); g++)
	{
		if (graph_datas[g].name == sub_graph_name)
		{
			src_graph = &graph_datas[g];
			break;
		}
	}

	// 循環参照チェック
	if (src_graph && IsAncestorGraph(graph_id, src_graph->id))
	{
		printf("Error: AddSubGrapNode - 上位階層「%s」(ID:%u) を下位階層(ID:%u) 内に配置することは循環参照となるため禁止されています。\n",
			sub_graph_name.c_str(), src_graph->id, graph_id);
		return;
	}

	uint32_t real_sub_graph_id = CreateSubGraph(sub_graph_name);

	for (size_t g = 0; g < graph_datas.size(); g++)
	{
		if (graph_datas[g].id != real_sub_graph_id && graph_datas[g].name == sub_graph_name)
		{
			src_graph = &graph_datas[g];
			break;
		}
	}

	// コピー元の階層が存在する場合は内部ノード・リンクを複製
	if (src_graph)
	{
		GraphData* dst_graph = &graph_datas.back();
		std::unordered_map<uint32_t, uint32_t> pin_id_map;

		for (size_t n = 0; n < src_graph->nodes.size(); n++)
		{
			const StateGraphNode* src_state_node = dynamic_cast<const StateGraphNode*>(src_graph->nodes[n].get());
			if (!src_state_node)
			{
				continue;
			}

			NodeBasicData copied_basic = src_state_node->GetNodeBasicData();
			copied_basic.id = FetchAndIncrementId();
			if (copied_basic.is_sub_graph)
			{
				copied_basic.sub_graph_id = CreateSubGraph(copied_basic.name);
			}
			else
			{
				constexpr uint32_t default_sub_id = 0;
				copied_basic.sub_graph_id = default_sub_id;
			}

			std::unique_ptr<StateGraphNode> copied_node = std::make_unique<StateGraphNode>();
			copied_node->Initialize(copied_basic);
			copied_node->SetAnimationData(src_state_node->GetAnimationData());
			copied_node->SetLinkColor(src_state_node->GetLinkColor());
			copied_node->SetActionCategory(src_state_node->GetActionCategory());

			// ピンの複製とIDマッピング
			for (const auto& pin : src_state_node->GetInputPins())
			{
				PinData new_pin;
				new_pin.pin_id = FetchAndIncrementId();
				new_pin.pin_name = pin.pin_name;
				new_pin.pin_type = pin.pin_type;
				copied_node->SetInputPin(new_pin);
				pin_id_map[pin.pin_id] = new_pin.pin_id;
			}
			for (const auto& pin : src_state_node->GetOutputPins())
			{
				PinData new_pin;
				new_pin.pin_id = FetchAndIncrementId();
				new_pin.pin_name = pin.pin_name;
				new_pin.pin_type = pin.pin_type;
				copied_node->SetOutputPin(new_pin);
				pin_id_map[pin.pin_id] = new_pin.pin_id;
			}

			dst_graph->nodes.push_back(std::move(copied_node));
		}

		// リンクの複製
		for (size_t l = 0; l < src_graph->links.size(); l++)
		{
			const GraphLink& src_link = src_graph->links[l];
			GraphLink copied_link;
			copied_link.id = FetchAndIncrementId();

			constexpr uint32_t invalid_id = 0;
			copied_link.start_pin_id = invalid_id;
			copied_link.end_pin_id = invalid_id;

			auto start_it = pin_id_map.find(src_link.start_pin_id);
			if (start_it != pin_id_map.end())
			{
				copied_link.start_pin_id = start_it->second;
			}
			auto end_it = pin_id_map.find(src_link.end_pin_id);
			if (end_it != pin_id_map.end())
			{
				copied_link.end_pin_id = end_it->second;
			}

			if (copied_link.start_pin_id != invalid_id && copied_link.end_pin_id != invalid_id)
			{
				copied_link.conditions = src_link.conditions;
				dst_graph->links.push_back(copied_link);
			}
		}
	}

	// 現在の階層へ親サブグラフノードを生成
	NodeBasicData sub_node_data = {};
	sub_node_data.id = FetchAndIncrementId();
	sub_node_data.name = sub_graph_name;
	sub_node_data.position = { click_x, click_y };
	sub_node_data.is_sub_graph = true;
	sub_node_data.sub_graph_id = real_sub_graph_id;
	sub_node_data.node_type = GraphNodeType::StateNode;

	std::unique_ptr<StateGraphNode> new_node = std::make_unique<StateGraphNode>();
	new_node->Initialize(sub_node_data);

	for (size_t g = 0; g < graph_datas.size(); g++)
	{
		if (graph_datas[g].id == graph_id)
		{
			graph_datas[g].nodes.push_back(std::move(new_node));
			printf("StateGraphDataManager: サブグラフノードを配置しました。ID: %u\n", sub_node_data.id);
			return;
		}
	}
	printf("Error: AddSubGrapNode - 指定された階層ID %d が見つかりませんでした。\n", graph_id);
}

//既存のノードをサブグラフに変換
void StateGraphDataManager::ConvertToSubGraph(uint32_t graph_id, uint32_t node_id)
{
	for (size_t g = 0; g < graph_datas.size(); g++)
	{
		if (graph_datas[g].id != graph_id)
		{
			continue;
		}

		for (size_t n = 0; n < graph_datas[g].nodes.size(); n++)
		{
			GraphNode* target_node = graph_datas[g].nodes[n].get();
			if (!target_node || target_node->GetNodeBasicData().id != node_id)
			{
				continue;
			}

			NodeBasicData basic = target_node->GetNodeBasicData();
			if (basic.is_sub_graph)
			{
				printf("StateGraphDataManager: ノード ID:%d は既にサブグラフです。\n", node_id);
				return;
			}

			std::string original_name = basic.name;
			uint32_t new_sub_graph_id = CreateSubGraph(original_name);
			basic.is_sub_graph = true;
			basic.sub_graph_id = new_sub_graph_id;
			basic.name = original_name + u8"サブステート";
			target_node->SetNodeBasicData(basic);

			printf("StateGraphDataManager: ノード「%s」(ID:%d) をサブグラフ(階層ID:%d)へ変換完了。\n",
				basic.name.c_str(), node_id, new_sub_graph_id);
			return;
		}
	}
}
