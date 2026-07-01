#include "StateGraphDataManager.h"

#include <cstdio>
#include <fstream>
#include <iomanip>

//コンストラクタ
StateGraphDataManager::StateGraphDataManager()
{
	next_id = 100;
	GraphData root_graph;	//ルート階層情報
	root_graph.id = 0;
	root_graph.name = u8"ルート";
	layer_datas.push_back(root_graph);
}

//ファイルに保存
void StateGraphDataManager::SaveToFile(const std::string& file_path)
{
	nlohmann::json root_json;	//JSONのルートオブジェクト
	root_json["NextID"] = next_id; // IDカウンターの保存
	root_json["TargetModelPath"] = target_model_path;	//モデルパスを保存
	nlohmann::json layers_array = nlohmann::json::array();	//全階層情報を格納する配列オブジェクト

	//全階層情報を走査してパッキング
	for (size_t g = 0; g < layer_datas.size(); g++)
	{
		const GraphData& graph = layer_datas[g];	//現在の階層データ
		nlohmann::json graph_json;	//一つの階層情報を格納するJSONオブジェクト
		graph_json["GraphID"] = graph.id; // 階層IDの保存
		graph_json["GraphName"] = graph.name; // 階層名の保存

		nlohmann::json nodes_array = nlohmann::json::array();	//ノードデータを格納する一時配列

		//現在の階層のすべてのノードを走査
		for (size_t n = 0; n < graph.nodes.size(); n++)
		{
			const GraphNode& node = graph.nodes[n];	//ループ対象のノードデータ
			nlohmann::json node_json;	//単一ノード格納用のJSON

			node_json["ID"] = node.id; // ノードID
			node_json["Name"] = node.name; // ステート名
			node_json["PosX"] = node.position_x; // 座標X
			node_json["PosY"] = node.position_y; // 座標Y
			node_json["IsSubGraph"] = node.is_sub_graph; // サブグラフフラグ
			node_json["SubGraphID"] = node.sub_graph_id; // 紐付く下位階層ID
			node_json["ActionCategory"] = node.action_category;	// アクション値
			node_json["AnimationName"] = node.animation_name;	// アニメ名
			node_json["IsLoop"] = node.is_loop;					// 再生フラグ
			node_json["LinkColorR"] = node.link_color_r;
			node_json["LinkColorG"] = node.link_color_g;
			node_json["LinkColorB"] = node.link_color_b;

			//入力ピンのシリアライズ
			nlohmann::json inputs_array = nlohmann::json::array(); // 入力ピン用の一時配列
			for (const auto& pin : node.inputs)
			{
				inputs_array.push_back({ {"ID", pin.id}, {"Name", pin.name}, {"Kind", static_cast<int>(pin.kind)} });
			}
			node_json["Input"] = inputs_array; // ノードデータにバインド

			//出力ピンのシリアライズ
			nlohmann::json output_array = nlohmann::json::array(); // 出力ピン用の一時配列
			for (const auto& pin : node.outputs)
			{
				output_array.push_back({ {"ID", pin.id}, {"Name", pin.name}, {"Kind", static_cast<int>(pin.kind)} });
			}
			node_json["Output"] = output_array; // ノードデータにバインド

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
				cond_json["Type"] = static_cast<int>(cond.type); // 判定ノードの種類
				cond_json["HashKey"] = cond.hash_key; // 主ハッシュキー
				cond_json["RefValue"] = cond.reference_value; // 基準値
				cond_json["CompOp"] = cond.compare_operator; // 演算子
				cond_json["ParamSecond"] = cond.param_second; // 第2数値引数
				cond_json["SecondaryHash"] = cond.secondary_hash; //副ハッシュキーを保存
				conds_array.push_back(cond_json); // 配列へ追加
			}
			link_json["Conditions"] = conds_array; // リンクにバインド
			links_array.push_back(link_json); // 階層用リンク配列へプッシュ
		}
		graph_json["Links"] = links_array;
		layers_array.push_back(graph_json);
	}
	root_json["Layers"] = layers_array; // 大元に階層リストを格納

	//物理ファイルへの書き出し処理
	std::ofstream file_stream(file_path); // 保存用ファイルストリーム
	if (file_stream.is_open()) // オープン成功判定
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
		return false; // 読み込み処理のスキップ
	}

	nlohmann::json root_json;	//パース用ルートオブジェクト
	file_stream >> root_json; // JSONの一括パース

	//データ破損チェック
	if (root_json.find("NextID") == root_json.end() || root_json.find("Layers") == root_json.end())
	{
		printf("Error: LoadFromFile - 「%s」のデータ構造が不正です。\n", file_path.c_str());
		return false; // 読み込み中断
	}

	layer_datas.clear(); // 既存のコンテナのクリア
	next_id = root_json["NextID"]; // IDカウンターの復元

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
			printf("Error: LoadFromFile - 階層データのキー構造が壊れているため復元をスキップします。\n"); // 原因の可視化 [cite: 2026-01-06]
			return false;
		}

		GraphData graph;	//復元先の階層インスタンス
		graph.id = graph_json["GraphID"]; // 階層ID
		graph.name = graph_json["GraphName"]; // 階層名

		//ノード群の復元展開
		for (const auto& node_json : graph_json["Nodes"])
		{
			GraphNode node;	//復元先のノードインスタンス
			node.id = node_json["ID"]; // ID
			node.name = node_json["Name"]; // 名前
			node.position_x = node_json["PosX"]; // 座標X
			node_json["PosY"].get_to(node.position_y); // 座標Y
			node.is_sub_graph = node_json["IsSubGraph"]; // フラグ
			node.sub_graph_id = node_json["SubGraphID"]; // 下位ID
			node.is_loop = node_json.contains("IsLoop") ? node_json["IsLoop"].get<bool>() : true;
			node.link_color_r = node_json.contains("LinkColorR") ? node_json["LinkColorR"].get<float>() : 1.0f;
			node.link_color_g = node_json.contains("LinkColorG") ? node_json["LinkColorG"].get<float>() : 1.0f;
			node.link_color_b = node_json.contains("LinkColorB") ? node_json["LinkColorB"].get<float>() : 1.0f;
			
			if (node_json.find("ActionCategory") != node_json.end())
			{
				node.action_category = node_json["ActionCategory"];
			}
			if (node_json.find("AnimationName") != node_json.end())
			{
				node.animation_name = node_json["AnimationName"];
			}

			//入力ピンの復元
			for (const auto& pin_json : node_json["Input"])
			{
				GraphPin pin; // ピン構造体 
				pin.id = pin_json["ID"];
				pin.name = pin_json["Name"];
				pin.kind = static_cast<PinKind>(pin_json["Kind"].get<int>());
				pin.node_id = node.id;
				node.inputs.push_back(pin); // コンテナへ追加
			}

			//出力ピンの復元
			for (const auto& pin_json : node_json["Output"])
			{
				GraphPin pin; // ピン構造体 
				pin.id = pin_json["ID"];
				pin.name = pin_json["Name"];
				pin.kind = static_cast<PinKind>(pin_json["Kind"].get<int>());
				pin.node_id = node.id;
				node.outputs.push_back(pin); // コンテナへ追加
			}
			graph.nodes.push_back(node); // 階層データにノードを追加
		}

		//接続線リンク及び遷移条件の復元展開
		for (const auto& link_json : graph_json["Links"])
		{
			GraphLink link;	//復元先のリンクインスタンス
			link.id = link_json["ID"]; // リンクID
			link.start_pin_id = link_json["StartPinID"]; // 開始ピン
			link.end_pin_id = link_json["EndPinID"]; // 終了ピン

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
				link.conditions.push_back(cond); // リンクに条件を追加
			}
			graph.links.push_back(link); // 階層データにリンクを追加
		}
		layer_datas.push_back(graph); // マネージャーに階層を追加
	}
	printf("StateGraphDataManager: ファイル「%s」から全階層データを正常に復元ロードしました。\n", file_path.c_str());
	return true; // 読込成功
}

//ノードの生成
void StateGraphDataManager::AddNode(GraphData* current_graph, float click_x, float click_y, const std::string& node_name)
{
	//渡されたポインタが安全か確認
	if (!current_graph)
	{
		printf("Error: GraphDataManager::AddNode - current_graph が nullptr です。\n");
		return;
	}

	GraphNode new_node;	//新しいノード情報

	//ノードのパラメータ設定
	new_node.id = next_id++;
	new_node.name = node_name;
	new_node.position_x = click_x;
	new_node.position_y = click_y;
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

	printf("GraphDataManager: ノードデータを正常に追加しました。ID: %d, 入力ピンID: %d, 出力ピンID: %d\n",
		new_node.id, new_input.id, new_output.id);
}

//下位階層データを生成してIDを返す
uint32_t StateGraphDataManager::CreateNewSubGraph(const std::string& name)
{
	GraphData new_graph;	//新しい階層情報

	//パラメータ設定
	new_graph.id = FetchAndIncrementId();
	new_graph.name = name;
	layer_datas.push_back(new_graph);

	printf("StateGraphDataManager: 新しい下位階層データを作成しました。階層ID: %d, 名前: %s\n", new_graph.id, name.c_str());
	return new_graph.id;
}

//階層が空の場合に初期ノードを構築
void StateGraphDataManager::CheckAndInitDefaultNode(uint32_t graph_id)
{
	//初期ノードの構築判定と生成
	for (size_t g = 0; g < layer_datas.size(); g++)
	{
		if (layer_datas[g].id != graph_id)
		{
			continue;
		}

		//既にノードがある場合
		if (!layer_datas[g].nodes.empty())
		{
			return;
		}

		GraphNode test_node;
		test_node.id = FetchAndIncrementId();
		test_node.name = u8"待機状態";
		test_node.position_x = 100.0f;
		test_node.position_y = 100.0f;
		test_node.is_sub_graph = false;
		test_node.sub_graph_id = 0;

		// 入力ピン設定
		GraphPin test_input;
		test_input.id = FetchAndIncrementId();
		test_input.name = u8"入力";
		test_input.kind = PinKind::Input;
		test_input.node_id = test_node.id;
		test_node.inputs.push_back(test_input);

		// 出力ピン設定
		GraphPin test_output;
		test_output.id = FetchAndIncrementId();
		test_output.name = u8"出力";
		test_output.kind = PinKind::Output;
		test_output.node_id = test_node.id;
		test_node.outputs.push_back(test_output);

		layer_datas[g].nodes.push_back(test_node);
		printf("StateGraphDataManager: 階層ID %d に初期ノード(待機状態)を作成しました。\n", graph_id);
		return;
	}
}

//ピンIDを受け取り、接続ルールに準拠しているか判定
bool StateGraphDataManager::CheckCanConnect(uint32_t graph_id, uint32_t start_pin_id, uint32_t end_pin_id)
{
	const GraphPin* start_pin = nullptr;	//接続元のピン
	const GraphPin* end_pin = nullptr;		//接続先のピン

	for (size_t g = 0; g < layer_datas.size(); g++)
	{
		if (layer_datas[g].id != graph_id)
		{
			continue;
		}

		//現在の全ノードを走査して対象のピンデータを検索
		for (size_t i = 0; i < layer_datas[g].nodes.size(); i++)
		{

			const GraphNode& node = layer_datas[g].nodes[i];

			//入力ピンから検索
			for (size_t p = 0; p < node.inputs.size(); p++)
			{
				if (node.inputs[p].id == start_pin_id)	start_pin = &node.inputs[p];
				if (node.inputs[p].id == end_pin_id)	end_pin = &node.inputs[p];
			}

			//出力ピンから検索
			for (size_t p = 0; p < node.outputs.size(); p++)
			{
				if (node.outputs[p].id == start_pin_id)	start_pin = &node.outputs[p];
				if (node.outputs[p].id == end_pin_id)	end_pin = &node.outputs[p];
			}
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

//ノード削除
void StateGraphDataManager::DeleteNode(uint32_t graph_id, uint32_t target_node_id)
{
	for (size_t g = 0; g < layer_datas.size(); g++)
	{
		if (layer_datas[g].id != graph_id)
		{
			continue;
		}

		for (auto it = layer_datas[g].nodes.begin(); it != layer_datas[g].nodes.end();)
		{
			if (it->id == target_node_id)
			{
				std::string debug_name = it->name;
				it = layer_datas[g].nodes.erase(it);
				printf("StateGraphDataManager: ノードを削除しました。ID: %d, 名前: %s\n", target_node_id, debug_name.c_str());
				return;
			}
			else
			{
				it++;
			}
		}
	}
}

//リンクの削除
void StateGraphDataManager::DeleteLink(uint32_t graph_id, uint32_t target_link_id)
{
	for (size_t g = 0; g < layer_datas.size(); g++)
	{
		if (layer_datas[g].id != graph_id)
		{
			continue;
		}

		for (auto it = layer_datas[g].links.begin(); it != layer_datas[g].links.end();)
		{
			if (it->id == target_link_id)
			{
				it = layer_datas[g].links.erase(it);
				printf("StateGraphDataManager: リンクを削除しました。ID: %d\n", target_link_id);
				return;
			}
			else
			{
				it++;
			}
		}
	}
}

//遷移条件を追加
void StateGraphDataManager::AddConditionToLink(uint32_t graph_id, uint32_t link_id)
{
	//全ての階層情報を巡回して指定の階層を特定
	for (size_t g = 0; g < layer_datas.size(); g++)
	{
		//階層IDが一致しているか確認
		if (layer_datas[g].id == graph_id)
		{
			//階層内の全ての接続線を走査
			for (size_t l = 0; l < layer_datas[g].links.size(); l++)
			{
				//リンクIDが対象と一致したか判定
				if (layer_datas[g].links[l].id == link_id)
				{
					GraphTransitionCondition new_condition;	//新しい条件情報
					new_condition.hash_key = 0;
					new_condition.reference_value = 0.0f;
					new_condition.compare_operator = 0;

					layer_datas[g].links[l].conditions.push_back(new_condition);
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
	for (size_t g = 0; g < layer_datas.size(); g++)
	{
		//階層IDが一致しているか確認
		if (layer_datas[g].id == graph_id)
		{
			//階層内の全ての接続線を走査
			for (size_t l = 0; l < layer_datas[g].links.size(); l++)
			{
				//リンクIDが対象と一致したか判定
				if (layer_datas[g].links[l].id == link_id)
				{
					//配列の範囲外か判定
					if (condition_index >= layer_datas[g].links[l].conditions.size())
					{
						printf("Error: DeleteConditionFromLink - インデックス %zu が範囲外です。\n", condition_index); // デバッグ出力
						return;
					}
					auto target_iterator = layer_datas[g].links[l].conditions.begin() + condition_index;	//削除対象のイテレーター
					layer_datas[g].links[l].conditions.erase(target_iterator);
					printf("StateGraphDataManager: 階層ID %d のリンクID %d から条件インデックス %zu を削除しました。\n", graph_id, link_id, condition_index);
					return;
				}
			}
		}
	}
	printf("Error: DeleteConditionFromLink - 指定された階層ID %d またはリンクID %d が見つかりませんでした。\n", graph_id, link_id);
}

//指定されたピンIDが所属している親ノードのIDを逆引き取得
uint32_t StateGraphDataManager::GetNodeIdFromPinId(uint32_t graph_id, uint32_t pin_id)
{
	for (size_t g = 0; g < layer_datas.size(); g++)
	{
		if (layer_datas[g].id != graph_id)
		{
			continue;
		}

		for (size_t n = 0; n < layer_datas[g].nodes.size(); n++)
		{
			const GraphNode& node = layer_datas[g].nodes[n];
			for (size_t p = 0; p < node.inputs.size(); p++)
			{
				if (node.inputs[p].id == pin_id)
				{
					return node.id;
				}
			}

			for (size_t p = 0; p < node.outputs.size(); p++)
			{
				if (node.outputs[p].id == pin_id)
				{
					return node.id;
				}
			}
		}
	}
	return 0;
}

//指定されたノードIDが所属する階層のIDを検索して取得
uint32_t StateGraphDataManager::GetGraphIdFromNodeId(uint32_t node_id)
{
	uint32_t target_graph_id = UINT32_MAX;	//検索結果のグラフID

	//全ての階層データを巡回
	for (size_t g = 0; g < layer_datas.size(); g++)
	{
		//階層内の全ノードを走査
		for (size_t n = 0; n < layer_datas[g].nodes.size(); n++)
		{
			//目的のノードIDと一致したかを判定
			if (layer_datas[g].nodes[n].id == node_id)
			{
				target_graph_id = layer_datas[g].id;
				return target_graph_id;
			}
		}
	}

	if (target_graph_id == UINT32_MAX)
	{
		printf("Warning: StateGraphDataManager::GetGraphIdFromNodeId - ノードID:%d が見つかりませんでした。\n", node_id);
	}

	return UINT_MAX;
}

//指定されたノードIDを出発基とする全てのリンクのポインタを取得
std::vector<GraphLink*> StateGraphDataManager::GetLinkesFromNode(uint32_t graph_id, uint32_t node_id)
{
	std::vector<GraphLink*> result_linkes;	//出発元のリンク配列

	//該当する階層情報を特定
	for (size_t g = 0; g < layer_datas.size(); g++)
	{
		//階層IDが一致したかを判定
		if (layer_datas[g].id == graph_id)
		{
			//階層内のすべてのリンクを走査
			for (size_t l = 0; l < layer_datas[g].links.size(); l++)
			{
				uint32_t src_node_id = GetNodeIdFromPinId(graph_id, layer_datas[g].links[l].start_pin_id);	//リンクの開始ピンからノードIDを逆引き

				//リンクの出発元が対象のノードIDと一致したかを判定する条件分岐
				if (src_node_id == node_id)
				{
					result_linkes.push_back(&layer_datas[g].links[l]);
				}
			}
			break;
		}
	}
	return result_linkes;
}

//サブグラフノードの生成
void StateGraphDataManager::AddSubGrapNode(uint32_t graph_id, float click_x, float click_y, const std::string& name)
{
	std::string sub_graph_name = name;	//サブグラフの名前
	uint32_t real_sub_graph_id = CreateNewSubGraph(sub_graph_name);	//新しい下位階層グラフ

	const GraphData* src_graph = nullptr;	//コピー元の階層ポインタ

	//全階層情報を走査して、名前が一致する既存のコピー元階層を検索
	for (size_t g = 0; g < layer_datas.size(); g++)
	{
		if (layer_datas[g].id != real_sub_graph_id && layer_datas[g].name == sub_graph_name)
		{
			src_graph = &layer_datas[g];
			break;
		}
	}

	//コピー元階層が見つかったか判定
	if (src_graph)
	{
		GraphData* dst_graph = &layer_datas.back();	//コピー先となる階層ポインタ
		std::unordered_map<uint32_t, uint32_t> pin_id_map;	//古いピンIDと新しいピンIDのハッシュマップ

		// コピー元のすべてのノードをループして複製
		for (size_t n = 0; n < src_graph->nodes.size(); n++)
		{
			const GraphNode& src_node = src_graph->nodes[n]; // コピー元のノード
			GraphNode copied_node;	// 複製先の新しいノード 

			// 基本情報のコピーとIDの割り当て
			copied_node.id = FetchAndIncrementId(); // 新しいノードIDを一意に発行
			copied_node.name = src_node.name;
			copied_node.position_x = src_node.position_x;
			copied_node.position_y = src_node.position_y;
			copied_node.is_sub_graph = src_node.is_sub_graph;
			copied_node.action_category = src_node.action_category;
			copied_node.animation_name = src_node.animation_name;
			copied_node.link_color_r = src_node.link_color_r;
			copied_node.link_color_g = src_node.link_color_g;
			copied_node.link_color_b = src_node.link_color_b;

			// 内部ノードがさらにサブグラフを持っているか判定
			if (src_node.is_sub_graph)
			{
				copied_node.sub_graph_id = CreateNewSubGraph(src_node.name); // ネスト階層の新設
			}
			else
			{
				const uint32_t default_sub_id = 0; // 下位階層なし時の判定値 
				copied_node.sub_graph_id = default_sub_id;
			}

			// 入力ピンの複製とID登録
			for (size_t pin_idx = 0; pin_idx < src_node.inputs.size(); pin_idx++)
			{
				const GraphPin& src_pin = src_node.inputs[pin_idx];	// コピー元のピン
				GraphPin new_pin;	// 新しいピン 

				new_pin.id = FetchAndIncrementId(); // 新しいピンIDを発行
				new_pin.name = src_pin.name;
				new_pin.kind = src_pin.kind;
				new_pin.node_id = copied_node.id; // 新しい親ノードIDを紐付け

				copied_node.inputs.push_back(new_pin);
				pin_id_map[src_pin.id] = new_pin.id; // ハッシュマップへ即時登録
			}

			// 出力ピンの複製とID登録
			for (size_t pin_idx = 0; pin_idx < src_node.outputs.size(); pin_idx++)
			{
				const GraphPin& src_pin = src_node.outputs[pin_idx];	// コピー元のピン
				GraphPin new_pin;	// 新しいピン 

				new_pin.id = FetchAndIncrementId(); // 新しいピンIDを発行
				new_pin.name = src_pin.name;
				new_pin.kind = src_pin.kind;
				new_pin.node_id = copied_node.id; // 新しい親ノードIDを紐付け

				copied_node.outputs.push_back(new_pin);
				pin_id_map[src_pin.id] = new_pin.id; // ハッシュマップへ即時登録
			}
			dst_graph->nodes.push_back(copied_node); // ノードの複製を登録
		}

		// ハッシュマップ参照による接続線リンクの複製
		for (size_t l = 0; l < src_graph->links.size(); l++)
		{
			const GraphLink& src_link = src_graph->links[l];	// コピー元のリンク
			GraphLink copied_link;	// 新しいリンク 
			copied_link.id = FetchAndIncrementId(); // 新しいリンクIDを発行

			const uint32_t invalid_id = 0;	// ID未発見・無効時の判定値 
			copied_link.start_pin_id = invalid_id;
			copied_link.end_pin_id = invalid_id;

			auto start_it = pin_id_map.find(src_link.start_pin_id);	// 開始ピンの参照イテレーター 
			if (start_it != pin_id_map.end())
			{
				copied_link.start_pin_id = start_it->second; // 新しい開始ピンIDを取得
			}

			auto end_it = pin_id_map.find(src_link.end_pin_id);	// 終了ピンの参照イテレーター 
			if (end_it != pin_id_map.end())
			{
				copied_link.end_pin_id = end_it->second; // 新しい終了ピンIDを取得
			}

			// 両方のピンが新しいIDにマッピングされたか判定
			if (copied_link.start_pin_id != invalid_id && copied_link.end_pin_id != invalid_id)
			{
				copied_link.conditions = src_link.conditions;
				dst_graph->links.push_back(copied_link); // リンクの複製を登録
			}
		}
	}

	//現在の階層へのサブグラフ親ノードの生成と登録
	GraphNode new_node;	//サブグラフの親ノードデータ
	// パラメータ設定
	new_node.id = FetchAndIncrementId(); // ノード自体のIDを発行
	new_node.name = sub_graph_name;      // 指定されたステート名を設定
	new_node.position_x = click_x;       // 配置初期X座標
	new_node.position_y = click_y;       // 配置初期Y座標
	new_node.is_sub_graph = true;        // サブグラフ属性を有効化
	new_node.sub_graph_id = real_sub_graph_id; // 完全コピーが完了した内部の階層IDをリンク紐付け
	

	// キャンバスに表示される親ノード用の入力ピン設定
	GraphPin new_input;	// 新しい入力ピン情報 
	new_input.id = FetchAndIncrementId();
	new_input.name = u8"入力";
	new_input.kind = PinKind::Input;
	new_input.node_id = new_node.id;
	new_node.inputs.push_back(new_input);

	// キャンバスに表示される親ノード用の出力ピン設定
	GraphPin new_output;	// 新しい出力ピン情報 
	new_output.id = FetchAndIncrementId();
	new_output.name = u8"出力";
	new_output.kind = PinKind::Output;
	new_output.node_id = new_node.id;
	new_node.outputs.push_back(new_output);

	// 全ての階層情報から、指定された現在のグラフIDと一致するものを検索してノードを追加
	for (size_t g = 0; g < layer_datas.size(); g++)
	{
		if (layer_datas[g].id == graph_id)
		{
			layer_datas[g].nodes.push_back(new_node); // 現在の階層に登録してキャンバスに可視化させる
			printf("StateGraphDataManager: サブグラフノードを内部データごと完全複製して配置しました。ID: %d\n", new_node.id);
			return;
		}
	}
	printf("Error: AddSubGrapNode - 指定された階層ID %d が見つかりませんでした。\n", graph_id);
}

//既存のノードをサブグラフに変換
void StateGraphDataManager::ConvertToSubGraph(uint32_t graph_id, uint32_t node_id)
{
	//現在のグラフ内の全ノードを走査して対象のノードを検索
	for (size_t g = 0; g < layer_datas.size(); g++)
	{
		if (layer_datas[g].id != graph_id)
		{
			continue;
		}

		//該当する階層情報内で対象ノードを検索
		for (size_t n = 0; n < layer_datas[g].nodes.size(); n++)
		{
			GraphNode& target_node = layer_datas[g].nodes[n];	//対象ノード

			//変換対象のノードIDが一致したか確認
			if (target_node.id == node_id)
			{
				//既にサブグラフ化されている場合は早期リターン
				if (target_node.is_sub_graph)
				{
					printf("StateGraphDataManager: ノード ID:%d は既にサブグラフです。\n", node_id);
					return;
				}

				std::string original_name = target_node.name;	//対象のノード名
				uint32_t new_sub_graph_id = CreateNewSubGraph(original_name);	//新しいサブグラフID
				layer_datas[g].nodes[n].is_sub_graph = true;
				layer_datas[g].nodes[n].sub_graph_id = new_sub_graph_id;
				layer_datas[g].nodes[n].name = original_name + u8"サブステート";

				printf("StateGraphDataManager: ノード「%s」(ID:%d) をサブグラフ(階層ID:%d)へ変換完了。\n",
					layer_datas[g].nodes[n].name.c_str(), node_id, new_sub_graph_id);
				return;
			}
		}
	}
}
