#include "CooldownJudgment.h"

//====================
//コンストラクタ
//====================
CooldownJudgment::CooldownJudgment(std::reference_wrapper<const float> cooldown_time, std::reference_wrapper<const float> random_range)
	:base_duration(cooldown_time),
	random_range(random_range),
	current_duration(cooldown_time),
	last_execution_time(-cooldown_time * 2.0f)
{
}

//============
//判定処理
//============
bool CooldownJudgment::Check()
{
	//float current_total_time = TimeManager::Instance().GetTotalTime();

	//if (current_total_time - last_execution_time >= current_duration)
	//{
	//	return true;
	//}

	return false;
}

//====================================
//行動開始時に呼び出して時間を記録
//====================================
void CooldownJudgment::NotifyExecution(float current_time)
{
	last_execution_time = current_time;	//最後に実行した時間を更新
	
	//------------------------------------
	//短縮・延長を含めたランダム計算
	//------------------------------------
	static std::random_device rd;
	static std::mt19937 gen(rd());
	std::uniform_real_distribution<float>dist(-random_range, random_range);

	current_duration = base_duration + dist(gen);	//次回の待ち時間を「基本時間 + ランダム値」で決定

	if (current_duration < 0.5f)
	{
		current_duration = 0.5f;
	}
}

//================================
//クールタイムの残り時間を取得
//================================
float CooldownJudgment::GetRemainingTime() const
{
	//float current_total_time = TimeManager::Instance().GetTotalTime();
	//float elapsed = current_total_time - last_execution_time;
	//float remaining = current_duration - elapsed;

	//return remaining > 0.0f ? remaining : 0.0f;
	return 0.0f;
}

//=============================
//クールタイムの進行度を取得
//=============================
float CooldownJudgment::GetCooldownProgress() const
{
	//if (current_duration <= 0.0f) return 1.0f;

	//float current_total_time = TimeManager::Instance().GetTotalTime();
	//float elapsed = current_total_time - last_execution_time;
	//float progress = elapsed / current_duration;

	//return progress > 1.0f ? 1.0f : progress;
	return false;
}
