#include "SceneGame.h"
#include "../GameObjects/ObjectManager.h"
#include "../GameObjects/Objects/Stage.h"
#include "../Graphics/Graphics.h"
#include "../Camera/Camera.h"
#include "../Camera/FreeCamera.h"
#include "../Light/Light.h"
#include "../Graphics/ShapeRenderer.h"
#include "../GameObjects/Characters/Player.h"

//コンストラクタ
SceneGame::SceneGame()
{
	object_manager = std::make_unique<ObjectManager>();
	Stage* stage = object_manager->Instantiate<Stage>();

	Player* player = object_manager->Instantiate<Player>();

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

	ID3D11Device* device = Graphics::Instance().GetDevice();
	shape_renderer = std::make_unique<ShapeRenderer>(device);
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
		Graphics::Instance().UpdateSceneConstantBuffer(constants);
	}

	if (object_manager)
	{
		object_manager->Render(context);
	}

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
		object_manager->RenderDebug();
	}

	if (ImGui::Begin("Game Debug"))
	{
		if (camera)
		{
			camera->RenderGui();
		}
		if (ImGui::CollapsingHeader("Shape Generator", ImGuiDockNodeFlags_None));
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
	}
	ImGui::End();
#endif // USE_IMGUI
}
