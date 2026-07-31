#include "AnimationBlender.h"

#include <algorithm>

//補完の開始設定
void AnimationBlender::StartCrossFade(const std::string& prev_name, float prev_time, float duration)
{
	previous_anim_name = prev_name;
	previous_anim_time = prev_time;
	blend_duration = (duration > 0.0f) ? duration : 0.0f;
	blend_timer = 0.0f;
	is_blending = true;
}

//時間更新処理
void AnimationBlender::Update(float elapsed_time)
{
	if (!is_blending)return;
	blend_timer += elapsed_time;

	//目標時間に達したら補完完了
	if (blend_timer >= blend_duration)
	{
		blend_timer = blend_duration;
		is_blending = false;
	}
}

//補完状態のリセット
void AnimationBlender::Reset()
{
	previous_anim_name = "";
	previous_anim_time = 0.0f;
	blend_timer = 0.0f;
	blend_duration = 0.0f;
	is_blending = false;
}

//補完率の取得
float AnimationBlender::GetBlendFactor() const
{
	if (!is_blending || blend_duration <= 0.0f)return 1.0f;

	//経過時間の割合を算出
	float raw_factor = std::clamp(blend_timer / blend_duration, 0.0f, 1.0f);

	return CalculateSmoothFactor(raw_factor);
}

//補完率の計算
float AnimationBlender::CalculateSmoothFactor(float raw_factor) const
{
	//エルミート補完
	return raw_factor * raw_factor * (3.0f - 2.0f * raw_factor);
}
