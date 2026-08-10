#include "Stage.h"
#include "Gameplay/Components/Transform/TransformComponent.h"
#include "Gameplay/Components/Model/ModelComponent.h"
#include "Engine/Graphics/Renderers/Graphics.h"
#include "Engine/Collision/SpaceDivisionCast.h"
#include "Gameplay/GameObjects/ObjectFactory.h"

#include <imgui.h>

static AutoRegister<Stage> auto_register_stage("Stage");

// コンストラクタ
Stage::Stage()
{
	transform_component = AddComponent<TransformComponent>();
	model_component = AddComponent<ModelComponent>();

	if (model_component && transform_component)
	{
		model_component->SetTransformComponent(transform_component);
	}
	else
	{
		OutputDebugStringA("[Stage エラー] コンストラクタ: 基本コンポーネントの生成に失敗しました。\n");
	}
}

// デストラクタ
Stage::~Stage()
{
}

// 初期化（固定モデルの自動ロードは行わない）
void Stage::Initialize()
{
	is_active = true;

	auto device = Graphics::Instance().GetDevice();

	// 全コンポーネントの初期化実行
	GameObject::Initialize();

	// 空間分割キャストの生成
	space_division_cast = std::make_unique<SpaceDivisionCast>();

	space_collider.space_cast = space_division_cast.get();
	space_collider.attribute = ColliderAttribute::Stage;
	space_collider.is_active = true;
	AddCollider(&space_collider);

	shape_renderer = std::make_unique<ShapeRenderer>(device);

	// JsonSerializer および GuiInspector への登録
	SetupSerialization();
}

// 専用シリアライザおよび GuiInspector への登録
void Stage::SetupSerialization()
{
	GameObject::SetupSerialization();

	// 専用クラス JsonSerializer への登録
	if (serializer)
	{
		serializer->RegisterVariable(u8"空間分割エリア描画", &is_draw_areas);
	}

	// 専用クラス GuiInspector への登録（詳細パネルに自動表示）
	if (inspector)
	{
		inspector->RegisterVariable(u8"空間分割エリア描画", &is_draw_areas, u8"空間分割デバッグ");
	}
}

// 更新処理（後付けされたモデルデータの変更を自動検知して当たり判定を構築）
void Stage::Update(float elapsed_time)
{
	GameObject::Update(elapsed_time);

	// ModelComponent にモデルが設定・変更された場合、空間分割データを自動更新
	if (model_component)
	{
		auto latest_data = model_component->GetModelData();
		if (latest_data && latest_data != current_model_data)
		{
			current_model_data = latest_data;
			BuildCollisionData();
		}
	}
}

// 描画処理
void Stage::Render(ID3D11DeviceContext* context)
{
	GameObject::Render(context);
}

// デバッグ描画（空間分割グリッドの表示）
void Stage::RenderDebug(ShapeRenderer* renderer)
{
	if (!is_draw_areas || !space_division_cast) return;

	std::vector<DirectX::BoundingBox> bboxes = space_division_cast->GetAreaBoundingBoxes();

	DirectX::XMMATRIX stage_world = DirectX::XMMatrixIdentity();
	if (transform_component)
	{
		stage_world = transform_component->GetWorldMatrix();
	}

	if (!renderer) return;
	static constexpr float size_multiplier = 2.0f;

	DirectX::XMFLOAT3 scale = transform_component ? transform_component->GetScale() : DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f);
	DirectX::XMFLOAT4 rotation = transform_component ? transform_component->GetQuaternion() : DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

	for (size_t i = 0; i < bboxes.size(); i++)
	{
		const DirectX::BoundingBox& box = bboxes.at(i);
		DirectX::XMVECTOR local_center = DirectX::XMLoadFloat3(&box.Center);
		DirectX::XMVECTOR world_center = DirectX::XMVector3TransformCoord(local_center, stage_world);
		DirectX::XMFLOAT3 final_center_pos;
		DirectX::XMStoreFloat3(&final_center_pos, world_center);
		DirectX::XMFLOAT3 extents_size = box.Extents;
		DirectX::XMFLOAT3 full_size = {
			extents_size.x * size_multiplier * scale.x,
			extents_size.y * size_multiplier * scale.y,
			extents_size.z * size_multiplier * scale.z
		};
		renderer->DrawBox(final_center_pos, rotation, full_size, area_draw_color, ShapeDrawMode::Wireframe);
	}

	ID3D11DeviceContext* context = Graphics::Instance().GetContext();
	DirectX::XMFLOAT4X4 view = Graphics::Instance().GetViewMatrix();
	DirectX::XMFLOAT4X4 projection = Graphics::Instance().GetProjectionMatrix();
	renderer->Render(context, view, projection);
}

// 当たり判定用の空間分割キャストオブジェクトを取得
SpaceDivisionCast* Stage::GetSpaceDivisionCast()
{
	return space_division_cast.get();
}

// 空間分割データの構築処理
void Stage::BuildCollisionData()
{
	if (!model_component) return;

	std::shared_ptr<const GltfModelData> data = model_component->GetModelData();
	if (!data) return;

	if (!space_division_cast)
	{
		space_division_cast = std::make_unique<SpaceDivisionCast>();
		space_collider.space_cast = space_division_cast.get();
	}

	std::vector<DirectX::XMFLOAT3> local_vertices = data->GetVertices();
	std::vector<uint32_t> indices_data = data->GetIndices();

	if (local_vertices.empty() || indices_data.empty())return;

	DirectX::XMMATRIX world_matrix = transform_component->GetWorldMatrix();

	std::vector<DirectX::XMFLOAT3> world_vertices;
	world_vertices.reserve(local_vertices.size());

	for (const auto& local_pos : local_vertices)
	{
		DirectX::XMVECTOR v_local = DirectX::XMLoadFloat3(&local_pos);
		DirectX::XMVECTOR v_world = DirectX::XMVector3TransformCoord(v_local, world_matrix);
		DirectX::XMFLOAT3 world_pos;
		DirectX::XMStoreFloat3(&world_pos, v_world);
		world_vertices.push_back(world_pos);
	}

	space_division_cast->Build(world_vertices, indices_data);
	printf_s("[Stage 情報] BuildCollisionData: ワールド行列を反映して当たり判定を構築しました。\n");
}