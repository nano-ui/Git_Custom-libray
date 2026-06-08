#include "SceneGame.h"
#include "../GameObjects/ObjectManager.h"
#include "../GameObjects/Objects/Stage.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/framebuffer.h"
#include "../Camera/Camera.h"
#include "../Camera/FreeCamera.h"
#include "../Light/Light.h"
#include "../Graphics/ShapeRenderer.h"
#include "../GameObjects/Characters/Player.h"
#include "../Collision/CollisionManager.h"
#include "../Collision/CollisionExperiment.h"
#include "../Editor/ObjectEditor.h"
#include "../Shaders/SkyBox.h"
#include "SceneManager.h"

//コンストラクタ
SceneGame::SceneGame()
	:object_manager(std::make_unique<ObjectManager>())
	,collision_manager(std::make_unique<CollisionManager>())
{
	ObjectManager::Instance().SetCollisionManager(collision_manager.get());

	collision_experiment = std::make_unique<CollisionExperiment>(collision_manager.get());

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

	object_editor = std::make_unique<ObjectEditor>();
	object_editor->Initialize();
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
	object_editor.reset();
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

	//パス間で共有するライト空間変換行列の計算
	DirectX::XMFLOAT4X4 light_view_projection_matrix{};
	DirectX::XMFLOAT4 light_dir_vector{};
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


	//パイプラインのハザードを解消するためのテクスチャ解除処理
	if (shadow_fb)
	{
		ID3D11ShaderResourceView* null_srv_list[] = { nullptr };
		const UINT k_shader_shadow_srv_slot = 10;
		context->PSSetShaderResources(k_shader_shadow_srv_slot, 1, null_srv_list);
	}

	//シャドウマップ（深度バッファ）生成パス
	if (shadow_fb && camera && light && object_manager)
	{
		//シャドウマップバッファのクリアと有効化
		shadow_fb->clear(context, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f);
		shadow_fb->activate(context);

		//ライト空間用のシーン定数バッファの構築・更新
		scene_constants light_scene_constants{};
		light_scene_constants.view_projection = light_view_projection_matrix;
		light_scene_constants.light_view_projection = light_view_projection_matrix;
		light_scene_constants.light_direction = light_dir_vector;
		light_scene_constants.camera_position = camera->GetPosition();
		light_scene_constants.light_color = { 1.0f, 1.0f, 1.0f, 1.0f };
		light_scene_constants.ambient_color = { 1.0f, 1.0f, 1.0f, 1.0f };
		Graphics::Instance().UpdateSceneConstantBuffer(light_scene_constants);

		//深度値のみを描き込むステート群のバインド
		context->OMSetDepthStencilState(states->GetDepthStenceilState(1).Get(), 1);
		context->RSSetState(states->GetRasterizerState(2).Get());

		//パス1でのピクセルシェーダー警告を抑制するためのサンプラーバインド
		ID3D11SamplerState* shadow_sampler = Graphics::Instance().GetShadowSamplerState();
		const UINT k_shader_shadow_sampler_slot = 10;
		context->PSSetSamplers(k_shader_shadow_sampler_slot, 1, &shadow_sampler);

		//ライト視点でのシーンオブジェクト描画
		object_manager->Render(context);
		shadow_fb->deactivate(context);
	}

	//通常カメラからの本描画パス
	context->OMSetDepthStencilState(states->GetDepthStenceilState(1).Get(), 1);
	context->RSSetState(states->GetRasterizerState(2).Get());

	ID3D11SamplerState* sampler_p0 = states->GetSamplerState(0).Get(); //POINTサンプラー
	ID3D11SamplerState* sampler_p1 = states->GetSamplerState(1).Get(); //LINEARサンプラー
	ID3D11SamplerState* sampler_p2 = states->GetSamplerState(2).Get(); //ANISOTROPICサンプラー

	//各スロットへ1つずつバインドします
	context->PSSetSamplers(0, 1, &sampler_p0);
	context->PSSetSamplers(1, 1, &sampler_p1);
	context->PSSetSamplers(2, 1, &sampler_p2);

	//シャドウマップ用テクスチャおよびサンプラーを専用スロットへバインド
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

	if (shape_renderer && camera)
	{
		//平行光源の方向を可視化する球を描画
		const float k_light_debug_distance = 5.0f;
		DirectX::XMFLOAT4 light_dir = light->GetDirection();
		DirectX::XMFLOAT3 light_pos = {
			-light_dir.x * k_light_debug_distance,
			-light_dir.y * k_light_debug_distance,
			-light_dir.z * k_light_debug_distance
		};
		shape_renderer->DrawSphere(light_pos, 0.5f, { 1.0f, 1.0f, 0.0f, 1.0f }, ShapeDrawMode::Solid);


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

	if (skybox)
	{
		skybox->Render(context);
	}


#ifdef USE_IMGUI
	RenderGui();
	if (object_editor)
	{
		object_editor->RenderUi();
	}

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
		//object_manager->RenderGui();
		object_manager->RenderDebug(shape_renderer.get());
	}

	if (ImGui::Begin("Game Debug"))
	{
		bool check_paused = SceneManager::Instance().IsPaused();
		if (ImGui::Checkbox("Game Pause (F5)", &check_paused))
		{
			SceneManager::Instance().SetPauseState(check_paused);
		}
		if (camera)
		{
			camera->RenderGui();
		}

		//シャドウマップビューア項目の描画
		if (ImGui::CollapsingHeader("Shadow Map Viewer", ImGuiDockNodeFlags_None))
		{
			framebuffer* shadow_fb = Graphics::Instance().GetShadowFramebuffer();
			if (shadow_fb)
			{
				ID3D11ShaderResourceView* shadow_srv = shadow_fb->shader_resource_views[1].Get();
				ImGui::DragFloat("ShadowSize", &k_shadow_area_size, 0.1f);
				if (shadow_srv) 
				{
					static const float k_shadow_viewer_width = 256.0f;
					static const float k_shadow_viewer_height = 256.0f;

					ImGui::Text("Texture SRV Slot 10 (Format: R24_UNORM_X8)");
					ImGui::Image( 
						reinterpret_cast<ImTextureID>(shadow_srv),
						ImVec2(k_shadow_viewer_width, k_shadow_viewer_height)
					);
				}
				else 
				{
					ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Shadow SRV is Null.");
				}
			}
		}

		if (ImGui::CollapsingHeader("Shape Generator", ImGuiDockNodeFlags_None))
		{
			ImGui::RadioButton("Wireframe", &current_debug_draw_mode, 0);
			ImGui::RadioButton("Solid", &current_debug_draw_mode, 1);
			ImGui::RadioButton("Solid & Wireframe", &current_debug_draw_mode, 2);
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

		if (collision_experiment)
		{
			collision_experiment->RenderGui();
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
