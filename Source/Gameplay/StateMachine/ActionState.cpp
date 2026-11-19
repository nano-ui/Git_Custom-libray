#include "ActionState.h"
#include "Gameplay\Components\Movement\Move\MoveBase.h"
#include "Gameplay\Components\Movement\Rotation\RotationBase.h"
#include "Gameplay\Common\Blackboard.h"

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
	speed_key = Blackboard::GetKeyId(speed_name);
	my_pos_key = Blackboard::GetKeyId(my_pos_name);
	target_pos_key = Blackboard::GetKeyId(target_pos_name);
}

//更新
void ActionState::Update(float elapsed_time, Blackboard* blackboard)
{
	//パラメータ取得
	float speed = blackboard->GetValue(speed_key, speed);
	DirectX::XMFLOAT3 my_pos = blackboard->GetValue(my_pos_key, my_pos);
	DirectX::XMFLOAT3 target_pos = blackboard->GetValue(target_pos_key, target_pos);

	if (move_component)
	{
		move_component->Update(my_pos, target_pos, blackboard);
	}
}
