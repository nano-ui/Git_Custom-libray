#include "SceneTitle.h"
#include "../ObjectsRender/sprite_batch.h"
#include "../Graphics/Graphics.h"
#include "../Common/misc.h"

#include <imgui.h> 
#include <imgui_impl_dx11.h> 
#include <imgui_impl_win32.h>

static constexpr float logical_screen_width = 1280.0f; //ゲームの設計上の基本横幅
static constexpr float logical_screen_height = 720.0f; //ゲームの設計上の基本縦幅

//コンストラクタ
SceneTitle::SceneTitle()
{
	screen_size.x = 1280.0f;
	screen_size.y = 720.0f;
	title_pos.x = 0.0f;
	title_pos.y = 0.0f;
	color = { 1.0f,1.0f,1.0f,1.0f };
	angle = 0.0f;
}

//デストラクタ
SceneTitle::~SceneTitle()
{
	Finalize();
}

//初期化
void SceneTitle::Initialize()
{
	//画像読み込み
	ID3D11Device* device = Graphics::Instance().GetDevice();
	title_sprite = std::make_unique<sprite_batch>(device, L"Data/Sprite/Title/Title.png", 1);
}

//終了化
void SceneTitle::Finalize()
{
	if (title_sprite)title_sprite.reset();
}

//更新処理
void SceneTitle::Update(float elapsed_time)
{
	Scene::ImGuiScaleCorrection();
	RenderGui();
}

//描画処理
void SceneTitle::Render(float elapsed_time)
{
	ID3D11DeviceContext* context = Graphics::Instance().GetContext();
	auto states = Graphics::Instance().GetPipelineStates();
	context->OMSetDepthStencilState(states->GetDepthStenceilState(0).Get(), 1);
	context->RSSetState(states->GetRasterizerState(2).Get());
	context->PSSetSamplers(0, 1, states->GetSamplerState(0).GetAddressOf());
	Graphics::Instance().BeginFrame(0.2f, 0.2f, 0.2f, 1.0f);

	if (title_sprite)
	{
		title_sprite->begin(context);
		title_sprite->render(
			context,
			title_pos.x, title_pos.y,
			screen_size.x, screen_size.y,
			color.x, color.y, color.z, color.w,
			angle
		);
		title_sprite->end(context);
	}
#ifdef USE_IMGUI
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData()); 
#endif // USE_IMGUI

	Graphics::Instance().EndFrame();
}

//ImGuiデバッグ描画
void SceneTitle::RenderGui()
{
#ifdef USE_IMGUI
	if (ImGui::CollapsingHeader("SceneTitle", ImGuiTreeNodeFlags_None))
	{
		ImGui::DragFloat2("TitlePos", &title_pos.x, 0.1f);
		ImGui::DragFloat2("Size", &screen_size.x, 0.1f);
		ImGui::ColorEdit4("TitleColor", &color.x);
		ImGui::SliderFloat("Angle", &angle, 0.0f, 360.0f);
	}
#endif // DEBUG
}


