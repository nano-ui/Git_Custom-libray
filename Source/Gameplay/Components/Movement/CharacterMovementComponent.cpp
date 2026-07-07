#include "CharacterMovementComponent.h"

#include "Serialization\JsonSerializer.h"

#include <cmath>
#include <windows.h>

//コンストラクタ
CharacterMovementComponent::CharacterMovementComponent()
{
	velocity = { 0.0f,0.0f,0.0f };
	move_vec = { 0.0f,0.0f,0.0f };
	max_speed = 0.0f;
	move_speed = 0.0f;
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
	UpdateHorizontalMove(elapsed_time, position);
	UpdateRotation(elapsed_time, angle);
}

//シリアライズの登録処理
void CharacterMovementComponent::SetupSerialization(JsonSerializer* serializer)
{
	if (serializer)
	{
		serializer->RegisterVariable(u8"最高速度", &max_speed, u8"移動・物理パラメータ");
		serializer->RegisterVariable(u8"移動速度", &move_speed, u8"移動・物理パラメータ");
		serializer->RegisterVariable(u8"加速度", &acceleration, u8"移動・物理パラメータ");
		serializer->RegisterVariable(u8"摩擦力", &friction, u8"移動・物理パラメータ");
		serializer->RegisterVariable(u8"重力", &gravity, u8"移動・物理パラメータ");
		serializer->RegisterVariable(u8"空気抵抗", &air_control, u8"移動・物理パラメータ");
	}
	else
	{
		OutputDebugStringA("[CharacterMovementComponent Error] SetupSerialization: serializer is null!\n");
	}
}

//ImGui描画
void CharacterMovementComponent::DrawImGui()
{
	if (ImGui::CollapsingHeader(u8"移動・物理パラメータ"))
	{
		ImGui::InputFloat3(u8"移動速度ベクトル", &velocity.x, "%.3f");
		ImGui::DragFloat(u8"最高速度", &max_speed, 0.0f, 50.0f);
		ImGui::DragFloat(u8"移動速度", &move_speed, 0.0f, 50.0f);
		ImGui::DragFloat(u8"加速度", &acceleration, 0.0f, 200.0f);
		ImGui::DragFloat(u8"摩擦力", &friction, 0.0f, 100.0f);
		ImGui::DragFloat(u8"重力", &gravity, -50.0f, 0.0f);
		ImGui::DragFloat(u8"空気抵抗", &air_control, 0.0f, 1.0f);
		ImGui::Checkbox(u8"接地フラグ", &is_ground);
	}
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

//水平方向の速度計算
void CharacterMovementComponent::UpdateHorizontalVelocity(float elapsed_time)
{
	float length = sqrtf(velocity.x * velocity.x + velocity.z * velocity.z);


	//摩擦の計算処理
	if (length > 0.0f)
	{
		float current_friction = friction * elapsed_time;
		if (!is_ground)current_friction *= air_control;

		if (length > current_friction)
		{
			float vx = velocity.x / length;
			float vz = velocity.z / length;
			velocity.x -= vx * current_friction;
			velocity.z -= vz * current_friction;
		}
		else
		{
			velocity.x = 0.0f;
			velocity.z = 0.0f;
		}
	}

	//加速の計算処理
	if (length <= max_speed)
	{
		float move_vec_length = sqrtf((move_vec.x * move_vec.x) + (move_vec.z * move_vec.z));
		if (move_vec_length > 0.0f)
		{
			float current_acceleration = acceleration * elapsed_time;
			if (!is_ground)current_acceleration *= air_control;
			velocity.x += move_vec.x * current_acceleration;
			velocity.z += move_vec.z * current_acceleration;
			float new_length = sqrtf((velocity.x * velocity.x) + (velocity.z * velocity.z));
			if (new_length > max_speed)
			{
				float vx = velocity.x / new_length;
				float vz = velocity.z / new_length;
				velocity.x = vx * max_speed;
				velocity.z = vz * max_speed;
			}
		}
	}
	horizontal_move_speed = sqrtf(velocity.x * velocity.x + velocity.z * velocity.z);

	move_vec.x = 0.0f;
	move_vec.z = 0.0f;
}

//水平方向の座標移動
void CharacterMovementComponent::UpdateHorizontalMove(float elapsed_time, DirectX::XMFLOAT3& position)
{
	position.x += velocity.x * elapsed_time;
	position.z += velocity.z * elapsed_time;
}

//旋回の更新計算
void CharacterMovementComponent::UpdateRotation(float elapsed_time, DirectX::XMFLOAT3& angle)
{
	float length = sqrtf(move_vec.x * move_vec.x + move_vec.z * move_vec.z); // 入力の長さを算出

	// 有効な移動入力があるか確認
	if (length < 0.001f)
	{
		return;
	}

	float vx = move_vec.x / length; // 方向正規化
	float vz = move_vec.z / length; // 方向正規化
	float front_x = sinf(angle.y); // 現在の正面向きX
	float front_z = cosf(angle.y); // 現在の正面向きZ

	float dot = (front_x * vx) + (front_z * vz); // 内積計算
	float rot = 1.0f - dot; // 回転量

	float rot_speed = max_speed * elapsed_time; // 回転速度

	// 指定回転速度を超えるか確認
	if (rot > rot_speed)
	{
		rot = rot_speed; // 回転速度制限
	}

	float target_angle = atan2f(vx, vz); // 目標角度
	angle.y = target_angle; // 角度の適用
}