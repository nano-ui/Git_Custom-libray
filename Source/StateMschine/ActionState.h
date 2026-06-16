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
	ActionState(const std::string& name);

	//デストラクタ
	virtual ~ActionState() = default;

	//更新
	virtual void Update(float elapsed_time, StateBlackboard* blackboard)override;

private:
	std::unique_ptr<MoveBase> move_component;		//移動制御コンポーネント
	std::unique_ptr<RotationBase> rot_compinent;	//回転制御コンポーネント
	uint32_t speed_key;								//速度用ハッシュキー
};

