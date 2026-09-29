#include "AttachmentColliderComponent.h"
#include "Gameplay\Components\Model\ModelComponent.h"
#include "Gameplay\Components\Transform\TransformComponent.h"
#include "Engine\Graphics\Resources\Model.h"
#include "Engine\Graphics\Renderers\ShapeRenderer.h"
#include "ThiedParty\json.hpp"

#include <Windows.h>
#include <fstream>
#include <filesystem>

//コンストラクタ
AttachmentColliderComponent::AttachmentColliderComponent()
{
	component_name = "アタッチメントコライダーコンポーネント";
}

//デストラクタ
AttachmentColliderComponent::~AttachmentColliderComponent() = default;

//初期化処理
void AttachmentColliderComponent::Initializa(
	const std::string& model_file_path,
	std::shared_ptr<ModelComponent> model_comp,
	std::shared_ptr<TransformComponent> transform_comp)
{
	Component::Initialize();

	model_component = model_comp;
	transform_component = transform_comp;

	if (model_file_path.empty())
	{
		OutputDebugStringA("[AttachmentColliderComponent 警告] model_file_path が空です。\n");
		return;
	}

	//パスから純粋なモデル名を抽出
	std::filesystem::path path_obj(model_file_path);
	model_name = path_obj.stem().string();

	//保存仕様に基づいたJSONパスの構築
	std::string attach_json_path = "Data/Json/" + model_name + "/" + model_name + "_Attach.json";

	//JSONから読み込み
	if (!LoadFromJson(attach_json_path))
	{
		OutputDebugStringA("[AttachmentColliderComponent 警告] コライダー定義JSONのロードに失敗またはファイルが存在しません。\n");
		return;
	}

	//定義データから判定用CapsuleColliderを生成
	runtime_colliders.clear();
	for (const auto& item : collider_items)
	{
		if (!item)continue;

		auto capsule = std::make_unique<CapsuleCollider>();
		capsule->radius = item->radius;
		capsule->attribute = item->attribute;
		capsule->is_active = item->is_active;

		runtime_colliders.push_back(std::move(capsule));
	}
	OutputDebugStringA("[AttachmentColliderComponent] コライダー初期化が完了しました。\n");
}

//更新処理
void AttachmentColliderComponent::Update(float elapsed_time)
{
	if (!is_active)return;

	auto model_comp = model_component.lock();
	auto trans_comp = transform_component.lock();
	if (!model_comp || !trans_comp)
	{
		OutputDebugStringA("[AttachmentColliderComponent 警告] Update: ModelComponent または TransformComponent が無効です。\n");
		return;
	}

	//全コライダーの位置を更新
	for (size_t i = 0; i < collider_items.size() && i < runtime_colliders.size(); i++)
	{
		const auto& item = collider_items[i];
		auto& capsule = runtime_colliders[i];

		if (!item || !capsule)continue;

		//有効フラグの同期
		capsule->is_active = item->is_active;
		if (!capsule->is_active)continue;

		//移動座標の退避
		capsule->old_start_center = capsule->start_center;
		capsule->old_end_center = capsule->end_center;

		//現在のボーン姿勢から始点・終点を計算
		CalculateCapsulePoints(*item, model_comp.get(), capsule->start_center, capsule->end_center);
	}
}

//デバッグ描画処理
void AttachmentColliderComponent::RenderDebug(ShapeRenderer* renderer)
{
	if (!renderer || !is_active)return;

	for (const auto& capsule : runtime_colliders)
	{
		if (!capsule || !capsule->is_active)continue;

		//始点と終点から中心座標と高さを算出
		DirectX::XMVECTOR v_start = DirectX::XMLoadFloat3(&capsule->start_center);
		DirectX::XMVECTOR v_end = DirectX::XMLoadFloat3(&capsule->end_center);
		DirectX::XMVECTOR v_diff = DirectX::XMVectorSubtract(v_end, v_start);

		DirectX::XMVECTOR v_center = DirectX::XMVectorScale(DirectX::XMVectorAdd(v_start, v_end), 0.5f);
		DirectX::XMFLOAT3 center = {};
		DirectX::XMStoreFloat3(&center, v_center);

		float autual_len = DirectX::XMVectorGetX(DirectX::XMVector3Length(v_diff));
		float total_height = autual_len + (capsule->radius * 2.0f);

		//Y軸基準から終点方向への回転クォータニオンを算出
		DirectX::XMVECTOR default_up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
		DirectX::XMVECTOR dir = (autual_len > 1e-4f) ? DirectX::XMVector3Normalize(v_diff) : default_up;

		DirectX::XMVECTOR rot_quat = DirectX::XMQuaternionIdentity();
		DirectX::XMVECTOR rot_axis = DirectX::XMVector3Cross(default_up, dir);
		float dot = DirectX::XMVectorGetX(DirectX::XMVector3Dot(default_up, dir));

		if (dot < -0.9999f)
		{
			rot_quat = DirectX::XMQuaternionRotationAxis(DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), DirectX::XM_PI);
		}
		else if(dot < 0.9999f)
		{
			float angle = acosf(dot);
			rot_quat = DirectX::XMQuaternionRotationAxis(DirectX::XMVector3Normalize(rot_axis), angle);
		}

		DirectX::XMFLOAT4 rotation = {};
		DirectX::XMStoreFloat4(&rotation, rot_quat);

		//属性に応じたカラーリング
		DirectX::XMFLOAT4 color = { 0.2f,0.7f,1.0f,1.0f };
		switch (capsule->attribute)
		{
		case ColliderAttribute::Attack:
			color = { 1.0f,0.2f,0.2f,1.0f };
			break;
		case ColliderAttribute::Stage:
			color = { 0.2f,1.0f,0.2f,1.0f };
			break;
		case ColliderAttribute::Collision:
			color = { 0.2f,0.6f,1.0f,1.0f };
			break;
		default:
			color = { 0.6f,0.6f,0.6f,1.0f };
			break;
		}
		renderer->DrawCapsule(center, rotation, capsule->radius, total_height, color);
	}
}

//コライダー名を指定して有効/無効切り替え
void AttachmentColliderComponent::SetColliderActive(const std::string& target_name, bool is_active)
{
	bool is_found = false;
	for (size_t i = 0; i < collider_items.size() && i < runtime_colliders.size(); i++)
	{
		if (collider_items[i] && collider_items[i]->name == target_name)
		{
			collider_items[i]->is_active = is_active;
			runtime_colliders[i]->is_active = is_active;
			is_found = true;
			break;
		}
	}

	if (!is_found)
	{
		OutputDebugStringA("[AttachmentColliderComponent 警告] SetColliderActive: 指定されたコライダー名が見つかりません。\n");
	}
}

//攻撃判定属性のコライダーのみ一括で有効/無効切り替え
void AttachmentColliderComponent::SetAttackCollidersActive(bool is_active)
{
	for (size_t i = 0; i < collider_items.size() && i < runtime_colliders.size(); i++)
	{
		if (!collider_items[i] || !runtime_colliders[i])continue;

		//攻撃属性のみを対象に切り替える
		if (collider_items[i]->attribute == ColliderAttribute::Attack)
		{
			collider_items[i]->is_active = is_active;
			runtime_colliders[i]->is_active = is_active;
		}
	}
}

//全コライダーの一括有効/無効切り替え
void AttachmentColliderComponent::SetAllColluderActive(bool is_active)
{
	for (size_t i = 0; i < collider_items.size() && i < runtime_colliders.size(); i++)
	{
		if (collider_items[i])collider_items[i]->is_active = is_active;
		if (runtime_colliders[i]) runtime_colliders[i]->is_active = is_active;
	}
}

//JSONファイルからコライダー設定復元
bool AttachmentColliderComponent::LoadFromJson(const std::string& file_path)
{
	if (file_path.empty())return false;

	std::ifstream input_file(file_path);
	if (!input_file.is_open())return false;

	nlohmann::json root_array;
	try
	{
		input_file >> root_array;
	}
	catch (...)
	{
		input_file.close();
		OutputDebugStringA("[AttachmentColliderComponent エラー] JSON解析例外が発生しました。\n");
		return false;
	}
	input_file.close();

	if (!root_array.is_array())return false;

	collider_items.clear();
	for (const auto& item_json : root_array)
	{
		auto new_item = std::make_unique<ColliderAttachmentItem>();
		JsonSerializer serializer;
		new_item->SetupSerialization(&serializer);
		serializer.LoadFromObject(item_json);
		new_item->OnDeserialized();
		collider_items.push_back(std::move(new_item));
	}
	return true;
}

//カプセル座標の計算
bool AttachmentColliderComponent::CalculateCapsulePoints(
	const ColliderAttachmentItem& item,
	ModelComponent* model_comp,
	DirectX::XMFLOAT3& out_start,
	DirectX::XMFLOAT3& out_end)
{
	if (!model_comp || item.start_bone_name.empty())return false;

	//2ボーン連携モード
	if (item.is_two_bone_link && !item.end_bone_name.empty())
	{
		DirectX::XMFLOAT4X4 start_world = {};
		DirectX::XMFLOAT4X4 end_world = {};

		if(!model_comp->GetBoneWorldTransform(item.start_bone_name,start_world) ||
			!model_comp->GetBoneWorldTransform(item.end_bone_name, end_world))
		{
			return false;
		}

		out_start = { start_world._41,start_world._42,start_world._43 };
		out_end = { end_world._41,end_world._42,end_world._43 };
		return true;
	}

	//単一ボーンモード
	DirectX::XMFLOAT4X4 bone_world = {};
	if (!model_comp->GetBoneWorldTransform(item.start_bone_name, bone_world))return false;

	DirectX::XMMATRIX m_bone = DirectX::XMLoadFloat4x4(&bone_world);

	DirectX::XMMATRIX mat_rot = DirectX::XMMatrixRotationRollPitchYaw(
		DirectX::XMConvertToRadians(item.rotation.x),
		DirectX::XMConvertToRadians(item.rotation.y),
		DirectX::XMConvertToRadians(item.rotation.z)
	);

	DirectX::XMVECTOR local_dir = DirectX::XMVector3TransformNormal(
		DirectX::XMVectorSet(0.0f, item.height, 0.0f, 0.0f), mat_rot
	);

	DirectX::XMVECTOR local_start = DirectX::XMVectorSet(item.offset.x, item.offset.y, item.offset.z, 1.0f);
	DirectX::XMVECTOR local_end = DirectX::XMVectorAdd(local_start, local_dir);

	DirectX::XMVECTOR v_start = DirectX::XMVector3TransformCoord(local_start, m_bone);
	DirectX::XMVECTOR v_end = DirectX::XMVector3TransformCoord(local_end, m_bone);

	DirectX::XMStoreFloat3(&out_start, v_start);
	DirectX::XMStoreFloat3(&out_end, v_end);
	return true;
}
