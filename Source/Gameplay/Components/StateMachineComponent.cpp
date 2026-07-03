#include "StateMachineComponent.h"
#include "../Serialization/JsonSerializer.h"
#include "Gameplay\StateMachine\StateBlackboard.h"
#include "../ThiedParty/json.hpp"
#include "../Gameplay/Components/AnimationComponent.h"
#include "../Engine/Core/Input.h"


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
		state_machine_path = "Data/Json/Kari.json";
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
	if (!blackboard)
	{
		return;
	}

	if (current_node_id == UINT32_MAX || is_pending_reload)
	{
		is_pending_reload = false;

		if (state_machine_path.empty())
		{
			state_machine_path = "Data/Json/Kari.json";
		}

		LoadAnimationMap(blackboard);

		if (!runtime_nodes.empty())
		{
			bool is_current_node_valid = false;

			for (size_t n = 0; n < runtime_nodes.size(); n++)
			{
				if (runtime_nodes[n].id == current_node_id)
				{
					is_current_node_valid = true;
					current_animation_name = runtime_nodes[n].animation_name;
					current_animation_loop = runtime_nodes[n].is_loop;
					break;
				}
			}

			if (!is_current_node_valid)
			{
				const uint32_t root_layer_id = 0;
				auto entry_it = layer_entry_nodes.find(root_layer_id);
				current_node_id = (entry_it != layer_entry_nodes.end()) ? entry_it->second : runtime_nodes.front().id;

				for (size_t n = 0; n < runtime_nodes.size(); n++)
				{
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

	uint32_t next_node_id = current_node_id;
	bool is_transition_triggered = false;

	for (size_t i = 0; i < runtime_links.size(); i++)
	{
		const RuntimeLink& link = runtime_links[i];
		uint32_t src_node_id = GetNodeIdFromPinId(link.start_pin_id);

		bool is_src_node_active = false; //出発地有効フラグ
		uint32_t trace_id = current_node_id; //親を遡るための探索用追跡

		while (trace_id != UINT32_MAX)
		{
			if (src_node_id == trace_id)
			{
				is_src_node_active = true;
				break;
			}
			trace_id = GetParentNodeId(trace_id); //1階層上の親ノードIDを引っこ抜いて上書き
		}

		if (is_src_node_active)
		{
			bool is_all_conditions_met = !link.conditions.empty();

			for (size_t c = 0; c < link.conditions.size(); c++)
			{
				const auto& cond = link.conditions[c];	//現在の評価条件

				//条件が入力か判定
				if (static_cast<int>(cond.type) == static_cast<int>(ConditionNodeType::InputCheck))
				{
					int v_key = static_cast<int>(cond.hash_key);	//仮想キーコード
					int input_behavior = static_cast<int>(cond.param_second);	//入力形式
					bool is_key_ok = false;	//入力条件クリアフラグ

					if (input_behavior == 1)
					{
						is_key_ok = Input::Instance().IsKeyTrigger(v_key);
					}
					else
					{
						is_key_ok = Input::Instance().IsKeyPress(v_key);
					}

					if (!is_key_ok)
					{
						is_all_conditions_met = false;
						break;
					}
				}
				else
				{
					if (!link.conditions[c].IsJudgment(*blackboard))
					{
						is_all_conditions_met = false;
						break;
					}
				}
			}

			if (is_all_conditions_met)
			{
				next_node_id = GetNodeIdFromPinId(link.end_pin_id);
				is_transition_triggered = true;
				break;
			}
		}
	}

	if (is_transition_triggered && next_node_id != current_node_id)
	{
		current_node_id = next_node_id;

		bool check_sub_graph = true;

		while (check_sub_graph)
		{
			check_sub_graph = false;
			for (size_t n = 0; n < runtime_nodes.size(); n++)
			{
				if (runtime_nodes[n].id == current_node_id)
				{
					if (runtime_nodes[n].is_sub_graph)
					{
						uint32_t sub_layer_id = runtime_nodes[n].sub_graph_id;
						auto entry_it = layer_entry_nodes.find(sub_layer_id);

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

		for (size_t n = 0; n < runtime_nodes.size(); n++)
		{
			const RuntimeNode& node = runtime_nodes[n];

			if (node.id == current_node_id)
			{
				current_animation_name = node.animation_name;
				current_animation_loop = node.is_loop;
				break;
			}
		}
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
