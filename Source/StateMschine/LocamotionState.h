#pragma once

#include <DirectXMath.h>

#include "State.h"

class StateBlackboard;

class LocamotionState :public State
{
public:
	//コンストラクタ
	LocamotionState();

	//デストラクタ
	~LocamotionState();

	//状態に入った瞬間
	void Enter()override;

	//更新
	void Update(float elapsed_time, StateBlackboard* blackboard)override;

	//状態から抜ける瞬間
	void Exit()override;

private:
	//入力ベクトルの長さを計算
	float CalculateINputLength(const DirectX::XMFLOAT3& input);

private:
	float move_speed_max;	//この状態での最高移動速度
};

