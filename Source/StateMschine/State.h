#pragma once

#include <string>
#include <vector>
#include <memory>

#include "StateTransition.h"

class State;
class StateBlackboard;

class State
{
public:
	//コンストラクタ
	State(const std::string& name):state_name(name){}

	//デストラクタ
	virtual ~State() = default;

	//入力
	virtual void Enter() {}

	//更新
	virtual void Update(float elapsed_time, StateBlackboard* blackboard) = 0;

	//終了
	virtual void Exit() {}

	//遷移の追加
	void AddTransition(std::unique_ptr<StateTransition> transition)
	{
		transitions.push_back(std::move(transition));
	}

	//遷移リストの取得
	const std::vector<std::unique_ptr<StateTransition>>& GetTransitions()const { return transitions; }

	//状態名の取得
	const std::string& GetName()const { return state_name; }

private:
	std::string state_name;	//状態名
	std::vector<std::unique_ptr<StateTransition>> transitions;	//状態から伸びる遷移のリスト
};

