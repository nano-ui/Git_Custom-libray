#include "TacticalCommandController.h"
#include "Editor\GuiInspector.h"
#include "Serialization\JsonSerializer.h"
#include "Engine\Collision\CollisionManager.h"
#include "Engine\Camera\CameraManager.h"
#include "Engine\Graphics\Renderers\ShapeRenderer.h"
#include "Engine\Core\Log\Log.h"

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
