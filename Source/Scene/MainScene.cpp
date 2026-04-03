#include "../Scene/MainScene.h"
#include "../Graphics/Graphics.h"
#include <imgui.h>
#include "../Common/misc.h"
#include "../Graphics/shader.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

// コンストラクタ
MainScene::MainScene()
{
	cube.position = { 0.0f, -1.5f, 0.0f };
	cube.scale = { 0.4f, 0.4f, 0.4f };
	w_cube.position = { 3, 0, 0 };
	camera.position = { 0.0f, 0.0f, 10.0f };
	tb.brightness_threshold = 1.0f;

	// GLTFモデルの初期値
	gltf_position = { 0.0f, 0.0f, 0.0f };
	gltf_scale = { 0.05f, 0.05f, 0.05f };
	gltf_rotation = { 0.0f, 0.0f, 0.0f };
	DirectX::XMStoreFloat4x4(&gltf_model_transform, DirectX::XMMatrixIdentity());
}

// デストラクタ
MainScene::~MainScene()
{
	Finalize();
}

// 初期化処理
void MainScene::Initialize()
{
	auto device = Graphics::Instance().GetDevice();
	auto context = Graphics::Instance().GetContext();

	HRESULT hr{ S_OK };

	// シーン定数バッファオブジェクトを生成
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

	// 定数バッファ作成
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

	create_ps_from_cso(device, "blur_ps.cso", pixel_shaders[1].GetAddressOf());
	create_ps_from_cso(device, "blur_ps.cso", pixel_shaders[2].GetAddressOf());

	D3D11_BUFFER_DESC desc = {};
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	desc.ByteWidth = sizeof(BloomParams);
	desc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = &bloomParams;

	device->CreateBuffer(&desc, &initData, &bloomParamBuffer);

	// ここまでDirectX11の設定

	// ゲーム実行に必要なオブジェクトの初期化

	sprites[0] = std::make_unique<sprite>
		(device, L"./resources/cyberpunk.jpg");
	sprites[1] = std::make_unique<sprite>
		(device, L"./resources/player-sprites.png");
	sprites[2] = std::make_unique<sprite>
		(device, L"./resources/fonts/font0.png");

	sprute_batches[0] = std::make_unique<sprite_batch>
		(device, L"./resources/screenshot.jpg", 1);
	static_meshes[0] = std::make_unique<static_mesh>
		(device, L"./resources/Rock/Rock.obj");

	resource = std::make_shared<FbxSkinnedResource>(device);

	framebuffers[0] = std::make_unique<framebuffer>(device, 1280, 720);

	bit_block_transfer = std::make_unique<fullscreen_quad>(device);

	// GLTFモデルの初期化
	InitializeGltfModel();
	InitializeGltfShaders();

	OutputDebugStringA("MainScene::Initialize() completed\n");
}

// GLTFモデルの初期化
void MainScene::InitializeGltfModel()
{
	auto device = Graphics::Instance().GetDevice();

	if (!device)
	{
		OutputDebugStringA("ERROR: Device is not initialized\n");
		return;
	}

	// GLTFモデルを読み込む
	// パスはプロジェクトディレクトリの"Assets/Models/"に配置されていると想定
	const char* model_paths[] = {
		"resources/RPG-Character.glb",
	};

	for (const char* path : model_paths)
	{
		gltf_model = GlthStaticModel::LoadModel(device, path);
		if (gltf_model && !gltf_model->meshes.empty())
		{
			OutputDebugStringA("GLTF model loaded successfully from: ");
			OutputDebugStringA(path);
			OutputDebugStringA("\nMesh count: ");
			OutputDebugStringA(std::to_string(gltf_model->meshes.size()).c_str());
			OutputDebugStringA("\n");
			enable_gltf_rendering = true;
			return;
		}
	}

	OutputDebugStringA("WARNING: Failed to load GLTF model from all paths\n");
	enable_gltf_rendering = false;
}

//GLTFシェーダー初期化
void MainScene::InitializeGltfShaders()
{
	auto device = Graphics::Instance().GetDevice();

	HRESULT hr = S_OK;

	// 頂点シェーダー用の入力レイアウト定義
	D3D11_INPUT_ELEMENT_DESC input_element_descs[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TANGENT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	// 頂点シェーダー読み込み
	hr = create_vs_from_cso(
		device,
		"gltf_static_mesh_vsr.cso",
		gltf_vertex_shader.GetAddressOf(),
		gltf_input_layout.GetAddressOf(),
		input_element_descs,
		_countof(input_element_descs)
	);
	if (FAILED(hr))
	{
		OutputDebugStringA("ERROR: Failed to load GLTF vertex shader\n");
		return;
	}

	// ピクセルシェーダー読み込み
	hr = create_ps_from_cso(
		device,
		"gltf_static_mesh_ps.cso",
		gltf_pixel_shader.GetAddressOf()
	);
	if (FAILED(hr))
	{
		OutputDebugStringA("ERROR: Failed to load GLTF pixel shader\n");
		return;
	}

	// 定数バッファ作成
	D3D11_BUFFER_DESC cb_desc = {};
	cb_desc.ByteWidth = sizeof(GLTF_OBJECT_CONSTANT);
	cb_desc.Usage = D3D11_USAGE_DEFAULT;
	cb_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cb_desc.CPUAccessFlags = 0;

	hr = device->CreateBuffer(&cb_desc, nullptr, gltf_object_constant_buffer.GetAddressOf());
	if (FAILED(hr)) return;

	//マテリアル定数バッファ作成
	D3D11_BUFFER_DESC mat_cb_desc = {};
	mat_cb_desc.ByteWidth = (sizeof(GltfStaticMaterialData) + 15) & ~15;
	mat_cb_desc.Usage = D3D11_USAGE_DYNAMIC; // マテリアルごとに更新するためDYNAMIC
	mat_cb_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	mat_cb_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	hr = device->CreateBuffer(&mat_cb_desc, nullptr, gltf_material_constant_buffer.GetAddressOf());
	if (FAILED(hr))
	{
		OutputDebugStringA("ERROR: Failed to create GLTF constant buffer\n");
		return;
	}

	OutputDebugStringA("GLTF shaders initialized successfully\n");
}

// 終了処理
void MainScene::Finalize()
{
	// ゲーム内リソースの解放
	for (auto& sprite : sprites) sprite.reset();
	for (auto& batch : sprute_batches) batch.reset();
	for (auto& mesh : static_meshes) mesh.reset();
	for (auto& mesh : skinned_meshes) mesh.reset();
	for (auto& fb : framebuffers) fb.reset();

	for (auto& prim : geometric_primitives) prim.reset();

	if (bit_block_transfer) bit_block_transfer.reset();

	// GLTFシェーダーをクリア
	if (gltf_vertex_shader) gltf_vertex_shader.Reset();
	if (gltf_pixel_shader) gltf_pixel_shader.Reset();
	if (gltf_input_layout) gltf_input_layout.Reset();
	if (gltf_object_constant_buffer) gltf_object_constant_buffer.Reset();
	if (gltf_material_constant_buffer) gltf_material_constant_buffer.Reset();

	// GLTFモデル解放
	if (gltf_model)
	{
		gltf_model.reset();
	}

	// COMポインタを明示的に解放
	for (auto& cb : constnt_buffer) cb.Reset();
	for (auto& ps : pixel_shaders) ps.Reset();
	thresholdBuffer.Reset();
	bloomParamBuffer.Reset();
	blurDirectionBuffer.Reset();

	Graphics::Instance().Finalize();

	OutputDebugStringA("MainScene::Finalize() completed\n");
}

// 更新処理
void MainScene::Update(float elapsed_time)
{
#ifdef USE_IMGUI
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
#endif

#ifdef USE_IMGUI
	if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_None))
	{
		ImGui::SliderFloat2("Position", &position.x, 0, 1280);
		ImGui::SliderFloat2("Size", &size.x, 0, 720);
		ImGui::SliderFloat4("Color", &render_color.x, 0, 1);
		ImGui::SliderFloat("Angle", &angle, 0, 360);
	}

	if (ImGui::CollapsingHeader("Cube", ImGuiTreeNodeFlags_None))
	{
		ImGui::SliderFloat3("CubeScale", &cube.scale.x, 0, 10);
		ImGui::SliderFloat3("CubeRotation", &cube.rotation.x, 0, DirectX::XM_PI);
		ImGui::SliderFloat3("CubePosition", &cube.position.x, -10, 10);
		ImGui::SliderFloat("Head", &Long, 0, 300);
		ImGui::SliderFloat("HeadRotation", &Rotation, 0, XM_PI);
		ImGui::SliderFloat("Supp", &supplementation, 0.0f, 1.0f);
		ImGui::SliderFloat("Brightness", &tb.brightness_threshold, 0.0f, 1.0f);
		ImGui::SliderFloat("bloom_intensity", &bloomParams.bloom_intensity, 0.0f, 1.0f);
		ImGui::SliderFloat("gaussian_sigma", &bloomParams.gaussian_sigme, 0.0f, 1.0f);
	}

	if (ImGui::CollapsingHeader("W_Cube", ImGuiTreeNodeFlags_None))
	{
		ImGui::SliderFloat3("W_CubeScale", &W_Cubuposcale.x, 0, 10);
		ImGui::SliderFloat3("W_CubeRotation", &W_Cuburotation.x, 0, 0.01745f * 360);
		ImGui::SliderFloat3("W_CubePosition", &W_Cubuposition.x, -10, 10);
	}

	if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_None))
	{
		ImGui::SliderFloat3("Camera", &camera.position.x, -100, 100);
	}

	if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_None))
	{
		ImGui::SliderFloat4("light_direction", &data.light_direction.x, -1, 1);
	}

	// GLTFモデルのパラメータUI
	if (ImGui::CollapsingHeader("GLTF Model", ImGuiTreeNodeFlags_None))
	{
		ImGui::Checkbox("Enable GLTF Rendering", &enable_gltf_rendering);
		ImGui::SliderFloat3("GLTF Position", &gltf_position.x, -10, 10);
		ImGui::SliderFloat3("GLTF Scale", &gltf_scale.x, 0.001f, 5.0f);
		ImGui::SliderFloat3("GLTF Rotation", &gltf_rotation.x, 0, DirectX::XM_2PI);
		ImGui::SliderFloat("GLTF Rotation Speed", &gltf_rotation_speed, 0.0f, 5.0f);

		if (gltf_model)
		{
			ImGui::Text("Loaded GLTF Model");
			ImGui::Text("Mesh Count: %zu", gltf_model->meshes.size());
		}
		else
		{
			ImGui::TextColored(ImVec4(1, 0, 0, 1), "No GLTF Model Loaded");
		}
	}
#endif

	// GLTFモデルの回転更新
	if (enable_gltf_rendering && gltf_model)
	{
		gltf_rotation_y += DirectX::XM_PI * elapsed_time * gltf_rotation_speed;
		if (gltf_rotation_y > DirectX::XM_2PI)
		{
			gltf_rotation_y -= DirectX::XM_2PI;
		}
	}
}

// 描画処理
void MainScene::Render(float elapsed_time)
{
	HRESULT hr{ S_OK };

	auto context = Graphics::Instance().GetContext();
	auto states = Graphics::Instance().GetPipelineStates();

	Graphics::Instance().BeginFrame(0.2f, 0.2f, 0.2f, 1.0f);

	ID3D11ShaderResourceView* null_shader_resource_views[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT]{};
	context->VSSetShaderResources(0, _countof(null_shader_resource_views), null_shader_resource_views);
	context->PSSetShaderResources(0, _countof(null_shader_resource_views), null_shader_resource_views);

	// 定数バッファ更新
	context->UpdateSubresource(constnt_buffer[0].Get(), 0, 0, &data, 0, 0);
	context->VSSetConstantBuffers(1, 1, constnt_buffer[0].GetAddressOf());
	context->PSSetConstantBuffers(1, 1, constnt_buffer[0].GetAddressOf());

	// サンプラー設定
	context->PSSetSamplers(0, 1, states->GetSamplerState(0).GetAddressOf());
	context->PSSetSamplers(1, 1, states->GetSamplerState(1).GetAddressOf());
	context->PSSetSamplers(2, 1, states->GetSamplerState(2).GetAddressOf());

	// ビューポート設定
	context->OMSetDepthStencilState(states->GetDepthStenceilState(1).Get(), 1);
	context->OMSetBlendState(states->GetBlendState(0).Get(), nullptr, 0xFFFFFFFF);

	// ビュー・プロジェクション行列作成
	D3D11_VIEWPORT viewport;
	UINT num_viewports{ 1 };
	context->RSGetViewports(&num_viewports, &viewport);

	// カメラの設定
	DirectX::XMVECTOR eye{ DirectX::XMVectorSet(
		camera.position.x, camera.position.y, camera.position.z, 1) };
	DirectX::XMVECTOR focus{ DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f) };
	DirectX::XMVECTOR up{ DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f) };
	DirectX::XMMATRIX V{ DirectX::XMMatrixLookAtLH(eye, focus, up) };
	float aspect_ratio{ viewport.Width / viewport.Height };
	DirectX::XMMATRIX P{ DirectX::XMMatrixPerspectiveFovLH(
		DirectX::XMConvertToRadians(30),
		aspect_ratio, 0.1f, 100.0f
	) };
	DirectX::XMStoreFloat4x4(&data.view_projection, V * P);
	data.light_direction = { 0, 0, -1, 0 };
	data.camera_position.x = camera.position.x;
	data.camera_position.y = camera.position.y;
	data.camera_position.z = camera.position.z;
	data.camera_position.w = 0;

	float x{ 0 };
	float y{ 0 };

	framebuffers[0]->clear(context);
	framebuffers[0]->activate(context);

	// 深度テスト OFF
	context->OMSetDepthStencilState(states->GetDepthStenceilState(0).Get(), 1);
	context->RSSetState(states->GetRasterizerState(2).Get());

	// 背景描画
	sprute_batches[0]->begin(context);
	sprute_batches[0]->render(context, 0, 0, 1280, 720);
	sprute_batches[0]->end(context);

	// 深度テスト ON
	context->OMSetDepthStencilState(states->GetDepthStenceilState(1).Get(), 1);
	context->RSSetState(states->GetRasterizerState(0).Get());

	const DirectX::XMFLOAT4X4 coordinate_system_transforms[]
	{
		{ -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 },
		{ 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 },
		{ -1, 0, 0, 0, 0, 0, -1, 0, 0, 1, 0, 0, 0, 0, 0, 1 },
		{ 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1 },
	};

	const float scale_factor = 0.05f;

	DirectX::XMMATRIX C
	{
		DirectX::XMLoadFloat4x4(&coordinate_system_transforms[0])
		*
		DirectX::XMMatrixScaling(scale_factor, scale_factor, scale_factor)
	};

	// 深度テスト ON
	context->OMSetDepthStencilState(states->GetDepthStenceilState(1).Get(), 1);
	context->RSSetState(states->GetRasterizerState(0).Get());

	// geometric_primitives[0] の位置と回転と拡大縮小を計算
	DirectX::XMMATRIX S{ DirectX::XMMatrixScaling(
		cube.scale.x, cube.scale.y, cube.scale.z) };
	DirectX::XMMATRIX R{ DirectX::XMMatrixRotationRollPitchYaw(
		cube.rotation.x, cube.rotation.y, cube.rotation.z) };
	DirectX::XMMATRIX T{ DirectX::XMMatrixTranslation(
		cube.position.x, cube.position.y, cube.position.z) };

	DirectX::XMFLOAT4X4 world;
	DirectX::XMStoreFloat4x4(&world, C * S * R * T);

	// W_Cube描画
	S = { DirectX::XMMatrixScaling(
		w_cube.scale.x, w_cube.scale.y, w_cube.scale.z) };
	R = { DirectX::XMMatrixRotationRollPitchYaw(
		w_cube.rotation.x, w_cube.rotation.y, w_cube.rotation.z) };
	T = { DirectX::XMMatrixTranslation(
		w_cube.position.x, w_cube.position.y, w_cube.position.z) };

	DirectX::XMFLOAT4X4 W_world;
	DirectX::XMStoreFloat4x4(&W_world, C * S * R * T);
	geometric_primitives[1]->render(
		context,
		W_world,
		{ 0.5f, 0.8f, 0.2f, 1.0f }
	);

	// GLTFモデル描画
	if (enable_gltf_rendering && gltf_model)
	{
		DrawGltfModel(context);
	}

	// 深度テスト OFF
	context->OMSetDepthStencilState(states->GetDepthStenceilState(0).Get(), 1);
	context->RSSetState(states->GetRasterizerState(2).Get());

	framebuffers[0]->deactivate(context);

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	context->Map(thresholdBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	memcpy(mappedResource.pData, &tb, sizeof(tb));
	context->Unmap(thresholdBuffer.Get(), 0);

	context->PSSetConstantBuffers(0, 1, thresholdBuffer.GetAddressOf());

	context->UpdateSubresource(bloomParamBuffer.Get(), 0, nullptr, &bloomParams, 0, 0);
	context->PSSetConstantBuffers(1, 1, bloomParamBuffer.GetAddressOf());

	framebuffers[1]->clear(context);
	framebuffers[1]->activate(context);

	bit_block_transfer->blit(context, framebuffers[0]->shader_resource_views[0].GetAddressOf(), 0, 1, pixel_shaders[0].Get());

	framebuffers[1]->deactivate(context);

	ID3D11ShaderResourceView* shader_resource_view[2]
	{ framebuffers[0]->shader_resource_views[0].Get(),
	 framebuffers[1]->shader_resource_views[0].Get() };

	bit_block_transfer->blit(context, shader_resource_view, 0, 2, pixel_shaders[1].Get());

	// ImGUI描画
#ifdef USE_IMGUI
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
#endif

	Graphics::Instance().EndFrame();
}

// GLTFモデル描画ヘルパー関数
void MainScene::DrawGltfModel(ID3D11DeviceContext* context)
{
	auto device = Graphics::Instance().GetDevice();
	HRESULT hr = S_OK;

	if (!context || !gltf_model || gltf_model->meshes.empty())
	{
		return;
	}

	auto states = Graphics::Instance().GetPipelineStates();

	//変換行列の計算
	DirectX::XMMATRIX S{ DirectX::XMMatrixScaling(gltf_scale.x, gltf_scale.y, gltf_scale.z) };
	//DirectX::XMMATRIX R_auto{ DirectX::XMMatrixRotationY(gltf_rotation_y) };
	DirectX::XMMATRIX R{ DirectX::XMMatrixRotationRollPitchYaw(gltf_rotation.x, gltf_rotation.y, gltf_rotation.z) };
	DirectX::XMMATRIX T{ DirectX::XMMatrixTranslation(gltf_position.x, gltf_position.y, gltf_position.z) };

	DirectX::XMMATRIX world_transform = S * R * T;
	DirectX::XMStoreFloat4x4(&gltf_model_transform, world_transform);

	//オブジェクト定数バッファ (b1) の更新とセット
	GLTF_OBJECT_CONSTANT gltf_constants;
	gltf_constants.world = gltf_model_transform;
	gltf_constants.material_color = gltf_material_color;
	context->UpdateSubresource(gltf_object_constant_buffer.Get(), 0, 0, &gltf_constants, 0, 0);

	// register(b1) にオブジェクト情報をセット 
	context->VSSetConstantBuffers(1, 1, gltf_object_constant_buffer.GetAddressOf());

	//シーン定数バッファ (b2) のセット
	context->VSSetConstantBuffers(2, 1, constnt_buffer[0].GetAddressOf());
	context->PSSetConstantBuffers(2, 1, constnt_buffer[0].GetAddressOf());

	//サンプラーとパイプラインステートの設定
	context->PSSetSamplers(0, 1, states->GetSamplerState(0).GetAddressOf());
	context->PSSetSamplers(1, 1, states->GetSamplerState(1).GetAddressOf());
	context->PSSetSamplers(2, 1, states->GetSamplerState(2).GetAddressOf());

	context->OMSetDepthStencilState(states->GetDepthStenceilState(1).Get(), 1);
	context->RSSetState(states->GetRasterizerState(0).Get());

	//各メッシュを描画
	for (const auto& mesh : gltf_model->meshes)
	{
		GlthStaticModel::DrawMesh(
			context,
			mesh,
			gltf_model->materials,
			gltf_material_constant_buffer);
	}
}