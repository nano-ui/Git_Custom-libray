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
	state_graph_simulator = std::make_unique<StateGraphSimulator>();
	behavior_simulator = std::make_unique<BehaviorTreeSimulator>();
	state_machine_path = "";
	model_hash = 0;
	current_node_id = UINT32_MAX;
	current_animation_loop = true;
	current_blend_time = DEFAULT_STATE_BLEND_TIME;
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
				current_blend_time = DEFAULT_STATE_BLEND_TIME;
				break;
			}
		}
	}
}

//更新
void StateMachineComponent::Update(float elapsed_time, StateBlackboard* blackboard, bool is_anim_finished) 
{
	if (!blackboard)
	{
		printf("エラー: StateMachineComponent::Update - blackboard が nullptr です。\n");
		return;
	}

	if (current_node_id == UINT32_MAX || is_pending_reload)
	{
		is_pending_reload = false;

		if (state_machine_path.empty())
		{
			const std::string default_model_name = "Default";
			const std::string suffix_name = "_StateMachine";
			state_machine_path = PathHelper::GenerateJsonFilePath(default_model_name, suffix_name);
		}

		LoadAnimationMap(blackboard);

		if (runtime_nodes.empty())
		{
			printf("警告: StateMachineComponent::Update - runtime_nodes が空のため更新を中断します。\n");
			return;
		}
	}

	if (!state_graph_simulator)
	{
		printf("Error: StateMachineComponent::Update - state_graph_simulator が nullptr です。\n");
		return;
	}

	uint32_t current_graph_id = GetGraphIdFromNodeId(current_node_id);
	RuntimeLayerType current_layer_type = RuntimeLayerType::StateMachine;
	auto layer_it = layer_types.find(current_graph_id);
	if (layer_it != layer_types.end())
	{
		current_layer_type = layer_it->second;
	}

	if (current_layer_type == RuntimeLayerType::BehaviorTree)
	{
		if (last_bt_graph_id != current_graph_id)
		{
			behavior_simulator->SetupGraph(bt_nodes_map[current_graph_id], bt_links_map[current_graph_id]);
			last_bt_graph_id = current_graph_id;
			printf("StateMachineComponent: ビヘイビアツリー階層 (ID:%u) のシミュレーションを開始しました。\n", current_graph_id);
		}

		behavior_simulator->UpdateSimulation(elapsed_time, blackboard);

		const uint32_t sim_active_id = behavior_simulator->GetCurrentActiveNodeId();
		if (sim_active_id != UINT32_MAX)
		{
			current_node_id = sim_active_id;
			current_animation_name = behavior_simulator->GetCurrentAnimationName();
			current_animation_loop = behavior_simulator->GetCurrentAnimationLoop();
		}

		uint32_t parent_subgraph_node_id = UINT32_MAX;
		for (size_t i = 0; i < runtime_nodes.size(); i++)
		{
			if (runtime_nodes[i].is_sub_graph && runtime_nodes[i].sub_graph_id == current_graph_id)
			{
				parent_subgraph_node_id = runtime_nodes[i].id;
				break;
			}
		}

		if (parent_subgraph_node_id != UINT32_MAX)
		{
			for (const auto& link : runtime_links)
			{
				uint32_t src_node_id = GetNodeIdFromPinId(link.start_pin_id);
				if (src_node_id == parent_subgraph_node_id)
				{
					if (link.conditions.empty()) continue;

					bool all_conditions_met = true;
					for (const auto& cond : link.conditions)
					{
						if (!EvaluateCondition(cond, blackboard, is_anim_finished))
						{
							all_conditions_met = false;
							break;
						}
					}

					if (all_conditions_met)
					{
						uint32_t dst_node_id = GetNodeIdFromPinId(link.end_pin_id);
						if (dst_node_id != UINT32_MAX)
						{
							current_node_id = dst_node_id;
							current_blend_time = link.blend_time;
							behavior_simulator->InvalidataTree();
							last_bt_graph_id = UINT32_MAX;

							for (size_t n = 0; n < runtime_nodes.size(); n++)
							{
								if (runtime_nodes[n].id == current_node_id)
								{
									current_animation_name = runtime_nodes[n].animation_name;
									current_animation_loop = runtime_nodes[n].is_loop;
									break;
								}
							}

							state_graph_simulator->SetCurreentNodeID(current_node_id);
							printf("StateMachineComponent: ビヘイビアツリー階層 (ID:%u) からノード (ID:%u) へ遷移しました。\n", current_graph_id, current_node_id);
							return;
						}
					}
				}
			}
		}
	}
	else
	{
		const bool is_transitioned = state_graph_simulator->UpdateSimulation(elapsed_time, blackboard, is_anim_finished);

		current_node_id = state_graph_simulator->GetCurrentNodeID();
		current_animation_name = state_graph_simulator->GetAnimationName();
		current_animation_loop = state_graph_simulator->GetAnimationLoop();
		current_blend_time = state_graph_simulator->GetCurrentBlendTime();

		if (is_transitioned)
		{
			uint32_t next_graph_id = GetGraphIdFromNodeId(current_node_id);
			if (layer_types[next_graph_id] == RuntimeLayerType::BehaviorTree)
			{
				behavior_simulator->SetupGraph(bt_nodes_map[next_graph_id], bt_links_map[next_graph_id]);
				last_bt_graph_id = next_graph_id;
				behavior_simulator->UpdateSimulation(elapsed_time, blackboard);

				const uint32_t sim_active_id = behavior_simulator->GetCurrentActiveNodeId();
				if (sim_active_id != UINT32_MAX)
				{
					current_node_id = sim_active_id;
					current_animation_name = behavior_simulator->GetCurrentAnimationName();
					current_animation_loop = behavior_simulator->GetCurrentAnimationLoop();
				}
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

//アクションカテゴリを取得
ActionCategory StateMachineComponent::GetCurrentActionCategory() const
{
	return state_graph_simulator->GetCurrentActionCategory();
}

//現在のステートがルートモーションを有効にしているか取得
bool StateMachineComponent::IsCurrentRootMotionEnbled() const
{
	//シミュレータが存在する場合はその判定結果を返却
	if (state_graph_simulator)
	{
		return state_graph_simulator->GetRootMotionEnabled();
	}

	return false;
}

//単一の遷移条件が成立しているか判定
bool StateMachineComponent::EvaluateCondition(const TransitionCondition& cond, StateBlackboard* blackboard, bool is_anim_finished) const
{
	//アニメーション終了判定
	if (cond.type == ConditionNodeType::AnimationEnd)return is_anim_finished;

	//通常のブラックボード比較
	if (blackboard)return cond.IsJudgment(*blackboard);
}

//ステートマシンJSONからアニメーション対応表を抽出
void StateMachineComponent::LoadAnimationMap(StateBlackboard* blackboard)
{
	bool load_success = StateMachineDataLoader::LoadForRuntime(
		state_machine_path,
		runtime_nodes,
		runtime_links,
		layer_entry_nodes,
		animation_map,
		layer_types);

	if (!load_success)
	{
		return;
	}

	if (blackboard)
	{
		for (size_t l = 0; l < runtime_links.size(); l++)
		{
			for (size_t c = 0; c < runtime_links[l].conditions.size(); c++)
			{
				TransitionCondition& cond = runtime_links[l].conditions[c];

				if (cond.type == ConditionNodeType::NormalCompare && cond.hash_key != 0)
				{
					const BlackboardData& raw_data = blackboard->GetAttributeValue(cond.hash_key);

					const float* parsed_float_ptr = std::get_if<float>(&cond.reference_value);
					float parsed_float = parsed_float_ptr ? *parsed_float_ptr : 0.0f;

					if (std::holds_alternative<bool>(raw_data))
					{
						cond.reference_value = (parsed_float != 0.0f);
					}
					else if (std::holds_alternative<int>(raw_data))
					{
						cond.reference_value = static_cast<int>(parsed_float);
					}
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

	if (!runtime_nodes.empty())
	{
		bool node_exists = false;
		for (size_t i = 0; i < runtime_nodes.size(); i++)
		{
			if (runtime_nodes[i].id == current_node_id)
			{
				node_exists = true;
				break;
			}
		}

		if (!node_exists)
		{
			constexpr uint32_t root_layer_id = 0;
			auto entry_it = layer_entry_nodes.find(root_layer_id);
			if (entry_it != layer_entry_nodes.end())
			{
				current_node_id = entry_it->second;
			}
			else
			{
				current_node_id = runtime_nodes.front().id;
			}
		}
	}

	if (state_graph_simulator)
	{
		state_graph_simulator->SetupRuntimeGraph(runtime_nodes, runtime_links, layer_entry_nodes);
		state_graph_simulator->SetCurreentNodeID(current_node_id);
	}

	bt_nodes_map.clear();
	bt_links_map.clear();
	for (const auto& node : runtime_nodes)
	{
		if (layer_types[node.graph_id] == RuntimeLayerType::BehaviorTree)
		{
			bt_nodes_map[node.graph_id].push_back(node);
		}
	}
	for (const auto& link : runtime_links)
	{
		uint32_t src_node_id = GetNodeIdFromPinId(link.start_pin_id);
		if (src_node_id != UINT32_MAX)
		{
			uint32_t graph_id = GetGraphIdFromNodeId(src_node_id);
			if (graph_id != UINT32_MAX && layer_types[graph_id] == RuntimeLayerType::BehaviorTree)
			{
				bt_links_map[graph_id].push_back(link);
			}
		}
	}
	last_bt_graph_id = UINT32_MAX;
}

//ピンIDから所属するノードIDを逆引き検索
uint32_t StateMachineComponent::GetNodeIdFromPinId(uint32_t pin_id) const
{
	for (size_t i = 0; i < runtime_nodes.size(); i++)
	{
		const SimulatorRuntimeNode& node = runtime_nodes[i]; //検索対象ノード

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

//ノードIDからグラフIDを取得
uint32_t StateMachineComponent::GetGraphIdFromNodeId(uint32_t node_id) const
{
	for (size_t i = 0; i < runtime_nodes.size(); i++)
	{
		if (runtime_nodes[i].id == node_id)
		{
			return runtime_nodes[i].graph_id;
		}
	}
	return UINT32_MAX;
}
