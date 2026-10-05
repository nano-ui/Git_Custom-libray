#pragma once
#include <string>

struct AnimationData
{
	std::string animation_name;		//アニメーション名
	bool is_loop = false;			//ループフラグ
	bool is_root_motion = false;	//ルートモーションフラグ
	float blend_duration = 0.2f;	//ブレンド時間
};