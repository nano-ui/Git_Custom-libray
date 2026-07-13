#include "ModelPreviewScene.h"
#include "Engine\Graphics\framebuffer.h"
#include "Engine\Graphics\Graphics.h"
#include "Engine\Graphics\Model.h"
#include "Engine\Camera\FreeCamera.h"

#include <windows.h>
#include <imgui.h>

//コンストラクタ
ModelPreviewScene::ModelPreviewScene()
{

}

//デストラクタ
ModelPreviewScene::~ModelPreviewScene() = default;

//初期化
void ModelPreviewScene::Initialize()
{
	//プレビュー画面の解像度設定
	const uint32_t buffer_width = 1280;
	const uint32_t buffer_height = 720;

	//フレームバッファを生成
	frame_buffer = std::make_unique<framebuffer>(
		Graphics::Instance().GetDevice(),
		buffer_width,
		buffer_height
	);

	camera = std::make_unique<FreeCamera>();
	camera->Initialize();
}

//更新
void ModelPreviewScene::Update(float elapsed_time)
{
	camera->Update(elapsed_time);
	if (model)model->Update(elapsed_time);
}

//描画
void ModelPreviewScene::Render(ID3D11DeviceContext* immediate_context)
{
	frame_buffer->clear(immediate_context);
	frame_buffer->activate(immediate_context);

	DirectX::XMFLOAT4X4 world_matrix;
	DirectX::XMStoreFloat4x4(&world_matrix, DirectX::XMMatrixIdentity());
	if (model)model->Render(immediate_context, world_matrix);

	frame_buffer->deactivate(immediate_context);
}

//ImGui描画
void ModelPreviewScene::RenderGui()
{
	//プレビュー画面の表示サイズ設定
	const float view_width = 1280.0f;
	const float view_height = 720.0f;

	if (ImGui::Begin("Animation Preview"))
	{
		ImGui::Image(
			reinterpret_cast<ImTextureID>(frame_buffer->shader_resource_views[0].Get()),
			ImVec2(view_width, view_height)
		);
	}
	ImGui::End();
}

//モデル読み込み
void ModelPreviewScene::LoadModel(const std::string& file_path)
{
	model = std::make_unique<Model>(Graphics::Instance().GetDevice(), file_path);
}
