#include "ObjectClassConverter.h"
#include "Gameplay\GameObjects\GameObject.h"
#include "Gameplay\GameObjects\ObjectFactory.h"
#include "Gameplay\Components\Transform\TransformComponent.h"
#include "Gameplay\Components\Model\ModelComponent.h"
#include "Engine\Core\Log\Log.h"

#include <memory>

//クラス変換
GameObject* ObjectClassConverter::ConvertClass(GameObject* old_object, const std::string& target_class_name)
{
	//ガード処理
	if (!old_object)
	{
		LOG_ERROR({ u8"設定" }, u8"渡されたポインタがnullでした");
		return nullptr;
	}

	if (target_class_name.empty())
	{
		LOG_ERROR({ u8"設定" }, u8"変換先のクラス名が設定されていません");
		return nullptr;
	}

	//生成と構築
	ExtractedObjectData new_object_data = ExtractObjectData(old_object);
	GameObject* new_object = ObjectFactory::CreateAndRegister(target_class_name);
	if (!new_object)return nullptr;
	CreateNewObject(new_object, new_object_data);

	//初期化と破棄
	new_object->Initialize();
	old_object->Destory();

	return new_object;
}

//旧情報の抽出
ExtractedObjectData ObjectClassConverter::ExtractObjectData(GameObject* old_object) const
{
	ExtractedObjectData new_object_data = {};

	//姿勢情報の取得
	std::shared_ptr<TransformComponent> transform = old_object->GetComponent<TransformComponent>();
	if (!transform)
	{
		LOG_WARNING({ u8"取得" }, u8"姿勢情報が取得できませんでした");
		return new_object_data;
	}
	new_object_data.position = transform->GetPosition();
	new_object_data.rotation = transform->GetQuaternion();
	new_object_data.scale = transform->GetScale();

	//モデルパスの取得
	std::shared_ptr<ModelComponent> model = old_object->GetComponent<ModelComponent>();
	if (!model)
	{
		LOG_WARNING({ u8"取得" }, u8"モデル情報が取得できませんでした");
		return new_object_data;
	}
	new_object_data.model_path = model->GetModelPath();

	return new_object_data;
}

//新オブジェクトの構築
void ObjectClassConverter::CreateNewObject(GameObject* new_object, const ExtractedObjectData& data)const
{
	//姿勢情報の取得
	std::shared_ptr<TransformComponent> transform = new_object->GetComponent<TransformComponent>();
	if (!transform)transform = new_object->AddComponent<TransformComponent>();
	transform->SetPosition(data.position);
	transform->SetRotationQuaternion(data.rotation);
	transform->SetScale(data.scale);
	//モデルパスの取得
	std::shared_ptr<ModelComponent> model = new_object->GetComponent<ModelComponent>();
	if (!model)model = new_object->AddComponent<ModelComponent>();
	model->SetTransformComponent(transform);
	if (!model->LoadModel(data.model_path))return;
}
