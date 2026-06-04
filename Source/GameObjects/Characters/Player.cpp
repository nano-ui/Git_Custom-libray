#include "Player.h"

#include "../Input/Input.h"
#include "../Graphics/Graphics.h"
#include "../GameObjects/ObjectFactory.h"

#include <imgui.h>

static AutoRegister<Player> auto_register_player("Player");

//コンストラクタ
Player::Player()
{
	auto device = Graphics::Instance().GetDevice();
	character = std::make_unique<Model>(device, "Data/Model/Character/unitychan.glb");
	move_speed = 5.0f;
	height = 0.8f;
	radius = 0.4f;
	offset_y = 0.4f;
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

	//当たり判定の初期設定
	capsule_collider.radius = radius;
	capsule_collider.attribute = ColliderAttribute::Collision;
	capsule_collider.listener = this;
	capsule_collider.is_active = true;
	AddCollider(&capsule_collider);
}

//更新処理
void Player::Update(float elapsed_time)
{
	capsule_collider.old_start_center = position;
	capsule_collider.old_start_center.y = position.y + offset_y;
	capsule_collider.old_end_center = position;
	capsule_collider.old_end_center.y += height + offset_y;

	UpdateInput(elapsed_time);
	Character::Update(elapsed_time);

	capsule_collider.start_center = position;
	capsule_collider.old_start_center.y = position.y + offset_y;
	capsule_collider.end_center = position;
	capsule_collider.end_center.y += height + offset_y;
}

//デバッグ描画
void Player::RenderDebug(ShapeRenderer* renderer)
{
	if (!capsule_collider.is_active || !renderer) return;

	//ShapeRendererの仕様に合わせたパラメータの変換
	DirectX::XMFLOAT3 cap_center = {
		position.x,
		position.y + offset_y + (height * 0.5f),
		position.z
	};
	float total_height = height + (capsule_collider.radius * 2.0f);

	//既存関数の呼び出し
	DirectX::XMFLOAT4 color = { 0.0f, 1.0f, 0.0f, 1.0f };
	renderer->DrawCapsule(
		cap_center,
		rotation,
		capsule_collider.radius,
		total_height,
		color,
		ShapeDrawMode::Wireframe
	);
}

//ImGuiデバッグ描画
void Player::RenderGui()
{
	ImGui::Begin("Player");
	Character::RenderGui();
	ImGui::DragFloat("PlayerSpeed",&move_speed, 0.1f);

	//当たり判定パラメータ設定
	if (ImGui::CollapsingHeader("Capsule Collider", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Checkbox("Is Active", &capsule_collider.is_active);
		constexpr float min_val = 0.1f;
		constexpr float max_radius = 20.0f;
		constexpr float max_height = 100.0f;
		if (ImGui::DragFloat("Radius", &capsule_collider.radius, min_val, max_radius))
		{
			radius = capsule_collider.radius;
		}
		ImGui::DragFloat("Height", &height, min_val, max_height);
	}

	ImGui::End();
}

//衝突処理
void Player::OnCollisionHit(const CollisionResult& result)
{
	if (result.hit_attribute == ColliderAttribute::Stage)
	{
		ResolveStageCollision(result, capsule_collider, height, offset_y);
	}
	if (result.hit_attribute == ColliderAttribute::Collision)
	{
		ResolveDynamicCollision(result, capsule_collider, height, offset_y);
	}
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
