#include "Player.h"

#include "Engine\Core\Input.h"
#include "Engine\Graphics\Renderers\Graphics.h"
#include "Engine\Camera\CameraManager.h"
#include "Gameplay/GameObjects/ObjectFactory.h"
#include "Gameplay\StateMachine\StateBlackboard.h"
#include "Gameplay\Components\Editor\StateMachineComponent.h"
#include "Gameplay\Components\Transform\TransformComponent.h"
#include "Gameplay\Components\Model\ModelComponent.h"

#include <imgui.h>
#include <filesystem>

static AutoRegister<Player> auto_register_player("Player");

//コンストラクタ
Player::Player()
{
	move_speed = 5.0f;
	height = 0.8f;
	radius = 0.4f;
	offset_y = 0.5f;
}

//デストラクタ
Player::~Player()
{
	
}

//初期化処理
void Player::Initialize()
{
	model_component = GetComponent<ModelComponent>();
	if (!model_component) model_component = AddComponent<ModelComponent>();

	transform_component = GetComponent<TransformComponent>();
	if (!transform_component) transform_component = AddComponent<TransformComponent>();

	if (model_component && transform_component)
	{
		model_component->SetTransformComponent(transform_component);

		// モデルパスが一切設定されていない場合のみデフォルトモデルを読み込む
		if (model_component->GetModelPath().empty())
		{
			const std::string default_model_path = "Data/Model/Character/Player/Greystone_WhiteTiger.gltf";
			model_component->LoadModel(default_model_path);
		}

		std::filesystem::path path_obj(model_component->GetModelPath());
		std::string model_name = path_obj.stem().string();

		if (state_machine_component)
		{
			state_machine_component->SetModelName(model_name);
		}
	}

	Character::Initialize();

	if (state_machine_component && blackboard)
	{
		state_machine_component->Initialize(blackboard.get());
	}

	// 当たり判定の初期設定
	capsule_collider.radius = radius;
	capsule_collider.attribute = ColliderAttribute::Collision;
	capsule_collider.listener = this;
	capsule_collider.is_active = true;
	AddCollider(&capsule_collider);
}

//更新処理
void Player::Update(float elapsed_time)
{
	DirectX::XMFLOAT3 pos = transform_component ? transform_component->GetPosition() : DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);

	capsule_collider.old_start_center = pos;
	capsule_collider.old_start_center.y = pos.y + offset_y;
	capsule_collider.old_end_center = pos;
	capsule_collider.old_end_center.y += height + offset_y;

	UpdateInput(elapsed_time);
	Character::Update(elapsed_time);
	
	pos = transform_component ? transform_component->GetPosition() : DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);

	capsule_collider.start_center = pos;
	capsule_collider.start_center.y = pos.y + offset_y;
	capsule_collider.end_center = pos;
	capsule_collider.end_center.y += height + offset_y;
	CheckColliderSyncDebug();
}

//デバッグ描画
void Player::RenderDebug(ShapeRenderer* renderer)
{
	if (!capsule_collider.is_active || !renderer || !transform_component) return;

	DirectX::XMFLOAT3 pos = transform_component->GetPosition();
	DirectX::XMFLOAT4 rot = transform_component->GetQuaternion();

	DirectX::XMFLOAT3 cap_center = {
		pos.x,
		pos.y + offset_y + (height * 0.5f),
		pos.z
	};
	float total_height = height + (capsule_collider.radius * 2.0f);

	capsule_collider.radius = radius;

	DirectX::XMFLOAT4 color = { 0.0f, 1.0f, 0.0f, 1.0f };
	renderer->DrawCapsule(
		cap_center,
		rot,
		capsule_collider.radius,
		total_height,
		color,
		ShapeDrawMode::Wireframe
	);
}

//をシリアライザに登録
void Player::SetupSerialization()
{
	Character::SetupSerialization();
	if (state_machine_component && serializer)
	{
		state_machine_component->SetupSerialization(serializer.get());
	}
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

//アニメーション終了イベント
void Player::OnAnimationEnd(uint32_t state_key)
{

}

//入力更新処理
void Player::UpdateInput(float elapsed_time)
{
	//横方向ベクトルの算出
	float move_x = 0.0f;
	float move_z = 0.0f;

	//キーボード入力の検知
	if (Input::Instance().IsKeyPress('W'))move_z += 1.0f;
	if (Input::Instance().IsKeyPress('S'))move_z -= 1.0f;
	if (Input::Instance().IsKeyPress('A'))move_x -= 1.0f;
	if (Input::Instance().IsKeyPress('D'))move_x += 1.0f;

	DirectX::XMFLOAT3 move_vec = CameraManager::Instance().CalculateMoveVector(move_x, move_z);

	//移動・旋回処理
	Character::Move(elapsed_time, move_vec.x, move_vec.z, max_speed);
	Character::Tuen(elapsed_time, move_vec.x, move_vec.z, max_speed);
}

//プレイヤーの現在位置と、コライダーの現在位置のズレを出力
void Player::CheckColliderSyncDebug() const
{
	if (!transform_component) return;

	DirectX::XMFLOAT3 pos = transform_component->GetPosition();
	const float diffX = pos.x - capsule_collider.start_center.x;
	const float diffY = pos.y - capsule_collider.start_center.y;
	const float diffZ = pos.z - capsule_collider.start_center.z;

	const float tolerance = 0.001f;

	if (std::abs(diffX) > tolerance || std::abs(diffY) > tolerance || std::abs(diffZ) > tolerance)
	{
		constexpr int bufferSize = 256;
		char debugStr[bufferSize];

		std::snprintf(debugStr, bufferSize,
			"Sync Warning! Player(%.3f, %.3f, %.3f) Collider(%.3f, %.3f, %.3f)\n",
			pos.x, pos.y, pos.z,
			capsule_collider.start_center.x,
			capsule_collider.start_center.y,
			capsule_collider.start_center.z);
	}
}
