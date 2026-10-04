#pragma once
#include "JudgmentNode.h"

#include <random>
#include <functional>

class CooldownJudgment : public JudgmentNode
{
public:
	//コンストラクタ
	CooldownJudgment(std::reference_wrapper<const float> cooldown_time, std::reference_wrapper<const float> random_range);

	//判定
	bool Check()override;

	////クールダウン開始処理
	void StartCooldown();

	//更新処理
	void Update(float elapsed_time);

	//クールタイムの残り時間を取得
	float GetRemainingTime() const;

	//クールタイムの進行度を取得
	float GetCooldownProgress() const;

private:
	std::reference_wrapper<const float> base_duration;		//基本の待ち時間
	std::reference_wrapper<const float> random_range;		//ランダム幅
	float current_duration;			//加算されるランダムの最大幅
	float remaining_cooldown_time;	//残り時間
};

