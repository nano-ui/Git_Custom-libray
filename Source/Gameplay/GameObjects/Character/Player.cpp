#include "Player.h"

#include "Engine\Core\Input.h"
#include "Engine\Graphics\Renderers\Graphics.h"
#include "Engine\Camera\CameraManager.h"
#include "Gameplay/GameObjects/ObjectFactory.h"
#include "Gameplay\StateMachine\StateBlackboard.h"
#include "Gameplay\Components\Editor\StateMachineComponent.h"
#include "Gameplay\Components\Transform\TransformComponent.h"
#include "Gameplay\Components\Model\ModelComponent.h"
#include "Gameplay\Components\Movement\MovementComponent.h"

#include <imgui.h>
#include <filesystem>

static AutoRegister<Player> auto_register_player("Player");

//コンストラクタ
Player::Player()
{
}

//デストラクタ
Player::~Player()
{
	
}

//初期化処理
void Player::Initialize()
{
	// 基本コンポーネントの追加・設定
	model_component = GetComponent<ModelComponent>();
	if (!model_component) model_component = AddComponent<ModelComponent>();

	transform_component = GetComponent<TransformComponent>();
	if (!transform_component) transform_component = AddComponent<TransformComponent>();

	if (model_component && transform_component)
	{
		model_component->SetTransformComponent(transform_component);

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

	// 基底クラス初期化（MovementComponent 等のセットアップ）
	Character::Initialize();

	if (state_machine_component && blackboard)
	{
		state_machine_component->Initialize(blackboard.get());
	}
}

//更新処理
void Player::Update(float elapsed_time)
{
	UpdateInput(elapsed_time);
	Character::Update(elapsed_time);
}

//デバッグ描画
void Player::RenderDebug(ShapeRenderer* renderer)
{
	Character::RenderDebug(renderer);
}

//シリアライズ登録
void Player::SetupSerialization()
{
	Character::SetupSerialization();
	if (state_machine_component && serializer)
	{
		state_machine_component->SetupSerialization(serializer.get());
	}
}

//アニメーション終了イベント
void Player::OnAnimationEnd(uint32_t state_key)
{

}

//入力更新処理
void Player::UpdateInput(float elapsed_time)
{
	float move_x = 0.0f;
	float move_z = 0.0f;

	if (Input::Instance().IsKeyPress('W')) move_z += 1.0f;
	if (Input::Instance().IsKeyPress('S')) move_z -= 1.0f;
	if (Input::Instance().IsKeyPress('A')) move_x -= 1.0f;
	if (Input::Instance().IsKeyPress('D')) move_x += 1.0f;

	DirectX::XMFLOAT3 move_vec = CameraManager::Instance().CalculateMoveVector(move_x, move_z);

	float target_speed = movement_component ? movement_component->GetMaxSpeed() : 5.0f;

	Character::Move(elapsed_time, move_vec.x, move_vec.z, target_speed);
	Character::Tuen(elapsed_time, move_vec.x, move_vec.z, target_speed);
}