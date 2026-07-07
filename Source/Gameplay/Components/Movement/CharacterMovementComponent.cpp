#include "CharacterMovementComponent.h"

#include <cmath>
#include <windows.h>

//コンストラクタ
CharacterMovementComponent::CharacterMovementComponent()
{
	velocity = { 0.0f,0.0f,0.0f };
	move_vec = { 0.0f,0.0f,0.0f };
	max_speed = 0.0f;
	acceleration = 50.0f;
	friction = 15.0f;
	gravity = -10.0f;
	air_control = 0.3f;
	is_ground = false;
	horizontal_move_speed = 0.0f;
}

//デストラクタ
CharacterMovementComponent::~CharacterMovementComponent()
{

}

//移動・旋回・物理更新処理
void CharacterMovementComponent::Update(float elapsed_time, DirectX::XMFLOAT3& position, DirectX::XMFLOAT3& angle)
{
	if (elapsed_time <= 0.0f)
	{
		OutputDebugStringA("[CharacterMovementComponent Warning] Update: elapsed_time is zero or negative!\n");
		return;
	}
	UpdateVerticalVelocity(elapsed_time);
	UpdateHorizontalVelocity(elapsed_time);
	UpdateVerticalMove(elapsed_time, position);
	UpdateHorizontalMove(elapsed_time, angle);
}

//垂直方向の速度計算
void CharacterMovementComponent::UpdateVerticalVelocity(float elapsed_time)
{
	velocity.y += gravity * elapsed_time;
}

//垂直方向の座標移動
void CharacterMovementComponent::UpdateVerticalMove(float elapsed_time, DirectX::XMFLOAT3& position)
{
	position.y += velocity.y * elapsed_time;

}
