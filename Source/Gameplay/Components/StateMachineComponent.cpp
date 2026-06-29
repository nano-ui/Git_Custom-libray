#include "StateMachineComponent.h"
#include "../Serialization/JsonSerializer.h"
#include "../Gameplay/StateMachine/StateBlackboard.h"
#include "../ThiedParty/json.hpp"

#include <fstream>
#include <iostream>

//コンストラクタ
StateMachineComponent::StateMachineComponent()
{
	state_machine_path = "";
	model_hash = 0;
}

//デストラクタ
StateMachineComponent::~StateMachineComponent() = default;

//初期化
void StateMachineComponent::Initialize()
{
	//プレハブJSONからパスが正しく読み込まれているか判定
	if (!state_machine_path.empty())
	{
		LoadAnimationMap();
	}
	else
	{
		state_machine_path = "Data/Json/Kari.json";
		LoadAnimationMap();
	}
}

//更新
void StateMachineComponent::Update(float elapsed_time, StateBlackboard* blackboard)
{
	//ブラックボードが有効か確認
	if (!blackboard)
	{
		return;
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
void StateMachineComponent::LoadAnimationMap()
{
	std::ifstream input_file(state_machine_path);	//ファイルをオープン

	//ファイルが開けたか確認
	if (!input_file.is_open())
	{
		std::cerr << "Warning: StateMachineComponent - ファイルを開けませんでした: " << state_machine_path << std::endl;
		return;
	}

	nlohmann::json root_json;	//解析用オブジェクト
	input_file >> root_json;
	input_file.close();

	animation_map.clear();

	//ルートノードにLayersが含まれているか判定
	if (root_json.is_object() && root_json.contains("Layers") && root_json["Layers"].is_array()) 
	{
		//各レイヤーをループして走査
		for (size_t i = 0; i < root_json["Layers"].size(); i++)
		{
			const auto& layer = root_json["Layers"][i];

			//レイヤー内にNodesが含まれているか判定
			if (layer.contains("Nodes"))
			{
				//レイヤー内の全ノードをループして走査
				for (size_t j = 0; j < layer["Nodes"].size(); j++)
				{
					const auto& node = layer["Nodes"][j];

					//通常ステートかつアニメーション名が記載されているか判定
					if (node.contains("IsSubGraph") && !node["IsSubGraph"].get<bool>())
					{
						uint32_t node_id = node["ID"].get<uint32_t>();
						std::string anim_name = node["AnimationName"].get<std::string>();

						//アニメーション名が空欄でなければ対応表に追加
						if (!anim_name.empty())
						{
							animation_map[node_id] = anim_name;
						}
					}
				}
			}
		}
	}
	printf("StateMachineComponent: 「%s」から %zu 個のアニメーション対応ステートを自動適用しました。\n",
		state_machine_path.c_str(), animation_map.size());
}