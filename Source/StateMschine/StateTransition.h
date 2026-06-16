#pragma once
#include <cstdint>
#include <vector>

#include "StateBlackboard.h"

class StateTransition
{
public:
	//条件が満たされているか判定
	bool CanTransition(const StateBlackboard& blackboard)const;

private:
	uint32_t next_state_hash = 0;					//遷移先のステート
	std::vector<TransitionCondition> conditions;	//遷移するための条件
};

