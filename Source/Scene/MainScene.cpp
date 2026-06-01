#include "../Scene/MainScene.h"
#include "../Graphics/Graphics.h"
#include <imgui.h>
#include "../Common/misc.h"
#include "../Graphics/shader.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

//マジックナンバーを避けるための定数定義群
static constexpr UINT threshold_buffer_slot = 0;
static constexpr UINT main_framebuffer_idx = 0;
static constexpr UINT luminance_framebuffer_idx = 1;
static constexpr UINT srv_count_bloom = 2;
static constexpr float clear_color_r = 0.2f;
static constexpr float clear_color_g = 0.2f;
static constexpr float clear_color_b = 0.2f;
static constexpr float clear_color_a = 1.0f;

//コンストラクタ
MainScene::MainScene()
{
	cube.position = { 0.0f,-1.5f,0.0f };
	cube.scale = { 0.4f,0.4f,0.4f };
	w_cube.position = { 3,0,0 };
	camera.position = { 0.0f,0.0f,10.0f };
	tb.brightness_threshold = 1.0f;
	bloom_data.bloom_intensity = 1.0f;
	bloom_data.gaussian_sigma = 1.0f;
}

//デストラクタ
MainScene::~MainScene()
{
	Finalize();
}

//初期化
void MainScene::Initialize()
{
	auto device = Graphics::Instance().GetDevice();
	auto context = Graphics::Instance().GetContext();

	HRESULT hr{ S_OK };

	//シーン定数バッファオブジェクトを生成
	D3D11_BUFFER_DESC buffer_desc{};
	buffer_desc.ByteWidth = sizeof(scene_constants);
	buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	buffer_desc.CPUAccessFlags = 0;
	buffer_desc.MiscFlags = 0;
	buffer_desc.StructureByteStride = 0;
	hr = device->CreateBuffer(
		&buffer_desc,
		nullptr,
		constnt_buffer[0].GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	//定数バッファ作成
	D3D11_BUFFER_DESC cbd = {};
	cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbd.Usage = D3D11_USAGE_DYNAMIC;
	cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbd.ByteWidth = sizeof(ThresholdBuffer);

	device->CreateBuffer(&cbd, nullptr, &thresholdBuffer);

	create_ps_from_cso(device, "luminance_extraction_ps.cso", pixel_shaders[0].GetAddressOf());

	geometric_primitives[0] =
		std::make_unique<geometric_primitive>(device);

	geometric_primitives[1] =
		std::make_unique<geometric_primitive>(device);

	framebuffers[1] = std::make_unique<framebuffer>(device, 1280 / 2, 720 / 2);

	framebuffers[2] = std::make_unique<framebuffer>(device, 1280 / 2, 720 / 2);


	blur_shader = std::make_unique<BlurShader>();
	if (!blur_shader->Initialize())
	{
		_ASSERT_EXPR(false, L"Failed to initialize BlurShader");
	}

	luminance_shader = std::make_unique<LuminanceExtractionShader>();
	if (!luminance_shader->Initialize())
	{
		_ASSERT_EXPR(false, L"Failed to initialize LuminanceShader");
	}

	//上記までがDirectX11の初期設定処理

	//下記からはゲームで必要なオブジェクトの初期化

	sprites[0] = std::make_unique<sprite>
		(device, L"./resources/cyberpunk.jpg");
	sprites[1] = std::make_unique<sprite>
		(device, L"./resources/player-sprites.png");
	sprites[2] = std::make_unique<sprite>
		(device, L"./resources/fonts/font0.png");
	//sprute_batches[0] = std::make_unique<sprite_batch>
	//	(device.Get(), L"./resources/player-sprites.png", 2048);
	sprute_batches[0] = std::make_unique<sprite_batch>
		(device, L"./resources/screenshot.jpg", 1);
	static_meshes[0] = std::make_unique<static_mesh>
		(device, L"./resources/Rock/Rock.obj");

	//skinned_meshes[0] = std::make_unique<skinned_mesh>
	//	(device, "./resources/nico.fbx", true);

	//a

	fbx_model = std::make_unique<Model>(device, "./resources/nico.fbx");
	fbx_model->PlayAnimation("NIC_Idle", true);

	//gltf_models[0] = std::make_unique<GltfModel>(device,
	//	"./glTF-Sample-Models-main/2.0/CesiumMan/glTF-Binary/CesiumMan.glb");

	//gltf_model_data = std::make_shared<GltfModelData>(device,"./glTF-Sample-Models-main/2.0/CesiumMan/glTF-Binary/CesiumMan.glb");
	gltf_model = std::make_unique<Model>(device, "./glTF-Sample-Models-main/2.0/CesiumMan/glTF-Binary/CesiumMan.glb");
	gltf_model->PlayAnimation("anim_0", true);


	//skinned_meshes[0] = make_unique<skinned_mesh>(device.Get(), "./resources/AimTest/MNK_Mesh.fbx");
	//skinned_meshes[0]->append_animations("./resources/AimTest/Aim_Space.fbx", 0);

	framebuffers[0] = std::make_unique<framebuffer>(device, 1280, 720);

	bit_block_transfer = std::make_unique<fullscreen_quad>(device);
}

//終了処理
void MainScene::Finalize()
{
	//ゲームリソースの開放
	for (auto& sprite : sprites) sprite.reset();
	for (auto& batch : sprute_batches) batch.reset();
	for (auto& mesh : static_meshes) mesh.reset();
	for (auto& mesh : skinned_meshes)mesh.reset();
	for (auto& fb : framebuffers)fb.reset();

	blur_shader.reset();

	for (auto& prim : geometric_primitives)prim.reset();

	if (bit_block_transfer)bit_block_transfer.reset();

	//COMポインタも明示的に開放
	for (auto& cb : constnt_buffer) cb.Reset();
	for (auto& ps : pixel_shaders)ps.Reset();
	thresholdBuffer.Reset();
	bloomParamBuffer.Reset();
	blurDirectionBuffer.Reset();
}

//更新処理
void MainScene::Update(float elapsed_time)
{
#ifdef USE_IMGUI
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
#endif

	fbx_model->Update(elapsed_time);
	gltf_model->Update(elapsed_time);
}

//描画処理
void MainScene::Render(float elapsed_time)
{
	auto context = Graphics::Instance().GetContext();
	Graphics::Instance().BeginFrame(clear_color_r, clear_color_g, clear_color_b, clear_color_a);

	// 前のフレームに残ったサンプラーやテクスチャのバインド情報をリセットして高速化を維持します
	ID3D11ShaderResourceView* null_srvs[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT]{};
	context->VSSetShaderResources(0, _countof(null_srvs), null_srvs);
	context->PSSetShaderResources(0, _countof(null_srvs), null_srvs);

	// -- 大まかな手順の始まり --
	// 2. シーン全体の共通行列・ポストプロセス群の段階的処理を実行
	UpdateSceneConstants(); // 定数バッファの計算と更新処理
	RenderSceneObjects();   // メインバッファへの各種オブジェクトの書き込み処理
	RenderPostProcesses();  // 輝度抽出とコンポーネントによるブルーム合成処理

	// -- 大まかな手順の始まり --
	// 3. 開発時専用のデバッグUI表示処理
#ifdef USE_IMGUI
	RenderGui(); // 登録された調整用のパネル変数を構築
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
#endif

	// フロントバッファとバックバッファのフリップを行い画面を表示します
	Graphics::Instance().EndFrame();
}

//ImGuiデバッグ描画
void MainScene::RenderGui()
{
#ifdef USE_IMGUI
	if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_None))
	{
		//位置
		ImGui::SliderFloat2("Position", &position.x, 0, 1280);
		ImGui::SliderFloat2("Size", &size.x, 0, 720);
		//色
		ImGui::SliderFloat4("Color", &render_color.x, 0, 1);
		//角度
		ImGui::SliderFloat("Angle", &angle, 0, 360);
	}

	if (ImGui::CollapsingHeader("Cube", ImGuiTreeNodeFlags_None))
	{
		ImGui::SliderFloat3("CubeScale", &cube.scale.x, 0, 10);
		ImGui::SliderFloat3("CubeRotation", &cube.rotation.x, -DirectX::XM_PI, DirectX::XM_PI);
		ImGui::SliderFloat3("CubePosion", &cube.position.x, -10, 10);
		ImGui::SliderFloat("Head", &Long, 0, 300);
		ImGui::SliderFloat("HeadRotation", &Rotation, 0, XM_PI);
		ImGui::SliderFloat("Supp", &supplementation, 0.0f, 1.0f);
		ImGui::SliderFloat("Brightness", &tb.brightness_threshold, 0.0f, 1.0f);
		ImGui::SliderFloat("bloom_intensity", &bloom_data.bloom_intensity, 0.0f, 1.0f);
		ImGui::SliderFloat("gaussian_sigma", &bloom_data.gaussian_sigma, 0.0f, 1.0f);
	}

	if (ImGui::CollapsingHeader("W_Cube", ImGuiTreeNodeFlags_None))
	{
		ImGui::SliderFloat3("W_CubeScale", &W_Cubuposcale.x, 0, 10);
		ImGui::SliderFloat3("W_CubeRotation", &W_Cuburotation.x, 0, 0.01745f * 360);
		ImGui::SliderFloat3("W_CubePosion", &W_Cubuposition.x, 10, 10);
	}
	if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_None))
	{
		ImGui::SliderFloat3("Camera", &camera.position.x, -100, 100);
	}
	if (ImGui::CollapsingHeader("light", ImGuiTreeNodeFlags_None))
	{
		ImGui::SliderFloat4("light_direction", &data.light_direction.x, -1, 1);
	}
#endif // DEBUG
}

//カメラとライトの行列計算を行い、シーン共通定数バッファをGPUへ転送
void MainScene::UpdateSceneConstants()
{
	auto context = Graphics::Instance().GetContext();
	auto states = Graphics::Instance().GetPipelineStates();

	// ビューポートの情報を取得してアスペクト比を動的に求めます
	D3D11_VIEWPORT viewport;
	UINT num_viewports{ 1 };
	context->RSGetViewports(&num_viewports, &viewport);

	// カメラの視線行列(ビューマトリクス)を左手系で算出します
	DirectX::XMVECTOR eye{ DirectX::XMVectorSet(camera.position.x, camera.position.y, camera.position.z, 1.0f) };
	DirectX::XMVECTOR focus{ DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f) };
	DirectX::XMVECTOR up{ DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f) };
	DirectX::XMMATRIX V{ DirectX::XMMatrixLookAtLH(eye, focus, up) };

	// 透視投影行列(プロジェクションマトリクス)を算出します
	float aspect_ratio{ viewport.Width / viewport.Height };
	DirectX::XMMATRIX P{ DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(30), aspect_ratio, 0.1f, 100.0f) };

	// 算出した行列を合成し、ライト等の基本パラメータとともに構造体へ保存します
	DirectX::XMStoreFloat4x4(&data.view_projection, V * P);
	data.light_direction = { 0.0f, 0.0f, -1.0f, 0.0f };
	data.camera_position = { camera.position.x, camera.position.y, camera.position.z, 1.0f };

	// 共通の定数リソースバッファを高速上書き更新して各シェーダーステージにセットします
	context->UpdateSubresource(constnt_buffer[0].Get(), 0, 0, &data, 0, 0);
	context->VSSetConstantBuffers(1, 1, constnt_buffer[0].GetAddressOf());
	context->PSSetConstantBuffers(1, 1, constnt_buffer[0].GetAddressOf());

	// 基本的なサンプラーステートを描画に先んじて共通バインドします
	context->PSSetSamplers(0, 1, states->GetSamplerState(0).GetAddressOf());
	context->PSSetSamplers(1, 1, states->GetSamplerState(1).GetAddressOf());
	context->PSSetSamplers(2, 1, states->GetSamplerState(2).GetAddressOf());
}

//シーン内の背景、3Dモデル、幾何プリミティブをメインバッファに描画
void MainScene::RenderSceneObjects()
{
	auto context = Graphics::Instance().GetContext();
	auto states = Graphics::Instance().GetPipelineStates();

	// 描画ターゲットをメインのシーン用フレームバッファに指定しクリアします
	framebuffers[main_framebuffer_idx]->clear(context);
	framebuffers[main_framebuffer_idx]->activate(context);

	// ------------------------------------------------------------------------------
	// -- 背景スプライトの描画 --
	// ------------------------------------------------------------------------------
	context->OMSetDepthStencilState(states->GetDepthStenceilState(0).Get(), 1); // 深度テストOFF
	context->RSSetState(states->GetRasterizerState(2).Get());                  // カリングなし

	sprute_batches[0]->begin(context);
	sprute_batches[0]->render(context, 0, 0, 1280, 720);
	sprute_batches[0]->end(context);

	// ------------------------------------------------------------------------------
	// -- 3Dオブジェクトの描画設定 --
	// ------------------------------------------------------------------------------
	context->OMSetDepthStencilState(states->GetDepthStenceilState(1).Get(), 1); // 深度テストON
	context->RSSetState(states->GetRasterizerState(0).Get());                  // 通常カリングあり

	const DirectX::XMFLOAT4X4 coordinate_system_transforms[]{
		{ -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 },
		{ 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 },
		{ -1, 0, 0, 0, 0, 0, -1, 0, 0, 1, 0, 0, 0, 0, 0, 1 },
		{ 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1 },
	};
	const float scale_factor = 0.05f;
	DirectX::XMMATRIX C{ DirectX::XMLoadFloat4x4(&coordinate_system_transforms[0]) * DirectX::XMMatrixScaling(scale_factor, scale_factor, scale_factor) };

	// 幾何学プリミティブの行列計算と描画を実行します
	DirectX::XMMATRIX S{ DirectX::XMMatrixScaling(w_cube.scale.x, w_cube.scale.y, w_cube.scale.z) };
	DirectX::XMMATRIX R{ DirectX::XMMatrixRotationRollPitchYaw(w_cube.rotation.x, w_cube.rotation.y, w_cube.rotation.z) };
	DirectX::XMMATRIX T{ DirectX::XMMatrixTranslation(w_cube.position.x, w_cube.position.y, w_cube.position.z) };
	DirectX::XMFLOAT4X4 W_world;
	DirectX::XMStoreFloat4x4(&W_world, C * S * R * T);

	geometric_primitives[1]->render(context, W_world, { 0.5f, 0.8f, 0.2f, 1.0f });

	// ------------------------------------------------------------------------------
	// -- アニメーションFBXモデルの描画 --
	// ------------------------------------------------------------------------------
	if (fbx_model)
	{
		cube.scale.x = cube.scale.y = cube.scale.z = 1.0f;
		DirectX::XMMATRIX MS{ DirectX::XMMatrixScaling(cube.scale.x, cube.scale.y, cube.scale.z) };
		DirectX::XMMATRIX MR{ DirectX::XMMatrixRotationRollPitchYaw(cube.rotation.x, cube.rotation.y, cube.rotation.z) };
		DirectX::XMMATRIX MT{ DirectX::XMMatrixTranslation(cube.position.x, cube.position.y, cube.position.z) };
		DirectX::XMFLOAT4X4 world;
		DirectX::XMStoreFloat4x4(&world, MS * MR * MT);

		gltf_model->Render(context, world);
	}
}

//輝度抽出とBlurShaderコンポーネントによるポストプロセスエフェクトを実行
void MainScene::RenderPostProcesses()
{
	auto context = Graphics::Instance().GetContext();
	auto states = Graphics::Instance().GetPipelineStates();

	// ポストプロセス用に深度テストを無効、カリングをなしに統合設定して効率化します
	context->OMSetDepthStencilState(states->GetDepthStenceilState(0).Get(), 1);
	context->RSSetState(states->GetRasterizerState(2).Get());

	// ------------------------------------------------------------------------------
	// -- パス1: 輝度抽出処理 --
	// ------------------------------------------------------------------------------
	framebuffers[main_framebuffer_idx]->deactivate(context); // メインカラーバッファのバインド解除

	luminance_shader->SetThreshold(tb.brightness_threshold);
	luminance_shader->Apply();

	// 抽出結果を書き込む縮小バッファを有効化します
	framebuffers[1]->clear(context);
	framebuffers[1]->activate(context);

	// 輝度抽出ピクセルシェーダー(pixel_shaders[0])を用いて全画面描画を実行します
	bit_block_transfer->blit(context, framebuffers[0]->shader_resource_views[0].GetAddressOf(), 0, 1, luminance_shader->GetPixelShader());
	framebuffers[1]->deactivate(context);

	// ------------------------------------------------------------------------------
	// -- パス2: ブルーム（ブラー）合成処理 --
	// ------------------------------------------------------------------------------
	// テクスチャスロット t0 (シーン) と t1 (輝度抽出テクスチャ) をまとめて配列化します
	ID3D11ShaderResourceView* shader_resource_views[srv_count_bloom]{
		framebuffers[main_framebuffer_idx]->shader_resource_views[0].Get(),
		framebuffers[luminance_framebuffer_idx]->shader_resource_views[0].Get()
	};

	// コンポーネントに最新パラメータを設定してパイプラインへバインドを実行します
	blur_shader->SetBloomParams(bloom_data);
	blur_shader->Apply();

	bit_block_transfer->blit(context, shader_resource_views, 0, srv_count_bloom, blur_shader->GetPixelShader());
}
