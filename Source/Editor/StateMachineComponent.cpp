#include "StateMachineComponent.h"

#include "../GameObjects/GameObject.h"
#include "../StateMschine/StateMachine.h"
#include "../StateMschine/State.h"
#include "../StateMschine/StateBlackboard.h"
#include "../StateMschine/VariableTransition.h"
#include "../Serialization/JsonSerializer.h"

#include <fstream>
#include <cassert>
#include "StateNodeData.h"

//共通条件と個別条件を組み合わせる複合遷移クラス
class CombinedAndTransition : public Transition
{
public:
	//コンストラクタ
	CombinedAndTransition(const std::string& next_state_name,
		std::unique_ptr<VariableTransition> first_cond,
		std::unique_ptr<VariableTransition> second_cond)
		: Transition(next_state_name)
		, condition_first(std::move(first_cond))
		, condition_second(std::move(second_cond))
	{
	}

	//デストラクタ
	~CombinedAndTransition() override = default;

	//両方の条件を満たしているかチェック）
	bool IsTriggered(StateBlackboard* blackboard) override
	{
		//エラーが起きる可能性のある個所へのデバッグ出力とガード
		if (!condition_first || !condition_second)
		{
#ifdef _DEBUG
			OutputDebugStringA("[Error] CombinedAndTransition::IsTriggered - Sub-condition is null.\n");
#endif
			return false;
		}

		//2つの条件がどちらも真（true）である場合のみ遷移を許可する
		return condition_first->IsTriggered(blackboard) && condition_second->IsTriggered(blackboard);
	}

private:
	std::unique_ptr<VariableTransition> condition_first;  //1つ目の判定条件（共通条件など）
	std::unique_ptr<VariableTransition> condition_second; //2つ目の判定条件（個別条件など）
};

//実行時に動的にステートを生成
class DynamicCustomState : public State
{
public:
	//コンストラクタ
	DynamicCustomState(const std::string& name, const std::string& anim_name, bool loop, ActionCategory category)
		: State(name), clip_name(anim_name), is_loop(loop), state_category(category) {
	}

	//更新
	void Update(float elapsed_time, StateBlackboard* blackboard) override
	{
		if (!blackboard) return;
		blackboard->action_category = state_category;

		if (state_category == ActionCategory::Locomotion)
		{
			blackboard->target_move_speed = 5.0f; 
		}
		else
		{
			blackboard->target_move_speed = 0.0f;
		}
	}

	//アニメーション名を取得
	const std::string& GetAnimClipName() const { return clip_name; }

	//ループフラグを取得
	bool IsAnimLoop() const { return is_loop; }

private:
	std::string clip_name;				//アニメーション名
	bool is_loop;						//ループフラグ
	ActionCategory state_category;		//ステートの属性
};

//コンストラクタ
StateMachineComponent::StateMachineComponent(GameObject* owner_object)
	:EditorConponents(owner_object)
	,state_machine(nullptr)
	,state_machine_config_path("")
{

}

//デストラクタ
StateMachineComponent::~StateMachineComponent()
{

}

//初期化
void StateMachineComponent::Initialize()
{

}

//更新
void StateMachineComponent::Update(float elapsed_time)
{
	if (!state_machine || !is_active)return;

	state_machine->Update(elapsed_time);

	if (owner)
	{
		Model* target_model = owner->GetModel();
		if (target_model)
		{
			target_model->Update(elapsed_time);
			std::string current_clip = GetCurrentStateAnimationClip();
			bool is_loop = IsCurrentStateAnimLoop();

			if (!current_clip.empty() && current_clip != privious_clip_name)
			{
				target_model->PlayAnimation(current_clip, is_loop);
				privious_clip_name = current_clip;
			}
		}
	}
}

//シリアライズの窓口
void StateMachineComponent::SetupSerialization(JsonSerializer* serializer)
{
	if (!serializer) return; 
	serializer->RegisterVariable("StateMachineConfigPath", &state_machine_config_path);
}

//シリアライズ読み込み完了後のフック
void StateMachineComponent::OnPostLoad()
{
	if (!state_machine_config_path.empty())
	{
		BuildStateMachineFronJson(state_machine_config_path);
	}
}

//エディタから直接パスを流し込まれる窓口
void StateMachineComponent::LoadStateMachineConfig(const std::string& file_path)
{
	state_machine_config_path = file_path;
	BuildStateMachineFronJson(file_path);
}

//アニメーション名を取得
std::string StateMachineComponent::GetCurrentStateAnimationClip() const
{
	if (state_machine)
	{
		State* current = state_machine->GetCurrentState();
		auto custom_state = dynamic_cast<DynamicCustomState*>(current);
		if (custom_state)
		{
			return custom_state->GetAnimClipName();
		}
	}
	return "Idle";
}

//ループフラグを取得
bool StateMachineComponent::IsCurrentStateAnimLoop() const
{
	if (state_machine)
	{
		State* current = state_machine->GetCurrentState();
		auto custom_state = dynamic_cast<DynamicCustomState*>(current);
		if (custom_state)
		{
			return custom_state->IsAnimLoop();
		}
	}
	return true;
}

//JSON解析
void StateMachineComponent::BuildStateMachineFronJson(const std::string& file_path)
{
	std::ifstream input_file(file_path);
	if (!input_file.is_open())
	{
		assert(false && "GameObject::BuildStateMachineFromJson - Failed to open setup JSON file.");
		return;
	}

	nlohmann::json root_json;
	input_file >> root_json;
	input_file.close();

	state_machine = std::make_unique<StateMachine>();

	std::string initial_state = "";
	if (root_json.contains("initial_state"))
	{
		initial_state = root_json["initial_state"].get<std::string>();
	}

	if (root_json.contains("states") && root_json["states"].is_array())
	{
		//JSON内に記録されている各ステート情報を走査してマップに一時登録するための準備
		std::unordered_map<std::string, const nlohmann::json*> json_state_lookups;
		for (const auto& node_json : root_json["state"])
		{
			std::string name = node_json["name"].get<std::string>();
			std::string clip = node_json["clip_name"].get<std::string>();
			bool loop = node_json["is_loop"].get<bool>();

			//ステート名から適切なアクションカテゴリーを識別
			ActionCategory category = ActionCategory::Idle;
			if (name == "Locomotion" || name == "Run") category = ActionCategory::Locomotion;
			else if (name == "Attack") category = ActionCategory::Attack;
			else if (name == "Avoid")  category = ActionCategory::Avoid;

			auto state_ptr = std::make_unique<DynamicCustomState>(name, clip, loop, category);

			if (node_json.contains("transitions") && node_json["transitions"].is_array())
			{
				for (const auto& trans_json : node_json["transitions"])
				{
					std::string next_state = trans_json["next_state"].get<std::string>();
					TransitionBlendMode blend = static_cast<TransitionBlendMode>(trans_json["blend_mode"].get<int>());

					//個別条件のパラメータ
					ConditionType type = static_cast<ConditionType>(trans_json["condition_type"].get<int>());
					ConditionOp op = static_cast<ConditionOp>(trans_json["operator_type"].get<int>());
					float comp_value = trans_json["compare_value"].get<float>();
					std::string act_name = trans_json["target_action_name"].get<std::string>();

					//個別（固有）の条件遷移インスタンスを作成
					std::unique_ptr<VariableTransition> individual_trans;
					if (type == ConditionType::ActionPressed)
					{
						individual_trans = std::make_unique<VariableTransition>(next_state, type, op, act_name);
					}
					else
					{
						individual_trans = std::make_unique<VariableTransition>(next_state, type, op, comp_value);
					}

					bool is_combined_successful = false;

					//組み合わせ（AND）モード、かつノード側に共通条件が設定されている場合
					if (blend == TransitionBlendMode::CombineWithCommon) 
					{
						//遷移先（ターゲット）のステートノード情報を検索
						auto target_it = json_state_lookups.find(next_state);
						if (target_it != json_state_lookups.end())
						{
							const nlohmann::json& target_node_json = *(target_it->second);

							//接続先のステートノード側で「共通条件を強制する」が有効になっているか確認
							if (target_node_json.contains("has_common_condition") && target_node_json["has_common_condition"].get<bool>())
							{
								//接続先ノードが持つ共通の条件パラメータを動的に取得
								ConditionType c_type = static_cast<ConditionType>(target_node_json["common_condition_type"].get<int>());
								ConditionOp c_op = static_cast<ConditionOp>(target_node_json["common_operator_type"].get<int>());
								float c_value = target_node_json["common_compare_value"].get<float>();
								std::string c_act_name = target_node_json["common_target_action_name"].get<std::string>();

								//共通条件を評価するためのVariableTransitionを生成
								std::unique_ptr<VariableTransition> common_trans;
								if (c_type == ConditionType::ActionPressed)
								{
									common_trans = std::make_unique<VariableTransition>(next_state, c_type, c_op, c_act_name);
								}
								else
								{
									common_trans = std::make_unique<VariableTransition>(next_state, c_type, c_op, c_value);
								}

								//共通条件と個別条件を内包した複合遷移（AND）として登録
								auto combined_ptr = std::make_unique<CombinedAndTransition>(
									next_state, std::move(common_trans), std::move(individual_trans)
								);
								state_ptr->AddTransition(std::move(combined_ptr));
								is_combined_successful = true;
							}
						}
					}
					if (!is_combined_successful)
					{
						state_ptr->AddTransition(std::move(individual_trans));
					}
				}
			}
			state_machine->AddState(std::move(state_ptr));
		}
	}
}
