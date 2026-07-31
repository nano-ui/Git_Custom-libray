#pragma once

#include <string>

class AnimationBlender
{
public:
	//コンストラクタ
	AnimationBlender() = default;

	//デストラクタ
	~AnimationBlender() = default;

	//補完の開始設定
	void StartCrossFade(const std::string& prev_name, float prev_time, float duration);

	//時間更新処理
	void Update(float elapsed_time);

	//補完状態のリセット
	void Reset();

	//補完フラグの取得
	bool IsBlending()const { return is_blending; }

	//補完率の取得
	float GetBlendFactor()const;

	//遷移前のアニメーション名の取得
	const std::string& GetPreviousAnimName()const { return previous_anim_name; }

	//遷移前のアニメーション時間取得
	float GetPreviousAnimTime()const { return previous_anim_time; }

private:
	//補完率の計算
	float CalculateSmoothFactor(float raw_factor)const;

private:
	std::string previous_anim_name = "";	//遷移前のアニメーション名
	float previous_anim_time = 0.0f;		//遷移前のアニメーション再生時間
	float blend_timer = 0.0f;				//ブレンド経過時間
	float blend_duration = 0.2f;			//ブレンド目標時間
	bool is_blending = false;				//補完フラグ
};

