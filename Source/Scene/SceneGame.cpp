#include "SceneGame.h"
#include "../GameObjects/ObjectManager.h"
#include "../GameObjects/Objects/Stage.h"
#include "../Graphics/Graphics.h"
#include "../Camera/Camera.h"
#include "../Light/Light.h"

//コンストラクタ
SceneGame::SceneGame()
{
	object_manager = std::make_unique<ObjectManager>();
	Stage* stage = object_manager->Instantiate<Stage>();
}

//デストラクタ
SceneGame::~SceneGame()
{
}

//初期化
void SceneGame::Initialize()
{
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
}

//描画処理
void SceneGame::Render(float elapsed_time)
{
	ID3D11DeviceContext* context = Graphics::Instance().GetContext();
	auto states = Graphics::Instance().GetPipelineStates();
	context->OMSetDepthStencilState(states->GetDepthStenceilState(1).Get(), 1);
	context->RSSetState(states->GetRasterizerState(2).Get());
	context->PSSetSamplers(0, 1, states->GetSamplerState(0).GetAddressOf());
	Graphics::Instance().BeginFrame(0.2f, 0.2f, 0.2f, 1.0f);

	if (object_manager)
	{
		object_manager->Render(context);
		object_manager->RenderGui();
	}


#ifdef USE_IMGUI
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
#endif // USE_IMGUI

	Graphics::Instance().EndFrame();
}

//ImGuiデバッグ描画
void SceneGame::RenderGui()
{
}
