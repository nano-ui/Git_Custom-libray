#include "Stage.h"
#include "../ObjectsRender/Model.h"
#include "../Graphics/Graphics.h"
#include "../Collision/SpaceDivisionCast.h"

#include <imgui.h>

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
	//モデル読み込み
	auto device = Graphics::Instance().GetDevice();
	stage_model = std::make_unique<Model>(device, "Data/Model/Stage/ExampleStage.glb");

	//空間分割キャストの生成とデータ構築
	space_division_cast = std::make_unique<SpaceDivisionCast>();
	BuildCollisionData();
}

//更新処理
void Stage::Update(float elapsed_time)
{
	stage_model->Update(elapsed_time);
}

//描画処理
void Stage::Render(ID3D11DeviceContext* context)
{
	DirectX::XMMATRIX world_matrix = GetWorldMatrix();
	DirectX::XMFLOAT4X4 transform_matrix;
	DirectX::XMStoreFloat4x4(&transform_matrix, world_matrix);
	stage_model->Render(context, transform_matrix);
}

//デバッグ描画
void Stage::RenderDebug()
{

}

//ImGuiデバッグ描画
void Stage::RenderGui()
{
#ifdef USE_IMGUI
	if (ImGui::Begin("Stage Debug"))
	{
		if (ImGui::CollapsingHeader("Stage", ImGuiTreeNodeFlags_None))
		{
			ImGui::DragFloat3("Position", &position.x, 0.1f);
			ImGui::DragFloat4("Rotation", &rotation.x, DirectX::XM_1DIVPI);
			ImGui::DragFloat3("Scale", &scale.x, 0.1f);
		}
	}
	ImGui::End();
#endif // USE_IMGUI
}

//当たり判定用の空間分割キャストオブジェクトを取得
SpaceDivisionCast* Stage::GetSpaceDivisionCast()
{
	return space_division_cast.get();
}

//空間分割データの構築処理
void Stage::BuildCollisionData()
{
	//モデルから当たり判定用の頂点とインデックスを抽出
	std::vector<DirectX::XMFLOAT3> vertices_data;
	std::vector<uint32_t> indices_data;

	vertices_data = stage_model->GetVertices();
	indices_data = stage_model->GetIndices();

	//空間分割クラスへのデータ登録
	if (!vertices_data.empty() && !indices_data.empty())
	{
		space_division_cast->Build(vertices_data, indices_data);
	}
}
