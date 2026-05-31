#include "SceneGame.h"
#include "../GameObjects/ObjectManager.h"
#include "../GameObjects/Objects/Stage.h"
#include "../Graphics/Graphics.h"
#include "../Camera/Camera.h"
#include "../Camera/FreeCamera.h"
#include "../Light/Light.h"

//コンストラクタ
SceneGame::SceneGame()
{
	object_manager = std::make_unique<ObjectManager>();
	Stage* stage = object_manager->Instantiate<Stage>();

	camera = std::make_unique<FreeCamera>();
	light = std::make_unique <Light>();
}

//デストラクタ
SceneGame::~SceneGame()
{
}

//初期化
void SceneGame::Initialize()
{
	camera->Initialize();
	DirectX::XMFLOAT4 init_light_dir = { -0.5f, -1.0f, 0.5f, 0.0f };
	if (light)
	{
		light->SetDirection(init_light_dir);
	}
}

//終了化
void SceneGame::Finalize()
{
}

//更新処理
void SceneGame::Update(float elapsed_time)
{
	if (object_manager)
	{
		object_manager->Update(elapsed_time);
	}
	if (camera)
	{
		camera->Update(elapsed_time);
	}
}

//描画処理
void SceneGame::Render(float elapsed_time)
{
	ID3D11DeviceContext* context = Graphics::Instance().GetContext();
	auto states = Graphics::Instance().GetPipelineStates();
	context->OMSetDepthStencilState(states->GetDepthStenceilState(1).Get(), 1);
	context->RSSetState(states->GetRasterizerState(2).Get());

	ID3D11SamplerState* sampler_states[] = {
		states->GetSamplerState(0).Get(),
		states->GetSamplerState(1).Get(),
		states->GetSamplerState(2).Get()
	};

	context->PSSetSamplers(0, 3, sampler_states);

	scene_constants constants{};
	if (camera && light)
	{
		constants.view_projection = camera->GetViewProjectionMatrix();
		constants.light_direction = light->GetDirection();
		constants.camera_position = camera->GetPosition();
		Graphics::Instance().UpdateSceneConstantBuffer(constants);
	}

	if (object_manager)
	{
		object_manager->Render(context);
		object_manager->RenderGui();
	}


#ifdef USE_IMGUI
	RenderGui();
#endif // USE_IMGUI
}

//ImGuiデバッグ描画
void SceneGame::RenderGui()
{
#ifdef USE_IMGUI
	if (ImGui::Begin("Game Debug"))
	{
		if (camera)
		{
			camera->RenderGui();
		}
	}
	ImGui::End();
#endif // USE_IMGUI
}
