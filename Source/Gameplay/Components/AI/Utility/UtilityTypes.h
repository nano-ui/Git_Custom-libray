#pragma once
#include <functional>
#include <vector>
#include <string>

//行動候補情報
struct UtilityAction
{
	std::string action_name;									//行動名
	std::vector<std::function<float()>> required_conditions;	//必須条件群
	std::vector<std::function<float()>> bonus_conditions;		//ボーナス条件群
	float weight;												//重み
	float latest_score;											//最新のスコア

	//コンストラクタ
	UtilityAction()
	{
		action_name = "";
		required_conditions = {};
		bonus_conditions = {};
		weight = 1.0f;
		latest_score = -1.0f;
	}
};

//AI全体の設計情報
struct UtilitySettings
{
	float evaluate_interval;	//更新時間
	float inertia_bonus;		//維持ボーナス
	bool notify_on_change_only;	//変化フラグ

	//コンストラクタ
	UtilitySettings()
	{
		evaluate_interval = 5.0f;
		inertia_bonus = 0.1f;
		notify_on_change_only = false;
	}
};