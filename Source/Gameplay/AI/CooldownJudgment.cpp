#include "CooldownJudgment.h"

//コンストラクタ
CooldownJudgment::CooldownJudgment(std::reference_wrapper<const float> cooldown_time, std::reference_wrapper<const float> random_range)
	:base_duration(cooldown_time),
	random_range(random_range),
	current_duration(cooldown_time),
	remaining_cooldown_time(0.0f)
{
}

//判定処理
bool CooldownJudgment::Check()
{
	if (remaining_cooldown_time <= 0.0f)
	{
		return true;
	}
	return false;
}

//クールダウン開始処理
void CooldownJudgment::StartCooldown()
{
	//乱数生成エンジンの初期化
	static std::random_device seed_generator;			//シード生成器
	static std::mt19937 random_engin(seed_generator());	//疑似乱数生成エンジン

	//ランダム変動幅の抽選
	const float range = random_range.get();								//変動範囲の絶対値
	std::uniform_real_distribution<float> distribution(-range, range);	//実数乱数分布クラス
	const float random_offset = distribution(random_engin);				//ランダムな変動値

	//クールダウンの計算
	current_duration = base_duration.get() + random_offset;

	//最低保証時の下限ガード
	constexpr float MINIMUM_COOLDOWN_DURATION = 0.5f;

	if (current_duration < MINIMUM_COOLDOWN_DURATION)
	{
		current_duration = MINIMUM_COOLDOWN_DURATION;
	}

	//タイマーの初期化
	remaining_cooldown_time = current_duration;
}

//更新処理
void CooldownJudgment::Update(float elapsed_time)
{
	//クールダウン中か判定
	if (remaining_cooldown_time > 0.0f)
	{
		remaining_cooldown_time -= elapsed_time;

		if (remaining_cooldown_time < 0.0f)
		{
			remaining_cooldown_time = 0.0f;
		}
	}
}

//クールタイムの残り時間を取得
float CooldownJudgment::GetRemainingTime() const
{
	return remaining_cooldown_time;
}

//クールタイムの進行度を取得
float CooldownJudgment::GetCooldownProgress() const
{
	if (current_duration <= 0.0f)
	{
		return 1.0f;
	}

	float total_elapsed_time = current_duration - remaining_cooldown_time;	//経過時間

	//進行度 = 経過時間/全体時間
	float progress = total_elapsed_time / current_duration;					//進行度

	//最終的な進行度 = min(1.0f, max(0.0f,進行度))
	float final_progress = std::min(1.0f, std::max(0.0f, progress));		//最終的な進行度
	
	return final_progress;
}
