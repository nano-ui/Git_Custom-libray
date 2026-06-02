#include "Player.h"

#include "../Input/Input.h"
#include "../Graphics/Graphics.h"

#include <imgui.h>

//コンストラクタ
Player::Player()
{
	auto device = Graphics::Instance().GetDevice();
	character = std::make_unique<Model>(device, "Data/Model/Character/unitychan.glb");
	move_speed = 5.0f;
}

//デストラクタ
Player::~Player()
{

}

//初期化処理
void Player::Initialize()
{
	Character::Initialize();
	position = { 0.0f,0.0f,0.0f };
}

//更新処理
void Player::Update(float elapsed_time)
{
	UpdateInput(elapsed_time);
	Character::Update(elapsed_time);
}

//デバッグ描画
void Player::RenderDebug()
{
}

//ImGuiデバッグ描画
void Player::RenderGui()
{
	Character::RenderGui();
	ImGui::Begin("Player");
	ImGui::DragFloat("PlayerSpeed",&move_speed, 0.1f);
	ImGui::End();
}

//入力更新処理
void Player::UpdateInput(float elapsed_time)
{
	//横方向ベクトルの算出
	float move_x = 0.0f;
	float move_z = 0.0f;

	//キーボード入力の検知
	if (Input::Instance().IsKeyPress('W'))
	{
		move_z += 1.0f;
	}
	if (Input::Instance().IsKeyPress('S'))
	{
		move_z -= 1.0f;
	}
	if (Input::Instance().IsKeyPress('A'))
	{
		move_x -= 1.0f;
	}
	if (Input::Instance().IsKeyPress('D'))
	{
		move_x += 1.0f;
	}

	//移動・旋回処理
	Character::Move(elapsed_time, move_x, move_z, move_speed);
	Character::Tuen(elapsed_time, move_x, move_z, move_speed);
}
