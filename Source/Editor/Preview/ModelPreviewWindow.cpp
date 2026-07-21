#include "ModelPreviewWindow.h"
#include "Engine\Graphics\framebuffer.h"
#include "Engine\Graphics\Graphics.h"
#include "Engine\Graphics\Model.h"
#include "Engine\Graphics\Shaders\SkyBox.h"
#include "Engine\Camera\FreeCamera.h"
#include "Engine\Graphics\ShapeRenderer.h"
#include "Editor\EditorMediator.h"

#include <windows.h>
#include <imgui.h>

static constexpr uint32_t preview_buffer_width = 1280;   //プレビュー用レンダーテクスチャの解像度（幅）
static constexpr uint32_t preview_buffer_height = 720;   //プレビュー用レンダーテクスチャの解像度（高さ）
static constexpr float default_clear_color_r = 0.15f;    //プレビュー背景のクリア色(R)
static constexpr float default_clear_color_g = 0.15f;    //プレビュー背景のクリア色(G)
static constexpr float default_clear_color_b = 0.15f;    //プレビュー背景のクリア色(B)
static constexpr float default_clear_color_a = 1.0f;     //プレビュー背景のクリア色(A)

//コンストラクタ
ModelPreviewWindow::ModelPreviewWindow()
{

}

//デストラクタ
ModelPreviewWindow::~ModelPreviewWindow() = default;

//初期化処理
void ModelPreviewWindow::Initialize()
{
	ID3D11Device* device = Graphics::Instance().GetDevice();

	frame_buffer = std::make_unique<framebuffer>(
		device,
		preview_buffer_width,
		preview_buffer_height
	);

	camera = std::make_unique<FreeCamera>();
	camera->Initialize();

	shape_renderer = std::make_unique<ShapeRenderer>(device);

	skybox = std::make_unique<SkyBox>();
	if (skybox)
	{
		skybox->Initialize(
			device,
			L"Data/Sprite/SkyTexture/skybox.dds",			// 背景用キューブマップ
			L"Data/Sprite/SkyTexture/diffuse_iem.dds",		// IBL拡散反射テクスチャ
			L"Data/Sprite/SkyTexture/specular_pmrem.dds",	// IBL鏡面反射テクスチャ
			L"Data/Sprite/SkyTexture/lut_ggx.dds"			// IBLルックアップテーブル
		);
	}

	EditorMediator::Instance().RegisterModelPreviewWindow(this);
}

//更新処理
void ModelPreviewWindow::Update(float elapsed_time)
{
	if (is_viewport_active)
	{
		camera->Update(elapsed_time);
	}

	if (model)
	{
		if (is_playing)
		{
			float adusted_time = elapsed_time * animation_speed;
			model->Update(adusted_time);
		}
	}
}

//描画処理
void ModelPreviewWindow::Render(ID3D11DeviceContext* immediate_context)
{
	//専用バッファをクリアし、描画先に設定
	frame_buffer->clear(immediate_context, default_clear_color_r, default_clear_color_g, default_clear_color_b, default_clear_color_a);
	frame_buffer->activate(immediate_context);

	//描画ステートの明示的セット
	PipelineStates* states = Graphics::Instance().GetPipelineStates();
	immediate_context->OMSetDepthStencilState(states->GetDepthStenceilState(1).Get(), 1);
	immediate_context->RSSetState(states->GetRasterizerState(0).Get());
	ID3D11SamplerState* sampler_p0 = states->GetSamplerState(0).Get();
	ID3D11SamplerState* sampler_p1 = states->GetSamplerState(1).Get();
	ID3D11SamplerState* sampler_p2 = states->GetSamplerState(2).Get();

	immediate_context->PSSetSamplers(0, 1, &sampler_p0);
	immediate_context->PSSetSamplers(1, 1, &sampler_p1);
	immediate_context->PSSetSamplers(2, 1, &sampler_p2);


	//プレビューカメラのビュー・射影行列をシーン定数バッファに適用して光源をセット
	scene_constants constants = {};
	constants.view_projection = camera->GetViewProjectionMatrix();
	constants.camera_position = camera->GetPosition();
	constants.light_direction = { -0.5f, -1.0f, 0.5f, 0.0f }; 
	constants.light_color = { 1.2f, 1.2f, 1.2f, 1.0f };       
	constants.ambient_color = { 1.0f, 1.0f, 1.0f, 1.0f };
	constants.light_view_projection = constants.view_projection;

	Graphics::Instance().UpdateSceneConstantBuffer(constants);

	skybox->BindIblTextures(immediate_context);

	//スケール、回転(度数からラジアンに変換)、位置のワールド変換行列を合成
	if (model)
	{
		DirectX::XMMATRIX scaling = DirectX::XMMatrixScaling(model_scale.x, model_scale.y, model_scale.z);
		DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationRollPitchYaw(
			DirectX::XMConvertToRadians(model_rotation.x),
			DirectX::XMConvertToRadians(model_rotation.y),
			DirectX::XMConvertToRadians(model_rotation.z)
		);
		DirectX::XMMATRIX translation = DirectX::XMMatrixTranslation(model_position.x, model_position.y, model_position.z);
		DirectX::XMMATRIX world = scaling * rotation * translation;
		DirectX::XMFLOAT4X4 world_matrix;
		DirectX::XMStoreFloat4x4(&world_matrix, world);
		model->Render(immediate_context, world_matrix);
	}

	if (show_grid && shape_renderer)
	{
		static bool is_logged = false;
		if (!is_logged)
		{
			OutputDebugStringA("[ModelPreview] Grid rendering has been initialized.\n");
			is_logged = true;
		}

		const DirectX::XMFLOAT3 grid_center = { 0.0f, 0.0f, 0.0f };
		shape_renderer->DrawGrid(grid_center, grid_size, grid_divisions, grid_color);
		shape_renderer->Render(immediate_context, camera->GetView(), camera->GetProjection());
	}

	skybox->Render(immediate_context);
	frame_buffer->deactivate(immediate_context);
}

//ImGui描画
void ModelPreviewWindow::RenderGui()
{
	//ユーザーが自由に配置できるよう、初期ウィンドウサイズを指定してドッキング可能なウィンドウを開始
	ImGui::SetNextWindowSize(ImVec2(750.0f, 450.0f), ImGuiCond_FirstUseEver);
	if (ImGui::Begin(u8"モデルビュー", nullptr, ImGuiWindowFlags_NoScrollbar))
	{
		float avail_width = ImGui::GetContentRegionAvail().x;
		float avail_height = ImGui::GetContentRegionAvail().y;

		ID3D11ShaderResourceView* srv = frame_buffer->shader_resource_views[0].Get();
		if (srv)
		{
			ImGui::Image(
				reinterpret_cast<ImTextureID>(srv),
				ImVec2(avail_width, avail_height)
			);
			if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
			{
				is_viewport_active = true;
			}
			if (!ImGui::IsMouseDown(ImGuiMouseButton_Right))
			{
				is_viewport_active = false;
			}
		}
		else
		{
			is_viewport_active = false;
		}
	}
	ImGui::End();

	DrawControlPanel();
}

//外部からモデル読み込み
void ModelPreviewWindow::LoadModel(const std::string& file_path)
{
	if (file_path.empty())return;

	ID3D11Device* device = Graphics::Instance().GetDevice();

	//前のモデルを破棄して新しいモデルをロード
	model = std::make_unique<Model>(device, file_path);
	current_model_path = file_path;
	animation_names = model->GetAnimationNames();
	selected_animation_index = -1;

	if (!animation_names.empty())
	{
		selected_animation_index = 0;
		model->PlayAnimation(animation_names[0], is_loop);
	}
	else
	{
		selected_animation_index = -1;
	}

}

//再生速度設定
void ModelPreviewWindow::SetAnimationSpeed(float speed)
{
	animation_speed = speed;
}

//アニメーションフラグ設定
void ModelPreviewWindow::SetPlaying(bool playing)
{
	is_playing = playing;
}

//アニメーション時間設定
void ModelPreviewWindow::SetAnimationTime(float time)
{
	if (model)model->SetAnimationTime(time);
}

//現在の再生経過時間を取得
float  ModelPreviewWindow::GetAnimationCurrentTime() const
{
	if (model)return model->GetAnimationTime();
}

//アニメーションの総時間を取得
float ModelPreviewWindow::GetAnimationDuration() const
{
	if (model)return model->GetAnimationDuration();
}

//モデル名取得
std::string ModelPreviewWindow::GetModelName()
{
	return current_model_path;
}

//アニメーション名を取得
std::string ModelPreviewWindow::GetAnimationName()
{
	//インデックスが安全な範囲内にあるかチェック
	if (selected_animation_index >= 0 && static_cast<size_t>(selected_animation_index) < animation_names.size())
	{
		return animation_names[selected_animation_index];
	}

	return "";
}

//UIコントロール描画
void ModelPreviewWindow::DrawControlPanel()
{
	ImGui::SetNextWindowSize(ImVec2(280.0f, 450.0f), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin(u8"モデルの詳細"))
	{
		ImGui::End();
		return;
	}

	ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), u8"モデルの詳細");
	ImGui::Separator();
	ImGui::Spacing();

	if (!current_model_path.empty())
	{
		ImGui::TextWrapped(u8"ファイル: %s", current_model_path.c_str());
		ImGui::Spacing();

		//拡大・回転パラメータをリアルタイム編集できるようにImGuiで描画
		ImGui::Text(u8"トランスフォーム");
		ImGui::DragFloat3(u8"位置", &model_position.x, 0.05f, -100.0f, 100.0f);
		ImGui::DragFloat3(u8"拡大率", &model_scale.x, 0.01f, 0.01f, 10.0f);
		ImGui::DragFloat3(u8"角度", &model_rotation.x, 0.5f, -360.0f, 360.0f);

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), u8"グリッド設定");
		ImGui::Checkbox(u8"グリッドを表示", &show_grid);
		if (show_grid)
		{
			ImGui::DragFloat(u8"グリッドサイズ", &grid_size, 0.5f, 1.0f, 100.0f);
			ImGui::DragInt(u8"分割数", &grid_divisions, 1, 1, 100);
			ImGui::ColorEdit4(u8"線の色", &grid_color.x);
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		//モーションアニメーションコントロール
		ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), u8"アニメーション");
		if (!animation_names.empty())
		{
			ImGui::Checkbox(u8"ループ再生", &is_loop);

			const char* current_anim_label = "Stop/None";
			if (selected_animation_index >= 0 && selected_animation_index < static_cast<int>(animation_names.size()))
			{
				current_anim_label = animation_names[selected_animation_index].c_str();
			}

			//モーションをリスト選択して動的にアニメーション再生
			if (ImGui::BeginCombo(u8"アニメーション選択", current_anim_label))
			{
				for (int i = 0; i < static_cast<int>(animation_names.size()); i++)
				{
					bool is_selected = (selected_animation_index == i);
					if (ImGui::Selectable(animation_names[i].c_str(), is_selected))
					{
						selected_animation_index = i;
						model->PlayAnimation(animation_names[i], is_loop);
					}
				}
				ImGui::EndCombo();
			}
		}
		else
		{
			ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), u8"モデルにアニメーションがありません");
		}
	}
	else
	{
		ImGui::TextWrapped(u8"モデルを選択してください");
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), u8"Gltf、Glbモデルをダブルクリックしてモデル読み込みをしてください");
	}
	ImGui::End();
}
