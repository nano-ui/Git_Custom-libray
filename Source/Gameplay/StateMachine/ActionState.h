#pragma once
#include "State.h"

#include <memory>
#include <string>

class MoveBase;
class RotationBase;

class ActionState : public State
{
public:
	//コンストラクタ
	ActionState(
		const std::string& name,
		std::unique_ptr<MoveBase> move,
		std::unique_ptr<RotationBase>rot,
		const std::string& speed_name,
		const std::string& my_pos_name,
		const std::string& target_pos_name);

	//デストラクタ
	virtual ~ActionState() = default;

	//更新
	virtual void Update(float elapsed_time, StateBlackboard* blackboard)override;

private:
	std::unique_ptr<MoveBase> move_component;		//移動制御コンポーネント
	std::unique_ptr<RotationBase> rot_compinent;	//回転制御コンポーネント
	uint32_t speed_key;								//速度用ハッシュキー
	uint32_t my_pos_key;							//自身の座標用ハッシュキー
	uint32_t target_pos_key;						//対象の座標用ハッシュキー
};

