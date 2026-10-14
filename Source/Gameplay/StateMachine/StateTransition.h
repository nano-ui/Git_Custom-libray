#pragma once
#include <cstdint>
#include <vector>

#include "StateBlackboard.h"

class StateTransition
{
public:
	//条件が満たされているか判定
	bool CanTransition(const StateBlackboard& blackboard)const;

	//遷移条件の追加
	void AddCondition(TransitionCondition& cond) { conditions.push_back(cond); }

	//遷移先のステートのハッシュ値を取得
	uint32_t GetNextStateHash() const { return next_state_hash; }

	//ブレンド時間を取得
	float GetBlendTime()const { return blend_time; }

	//ブレンド時間を設定
	void SetBlendTime(float blend) { blend_time = blend; }

private:
	uint32_t next_state_hash = 0;					//遷移先のステート
	std::vector<TransitionCondition> conditions;	//遷移するための条件
	float blend_time = 0.0f;						//ブレンド時間
};

