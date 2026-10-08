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
void StateMachineComponent::Update(float elapsed_time, StateBlackboard* blackboard, bool is_anim_finished) 
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

		if (runtime_nodes.empty())
		{
			return;
		}
	}

	if (!state_graph_simulator)
	{
		printf("Error: StateMachineComponent::Update - state_graph_simulator が nullptr です。\n");
		return;
	}

	//シミュレータによるステートマシン遷移更新
	const bool is_transitioned = state_graph_simulator->UpdateSimulation(elapsed_time, blackboard, is_anim_finished);

	current_node_id = state_graph_simulator->GetCurrentNodeID();
	current_animation_name = state_graph_simulator->GetAnimationName();
	current_animation_loop = state_graph_simulator->GetAnimationLoop();
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

	//キー入力判定
	if (cond.type == ConditionNodeType::InputCheck)
	{
		int v_key_code = static_cast<int>(cond.hash_key);
		int input_behavior_mode = static_cast<int>(cond.param_second);

		constexpr int mode_press = 0;     // 押されている間
		constexpr int mode_trigger = 1;   // 押された瞬間
		constexpr int mode_not_press = 2; // 未入力
		constexpr int mode_release = 3;   // 離された瞬間

		if (v_key_code == 0) return false; // キー未設定時は不成立

		if (input_behavior_mode == mode_trigger)      return Input::Instance().IsKeyTrigger(v_key_code);
		else if (input_behavior_mode == mode_press)   return Input::Instance().IsKeyPress(v_key_code);
		else if (input_behavior_mode == mode_not_press) return !Input::Instance().IsKeyPress(v_key_code);
		else if (input_behavior_mode == mode_release)  return Input::Instance().IsKeyRelease(v_key_code);

		return false;
	}

	//通常のブラックボード比較
	if (blackboard)return cond.IsJudgment(*blackboard);
}

//ステートマシンJSONからアニメーション対応表を抽出
void StateMachineComponent::LoadAnimationMap(StateBlackboard* blackboard)
{
	std::ifstream input_file(state_machine_path);

	if (!input_file.is_open())
	{
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

	if (root_json.is_object() && root_json.contains("Layers") && root_json["Layers"].is_array())
	{
		//------------------------------------------------------------
		// 1パス目: サブグラフの親子関係（所属レイヤー）を先行スキャン
		//------------------------------------------------------------
		for (size_t i = 0; i < root_json["Layers"].size(); i++)
		{
			const auto& layer = root_json["Layers"][i];

			if (layer.contains("Nodes") && layer["Nodes"].is_array())
			{
				for (size_t j = 0; j < layer["Nodes"].size(); j++)
				{
					const auto& node_json = layer["Nodes"][j];
					if (!node_json.is_object()) continue;

					// サブグラフフラグ判定 (新形式: "サブグラフフラグ" / 旧形式: "IsSubGraph")
					bool is_sub_graph = false;
					if (node_json.contains(u8"サブグラフフラグ")) is_sub_graph = node_json[u8"サブグラフフラグ"].get<bool>();
					else if (node_json.contains("IsSubGraph")) is_sub_graph = node_json["IsSubGraph"].get<bool>();

					if (is_sub_graph)
					{
						// サブグラフID取得 (新形式: "サブグラフID" / 旧形式: "SubGraphID")
						uint32_t sub_graph_id = UINT32_MAX;
						if (node_json.contains(u8"サブグラフID")) sub_graph_id = node_json[u8"サブグラフID"].get<uint32_t>();
						else if (node_json.contains("SubGraphID")) sub_graph_id = node_json["SubGraphID"].get<uint32_t>();

						// ノードID取得 (新形式: "ノードID" / 旧形式: "ID")
						uint32_t node_id = UINT32_MAX;
						if (node_json.contains(u8"ノードID")) node_id = node_json[u8"ノードID"].get<uint32_t>();
						else if (node_json.contains("ID")) node_id = node_json["ID"].get<uint32_t>();

						if (sub_graph_id != UINT32_MAX && node_id != UINT32_MAX)
						{
							sub_graph_to_parent_map[sub_graph_id] = node_id;
						}
					}
				}
			}
		}

		//------------------------------------------------------------
		// 2パス目: 本番ノード・リンクのパース
		//------------------------------------------------------------
		for (size_t i = 0; i < root_json["Layers"].size(); i++)
		{
			const auto& layer = root_json["Layers"][i];
			uint32_t layer_graph_id = layer["GraphID"].get<uint32_t>();

			uint32_t parent_id = UINT32_MAX;
			auto parent_it = sub_graph_to_parent_map.find(layer_graph_id);
			if (parent_it != sub_graph_to_parent_map.end())
			{
				parent_id = parent_it->second;
			}

			if (layer.contains("Nodes") && layer["Nodes"].is_array())
			{
				for (size_t j = 0; j < layer["Nodes"].size(); j++)
				{
					const auto& node_json = layer["Nodes"][j];

					if (node_json.is_object())
					{
						SimulatorRuntimeNode node;

						// ノードID (新形式: "ノードID" / 旧形式: "ID")
						if (node_json.contains(u8"ノードID")) node.id = node_json[u8"ノードID"].get<uint32_t>();
						else if (node_json.contains("ID")) node.id = node_json["ID"].get<uint32_t>();

						// ノード名 (新形式: "ノード名" / 旧形式: "Name")
						if (node_json.contains(u8"ノード名")) node.name = node_json[u8"ノード名"].get<std::string>();
						else if (node_json.contains("Name")) node.name = node_json["Name"].get<std::string>();

						// アニメーション名 (新形式: "アニメーション名" / 旧形式: "AnimationName")
						if (node_json.contains(u8"アニメーション名")) node.animation_name = node_json[u8"アニメーション名"].get<std::string>();
						else if (node_json.contains("AnimationName")) node.animation_name = node_json["AnimationName"].get<std::string>();
						else node.animation_name = "";

						// サブグラフフラグ
						if (node_json.contains(u8"サブグラフフラグ")) node.is_sub_graph = node_json[u8"サブグラフフラグ"].get<bool>();
						else if (node_json.contains("IsSubGraph")) node.is_sub_graph = node_json["IsSubGraph"].get<bool>();

						// サブグラフID
						if (node_json.contains(u8"サブグラフID")) node.sub_graph_id = node_json[u8"サブグラフID"].get<uint32_t>();
						else if (node_json.contains("SubGraphID")) node.sub_graph_id = node_json["SubGraphID"].get<uint32_t>();

						// ループフラグ
						if (node_json.contains(u8"ループフラグ")) node.is_loop = node_json[u8"ループフラグ"].get<bool>();
						else if (node_json.contains("IsLoop")) node.is_loop = node_json["IsLoop"].get<bool>();
						else node.is_loop = true;

						// ルートモーションフラグ
						if (node_json.contains(u8"ルートモーションフラグ")) node.is_root_motion = node_json[u8"ルートモーションフラグ"].get<bool>();
						else if (node_json.contains("IsRootMotion")) node.is_root_motion = node_json["IsRootMotion"].get<bool>();
						else node.is_root_motion = false;

						node.parent_node_id = parent_id;

						// 入力ピン (新形式: "入力ピン" / 旧形式: "Input")
						const char* input_key = node_json.contains(u8"入力ピン") ? u8"入力ピン" : (node_json.contains("Input") ? "Input" : nullptr);
						if (input_key && node_json[input_key].is_array())
						{
							for (size_t p = 0; p < node_json[input_key].size(); p++)
							{
								const auto& pin_obj = node_json[input_key][p];
								uint32_t pin_id = pin_obj.contains(u8"ピンID") ? pin_obj[u8"ピンID"].get<uint32_t>() : pin_obj["ID"].get<uint32_t>();
								node.inputs.push_back(pin_id);
							}
						}

						// 出力ピン (新形式: "出力ピン" / 旧形式: "Output")
						const char* output_key = node_json.contains(u8"出力ピン") ? u8"出力ピン" : (node_json.contains("Output") ? "Output" : nullptr);
						if (output_key && node_json[output_key].is_array())
						{
							for (size_t p = 0; p < node_json[output_key].size(); p++)
							{
								const auto& pin_obj = node_json[output_key][p];
								uint32_t pin_id = pin_obj.contains(u8"ピンID") ? pin_obj[u8"ピンID"].get<uint32_t>() : pin_obj["ID"].get<uint32_t>();
								node.outputs.push_back(pin_id);
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

			// リンク・条件のパース
			if (layer.contains("Links") && layer["Links"].is_array())
			{
				for (size_t j = 0; j < layer["Links"].size(); j++)
				{
					const auto& link_json = layer["Links"][j];

					SimulatorRuntimeLink link;
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
								else if (std::holds_alternative<DirectX::XMFLOAT3>(raw_data))
								{
									DirectX::XMFLOAT3 vec_ref = { 0.0f, 0.0f, 0.0f };
									if (cond_json.contains("VectorRefValue") && cond_json["VectorRefValue"].is_array() && cond_json["VectorRefValue"].size() == 3)
									{
										vec_ref.x = cond_json["VectorRefValue"][0].get<float>();
										vec_ref.y = cond_json["VectorRefValue"][1].get<float>();
										vec_ref.z = cond_json["VectorRefValue"][2].get<float>();
									}
									cond.reference_value = vec_ref;
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

	// シミュレータへのグラフデータ移譲
	if (state_graph_simulator)
	{
		state_graph_simulator->SetupRuntimeGraph(runtime_nodes, runtime_links, layer_entry_nodes);
		state_graph_simulator->SetCurreentNodeID(current_node_id);
	}
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
