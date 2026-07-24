#include "SceneGame.h"
#include "Gameplay/GameObjects/ObjectManager.h"
#include "Engine\Graphics\Renderers\Graphics.h"
#include "Engine\Graphics\Device\framebuffer.h"
#include "Engine/Camera/Camera.h"
#include "Engine/Camera/FreeCamera.h"
#include "Engine\Graphics\Resources\Light.h"
#include "Engine\Graphics\Renderers\ShapeRenderer.h"
#include "Editor\EditorManager.h"
#include "Engine/Collision/CollisionManager.h"
#include "Engine/Collision/CollisionExperiment.h"
#include "Gameplay/GameObjects/Character/Character.h"
#include "Engine/Graphics/Shaders/SkyBox.h"
#include "SceneManager.h"

//コンストラクタ
SceneGame::SceneGame()
	:object_manager(std::make_unique<ObjectManager>())
	,collision_manager(std::make_unique<CollisionManager>())
{
	ObjectManager::Instance().SetCollisionManager(collision_manager.get());

	collision_experiment = std::make_unique<CollisionExperiment>(collision_manager.get());

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
	collision_experiment.reset();
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
	editor_manager = std::make_unique<EditorManager>();
	editor_manager->Initialize();
}

//終了化
void SceneGame::Finalize()
{
	debug_shapes.clear();
	if (shape_renderer)
	{
		shape_renderer.reset();
	}
	if (object_manager)
	{
		object_manager.reset();
	}
	if (camera)
	{
		camera.reset();
	}
	if (light)
	{
		light.reset();
	}
}

//更新処理
void SceneGame::Update(float elapsed_time)
{
	
#ifdef USE_IMGUI
	const ImGuiIO& io = ImGui::GetIO();
	if (camera && !io.WantCaptureMouse)
	{
		camera->Update(elapsed_time);
	}
#else
	if (camera)
	{
		camera->Update(elapsed_time);
	}
#endif
	if (editor_manager)editor_manager->Update(elapsed_time);

	if (editor_manager && !editor_manager->IsGameViewportActive())
	{
		return;
	}

	if (SceneManager::Instance().IsPaused())
	{
		return;
	}

	if (object_manager)
	{
		object_manager->Update(elapsed_time);
	}

	if (collision_experiment)
	{
		collision_experiment->Update(elapsed_time);
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
	framebuffer* shadow_fb = Graphics::Instance().GetShadowFramebuffer();

	if (editor_manager && !editor_manager->IsGameViewportActive())
	{
#ifdef USE_IMGUI
		editor_manager->RenderPreviews(context);
		RenderGui(); 
#endif
		return;
	}

	// パス間で共有するライト空間変換行列の計算
	DirectX::XMFLOAT4X4 light_view_projection_matrix{};
	DirectX::XMFLOAT4 light_dir_vector{};

	// ライトが有効か判定
	if (light)
	{
		const float k_light_camera_distance = 100.0f;
		const float k_light_near_clip = 0.1f;
		const float k_light_far_clip = 300.0f;

		light_dir_vector = light->GetDirection();
		DirectX::XMFLOAT3 camera_focus = camera->GetFocus();
		DirectX::XMVECTOR target_pos = DirectX::XMLoadFloat3(&camera_focus);
		DirectX::XMVECTOR light_pos = DirectX::XMLoadFloat4(&light_dir_vector);
		light_pos = DirectX::XMVectorScale(light_pos, -k_light_camera_distance);
		DirectX::XMMATRIX light_view = DirectX::XMMatrixLookAtLH(
			light_pos,
			target_pos,
			DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)
		);
		DirectX::XMMATRIX light_projection = DirectX::XMMatrixOrthographicLH(
			k_shadow_area_size,
			k_shadow_area_size,
			k_light_near_clip,
			k_light_far_clip
		);
		DirectX::XMStoreFloat4x4(&light_view_projection_matrix, light_view * light_projection);
	}

	// パイプラインのハザードを解消するためのテクスチャ解除処理
	if (shadow_fb)
	{
		ID3D11ShaderResourceView* null_srv_list[] = { nullptr };
		const UINT k_shader_shadow_srv_slot = 10;
		context->PSSetShaderResources(k_shader_shadow_srv_slot, 1, null_srv_list);
	}

	// シャドウマップ（深度バッファ）生成パス
	if (shadow_fb && camera && light && object_manager)
	{
		shadow_fb->clear(context, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f);
		shadow_fb->activate(context);

		scene_constants light_scene_constants{};
		light_scene_constants.view_projection = light_view_projection_matrix;
		light_scene_constants.light_view_projection = light_view_projection_matrix;
		light_scene_constants.light_direction = light_dir_vector;
		light_scene_constants.camera_position = camera->GetPosition();
		light_scene_constants.light_color = { 1.0f, 1.0f, 1.0f, 1.0f };
		light_scene_constants.ambient_color = { 1.0f, 1.0f, 1.0f, 1.0f };
		Graphics::Instance().UpdateSceneConstantBuffer(light_scene_constants);

		context->OMSetDepthStencilState(states->GetDepthStenceilState(1).Get(), 1);
		context->RSSetState(states->GetRasterizerState(2).Get());

		ID3D11SamplerState* shadow_sampler = Graphics::Instance().GetShadowSamplerState();
		const UINT k_shader_shadow_sampler_slot = 10;
		context->PSSetSamplers(k_shader_shadow_sampler_slot, 1, &shadow_sampler);

		object_manager->Render(context);
		shadow_fb->deactivate(context);
	}

	context->OMSetDepthStencilState(states->GetDepthStenceilState(1).Get(), 1);
	context->RSSetState(states->GetRasterizerState(0).Get());

	ID3D11SamplerState* sampler_p0 = states->GetSamplerState(0).Get();
	ID3D11SamplerState* sampler_p1 = states->GetSamplerState(1).Get();
	ID3D11SamplerState* sampler_p2 = states->GetSamplerState(2).Get();

	context->PSSetSamplers(0, 1, &sampler_p0);
	context->PSSetSamplers(1, 1, &sampler_p1);
	context->PSSetSamplers(2, 1, &sampler_p2);

	if (shadow_fb)
	{
		ID3D11ShaderResourceView* shadow_srv = shadow_fb->shader_resource_views[1].Get();
		ID3D11SamplerState* shadow_sampler = Graphics::Instance().GetShadowSamplerState();
		const UINT k_shader_shadow_srv_slot = 10;
		const UINT k_shader_shadow_sampler_slot = 10;
		context->PSSetShaderResources(k_shader_shadow_srv_slot, 1, &shadow_srv);
		context->PSSetSamplers(k_shader_shadow_sampler_slot, 1, &shadow_sampler);
	}

	scene_constants constants{};

	// カメラとライトが有効か判定
	if (camera && light)
	{
		constants.view_projection = camera->GetViewProjectionMatrix();
		constants.light_direction = light->GetDirection();
		constants.camera_position = camera->GetPosition();
		constants.light_color = { 1.0f,1.0f,1.0f,1.0f };
		constants.ambient_color = { 1.0f,1.0f,1.0f,1.0f };
		constants.light_view_projection = light_view_projection_matrix;
		Graphics::Instance().UpdateSceneConstantBuffer(constants);
	}

	if (skybox)
	{
		skybox->BindIblTextures(context);
	}

	if (object_manager)
	{
		object_manager->Render(context);
	}

	if (collision_experiment)
	{
		collision_experiment->Render(shape_renderer.get());
	}

	// レンダラーとカメラが有効か判定
	if (shape_renderer && camera)
	{
		const float k_light_debug_distance = 5.0f;
		DirectX::XMFLOAT4 light_dir = light->GetDirection();
		DirectX::XMFLOAT3 light_pos = {
			-light_dir.x * k_light_debug_distance,
			-light_dir.y * k_light_debug_distance,
			-light_dir.z * k_light_debug_distance
		};
		shape_renderer->DrawSphere(light_pos, 0.5f, { 1.0f, 1.0f, 0.0f, 1.0f }, ShapeDrawMode::Solid);

		for (const debug_shape& shape : debug_shapes)
		{
			ShapeDrawMode mode = static_cast<ShapeDrawMode>(shape.draw_mode);
			DirectX::XMFLOAT4 rotation = { 0.0f,0.0f,0.0f,1.0f };

			if (shape.type == debug_shape_type::box)
			{
				shape_renderer->DrawBox(shape.position, rotation, { 1.0f, 1.0f, 1.0f }, shape.color, mode);
			}
			if (shape.type == debug_shape_type::sphere)
			{
				shape_renderer->DrawSphere(shape.position, 0.5f, shape.color, mode);
			}
			if (shape.type == debug_shape_type::cylinder)
			{
				shape_renderer->DrawCylinder(shape.position, rotation, 0.5f, 1.0f, shape.color, mode);
			}
			if (shape.type == debug_shape_type::capsule)
			{
				shape_renderer->DrawCapsule(shape.position, rotation, 0.5f, 3.0f, shape.color, mode);
			}
		}
		shape_renderer->Render(context, camera->GetView(), camera->GetProjection());
	}

	if (skybox)
	{
		skybox->Render(context);
	}

#ifdef USE_IMGUI
	RenderGui();
#endif
}

//ImGuiデバッグ描画
void SceneGame::RenderGui()
{
#ifdef USE_IMGUI
	if (object_manager)
	{
		//object_manager->RenderGui();
		object_manager->RenderDebug(shape_renderer.get());
	}

	editor_manager->RenderGui(camera.get(), collision_manager.get());

#endif // USE_IMGUI
}
