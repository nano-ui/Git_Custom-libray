#include "StateMachine.h"

#include "State.h"
#include "StateBlackboard.h"

//コンストラクタ
StateMachine::StateMachine()
	:current_state(nullptr)
{
	blackboard = std::make_unique<StateBlackboard>();
}

//デストラクタ
StateMachine::~StateMachine()
{

}

//更新
void StateMachine::Update(float elapsed_time)
{
	if (!current_state)return;

	//現在の状態から伸びている遷移条件を全てループしてチェック
	for (const auto& transition : current_state->GetTransitions())
	{
		if (!transition)continue;

		//条件が満たされた瞬間、その遷移先へ切り替える
		if (transition->IsTriggered(blackboard.get()))
		{
			DoTransition(transition->GetNextState());
			break;
		}
	}
	current_state->Update(elapsed_time, blackboard.get());
}

//状態の追加
void StateMachine::AddState(std::unique_ptr<State> state)
{
	if (!state)return;

	std::string name = state->GetName();
	state_map[name] = std::move(state);

	if (!current_state)
	{
		current_state = state_map[name].get();
		current_state->Enter();
	}
}

//外部からの状態の切り替え
void StateMachine::ChangeState(const std::string& name)
{
	DoTransition(name);
}

//実際の遷移処理
void StateMachine::DoTransition(const std::string& name)
{
	//目的の状態が辞書に登録されているか検索
	auto iterator = state_map.find(name);
	if (iterator == state_map.end())
	{
		return;
	}

	//現在の状態の終了イベントを呼び出す
	if (current_state)
	{
		current_state->Exit();
	}

	//新しい状態へポインタを切り替え
	current_state = iterator->second.get();

	//新しい状態の開始イベントを呼び出す
	if (current_state)
	{
		current_state->Enter();
	}
}
