#include "SceneGame.h"

//コンストラクタ
SceneGame::SceneGame()
{
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
