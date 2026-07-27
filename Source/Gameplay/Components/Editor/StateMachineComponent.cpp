#include "StateMachineComponent.h"
#include "Serialization/JsonSerializer.h"
#include "Gameplay\StateMachine\StateBlackboard.h"
#include "ThiedParty/json.hpp"
#include "Gameplay\Components\Animation\AnimationComponent.h"
#include "Engine/Core/Input.h"
#include "Editor\PathHelper.h"

#include <fstream>
#include <iostream>

//コンストラクタ
StateMachineComponent::StateMachineComponent()
{
	state_machine_path = "";
	model_hash = 0;
	current_node_id = UINT32_MAX;
	current_animation_loop = true;
}

//デストラクタ
StateMachineComponent::~StateMachineComponent() = default;

//初期化
void StateMachineComponent::Initialize(StateBlackboard* blackboard)
{
	//プレハブJSONからパスが正しく読み込まれているか判定
	if (!state_machine_path.empty())
	{
		LoadAnimationMap(blackboard);
	}
	else
	{
		const std::string default_model_name = "Default";
		const std::string suffix_name = "_StateMachine";
		state_machine_path = PathHelper::GenerateJsonFilePath(default_model_name, suffix_name);
		LoadAnimationMap(blackboard);
	}

	if (!runtime_nodes.empty())
	{
		const uint32_t root_layer_id = 0; // ルートレイヤーのID定数
		auto entry_it = layer_entry_nodes.find(root_layer_id); // ルートの先頭ステートを検索

		if (entry_it != layer_entry_nodes.end())
		{
			current_node_id = entry_it->second; // 初期ステートを設定
		}
		else
		{
			current_node_id = runtime_nodes.front().id;
		}

		for (size_t n = 0; n < runtime_nodes.size(); n++)
		{
			if (runtime_nodes[n].id == current_node_id)
			{
				current_animation_name = runtime_nodes[n].animation_name;
				break;
			}
		}
	}
}

//更新
void StateMachineComponent::Update(float elapsed_time, StateBlackboard* blackboard)
{
	//ブラックボードポインタが有効であるかを判定
	if (!blackboard)
	{
		return;
	}

	//現在のノードIDが無効、または再読み込み要求が保留されているかを判定
	if (current_node_id == UINT32_MAX || is_pending_reload)
	{
		is_pending_reload = false;

		//ファイルパスが空であるかを判定
		if (state_machine_path.empty())
		{
			const std::string default_model_name = "Default";
			const std::string suffix_name = "_StateMachine";
			state_machine_path = PathHelper::GenerateJsonFilePath(default_model_name, suffix_name);
		}

		LoadAnimationMap(blackboard);

		//展開された実行時ノードが空でないかを判定
		if (!runtime_nodes.empty())
		{
			bool is_current_node_valid = false; // 現在のノードが有効かを表す判定フラグ

			//全ノードを走査して現在のノードIDが存在するか確認するループ処理
			for (size_t n = 0; n < runtime_nodes.size(); n++)
			{
				//ノードIDが一致したかを判定
				if (runtime_nodes[n].id == current_node_id)
				{
					is_current_node_valid = true;
					current_animation_name = runtime_nodes[n].animation_name;
					current_animation_loop = runtime_nodes[n].is_loop;
					break;
				}
			}

			//現在のノードIDがデータ内に見つからなかったかを判定
			if (!is_current_node_valid)
			{
				const uint32_t root_layer_id = 0; // ルートレイヤーを表す定数
				auto entry_it = layer_entry_nodes.find(root_layer_id); // エントリーノードの検索結果イテレーター
				current_node_id = (entry_it != layer_entry_nodes.end()) ? entry_it->second : runtime_nodes.front().id;

				//初期化されたノードIDのアニメーション情報を適用するためのループ処理
				for (size_t n = 0; n < runtime_nodes.size(); n++)
				{
					//ノードIDが一致したかを判定
					if (runtime_nodes[n].id == current_node_id)
					{
						current_animation_name = runtime_nodes[n].animation_name;
						current_animation_loop = runtime_nodes[n].is_loop;
						break;
					}
				}
			}
		}
		else
		{
			return;
		}
	}

	uint32_t current_parent_id = GetParentNodeId(current_node_id); // 現在滞在しているノードの親サブグラフID
	size_t global_max_conditions = 0; // 同一階層内で条件をクリアしたリンクの最大条件数

	// 同一階層内でのグローバル最大条件数をあらかじめ算出するための1パス目のループ処理
	for (size_t i = 0; i < runtime_links.size(); i++)
	{
		const RuntimeLink& link = runtime_links[i]; // 精査対象の実行時リンク情報
		uint32_t src_node_id = GetNodeIdFromPinId(link.start_pin_id); // リンクの出発元ノードID

		// 出発元ノードの親階層が現在のアクティブ階層と完全に一致するかを判定
		if (GetParentNodeId(src_node_id) == current_parent_id)
		{
			bool is_all_conditions_met = !link.conditions.empty(); // 該当リンクの全条件を満たしたかを表す判定フラグ

			// リンクに設定されたすべての遷移条件を個別に精査するループ処理
			for (size_t c = 0; c < link.conditions.size(); c++)
			{
				const auto& cond = link.conditions[c]; // 評価対象の遷移条件

				// 条件判定のタイプを入力チェックであるかを判定
				if (static_cast<int>(cond.type) == static_cast<int>(ConditionNodeType::InputCheck))
				{
					int v_key = static_cast<int>(cond.hash_key); // 仮想キーコード
					int input_behavior = static_cast<int>(cond.param_second); // キー入力形式
					bool is_key_ok = false; // 入力クリア判定フラグ
					constexpr int mode_trigger = 1; // 押された瞬間を表す定数

					// 入力形式がトリガーモードであるかを判定
					if (input_behavior == mode_trigger)
					{
						is_key_ok = Input::Instance().IsKeyTrigger(v_key);
					}
					else
					{
						is_key_ok = Input::Instance().IsKeyPress(v_key);
					}

					// キー入力条件を満たせなかったかを判定
					if (!is_key_ok)
					{
						is_all_conditions_met = false;
						break;
					}
				}
				else
				{
					// ブラックボードの通常判定条件を満たしていないかを判定
					if (!link.conditions[c].IsJudgment(*blackboard))
					{
						is_all_conditions_met = false;
						break;
					}
				}
			}

			// 該当リンクのすべての条件をクリアできたかを判定
			if (is_all_conditions_met)
			{
				// 今回の条件数がこれまでの最大条件数を超えているかを判定
				if (link.conditions.size() > global_max_conditions)
				{
					global_max_conditions = link.conditions.size();
				}
			}
		}
	}

	uint32_t next_node_id = current_node_id; // 遷移先として選ばれるノードID
	bool is_transition_triggered = false; // 遷移トリガーを引くかを表す判定フラグ
	const RuntimeLink* best_link = nullptr; // 条件を満たした中から最も条件数の多い最適なリンク
	size_t max_conditions = 0; // 満たされた条件の最大数

	// 条件を満たす最適な遷移先リンクを決定するための2パス目のループ処理
	for (size_t i = 0; i < runtime_links.size(); i++)
	{
		const RuntimeLink& link = runtime_links[i]; // 精査対象の実行時リンク情報
		uint32_t src_node_id = GetNodeIdFromPinId(link.start_pin_id); // リンクの出発元ノードID

		// 同一階層のリンクでありながら条件数がグローバル最大条件数に満たないかを判定
		if (GetParentNodeId(src_node_id) == current_parent_id && link.conditions.size() < global_max_conditions)
		{
			continue;
		}

		bool is_src_node_active = false; // 出発元のノードがアクティブなツリーに含まれるかを表すフラグ
		uint32_t trace_id = current_node_id; // 親サブグラフをルートに向かって遡るための探索用追跡

		// 現在のアクティブノードから親階層をルートに向かって巡回するループ処理
		while (trace_id != UINT32_MAX)
		{
			// 出発元ノードIDがアクティブなツリー経路に一致したかを判定
			if (src_node_id == trace_id)
			{
				is_src_node_active = true;
				break;
			}
			trace_id = GetParentNodeId(trace_id);
		}

		// 出発元ノードが有効なアクティブ状態にあるかを判定
		if (is_src_node_active)
		{
			bool is_all_conditions_met = !link.conditions.empty(); // 該当リンクのすべての条件を満たしたかを表すフラグ

			// リンクが持つすべての条件式を個別に精査するループ処理
			for (size_t c = 0; c < link.conditions.size(); c++)
			{
				const auto& cond = link.conditions[c]; // 評価対象の遷移条件

				// 条件判定の種類がキー入力チェックであるかを判定
				if (static_cast<int>(cond.type) == static_cast<int>(ConditionNodeType::InputCheck))
				{
					int v_key = static_cast<int>(cond.hash_key); // 仮想キーコード
					int input_behavior = static_cast<int>(cond.param_second); // キー入力形式
					bool is_key_ok = false; // 入力クリア判定フラグ
					constexpr int mode_trigger = 1; // 押された瞬間

					// 入力検知形式がトリガーモードであるかを判定
					if (input_behavior == mode_trigger)
					{
						is_key_ok = Input::Instance().IsKeyTrigger(v_key);
					}
					else
					{
						is_key_ok = Input::Instance().IsKeyPress(v_key);
					}

					// キー入力条件を満たせなかったかを判定
					if (!is_key_ok)
					{
						is_all_conditions_met = false;
						break;
					}
				}
				else
				{
					// ブラックボードを用いた通常比較や距離判定などの条件を満たしていないかを判定
					if (!link.conditions[c].IsJudgment(*blackboard))
					{
						is_all_conditions_met = false;
						break;
					}
				}
			}

			// 該当リンクのすべての遷移条件を完全にクリアしたかを判定
			if (is_all_conditions_met)
			{
				// 現在のリンクの条件数がこれまでの最大適合条件数より多いか、または最初の適合リンクであるかを判定
				if (!best_link || link.conditions.size() > max_conditions)
				{
					best_link = &link;
					max_conditions = link.conditions.size();
				}
			}
		}
	}

	// すべてのリンク走査後に条件をクリアした最適なリンク候補が確定したかを判定
	if (best_link)
	{
		next_node_id = GetNodeIdFromPinId(best_link->end_pin_id);
		is_transition_triggered = true;
	}

	// 遷移トリガーが引かれ、かつ現在のノードIDから変更が発生したかを判定
	if (is_transition_triggered && next_node_id != current_node_id)
	{
		// 遷移が確定した瞬間の詳細を可視化するためのログ出力処理
		char debug_message[256];
		sprintf_s(debug_message, sizeof(debug_message), "[StateMachineComponent] Transition Success: Node ID %u -> Node ID %u (Link ID: %u, Max Conditions: %zu)\n", current_node_id, next_node_id, best_link->id, max_conditions);
		//OutputDebugStringA(debug_message);

		current_node_id = next_node_id;

		bool check_sub_graph = true; // 下位のサブグラフへ潜り込む必要があるかを表すループ制御フラグ

		// 下位の階層にサブグラフが存在する限り自動展開して潜り込みを続けるループ処理
		while (check_sub_graph)
		{
			check_sub_graph = false;

			// 全実行時ノードから展開先のサブグラフエントリーIDを探索するループ処理
			for (size_t n = 0; n < runtime_nodes.size(); n++)
			{
				// 現在の目的地ノードIDと一致したかを判定
				if (runtime_nodes[n].id == current_node_id)
				{
					// 該当ノードがサブグラフ属性を持っているかを判定
					if (runtime_nodes[n].is_sub_graph)
					{
						uint32_t sub_layer_id = runtime_nodes[n].sub_graph_id; // 下位グラフの固有レイヤーID
						auto entry_it = layer_entry_nodes.find(sub_layer_id); // レイヤー固有のエントリーエントリー検索結果イテレーター

						// 該当階層の先頭エントリーIDがマップ内に存在するか判定
						if (entry_it != layer_entry_nodes.end())
						{
							current_node_id = entry_it->second;
							check_sub_graph = true;
						}
					}
					break;
				}
			}
		}

		// 最終確定した遷移先ノードに対応するアニメーション情報を反映するためのループ処理
		for (size_t n = 0; n < runtime_nodes.size(); n++)
		{
			const RuntimeNode& node = runtime_nodes[n]; // 走査対象の実行時ノード情報

			// ノードIDが最終的な現在のアクティブIDと一致したかを判定
			if (node.id == current_node_id)
			{
				current_animation_name = node.animation_name;
				current_animation_loop = node.is_loop;
				break;
			}
		}
	}
}

//モデル名設定
void StateMachineComponent::SetModelName(const std::string& model_name)
{
	const std::string suffix_name = "_StateMachine";
	std::string generated_path = PathHelper::GenerateJsonFilePath(model_name, suffix_name);

	if (!generated_path.empty())
	{
		state_machine_path = generated_path;
		printf("StateMachineComponent: モデル名「%s」からパス「%s」を設定しました。\n",
			model_name.c_str(), state_machine_path.c_str());
	}
	else
	{
		printf("Error: StateMachineComponent::SetModelName - パスの構築に失敗しました。\n");
	}
}

//シリアライズ登録
void StateMachineComponent::SetupSerialization(JsonSerializer* serializer)
{
	//シリアライザのポインタが有効か確認
	if (serializer)
	{
		serializer->RegisterVariable("StateMachinePath", &state_machine_path);
	}
}

//現在のステートがルートモーションを有効にしているか取得
bool StateMachineComponent::IsCurrentRootMotionEnbled() const
{
	for (size_t i = 0; i < runtime_nodes.size(); i++)
	{
		//現在のアクティブノードに一致するか判定
		if (runtime_nodes[i].id == current_node_id)
		{
			return runtime_nodes[i].is_root_motion;
		}
	}

	return false;
}

//ステートマシンJSONからアニメーション対応表を抽出
void StateMachineComponent::LoadAnimationMap(StateBlackboard* blackboard)
{
	std::ifstream input_file(state_machine_path);

	if (!input_file.is_open())
	{
		std::cerr << "Warning: StateMachineComponent - ファイルを開けませんでした: " << state_machine_path << std::endl;
		return;
	}

	nlohmann::json root_json;
	input_file >> root_json;
	input_file.close();

	animation_map.clear();
	runtime_nodes.clear();
	runtime_links.clear();
	layer_entry_nodes.clear();

	std::unordered_map<uint32_t, uint32_t> sub_graph_to_parent_map;

	if(root_json.is_object() && root_json.contains("Layers") && root_json["Layers"].is_array())
	{
		// すべてのサブグラフノードの親子関係（所属レイヤー）を事前に高速スキャンしてマップへ先行登録
		for (size_t i = 0; i < root_json["Layers"].size(); i++)
		{
			const auto& layer = root_json["Layers"][i]; // スキャン用一時レイヤー

			if (layer.contains("Nodes") && layer["Nodes"].is_array())
			{
				for (size_t j = 0; j < layer["Nodes"].size(); j++)
				{
					const auto& node_json = layer["Nodes"][j]; // スキャン用一時ノード

					if (node_json.is_object() && node_json.contains("IsSubGraph") && node_json["IsSubGraph"].get<bool>())
					{
						sub_graph_to_parent_map[node_json["SubGraphID"].get<uint32_t>()] = node_json["ID"].get<uint32_t>();
					}
				}
			}
		}

		// 本番パース。1パス目で集計した親子関係マップから親IDを割り出しながらアロケーション
		for (size_t i = 0; i < root_json["Layers"].size(); i++)
		{
			const auto& layer = root_json["Layers"][i];
			uint32_t layer_graph_id = layer["GraphID"].get<uint32_t>();

			uint32_t parent_id = UINT32_MAX; // 所属する親サブグラフノードIDの初期化
			auto parent_it = sub_graph_to_parent_map.find(layer_graph_id); // 自分のレイヤーの親を検索

			if (parent_it != sub_graph_to_parent_map.end())
			{
				parent_id = parent_it->second; // 親サブグラフのノードIDを代入
			}

			if (layer.contains("Nodes") && layer["Nodes"].is_array())
			{
				for (size_t j = 0; j < layer["Nodes"].size(); j++)
				{
					const auto& node_json = layer["Nodes"][j];

					if (node_json.is_object())
					{
						RuntimeNode node;
						node.id = node_json["ID"].get<uint32_t>();
						node.name = node_json["Name"].get<std::string>();
						node.animation_name = node_json.contains("AnimationName") ? node_json["AnimationName"].get<std::string>() : "";
						node.is_sub_graph = node_json["IsSubGraph"].get<bool>();
						node.sub_graph_id = node_json["SubGraphID"].get<uint32_t>();
						node.is_loop = node_json.contains("IsLoop") ? node_json["IsLoop"].get<bool>() : true;
						node.is_root_motion = node_json.contains("IsRootMotion") ? node_json["IsRootMotion"].get<bool>() : false;
						node.parent_node_id = parent_id;

						if (node_json.contains("Input") && node_json["Input"].is_array())
						{
							for (size_t p = 0; p < node_json["Input"].size(); p++)
							{
								node.inputs.push_back(node_json["Input"][p]["ID"].get<uint32_t>());
							}
						}

						if (node_json.contains("Output") && node_json["Output"].is_array())
						{
							for (size_t p = 0; p < node_json["Output"].size(); p++)
							{
								node.outputs.push_back(node_json["Output"][p]["ID"].get<uint32_t>());
							}
						}

						if (j == 0)
						{
							layer_entry_nodes[layer_graph_id] = node.id;
						}

						if (!node.animation_name.empty())
						{
							animation_map[node.id] = node.animation_name;
						}

						runtime_nodes.push_back(node);
					}
				}
			}

			if (layer.contains("Links") && layer["Links"].is_array())
			{
				for (size_t j = 0; j < layer["Links"].size(); j++)
				{
					const auto& link_json = layer["Links"][j];

					RuntimeLink link;
					link.id = link_json["ID"].get<uint32_t>();
					link.start_pin_id = link_json["StartPinID"].get<uint32_t>();
					link.end_pin_id = link_json["EndPinID"].get<uint32_t>();

					if (link_json.contains("Conditions") && link_json["Conditions"].is_array())
					{
						for (size_t c = 0; c < link_json["Conditions"].size(); c++)
						{
							const auto& cond_json = link_json["Conditions"][c];

							TransitionCondition cond;
							cond.type = static_cast<ConditionNodeType>(cond_json["Type"].get<int>());
							cond.hash_key = cond_json["HashKey"].get<uint32_t>();
							cond.compart_op = static_cast<CompareOperator>(cond_json["CompOp"].get<int>());
							cond.param_second = cond_json["ParamSecond"].get<float>();
							cond.secondary_hash = cond_json["SecondaryHash"].get<uint32_t>();

							float raw_ref_value = cond_json["RefValue"].get<float>();

							if (blackboard && cond.type == ConditionNodeType::NormalCompare && cond.hash_key != 0)
							{
								const BlackboardData& raw_data = blackboard->GetAttributeValue(cond.hash_key);

								if (std::holds_alternative<bool>(raw_data))
								{
									cond.reference_value = (raw_ref_value != 0.0f);
								}
								else if (std::holds_alternative<int>(raw_data))
								{
									cond.reference_value = static_cast<int>(raw_ref_value);
								}
								else
								{
									cond.reference_value = raw_ref_value;
								}
							}
							else
							{
								cond.reference_value = raw_ref_value;
							}

							link.conditions.push_back(cond);
						}
					}

					runtime_links.push_back(link);
				}
			}
		}
	}

	if (runtime_nodes.empty())
	{
		printf("[Warning] StateMachineComponent: 実行時ステート構造が1つも展開されませんでした。現在のパス: %s\n", state_machine_path.c_str());
	}
	else
	{
		printf("StateMachineComponent: 「%s」から %zu 個のノードと %zu 個の接続線を自律脳みそへ展開しました。\n",
			state_machine_path.c_str(), runtime_nodes.size(), runtime_links.size());
	}
}

//ピンIDから所属するノードIDを逆引き検索
uint32_t StateMachineComponent::GetNodeIdFromPinId(uint32_t pin_id) const
{
	for (size_t i = 0; i < runtime_nodes.size(); i++)
	{
		const RuntimeNode& node = runtime_nodes[i]; //検索対象ノード

		for (size_t p = 0; p < node.inputs.size(); p++)
		{
			if (node.inputs[p] == pin_id)
			{
				return node.id;
			}
		}

		for (size_t p = 0; p < node.outputs.size(); p++)
		{
			if (node.outputs[p] == pin_id)
			{
				return node.id;
			}
		}
	}

	return UINT32_MAX;
}

//指定されたノードIDの親サブグラフノードIDを逆引き
uint32_t StateMachineComponent::GetParentNodeId(uint32_t node_id) const
{
	for (size_t i = 0; i < runtime_nodes.size(); i++)
	{
		if (runtime_nodes[i].id == node_id)
		{
			return runtime_nodes[i].parent_node_id;
		}
	}

	return UINT32_MAX;
}
