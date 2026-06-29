#include "StateMachineComponent.h"
#include "../Serialization/JsonSerializer.h"
#include "../Gameplay/StateMachine/StateBlackboard.h"
#include "../ThiedParty/json.hpp"
#include "../Gameplay/Components/AnimationComponent.h"

#include <fstream>
#include <iostream>

//コンストラクタ
StateMachineComponent::StateMachineComponent()
{
	state_machine_path = "";
	model_hash = 0;
	current_node_id = UINT32_MAX;
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

	if (current_node_id == UINT32_MAX)
	{
		// パスが完全に空っぽか判定
		if (state_machine_path.empty())
		{
			state_machine_path = "Data/Json/Kari.json"; // 最低限のフォールバックパスを設定
		}

		LoadAnimationMap(blackboard); // グラフ構造を再ロードしてメモリに展開

		// 正しくノードが展開されたか確認判定
		if (!runtime_nodes.empty())
		{
			current_node_id = runtime_nodes.front().id; // 先頭の有効なノードIDをセット
			current_animation_name = runtime_nodes.front().animation_name;
		}
		else
		{
			return;
		}
	}

	uint32_t next_node_id = current_node_id; // 次の遷移先候補ノードIDの初期化変数
	bool is_transition_triggered = false;     // 実際に遷移を発生させるかどうかの条件判定フラグ

	for (size_t i = 0; i < runtime_links.size(); i++)
	{
		const RuntimeLink& link = runtime_links[i]; // ループ内の精査対象リンク
		uint32_t src_node_id = GetNodeIdFromPinId(link.start_pin_id); // リンク元のノードIDをピンから逆引き抽出

		if (src_node_id == current_node_id)
		{
			bool is_all_conditions_met = !link.conditions.empty(); // 全条件クリアフラグ

			for (size_t c = 0; c < link.conditions.size(); c++)
			{
				if (!link.conditions[c].IsJudgment(*blackboard))
				{
					is_all_conditions_met = false;
					break;
				}
			}

			if (is_all_conditions_met)
			{
				next_node_id = GetNodeIdFromPinId(link.end_pin_id); // 遷移先のノードIDをピンから逆引き抽出
				is_transition_triggered = true;
				break;
			}
		}
	}

	if (is_transition_triggered && next_node_id != current_node_id)
	{
		current_node_id = next_node_id; // 現在のステートIDを更新

		bool check_sub_graph = true; // ループ制御フラグ

		while (check_sub_graph)
		{
			check_sub_graph = false;
			for (size_t n = 0; n < runtime_nodes.size(); n++)
			{
				if (runtime_nodes[n].id == current_node_id)
				{
					if (runtime_nodes[n].is_sub_graph)
					{
						uint32_t sub_layer_id = runtime_nodes[n].sub_graph_id; // 下位レイヤーのID
						auto entry_it = layer_entry_nodes.find(sub_layer_id); // その階層の先頭ステートを検索

						if (entry_it != layer_entry_nodes.end())
						{
							current_node_id = entry_it->second; // 内部の初期ステートIDへ差し替え
							check_sub_graph = true; // 潜った先がさらにサブグラフだった場合に備えてループを継続
						}
					}
					break;
				}
			}
		}
		for (size_t n = 0; n < runtime_nodes.size(); n++)
		{
			const RuntimeNode& node = runtime_nodes[n]; // 精査対象ノード

			if (node.id == current_node_id)
			{
				current_animation_name = node.animation_name;
				break;
			}
		}
	}
}

//シリアライズ変数登録
void StateMachineComponent::SetupSerialization(JsonSerializer* serializer)
{
	//シリアライザのポインタが有効か確認
	if (serializer)
	{
		serializer->RegisterVariable("StateMachinePath", &state_machine_path);
	}
}

//ステートマシンJSONからアニメーション対応表を抽出
void StateMachineComponent::LoadAnimationMap(StateBlackboard* blackboard)
{
	std::ifstream input_file(state_machine_path); // ファイルのストリーム変数

	if (!input_file.is_open())
	{
		std::cerr << "Warning: StateMachineComponent - ファイルを開けませんでした: " << state_machine_path << std::endl;
		return;
	}

	nlohmann::json root_json; //パース用オブジェクト変数
	input_file >> root_json;
	input_file.close();

	animation_map.clear();
	runtime_nodes.clear();
	runtime_links.clear();

	if (root_json.is_object() && root_json.contains("Layers") && root_json["Layers"].is_array())
	{
		for (size_t i = 0; i < root_json["Layers"].size(); i++)
		{
			const auto& layer = root_json["Layers"][i]; //現在走査中のレイヤーデータ
			uint32_t layer_graph_id = layer["GraphID"].get<uint32_t>(); //レイヤー自体の識別ID

			if (layer.contains("Nodes") && layer["Nodes"].is_array())
			{
				for (size_t j = 0; j < layer["Nodes"].size(); j++)
				{
					const auto& node_json = layer["Nodes"][j]; // 現在走査中のノードJSON

					if (node_json.is_object())
					{
						RuntimeNode node; // メモリに登録するランタイムノード変数
						node.id = node_json["ID"].get<uint32_t>();
						node.name = node_json["Name"].get<std::string>();
						node.animation_name = node_json.contains("AnimationName") ? node_json["AnimationName"].get<std::string>() : "";
						node.is_sub_graph = node_json["IsSubGraph"].get<bool>();
						node.sub_graph_id = node_json["SubGraphID"].get<uint32_t>();

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

						// 各階層のj番目（最初）のノードを、そのレイヤーの初期エントリーノードとして記憶
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
					const auto& link_json = layer["Links"][j]; //現在走査中の接続線JSON

					RuntimeLink link; //メモリに登録するランタイムリンク変数
					link.id = link_json["ID"].get<uint32_t>();
					link.start_pin_id = link_json["StartPinID"].get<uint32_t>();
					link.end_pin_id = link_json["EndPinID"].get<uint32_t>();

					if (link_json.contains("Conditions") && link_json["Conditions"].is_array())
					{
						for (size_t c = 0; c < link_json["Conditions"].size(); c++)
						{
							const auto& cond_json = link_json["Conditions"][c]; // 遷移条件JSON

							TransitionCondition cond; //メモリに登録するランタイム条件変数
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