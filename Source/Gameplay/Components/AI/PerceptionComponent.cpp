#include "PerceptionComponent.h"
#include "Gameplay\GameObjects\GameObject.h"
#include "Gameplay\GameObjects\GameObjectTag.h"
#include "Gameplay\GameObjects\ObjectManager.h"
#include "Gameplay\Components\Sensor\VisionSensorComponent.h"
#include "Gameplay\Components\AI\HateComponent.h"
#include "Gameplay\Components\Transform\TransformComponent.h"
#include "Editor\GuiInspector.h"
#include "Serialization\JsonSerializer.h"
#include "Engine\Core\Log\Log.h"

//コンストラクタ
PerceptionComponent::PerceptionComponent()
{
	Initialize();
	SetComponentName(u8"知覚コンポーネント");
}

//デストラクタ
PerceptionComponent::~PerceptionComponent() = default;

//初期化処理
void PerceptionComponent::Initialize()
{
	hate_per_second = 0.0f;
	target_tag = GameObjectTag::None;
}

//更新処理
void PerceptionComponent::Update(float elapsed_time)
{
	auto sensor = vision_sensor.lock();
	if (!sensor)
	{
		LOG_WARNING({ u8"設定" }, u8"視界の持ち主の設定ができませんでした");
		return;
	}
	auto my_hate = hate.lock();
	if (!my_hate)
	{
		LOG_WARNING({ u8"設定" }, u8"ヘイトの持ち主の設定ができませんでした");
		return;
	}

	for (const auto& obj : ObjectManager::Instance().GetGameObjects())
	{
		if (!obj || !obj->IsActive())
		{
			LOG_WARNING({ u8"存在",u8"設定" }, u8"オブジェクトが存在しません");
			continue;
		}
		if (obj->HasTag(target_tag))
		{
			auto transform = obj->GetComponent<TransformComponent>();
			if (!transform)
			{
				LOG_WARNING({ u8"設定" }, u8"対象オブジェクトに TransformComponent が存在しません");
				continue;
			}
			DirectX::XMFLOAT3 pos = transform->GetPosition();
			uint32_t target_id = obj->GetObjectId();

			if (sensor->IsTargetInSight(pos))
			{
				float hate_value = hate_per_second * elapsed_time;
				my_hate->AddHate(target_id, hate_value);
			}
		}
	}
}

//JSON保存用変数登録
void PerceptionComponent::SetupSerialization(JsonSerializer* serializer)
{
	Component::SetupSerialization(serializer);
	serializer->RegisterVariable(u8"ヘイト増加量", &hate_per_second, SerializeTarget::Detail);
}

//GuiInspector専用のインスペクター項目登録
void PerceptionComponent::SetupInspector(GuiInspector* inspector)
{
	Component::SetupInspector(inspector);
	inspector->RegisterVariable(u8"ヘイト増加量", &hate_per_second, GetComponentName());
}