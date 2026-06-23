#include "ActionState.h"
#include "../Gameplay/Movement/MoveBase.h"
#include "../Gameplay/Movement/RotationBase.h"

//コンストラクタ
ActionState::ActionState(
	const std::string& name,
	std::unique_ptr<MoveBase> move,
	std::unique_ptr<RotationBase> rot,
	const std::string& speed_name,
	const std::string& my_pos_name,
	const std::string& target_pos_name)
	:State(name)
	,move_component(std::move(move))
	,rot_compinent(std::move(rot))
{
	speed_key = StateBlackboard::CalculateHash(speed_name);
	my_pos_key = StateBlackboard::CalculateHash(my_pos_name);
	target_pos_key = StateBlackboard::CalculateHash(target_pos_name);
}

//更新
void ActionState::Update(float elapsed_time, StateBlackboard* blackboard)
{
	//パラメータ取得
	float speed = std::get<float>(blackboard->GetAttributeValue(speed_key));
	DirectX::XMFLOAT3 my_pos = std::get<DirectX::XMFLOAT3>(blackboard->GetAttributeValue(my_pos_key));
	DirectX::XMFLOAT3 target_pos = std::get<DirectX::XMFLOAT3>(blackboard->GetAttributeValue(target_pos_key));

	if (move_component)
	{
		move_component->Update(my_pos, target_pos, blackboard);
	}
}
