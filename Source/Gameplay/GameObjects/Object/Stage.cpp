#include "Stage.h"
#include "Gameplay/Components/Transform/TransformComponent.h"
#include "Gameplay/Components/Model/ModelComponent.h"
#include "Engine/Graphics/Renderers/Graphics.h"
#include "Engine/Collision/SpaceDivisionCast.h"
#include "Gameplay/GameObjects/ObjectFactory.h"

#include <imgui.h>

static AutoRegister<Stage> auto_register_stage("Stage");

//コンストラクタ
Stage::Stage()
{

}

//デストラクタ
Stage::~Stage()
{
}

//初期化
void Stage::Initialize()
{
	auto device = Graphics::Instance().GetDevice();

	//トランスフォームコンポーネントの追加
	auto transform = AddComponent<TransformComponent>();
	if (transform)transform->SetPosition({ 0.0f,0.0f,0.0f });

	//モデルコンポーネントの追加とモデル読み込み
	auto model_comp = AddComponent<ModelComponent>();
	if (model_comp)
	{
		model_comp->SetTransformComponent(transform);
		if (!model_comp->LoadModel("Data/Model/Stage/ExampleStage.glb"))
		{
			OutputDebugStringA("[Stage エラー] Initialize: ステージモデルの初期化・読み込みに失敗しました。\n");
		}
	}

	//全コンポーネントの初期化
	GameObject::Initialize();

	//空間分割キャストの生成とデータ構築
	space_division_cast = std::make_unique<SpaceDivisionCast>();
	BuildCollisionData();

	space_collider.space_cast = space_division_cast.get();
	space_collider.attribute = ColliderAttribute::Stage;
	space_collider.is_active = true;
	AddCollider(&space_collider);
	shape_renderer = std::make_unique<ShapeRenderer>(device);
	SetupSerialization();
}

//更新処理
void Stage::Update(float elapsed_time)
{
	GameObject::Update(elapsed_time);
}

//描画処理
void Stage::Render(ID3D11DeviceContext* context)
{
	GameObject::Render(context);
}

//デバッグ描画
void Stage::RenderDebug(ShapeRenderer* renderer)
{
	//描画フラグのチェック
	if (!is_draw_areas)return;

	//境界線データを取得
	std::vector<DirectX::BoundingBox> bboxes = space_division_cast->GetAreaBoundingBoxes();

	//ワールド行列を取得
	DirectX::XMMATRIX stage_world = DirectX::XMMatrixIdentity();
	auto transform = GetComponent<TransformComponent>();
	if (transform)stage_world = transform->GetWorldMatrix();

	//取得した境界線を全て描画登録
	if (!renderer)return;
	static constexpr float size_multiplier = 2.0f;

	DirectX::XMFLOAT3 scale = transform ? transform->GetScale() : DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f);
	DirectX::XMFLOAT4 rotation = transform ? transform->GetQuaternion() : DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

	//取得した全ての境界線データをループ
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

	//画面への描画
	ID3D11DeviceContext* context = Graphics::Instance().GetContext();
	DirectX::XMFLOAT4X4 view = Graphics::Instance().GetViewMatrix();
	DirectX::XMFLOAT4X4 projection = Graphics::Instance().GetProjectionMatrix();
	renderer->Render(context, view, projection);
}

//当たり判定用の空間分割キャストオブジェクトを取得
SpaceDivisionCast* Stage::GetSpaceDivisionCast()
{
	return space_division_cast.get();
}

//空間分割データの構築処理
void Stage::BuildCollisionData()
{
	auto model_comp = GetComponent<ModelComponent>();
	if (!model_comp)
	{
		OutputDebugStringA("[Stage エラー] BuildCollisionData: ModelComponent がアタッチされていません。\n");
		return;
	}
	std::shared_ptr<const GltfModelData> data = model_comp->GetModelData();
	if (!data)
	{
		OutputDebugStringA("[Stage エラー] BuildCollisionData: GltfModelData の取得に失敗しました。\n");
		return;
	}

	//モデルから当たり判定用の頂点とインデックスを抽出
	std::vector<DirectX::XMFLOAT3> vertices_data = data->GetVertices();
	std::vector<uint32_t> indices_data = data->GetIndices();

	if (!vertices_data.empty() && !indices_data.empty())
	{
		space_division_cast->Build(vertices_data, indices_data);
	}
}
