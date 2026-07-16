#include "ModelPreviewWindow.h"
#include "Engine\Graphics\framebuffer.h"
#include "Engine\Graphics\Graphics.h"
#include "Engine\Graphics\Model.h"
#include "Engine\Graphics\Shaders\SkyBox.h"
#include "Engine\Camera\FreeCamera.h"

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
}

//更新処理
void ModelPreviewWindow::Update(float elapsed_time)
{
	if (is_viewport_active)
	{
		camera->Update(elapsed_time);
	}

	if (model)model->Update(elapsed_time);
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
		DirectX::XMMATRIX world = scaling * rotation * translation;		DirectX::XMFLOAT4X4 world_matrix;
		DirectX::XMStoreFloat4x4(&world_matrix, world);
		model->Render(immediate_context, world_matrix);
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

		static constexpr float control_panel_width = 250.0f;
		float preview_width = avail_width - control_panel_width - 10.0f;
		if (preview_width < 100.0f)preview_width = 100.0f;

		//左半分: コントロールパラメータパネル
		ImGui::BeginChild(u8"コントロールパネル", ImVec2(control_panel_width, avail_height), true);
		DrawControlPanel();
		ImGui::EndChild();

		ImGui::SameLine();

		//右半分: 3Dビューポート
		ImGui::BeginChild(u8"プレビュービューポート", ImVec2(preview_width, avail_height), false);
		ID3D11ShaderResourceView* srv = frame_buffer->shader_resource_views[0].Get();
		if (srv)
		{
			ImGui::Image(
				reinterpret_cast<ImTextureID>(srv),
				ImVec2(preview_width, avail_height)
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
		ImGui::EndChild();
	}
	ImGui::End();
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
}

//UIコントロール描画
void ModelPreviewWindow::DrawControlPanel()
{
	ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), u8"モデルの詳細");
	ImGui::Separator();
	ImGui::Spacing();

	if (!current_model_path.empty())
	{
		ImGui::TextWrapped(u8"ファイル: %s", current_model_path.c_str());
		ImGui::Spacing();

		//拡大・回転パラメータをリアルタイム編集できるようにImGuiで描画
		ImGui::Text(u8"トランスフォーム");
		ImGui::DragFloat3(u8"位置(X/Y/Z)", &model_position.x, 0.05f, -100.0f, 100.0f);
		ImGui::DragFloat3(u8"拡大率", &model_scale.x, 0.01f, 0.01f, 10.0f);
		ImGui::DragFloat3(u8"角度", &model_rotation.x, 0.5f, -360.0f, 360.0f);

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
}
