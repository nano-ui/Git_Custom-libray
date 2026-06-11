#pragma once

#include <DirectXMath.h>

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

public:
	DirectX::XMFLOAT3 move_input;	//移動入力
	bool is_jump_pressed;			//ジャンプボタン
	bool is_attack_pressed;			//攻撃ボタン
	bool is_avoid_pressed;			//回避ボタン

	ActionCategory action_category;	//アクションカテゴリー
	int sub_action_id;				//技ID等の詳細な識別番号
	float target_move_speed;		//目標移動速度

	bool is_grounded;					//接地フラグ
	DirectX::XMFLOAT3 current_velocity;	//現在の速度
};

