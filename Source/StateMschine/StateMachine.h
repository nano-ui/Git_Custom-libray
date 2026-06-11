#pragma once

#include <unordered_map>
#include <memory>
#include <string>

class State;
class StateBlackboard;

class StateMachine
{
public:
	//コンストラクタ
	StateMachine();

	//デストラクタ
	~StateMachine();

	//更新
	void Update(float elapsed_time);

	//状態の追加
	void AddState(std::unique_ptr<State> state);

	//外部からの状態の切り替え
	void ChangeState(const std::string& name);

	//ブラックボードへのアクセス
	StateBlackboard* GetBlackboard()const { return blackboard.get(); }

private:
	//実際の遷移処理
	void DoTransition(const std::string& name);

private:
	std::unordered_map<std::string, std::unique_ptr<State>> state_map;	//状態名で検索できる辞書
	State* current_state;							//現在のアクティブな状態
	std::unique_ptr<StateBlackboard> blackboard;	//データ箱
};

