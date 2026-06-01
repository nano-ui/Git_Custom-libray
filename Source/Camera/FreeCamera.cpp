#include "FreeCamera.h"
#include "../Input/Input.h"

#include <imgui.h>
#include <cmath>

//移動操作用仮想キーコード定数
static constexpr int key_move_forward = 'W';	//前進キーの仮想キーコード
static constexpr int key_move_backward = 'S';	//後退キーの仮想キーコード
static constexpr int key_move_left = 'A';		//左平行移動キーの仮想キーコード
static constexpr int key_move_right = 'D';		//右平行移動キーの仮想キーコード

//カメラ回転制限および初期値
static constexpr float max_pitch_angle = 89.0f;			//ジンバルロックを防ぐための限界画角設定（度数法）
static constexpr float default_move_speed = 10.0f;		//初期状態の1秒あたりの飛行移動速度
static constexpr float default_turn_sensitivity = 0.2f; //初期状態のマウスドラッグ回転感度
static constexpr float default_eye_x = 0.0f;			//ゲーム開始時の初期カメラ位置X
static constexpr float default_eye_y = 5.0f;			//地形を見下ろすためのカメラ高度Y
static constexpr float default_eye_z = -10.0f;			//後方に引くカメラ距離Z
static constexpr float default_focus_x = 0.0f;			//ゲーム開始時の初期注視点位置X
static constexpr float default_focus_y = 0.0f;			//ゲーム開始時の初期注視点位置Y
static constexpr float default_focus_z = 0.0f;			//ゲーム開始時の初期注視点位置Z

//コンストラクタ
FreeCamera::FreeCamera()
{
	rotation_angle = { 0.0f,0.0f };
	move_speed = default_move_speed;
	turn_sensitivity = default_turn_sensitivity;
}

//デストラクタ
FreeCamera::~FreeCamera()
{

}

//初期化処理
void FreeCamera::Initialize()
{
	//カメラトランスフォームの初期配置設定
	eye = { default_eye_x,default_eye_y,default_eye_z };
	focus = { default_focus_x,default_focus_y,default_focus_z };
	up = { 0.0f,1.0f,0.0f };
	SetLookAt(eye, focus, up);

	//生成された視線方向ベクトルから初期回転角度を逆算して同期
	rotation_angle.y = std::atan2f(front.x, front.z);
	rotation_angle.x = std::asinf(-front.y);
}

//更新処理
void FreeCamera::Update(float elapsed_time)
{
	//マウス操作：右ボタンドラッグによる視線回転角の計算
	if (Input::Instance().IsKeyPress(VK_RBUTTON))
	{
		float delta_x = Input::Instance().GetMouseDeltaX();
		float delta_y = Input::Instance().GetMouseDeltaY();
		rotation_angle.y += delta_x * turn_sensitivity * elapsed_time;
		rotation_angle.x += delta_y * turn_sensitivity * elapsed_time;
		constexpr float max_pitch_rad = DirectX::XMConvertToRadians(max_pitch_angle);
		if (rotation_angle.x > max_pitch_rad)rotation_angle.x = max_pitch_rad;
		if (rotation_angle.x < -max_pitch_rad)rotation_angle.x = -max_pitch_rad;
	}

	//新しい回転角度に基づいた視線方向ベクトルの再合成
	float cos_pitch = std::cosf(rotation_angle.x);
	float sin_pitch = std::sinf(rotation_angle.x);
	float cos_yaw = std::cosf(rotation_angle.y);
	float sin_yaw = std::sinf(rotation_angle.y);

	DirectX::XMVECTOR new_front = DirectX::XMVectorSet(
		sin_yaw * cos_pitch,
		-sin_pitch,
		cos_yaw * cos_pitch,
		0.0f
	);
	new_front = DirectX::XMVector3Normalize(new_front);

	DirectX::XMVECTOR world_up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	DirectX::XMVECTOR new_right = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(world_up, new_front));

	//キーボードおよびコントローラーに応じた並行移動ベクトルの集計
	DirectX::XMVECTOR move_vector = DirectX::XMVectorZero();

	//キーボード移動
	if (Input::Instance().IsKeyPress(key_move_forward))
	{
		move_vector = DirectX::XMVectorAdd(move_vector, new_front);
	}
	if (Input::Instance().IsKeyPress(key_move_backward))
	{
		move_vector = DirectX::XMVectorSubtract(move_vector, new_front);
	}
	if (Input::Instance().IsKeyPress(key_move_left))
	{
		move_vector = DirectX::XMVectorSubtract(move_vector, new_right);
	}
	if (Input::Instance().IsKeyPress(key_move_right))
	{
		move_vector = DirectX::XMVectorAdd(move_vector, new_right);
	}

	//コントローラー移動
	float stick_x = Input::Instance().GetLeftStickX();
	float stick_y = Input::Instance().GetLeftSticeY();
	move_vector = DirectX::XMVectorAdd(move_vector, DirectX::XMVectorMultiply(new_front, DirectX::XMVectorReplicate(stick_y)));
	move_vector = DirectX::XMVectorAdd(move_vector, DirectX::XMVectorMultiply(new_right, DirectX::XMVectorReplicate(stick_x)));
	
	//新しいカメラ座標・注視点の計算とビュー行列への反映
	if (!DirectX::XMVector3Equal(move_vector, DirectX::XMVectorZero()))
	{
		move_vector = DirectX::XMVector3Normalize(move_vector);
	}
	DirectX::XMVECTOR current_eye = DirectX::XMLoadFloat3(&eye);
	current_eye = DirectX::XMVectorMultiplyAdd(move_vector, DirectX::XMVectorReplicate(move_speed * elapsed_time), current_eye);
	DirectX::XMStoreFloat3(&eye, current_eye);
	DirectX::XMVECTOR current_focus = DirectX::XMVectorAdd(current_eye, new_front);
	DirectX::XMStoreFloat3(&focus, current_focus);
	DirectX::XMFLOAT3 absolute_up = { 0.0f,1.0f,0.0f };
	SetLookAt(eye, focus, absolute_up);
}

//ImGui描画処理
void FreeCamera::RenderGui()
{
#ifdef USE_IMGUI
	if (ImGui::CollapsingHeader("FreeCamera", ImGuiTreeNodeFlags_None))
	{
		ImGui::DragFloat("Speed", &move_speed, 0.1f);
		ImGui::DragFloat("Sensitivity", &turn_sensitivity, 0.01f);
		ImGui::InputFloat3("Position", &eye.x, "%.3f", ImGuiInputTextFlags_ReadOnly);
		ImGui::InputFloat3("FrontVec", &front.x, "%.3f", ImGuiInputTextFlags_ReadOnly);
	}

#endif // USE_IMGUI

}