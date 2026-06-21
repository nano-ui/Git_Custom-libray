#pragma once
#include "JudgmentNode.h"

#include <random>
#include <functional>

class CooldownJudgment : public JudgmentNode
{
public:
	CooldownJudgment(std::reference_wrapper<const float> cooldown_time, std::reference_wrapper<const float> random_range);

	bool Check()override;

	//行動開始時に呼び出して時間を記録
	void NotifyExecution(float current_time);

	//クールタイムの残り時間を取得
	float GetRemainingTime() const;

	//クールタイムの進行度を取得
	float GetCooldownProgress() const;

private:
	std::reference_wrapper<const float> base_duration;		//基本の待ち時間
	std::reference_wrapper<const float> random_range;		//ランダム幅
	float current_duration;		//加算されるランダムの最大幅
	float last_execution_time;	//最後に実行した時間
};

