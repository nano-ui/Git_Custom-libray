#pragma once

#include <DirectXMath.h>

class JsonSerializer;

class CharacterMovementComponent
{
public:
	//コンストラクタ
	CharacterMovementComponent();

	//デストラクタ
	~CharacterMovementComponent();

	//移動・旋回・物理更新処理
	void Update(float elapsed_time, DirectX::XMFLOAT3& position, DirectX::XMFLOAT3& angle);

	//ジャンプ処理
	void Jump(float speed) { velocity.y += speed; }

	//シリアライズの登録処理
	void SetupSerialization(JsonSerializer* serializer);

	//ImGui描画
	void DrawImGui();

	//移動方向の設定
	void SetMoveVec(DirectX::XMFLOAT3 move_velocity) { move_vec = move_velocity; }

	//接地フラグの取得
	bool IsGround()const { return is_ground; }

	//接地フラグの設定
	void SetGround(bool ground) { is_ground = ground; }

	//移動速度ベクトル取得
	DirectX::XMFLOAT3 GetVelocity()const { return velocity; }

	//移動速度ベクトル設定
	void SetVelocity(const DirectX::XMFLOAT3& val) { velocity = val; }

	//最高速度取得
	float GetMaxSpeed()const { return max_speed; }

	//最高速度設定
	void SetMaxSpeed(float speed) { max_speed = speed; }

	//移動速度取得
	float GetMoveSpeed()const { return move_speed; }

	//移動速度設定
	void SetMoveSpeed(float speed) { move_speed = speed; }

	//加速度を取得
	float GetAcceleration()const { return acceleration; }

	//加速度を設定
	void SetAcceleration(float accel) { acceleration = accel; }

	//摩擦力を取得
	float GetFriction()const { return friction; }

	//摩擦力を設定
	void SetFriction(float frict) { friction = frict; }

private:
	//垂直方向の速度計算
	void UpdateVerticalVelocity(float elapsed_time);

	//垂直方向の座標移動
	void UpdateVerticalMove(float elapsed_time, DirectX::XMFLOAT3& position);

	//水平方向の速度計算
	void UpdateHorizontalVelocity(float elapsed_time);

	//水平方向の座標移動
	void UpdateHorizontalMove(float elapsed_time, DirectX::XMFLOAT3& position);

	//旋回の更新計算
	void UpdateRotation(float elapsed_time, DirectX::XMFLOAT3& angle);

private:
	DirectX::XMFLOAT3 velocity;		//移動速度ベクトル
	DirectX::XMFLOAT3 move_vec;		//移動指示ベクトル
	float max_speed;				//最大移動速度
	float move_speed;				//移動速度
	float acceleration;				//加速度
	float friction;					//摩擦力
	float air_control;				//空中での制御比率
	float horizontal_move_speed;	//水平移動速度
	float gravity;					//重力
	bool is_ground;					//接地フラグ
};

