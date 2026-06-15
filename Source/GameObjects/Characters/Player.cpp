#include "Player.h"

#include "../Input/Input.h"
#include "../Graphics/Graphics.h"
#include "../GameObjects/ObjectFactory.h"
#include "../Editor/StateMachineComponent.h"
#include "../StateMschine/StateBlackboard.h"
#include "../StateMschine/StateMachine.h"
#include "../StateMschine/State.h"

#include <imgui.h>

static AutoRegister<Player> auto_register_player("Player");

//コンストラクタ
Player::Player()
{
	auto device = Graphics::Instance().GetDevice();
	character = std::make_unique<Model>(device, "Data/Model/Character/RPG-Character.glb");
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
	Character::Initialize();
	position = { 0.0f,0.0f,0.0f };

	//当たり判定の初期設定
	capsule_collider.radius = radius;
	capsule_collider.attribute = ColliderAttribute::Collision;
	capsule_collider.listener = this;
	capsule_collider.is_active = true;
	AddCollider(&capsule_collider);
	character->PlayAnimation("Idle", true);

	AddComponent<StateMachineComponent>();
	SetupSerialization();
}

//更新処理
void Player::Update(float elapsed_time)
{
	capsule_collider.old_start_center = position;
	capsule_collider.old_start_center.y = position.y + offset_y;
	capsule_collider.old_end_center = position;
	capsule_collider.old_end_center.y += height + offset_y;

	//登録されているコンポーネントを型検索で取得
	StateMachineComponent* sm_comp = GetComponent<StateMachineComponent>();

	//F6ホットリロード処理
	if (Input::Instance().IsKeyTrigger(VK_F6) && sm_comp)
	{
		sm_comp->LoadStateMachineConfig(sm_comp->GetStateMachineCnfigPath());
	}

	//物理パラメータをブラックボードへ通知
	StateMachine* state_machine = sm_comp ? sm_comp->GetStateMachine() : nullptr;
	StateBlackboard* blackboard = state_machine ? state_machine->GetBlackboard() : nullptr;
	if (blackboard)
	{
		blackboard->is_grounded = is_ground; //
		blackboard->current_velocity = velocity; //
	}

	UpdateInput(elapsed_time);

	//所有している全コンポーネントのUpdateを一括実行
	for (const auto& comp : components)
	{
		if (comp && comp->IsActive()) comp->Update(elapsed_time);
	}

	//要求移動速度を元に移動実行
	if (blackboard)
	{
		Character::Move(elapsed_time, blackboard->move_input.x, blackboard->move_input.z, blackboard->target_move_speed); //
		Character::Tuen(elapsed_time, blackboard->move_input.x, blackboard->move_input.z, blackboard->target_move_speed); //
	}

	Character::Update(elapsed_time);
	character->Update(elapsed_time);

	//アニメーション再生
	if (sm_comp)
	{
		std::string target_clip = sm_comp->GetCurrentStateAnimationClip();
		bool is_loop = sm_comp->IsCurrentStateAnimLoop();

		static std::string current_playing_clip = "";
		if (target_clip != current_playing_clip)
		{
			character->PlayAnimation(target_clip, is_loop);
			current_playing_clip = target_clip;
		}
	}

	capsule_collider.start_center = position;
	capsule_collider.start_center.y = position.y + offset_y;
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

	capsule_collider.radius = radius;

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

//変数をシリアライザに登録
void Player::SetupSerialization()
{
	Character::SetupSerialization();
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
