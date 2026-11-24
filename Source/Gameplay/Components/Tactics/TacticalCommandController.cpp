#include "TacticalCommandController.h"
#include "SquadManager.h"
#include "Editor\Inspector\GuiInspector.h"
#include "Serialization\JsonSerializer.h"
#include "Engine\Collision\CollisionManager.h"
#include "Engine\Camera\CameraManager.h"
#include "Engine\Camera\Camera.h"
#include "Engine\Graphics\Renderers\ShapeRenderer.h"
#include "Engine\Graphics\Renderers\Graphics.h"
#include "Engine\Core\Log\Log.h"
#include "Engine\Core\Input.h"
#include "Gameplay\Scene\SceneManager.h"
#include "Gameplay\Common\Blackboard.h"
#include "Gameplay\Components\Transform\TransformComponent.h"
#include "Gameplay\Components\AI\TargetSelectorComponent.h"
#include "Gameplay\Components\Tactics\TacticalTypes.h"
#include "Gameplay\Components\Tactics\SquadManager.h"
#include "Gameplay\Components\Tactics\Formation\FormationBase.h"
#include "Gameplay\Components\Tactics\Formation\CircleFormation.h"
#include "Gameplay\Components\Tactics\Formation\FishScaleFormation.h"
#include "Gameplay\Components\Tactics\Formation\VShapedFormation.h"
#include "Gameplay\Components\Tactics\Formation\CraneWingFormation.h"
#include "Gameplay\Components\Tactics\Formation\RectangleFormation.h"
#include "Utility\RayUtility.h"
#include <Engine\Core\MacroTracy.h>

//コンストラクタ
TacticalCommandController::TacticalCommandController()
{
	Initialize();
	SetComponentName(u8"戦略コンポーネント");
}

//デストラクタ
TacticalCommandController::~TacticalCommandController() = default;

//初期化処理
void TacticalCommandController::Initialize()
{
	safe_position = { 0.0f,0.0f,0.0f };
	target_position = { 0.0f,0.0f,0.0f };
	max_ray_distance = 10.0f;
	has_target = false;
	collision_manager = nullptr;
	formation_map[TacticalType::Circle] = std::make_shared<CircleFormation>();
	formation_map[TacticalType::FishScale] = std::make_shared<FishScaleFormation>();
	formation_map[TacticalType::CraneWing] = std::make_shared<CraneWingFormation>();
	formation_map[TacticalType::VShaped] = std::make_shared<VShapedFormation>();
	formation_map[TacticalType::Rectangle] = std::make_shared<RectangleFormation>();
	current_formation = formation_map[TacticalType::Rectangle];
}

//更新処理
void TacticalCommandController::Update(float elapsed_time)
{
	BS_PROFILE_FUNCTION();

	//プレイヤーのマウス入力による目標座標の更新
	if (SceneManager::Instance().IsPaused() && Input::Instance().IsMouseTrigger(Input::mouse_left))
	{
		if (collision_manager)
		{

			auto camera = CameraManager::Instance().GetActiveCamera();
			if (!camera)return;

			DirectX::XMFLOAT3 ray_start = { 0.0f,0.0f,0.0f };
			DirectX::XMFLOAT3 ray_end = { 0.0f,0.0f,0.0f };

			bool is_ray = RayUtility::ScreenToWorldRay(
				Graphics::Instance().GetCurrentHeight(),
				Graphics::Instance().GetCurrentWidth(),
				{ Input::Instance().GetMousePosX(),Input::Instance().GetMousePosY() },
				camera->GetView(), camera->GetProjection(),
				ray_start, ray_end
			);

			if (!is_ray)return;
			bool is_hit = collision_manager->RayCastSpace(ray_start, ray_end, target_position);

			if (is_hit)
			{
				current_formation->SetTargetPos(target_position);
				has_target = true;
			}
		}
	}

	//部隊ブラックボードから現在の戦術を取得
	std::shared_ptr<Blackboard> blackboard = SquadManager::Instance().GetSquadBlackboard();
	if (!blackboard)return;
	int tactical = blackboard->GetValue(current_tactical_type, static_cast<int>(TacticalType::None));
	TacticalType tactical_type = static_cast<TacticalType>(tactical);

	//戦術に応じた行動モードの伝達と座標の配布
	switch (tactical_type)
	{
	case TacticalType::None:
		SquadManager::Instance().SetTacticalAction(TacticalAction::None);
		//SquadManager::Instance().CommandMode(target_position);
		break;
	case TacticalType::Retreat:
	{
		SquadManager::Instance().SetTacticalAction(TacticalAction::TargetPoint);
		std::shared_ptr<TargetSelectorComponent> selector = target_selector.lock();
		if (!selector)
		{
			LOG_WARNING({ u8"取得" }, u8"TargetSelectorComponentが取得できませんでした");
			return;
		}
		std::shared_ptr<TransformComponent> transform = selector->GetTransformComponent();
		if (!transform)
		{
			LOG_WARNING({ u8"取得" }, u8"TransformComponentが取得できませんでした");
			return;
		};
		if (has_target)
		{
			target_position = transform->GetPosition();
			SquadManager::Instance().CommandMode(target_position);
		}
		else SquadManager::Instance().CommandMode(safe_position);
		break;
	}
	case TacticalType::Autonomous:
		SquadManager::Instance().SetTacticalAction(TacticalAction::Automous);
		break;
	case TacticalType::Circle:
	case TacticalType::FishScale:
	case TacticalType::CraneWing:
	case TacticalType::VShaped:
	case TacticalType::Rectangle:
	{
		SquadManager::Instance().SetTacticalAction(TacticalAction::Formation);
		current_formation = formation_map[tactical_type];
		std::shared_ptr<TargetSelectorComponent> selector = target_selector.lock();
		if (!selector)
		{
			LOG_WARNING({ u8"取得" }, u8"TargetSelectorComponentが取得できませんでした");
			return;
		}
		std::shared_ptr<TransformComponent> transform = selector->GetTransformComponent();
		if (!transform)
		{
			LOG_WARNING({ u8"取得" }, u8"TransformComponentが取得できませんでした");
			return;
		};
		if (!has_target)
		{
			target_position = transform->GetPosition();
		}
		current_formation->SetTargetPos(target_position);
		std::vector<DirectX::XMFLOAT3> target_positions = current_formation->CalculatePositions(SquadManager::Instance().GetMenberCount());
		SquadManager::Instance().CommandFormation(target_positions);
		break;
	}
	default:
		SquadManager::Instance().CommandMode(target_position);
		break;
	}
}

//デバッグ描画
void TacticalCommandController::RenderDebug(ShapeRenderer* renderer)
{
	if (!has_target || !renderer)return;
	DirectX::XMFLOAT4 color = { 0.8f,0.1f,0.4f,1.0f };
	DirectX::XMFLOAT4 safe_color = { 0.2f,0.4f,0.8f,1.0f };
	renderer->DrawSphere(target_position, 1.0f, color, ShapeDrawMode::Wireframe);
	renderer->DrawSphere(safe_position, 1.0f, safe_color, ShapeDrawMode::SolidAndWireframe);
}

//JSON保存用変数登録
void TacticalCommandController::SetupSerialization(JsonSerializer* serializer)
{
	Component::SetupSerialization(serializer);
	serializer->RegisterVariable(u8"レイの長さ", &max_ray_distance, SerializeTarget::Detail);
	formation_map[TacticalType::Circle]->SetupSerialization(serializer);
	formation_map[TacticalType::FishScale]->SetupSerialization(serializer);
	formation_map[TacticalType::CraneWing]->SetupSerialization(serializer);
	formation_map[TacticalType::VShaped]->SetupSerialization(serializer);
	formation_map[TacticalType::Rectangle]->SetupSerialization(serializer);
}

//GuiInspector専用のインスペクター項目登録
void TacticalCommandController::SetupInspector(GuiInspector* inspector)
{
	Component::SetupInspector(inspector);
	inspector->RegisterVariable(u8"レイの長さ", &max_ray_distance, GetComponentName());
	inspector->RegisterText(u8"対象の位置", &target_position, GetComponentName());
	inspector->RegisterVariable(u8"安全な座標", &safe_position, GetComponentName());
	inspector->RegisterText(u8"対象の存在フラグ", &has_target, GetComponentName());
	formation_map[TacticalType::Circle]->SetupInspector(inspector);
	formation_map[TacticalType::FishScale]->SetupInspector(inspector);
	formation_map[TacticalType::CraneWing]->SetupInspector(inspector);
	formation_map[TacticalType::VShaped]->SetupInspector(inspector);
	formation_map[TacticalType::Rectangle]->SetupInspector(inspector);
}

//当たり判定マネージャーの設定
void TacticalCommandController::SetCollisionManager(CollisionManager* manager)
{
	if (!manager)
	{
		LOG_WARNING({ u8"設定" }, u8"渡されたポインタがnullでした");
		return;
	}
	collision_manager = manager;
}

//対象検索コンポーネントの設定
void TacticalCommandController::SetTargetSelector(std::shared_ptr<TargetSelectorComponent> selector)
{
	target_selector = selector;
}
