#include "TacticalCommandController.h"
#include "SquadManager.h"
#include "Editor\GuiInspector.h"
#include "Serialization\JsonSerializer.h"
#include "Engine\Collision\CollisionManager.h"
#include "Engine\Camera\CameraManager.h"
#include "Engine\Camera\Camera.h"
#include "Engine\Graphics\Renderers\ShapeRenderer.h"
#include "Engine\Graphics\Renderers\Graphics.h"
#include "Engine\Core\Log\Log.h"
#include "Engine\Core\Input.h"
#include "Gameplay\Scene\SceneManager.h"
#include "Utility\RayUtility.h"

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
	target_position = { 0.0f,0.0f,0.0f };
	max_ray_distance = 10.0f;
	has_target = false;
	collision_manager = nullptr;
}

//更新処理
void TacticalCommandController::Update(float elapsed_time)
{
	if (!SceneManager::Instance().IsPaused() || !Input::Instance().IsMouseTrigger(Input::mouse_left))return;
	if (!collision_manager)return;

	auto camera = CameraManager::Instance().GetActiveCamera();
	if (!camera)return;

	DirectX::XMFLOAT3 ray_start = { 0.0f,0.0f,0.0f };
	DirectX::XMFLOAT3 ray_end = { 0.0f,0.0f,0.0f };

	bool is_ray = ScreenToWorldRay(
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
		SquadManager::Instance().CommandMode(target_position);
		LOG_SUCCESS({ u8"接触" }, u8"レイがヒットしました");
		has_target = true;
	}
	else
	{
		LOG_ERROR({ u8"接触" }, u8"レイがヒットしませんでした");
		has_target = false;
	}
}

//デバッグ描画
void TacticalCommandController::RenderDebug(ShapeRenderer* renderer)
{
	if (!has_target || !renderer)return;
	DirectX::XMFLOAT4 color = { 0.8f,0.1f,0.4f,1.0f };
	renderer->DrawSphere(target_position, 1.0f, color, ShapeDrawMode::Wireframe);
}

//JSON保存用変数登録
void TacticalCommandController::SetupSerialization(JsonSerializer* serializer)
{
	Component::SetupSerialization(serializer);
	serializer->RegisterVariable(u8"レイの長さ", &max_ray_distance, SerializeTarget::Detail);
}

//GuiInspector専用のインスペクター項目登録
void TacticalCommandController::SetupInspector(GuiInspector* inspector)
{
	Component::SetupInspector(inspector);
	inspector->RegisterVariable(u8"レイの長さ", &max_ray_distance, GetComponentName());
	inspector->RegisterText(u8"対象の位置", &target_position, GetComponentName());
	inspector->RegisterText(u8"対象の存在フラグ", &has_target, GetComponentName());
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