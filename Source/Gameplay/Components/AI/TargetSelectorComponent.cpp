#include "TargetSelectorComponent.h"
#include "Gameplay\GameObjects\GameObject.h"
#include "Gameplay\GameObjects\Character\Character.h"
#include "Gameplay\GameObjects\ObjectManager.h"
#include "Gameplay\Components\Transform\TransformComponent.h"
#include "Gameplay\Components\AI\HateComponent.h"
#include "Serialization\JsonSerializer.h"
#include "Editor\Inspector\GuiInspector.h"
#include "Engine\Core\Log\Log.h"

//コンストラクタ
TargetSelectorComponent::TargetSelectorComponent()
{
	SetComponentName(u8"対象検索コンポーネント");
	Initialize();
}

//デストラクタ
TargetSelectorComponent::~TargetSelectorComponent() = default;

//初期化処理
void TargetSelectorComponent::Initialize()
{
	target_transform.reset();
	target_character = nullptr;
	target_id = UINT32_MAX;
}

//更新処理
void TargetSelectorComponent::Update(float elapsed_time)
{
	//ガード処理
	std::shared_ptr<TransformComponent> transform = target_transform.lock();
	std::shared_ptr<HateComponent> hate = hate_component.lock();
	if (!hate)return;

	//ヘイト対象が存在しない場合のリセット処理
	if (!hate->HasTarget())
	{
		target_transform.reset();
		target_character = nullptr;
		target_id = UINT32_MAX;
		return;
	}

	//ヘイト対象の検索と座標・キャラクターの更新
	transform = SearchHateTarget(hate->GetTopHateTargetId());
	if (!transform)
	{
		target_transform.reset();
		target_character = nullptr;
		target_id = UINT32_MAX;
		return;
	}
	target_transform = transform;
	target_id = hate->GetTopHateTargetId();
}

//JSON保存用変数登録
void TargetSelectorComponent::SetupSerialization(JsonSerializer* serializer)
{

}

//GuiInspector専用のインスペクター項目登録
void TargetSelectorComponent::SetupInspector(GuiInspector* inspector)
{
	Component::SetupInspector(inspector);
	inspector->RegisterText(u8"ヘイト対象のID", &target_id, GetComponentName());
}

//ヘイトコンポーネントを設定
void TargetSelectorComponent::SetHateComponent(std::shared_ptr<HateComponent> hate)
{
	hate_component = hate;
}

//ヘイトコンポーネントの取得
std::shared_ptr<HateComponent> TargetSelectorComponent::GetHateComponent() const
{
	if (!hate_component.lock())
	{
		return nullptr;
	}
	return hate_component.lock();
}

//行列コンポーネントを設定
void TargetSelectorComponent::SetTargetTransform(std::shared_ptr<TransformComponent> transform)
{
	target_transform = transform;
}

//行列コンポーネントの取得
std::shared_ptr<TransformComponent> TargetSelectorComponent::GetTransformComponent() const
{
	if (!target_transform.lock())
	{
		return nullptr;
	}
	return target_transform.lock();
}

//対象のキャラを取得
Character* TargetSelectorComponent::GetTargetCharacter() const
{
	//保持している対象キャラクターのポインタを返却
	return target_character;
}

//ヘイト対象を検索して座標コンポーネントを取得
std::shared_ptr<TransformComponent> TargetSelectorComponent::SearchHateTarget(uint32_t search_id)
{
	//オブジェクト一覧の走査
	for (const auto& obj : ObjectManager::Instance().GetGameObjects())
	{
		//有効性とIDの一致判定
		if (!obj || !obj->IsActive())continue;
		if (obj->GetObjectId() != search_id)continue;

		//Character型への安全なダウンキャストと保持
		Character* character = dynamic_cast<Character*>(obj.get());
		if (!character)
		{
			LOG_WARNING({ "TargetSelector" }, u8"ヘイト対象をCharacter型へ変換できませんでした");
			target_character = nullptr;
		}
		else
		{
			target_character = character;
		}

		//座標コンポーネントの取得
		std::shared_ptr<TransformComponent> transform = obj->GetComponent<TransformComponent>();
		if (!transform)return nullptr;
		return transform;
	}
	return nullptr;
}
