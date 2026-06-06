#include "SceneGame.h"
#include "../GameObjects/ObjectManager.h"
#include "../GameObjects/Objects/Stage.h"
#include "../Graphics/Graphics.h"
#include "../Camera/Camera.h"
#include "../Camera/FreeCamera.h"
#include "../Light/Light.h"
#include "../Graphics/ShapeRenderer.h"
#include "../GameObjects/Characters/Player.h"
#include "../Collision/CollisionManager.h"
#include "../Shaders/SkyBox.h"
#include "../Shaders/ShadowMap.h"
#include "SceneManager.h"

//コンストラクタ
SceneGame::SceneGame()
{
	object_manager = std::make_unique<ObjectManager>();
	collision_manager = std::make_unique<CollisionManager>();
	object_manager->SetCollisionManager(collision_manager.get());

	Stage* stage = object_manager->Instantiate<Stage>();
	Player* player = object_manager->Instantiate<Player>();

	camera = std::make_unique<FreeCamera>();
	light = std::make_unique <Light>();
	skybox = std::make_unique<SkyBox>();
	skybox->Initialize(
		Graphics::Instance().GetDevice(),
		L"Data/Sprite/SkyTexture/skybox.dds",			//背景用のキューブマップテクスチャ
		L"Data/Sprite/SkyTexture/diffuse_iem.dds",		//IBL用の拡散反射テクスチャ
		L"Data/Sprite/SkyTexture/specular_pmrem.dds",	//IBL用の鏡面反射テクスチャ
		L"Data/Sprite/SkyTexture/lut_ggx.dds"			//IBL用のルックアップテーブル
	);
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

	ID3D11Device* device = Graphics::Instance().GetDevice();
	shape_renderer = std::make_unique<ShapeRenderer>(device);

	//シャドウマップの初期化
	shadow_map = std::make_unique<ShadowMap>();
	shadow_map->Initialize(device);
}

//終了化
void SceneGame::Finalize()
{
	debug_shapes.clear();
	if (shape_renderer)shape_renderer.reset();
	if (object_manager)object_manager.reset();
	if (camera)camera.reset();
	if (light)light.reset();
	if (shadow_map) shadow_map.reset();
}

//更新処理
void SceneGame::Update(float elapsed_time)
{
	if (camera)
	{
		camera->Update(elapsed_time);
	}
	if (SceneManager::Instance().IsPaused())
	{
		return;
	}

	if (object_manager)
	{
		object_manager->Update(elapsed_time);
	}
	if (collision_manager)
	{
		collision_manager->ExecuteCollision();
	}
}

//描画処理
void SceneGame::Render(float elapsed_time)
{
	ID3D11DeviceContext* context = Graphics::Instance().GetContext();
	auto states = Graphics::Instance().GetPipelineStates();

	//現在のレンダーターゲットとビューポートの退避
	ID3D11RenderTargetView* cached_rtv{ nullptr };
	ID3D11DepthStencilView* cached_dsv{ nullptr };
	context->OMGetRenderTargets(1, &cached_rtv, &cached_dsv);

	UINT num_viewports = 1;
	D3D11_VIEWPORT cached_viewport{};
	context->RSGetViewports(&num_viewports, &cached_viewport);

	//シャドウマップへの影の焼き付けパス
	if (shadow_map && light && object_manager)
	{
		DirectX::XMFLOAT4 light_dir = light->GetDirection();
		DirectX::XMFLOAT3 light_dir_3 = { light_dir.x, light_dir.y, light_dir.z };
		context->OMSetDepthStencilState(states->GetDepthStenceilState(1).Get(), 1);
		shadow_map->BeginCasterPass(context, light_dir_3);
		object_manager->RenderCaster(context);
	}

	//通常のシーン描画パスへの復帰
	context->OMSetRenderTargets(1, &cached_rtv, cached_dsv);
	context->RSSetViewports(1, &cached_viewport);
	if (cached_rtv) cached_rtv->Release();
	if (cached_dsv) cached_dsv->Release();

	//通常描画の共通パイプラインステート設定
	context->OMSetDepthStencilState(states->GetDepthStenceilState(1).Get(), 1);
	context->RSSetState(states->GetRasterizerState(2).Get());

	ID3D11SamplerState* sampler_p0 = states->GetSamplerState(0).Get(); //POINTサンプラー
	ID3D11SamplerState* sampler_p1 = states->GetSamplerState(1).Get(); //LINEARサンプラー
	ID3D11SamplerState* sampler_p2 = states->GetSamplerState(2).Get(); //ANISOTROPICサンプラー

	//各スロットへ1つずつバインドします
	context->PSSetSamplers(0, 1, &sampler_p0);
	context->PSSetSamplers(1, 1, &sampler_p1);
	context->PSSetSamplers(2, 1, &sampler_p2);

	scene_constants constants{};
	if (camera && light)
	{
		constants.view_projection = camera->GetViewProjectionMatrix();
		constants.light_direction = light->GetDirection();
		constants.camera_position = camera->GetPosition();
		constants.light_color = { 1.0f,1.0f,1.0f,1.0f };
		constants.ambient_color = { 1.0f,1.0f,1.0f,1.0f };
		Graphics::Instance().UpdateSceneConstantBuffer(constants);
	}

	//IBL環境マッピングテクスチャのバインド
	if (skybox)
	{
		skybox->BindIblTextures(context);
	}

	//シャドウマップの適用と本描画
	if (shadow_map)
	{
		static constexpr UINT shadow_texture_slot = 6;
		static constexpr UINT shadow_sampler_slot = 6;
		static constexpr UINT shadow_cb_slot = 6;

		shadow_map->BindReceiverPass(context, shadow_texture_slot, shadow_sampler_slot, shadow_cb_slot);
		if (object_manager)
		{
			object_manager->Render(context);
		}
	}

	//デバッグ形状の描画
	if (shape_renderer && camera)
	{
		for (const debug_shape & shape : debug_shapes)
		{
			ShapeDrawMode mode = static_cast<ShapeDrawMode>(shape.draw_mode);
			DirectX::XMFLOAT4 rotation = { 0.0f,0.0f,0.0f,1.0f };
			switch (shape.type)
			{
			case debug_shape_type::box:
				shape_renderer->DrawBox(shape.position, rotation, { 1.0f, 1.0f, 1.0f }, shape.color, mode);
				break;
			case debug_shape_type::sphere:
				shape_renderer->DrawSphere(shape.position, 0.5f, shape.color, mode);
				break;
			case debug_shape_type::cylinder:
				shape_renderer->DrawCylinder(shape.position, rotation, 0.5f, 1.0f, shape.color, mode);
				break;
			case debug_shape_type::capsule:
				shape_renderer->DrawCapsule(shape.position, rotation, 0.5f, 3.0f, shape.color, mode);
				break;
			}
		}
		shape_renderer->Render(context, camera->GetView(), camera->GetProjection());
	}

	//スカイボックス（背景）の描画
	if (skybox)
	{
		skybox->Render(context);
	}

	//影のリソース紐付け解除
	if (shadow_map)
	{
		static constexpr UINT shadow_texture_slot = 6;
		shadow_map->UnbindReceiverPass(context, shadow_texture_slot);
	}

#ifdef USE_IMGUI
	RenderGui();

#endif // USE_IMGUI
}

//ImGuiデバッグ描画
void SceneGame::RenderGui()
{
#ifdef USE_IMGUI
	//Scene::ImGuiScaleCorrection();
	ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

	if (object_manager)
	{
		object_manager->RenderGui();
		object_manager->RenderDebug(shape_renderer.get());
	}

	if (ImGui::Begin("Game Debug"))
	{
		bool check_paused = SceneManager::Instance().IsPaused();
		if (ImGui::Checkbox("Game Pause (F5)", &check_paused))
		{
			SceneManager::Instance().SetPauseState(check_paused);
		}

		if (skybox && skybox->GetSkyboxSrv())
		{
			ImGui::Text("SkyBox Texture Check");
			ImGui::Image(skybox->GetSkyboxSrv(), { 256, 256 });
		}

		//シャドウマップのパラメータ調整UI
		if (shadow_map)
		{
			if (ImGui::CollapsingHeader("ShadowMap Settings", ImGuiTreeNodeFlags_DefaultOpen))
			{
				DirectX::XMFLOAT3 color = shadow_map->GetShadowColor();
				if (ImGui::ColorEdit3("Shadow Color", &color.x))
				{
					shadow_map->SetShadowColor(color);
				}
				float bias = shadow_map->GetShadowBias();
				if (ImGui::SliderFloat("Shadow Bias", &bias, 0.0f, 0.01f, "%.4f"))
				{
					shadow_map->SetShadowBias(bias);
				}
				if (shadow_map && shadow_map->GetShaderResourceView())
				{
					ImGui::Separator();
					ImGui::Text("Shadow Map Texture");
					ImGui::Image(shadow_map->GetShaderResourceView(), { 256, 256 });
				}
			}
		}

		if (camera)
		{
			camera->RenderGui();
		}
		if (ImGui::CollapsingHeader("Shape Generator", ImGuiDockNodeFlags_None))
		{
			ImGui::RadioButton(u8"枠線のみ (Wireframe)", &current_debug_draw_mode, 0);
			ImGui::RadioButton(u8"面のみ (Solid)", &current_debug_draw_mode, 1);
			ImGui::RadioButton(u8"両方 (Solid & Wireframe)", &current_debug_draw_mode, 2);
			ImGui::ColorEdit4("Shape Color", &current_debug_color.x);
			ImGui::Spacing();
			if (camera)
			{
				DirectX::XMFLOAT3 focus = camera->GetFocus();
				ImGui::InputFloat3("Camera Focus", &focus.x, "%.3f", ImGuiInputTextFlags_ReadOnly);
				if (ImGui::Button("Spawn Box"))
				{
					debug_shapes.push_back({ debug_shape_type::box, focus, current_debug_draw_mode, current_debug_color });
				}
				ImGui::SameLine();
				if (ImGui::Button("Spawn Sphere"))
				{
					debug_shapes.push_back({ debug_shape_type::sphere, focus, current_debug_draw_mode, current_debug_color });
				}
				ImGui::SameLine();
				if (ImGui::Button("Spawn Cylinder"))
				{
					debug_shapes.push_back({ debug_shape_type::cylinder, focus, current_debug_draw_mode, current_debug_color });
				}
				ImGui::SameLine();
				if (ImGui::Button("Spawn Capsule"))
				{
					debug_shapes.push_back({ debug_shape_type::capsule, focus, current_debug_draw_mode, current_debug_color });
				}

				ImGui::Spacing();
				if (ImGui::Button("Clear All Shapes"))
				{
					debug_shapes.clear();
				}
			}
		}
		if (collision_manager)
		{
			collision_manager->RenderGui();
			collision_manager->RenderDebug(shape_renderer.get());
		}
	}
	ImGui::End();
#endif // USE_IMGUI
}
