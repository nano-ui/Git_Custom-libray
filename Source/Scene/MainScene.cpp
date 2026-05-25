#include "../Scene/MainScene.h"
#include "../Graphics/Graphics.h"
#include <imgui.h>
#include "../Common/misc.h"
#include "../Graphics/shader.h"
#include "../Shaders/LuminanceExtractionEffect.h"
#include "../Shaders/BlurEffect.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"



//コンストラクタ
MainScene::MainScene()
{
	Cubuposition = { 0.0f,-1.5f,0.0f };			//キューブの初期位置を設定します
	Cubuposcale = { 0.4f,0.4f,0.4f };			//キューブの初期スケールを設定します
	W_Cubuposition = { 3.0f,0.0f,0.0f };		//Wキューブの初期位置を設定します
	camera_positon = { 0.0f,0.0f,10.0f,1.0f };	//カメラの初期位置の設定
}

//デストラクタ
MainScene::~MainScene()
{
	Finalize();
}

//初期化
void MainScene::Initialize()
{
	auto device = Graphics::Instance().GetDevice();		//デバイスを取得します
	auto context = Graphics::Instance().GetContext();	//コンテキストを取得します

	// --------------------------------------------------
	// -- 各コンポーネントの生成と初期化 --
	// --------------------------------------------------
	camera_component = std::make_unique<Camera>();			//カメラコンポーネントを生成します
	light_component = std::make_unique<Light>();			//ライトコンポーネントを生成します
	shader_manager = std::make_unique<ShaderManager>();		//シェーダーマネージャーを生成します

	const int screen_width = 1280;							//画面幅を定数として定義します
	const int screen_height = 720;							//画面高さを定数として定義します
	shader_manager->Initalize(device, screen_width, screen_height);	//マネージャーの内部バッファを画面サイズで初期化します

	// --------------------------------------------------
	// -- ポストプロセスシェーダーの追加 --
	// --------------------------------------------------
	auto extract_effect = std::make_unique<LuminanceExtractionEffect>(1.0f);	//輝度抽出用のシェーダーを生成します
	extract_effect->Initialize(device);								//シェーダーを初期化します
	shader_manager->AddShader(std::move(extract_effect));			//マネージャーのリストにシェーダーを追加します

	float sigma = 3.0f;
	float intensity = 1.5f;
	auto blur_effect = std::make_unique<BlurEffect>(shader_manager->GetSceneSRV(), sigma, intensity);
	blur_effect->Initialize(device);								//シェーダーを初期化します
	shader_manager->AddShader(std::move(blur_effect));				//マネージャーのリストにシェーダーを追加します

	// --------------------------------------------------
	// -- モデルデータの読み込み --
	// --------------------------------------------------
	fbx_model = std::make_unique<Model>(device, "./resources/nico.fbx");								//統合ModelクラスでFBXを読み込みます
	fbx_model->PlayAnimation("NIC_Idle", true);															//FBXのアニメーションを再生します

	gltf_model = std::make_unique<Model>(device, "./glTF-Sample-Models-main/2.0/CesiumMan/glTF-Binary/CesiumMan.glb");	//統合ModelクラスでglTFを読み込みます
	gltf_model->PlayAnimation("anim_0", true);
}

//終了処理
void MainScene::Finalize()
{
}

//更新処理
void MainScene::Update(float elapsed_time)
{
//#ifdef USE_IMGUI
//	ImGui_ImplDX11_NewFrame();
//	ImGui_ImplWin32_NewFrame();
//	ImGui::NewFrame();
//#endif
//
//#ifdef USE_IMGUI
//	//ImGui::Begin("ImGUI");
//
//	//editInformation("skinned_meshs", cube);
//	//editInformation("geometric_primitives", w_cube);
//	//editInformation("camera", camera);
//
//	if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_None))
//	{
//		//位置
//		ImGui::SliderFloat2("Position", &position.x, 0, 1280);
//		ImGui::SliderFloat2("Size", &size.x, 0, 720);
//		//色
//		ImGui::SliderFloat4("Color", &render_color.x, 0, 1);
//		//角度
//		ImGui::SliderFloat("Angle", &angle, 0, 360);
//	}
//
//	if (ImGui::CollapsingHeader("Cube", ImGuiTreeNodeFlags_None))
//	{
//		ImGui::SliderFloat3("CubeScale", &cube.scale.x, 0, 10);
//		ImGui::SliderFloat3("CubeRotation", &cube.rotation.x, -DirectX::XM_PI, DirectX::XM_PI);
//		ImGui::SliderFloat3("CubePosion", &cube.position.x, -10, 10);
//		ImGui::SliderFloat("Head", &Long, 0, 300);
//		ImGui::SliderFloat("HeadRotation", &Rotation, 0, XM_PI);
//		ImGui::SliderFloat("Supp", &supplementation, 0.0f, 1.0f);
//		ImGui::SliderFloat("Brightness", &tb.brightness_threshold, 0.0f, 1.0f);
//		ImGui::SliderFloat("bloom_intensity", &bloomParams.bloom_intensity, 0.0f, 1.0f);
//		ImGui::SliderFloat("gaussian_sigma", &bloomParams.gaussian_sigme, 0.0f, 1.0f);
//	}
//
//	if (ImGui::CollapsingHeader("W_Cube", ImGuiTreeNodeFlags_None))
//	{
//		ImGui::SliderFloat3("W_CubeScale", &W_Cubuposcale.x, 0, 10);
//		ImGui::SliderFloat3("W_CubeRotation", &W_Cuburotation.x, 0, 0.01745f * 360);
//		ImGui::SliderFloat3("W_CubePosion", &W_Cubuposition.x, 10, 10);
//	}
//	if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_None))
//	{
//		ImGui::SliderFloat3("Camera", &camera.position.x, -100, 100);
//	}
//	if (ImGui::CollapsingHeader("light", ImGuiTreeNodeFlags_None))
//	{
//		ImGui::SliderFloat4("light_direction", &data.light_direction.x, -1, 1);
//	}
//
//
//	//ImGui::End();
//#endif
	fbx_model->Update(elapsed_time);
	gltf_model->Update(elapsed_time);
}

//描画処理
void MainScene::Render(float elapsed_time)
{
	auto context = Graphics::Instance().GetContext();				//コンテキストを取得します
	auto states = Graphics::Instance().GetPipelineStates();			//パイプラインステートを取得します

	Graphics::Instance().BeginFrame(0.2f, 0.2f, 0.2f, 1.0f);		//フレームの描画を開始し、背景色をクリアします

	// --------------------------------------------------
	// -- カメラとライトの情報を更新 --
	// --------------------------------------------------
	D3D11_VIEWPORT viewport;										//ビューポート構造体を宣言します
	UINT num_viewports{ 1 };										//取得するビューポートの数を設定します
	context->RSGetViewports(&num_viewports, &viewport);				//現在のビューポート情報を取得します

	camera_component->CalculateViewProjection(viewport.Width / viewport.Height);	//画面のアスペクト比を元にカメラ行列を計算します

	data.view_projection = camera_component->GetViewProjection();		//計算した行列をシーン定数データに代入します
	data.light_direction = light_component->GetDirection();				//ライトの方向をシーン定数データに代入します
	DirectX::XMFLOAT3 pos = camera_component->GetPosition();			//カメラの現在の座標を取得します
	data.camera_position = { pos.x, pos.y, pos.z, 1.0f };				//カメラの座標をシーン定数データに代入します

	// --------------------------------------------------
	// -- シェーダーマネージャーによる描画開始--
	// --------------------------------------------------
	shader_manager->BeginRender(context);							//描画先をマネージャー内の初期バッファに切り替えます

	// --------------------------------------------------
	// -- モデルの描画手順 --
	// --------------------------------------------------
	DirectX::XMMATRIX S{ DirectX::XMMatrixScaling(1.0f, 1.0f, 1.0f) };					//スケール行列を作成します
	DirectX::XMMATRIX R{ DirectX::XMMatrixRotationRollPitchYaw(0.0f, 0.0f, 0.0f) };		//回転行列を作成します
	DirectX::XMMATRIX T{ DirectX::XMMatrixTranslation(0.0f, 0.0f, 0.0f) };				//移動行列を作成します
	DirectX::XMFLOAT4X4 world;															//ワールド行列格納用の変数を宣言します
	DirectX::XMStoreFloat4x4(&world, S * R * T);										//各行列を乗算してワールド行列を計算します

	if (fbx_model) fbx_model->Render(context, world);				//FBXモデルを描画します
	if (gltf_model) gltf_model->Render(context, world);				//glTFモデルを描画します

	// --------------------------------------------------
	// -- シェーダーマネージャーによる最終描画 --
	// --------------------------------------------------
	auto back_buffer = Graphics::Instance().GetDirectXDevice()->GetRenderTargetView().Get();	//バックバッファを取得します
	shader_manager->EndRender(context, back_buffer);											//全てのエフェクトを合成して最終画面に出力します

	Graphics::Instance().EndFrame();								//フレームの描画を終了します
}
