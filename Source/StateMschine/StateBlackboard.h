#pragma once

#include <DirectXMath.h>
#include <unordered_map>
#include <string>

//キャラクターのアクションカテゴリー
enum class ActionCategory
{
	Idle,			//待機状態
	Locomotion,		//移動状態
	Airborne,		//空中状態
	Attack,			//攻撃状態
	Avoid,			//回避状態
	Skile,			//固有アビリティ状態
};

class StateBlackboard
{
public:
	//コンストラクタ
	StateBlackboard();

	//デストラクタ
	~StateBlackboard();

	//汎用アクセス関数
	bool IsActionPressed(const std::string& action_name)const;

	//入力状態を書き込む
	void SetActionState(const std::string& action_name, bool is_pressed);

public:
	DirectX::XMFLOAT3 move_input;	//移動入力
	uint32_t current_input_flags;	//合成された入力ビットフラグ

	ActionCategory action_category;	//アクションカテゴリー
	int sub_action_id;				//技ID等の詳細な識別番号
	float target_move_speed;		//目標移動速度

	bool is_grounded;					//接地フラグ
	DirectX::XMFLOAT3 current_velocity;	//現在の速度

private:
	std::unordered_map<std::string, bool> action_states;	//データマップ
};

