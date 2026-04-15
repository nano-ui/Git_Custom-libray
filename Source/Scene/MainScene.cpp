#include "../Scene/MainScene.h"
#include "../Graphics/Graphics.h"
#include <imgui.h>
#include "../Common/misc.h"
#include "../Graphics/shader.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"



//コンストラクタ
MainScene::MainScene()
{
	cube.position = { 0.0f,-1.5f,0.0f };
	cube.scale = { 0.4f,0.4f,0.4f };
	w_cube.position = { 3,0,0 };
	camera.position = { 0.0f,0.0f,10.0f };
	tb.brightness_threshold = 1.0f;
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
	resource = std::make_shared<FbxSkinnedResource>(device);

	resource->Load("./resources/nico.fbx");

	fbx_skinned_model = std::make_unique<FbxSkinnedModel>(resource);

	fbx_skinned_model->PlayAnimation("NIC_Idle");

	gltf_models[0] = std::make_unique<GltfMpdel>(device,
		"./glTF-Sample-Models-main/2.0/2CylinderEngine/glTF/2CylinderEngine.gltf");

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

	for (auto& prim : geometric_primitives)prim.reset();

	if (bit_block_transfer)bit_block_transfer.reset();

	//COMポインタも明示的に開放
	for (auto& cb : constnt_buffer) cb.Reset();
	for (auto& ps : pixel_shaders)ps.Reset();
	thresholdBuffer.Reset();
	bloomParamBuffer.Reset();
	blurDirectionBuffer.Reset();

	Graphics::Instance().Finalize();
}

//更新処理
void MainScene::Update(float elapsed_time)
{
#ifdef USE_IMGUI
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
#endif

#ifdef USE_IMGUI
	//ImGui::Begin("ImGUI");

	//editInformation("skinned_meshs", cube);
	//editInformation("geometric_primitives", w_cube);
	//editInformation("camera", camera);

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
		ImGui::SliderFloat3("CubeRotation", &cube.rotation.x, 0, DirectX::XM_PI);
		ImGui::SliderFloat3("CubePosion", &cube.position.x, -10, 10);
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


	//ImGui::End();
#endif

	if (fbx_skinned_model)
	{
		fbx_skinned_model->AnimationUpdate(elapsed_time);
	}
}

//描画処理
void MainScene::Render(float elapsed_time)
{
	HRESULT hr{ S_OK };

	auto context = Graphics::Instance().GetContext();
	auto states = Graphics::Instance().GetPipelineStates();

	Graphics::Instance().BeginFrame(0.2f, 0.2f, 0.2f, 1.0f);

	ID3D11ShaderResourceView* null_shader_resource_views[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT]{};
	context->VSSetShaderResources(0, _countof(null_shader_resource_views), null_shader_resource_views);//頂点シェーダーで使っていた読み取り用テクスチャを全部解除
	context->PSSetShaderResources(0, _countof(null_shader_resource_views), null_shader_resource_views);//ピクセルシェーダーで使っていた読み取り用テクスチャを全部解除

	//定数バッファ更新
	context->UpdateSubresource(constnt_buffer[0].Get(), 0, 0, &data, 0, 0);
	context->VSSetConstantBuffers(1, 1, constnt_buffer[0].GetAddressOf());

	context->PSSetConstantBuffers(1, 1, constnt_buffer[0].GetAddressOf());

	// ここからオブジェクトの描画

	// これがシェーダーに利用されるであろう設定
	context->PSSetSamplers(0, 1, states->GetSamplerState(0).GetAddressOf());
	context->PSSetSamplers(1, 1, states->GetSamplerState(1).GetAddressOf());
	context->PSSetSamplers(2, 1, states->GetSamplerState(2).GetAddressOf());

	// ここが毎オブジェクトごとの都度の設定
	context->OMSetDepthStencilState(states->GetDepthStenceilState(1).Get(), 1);

	context->OMSetBlendState(states->GetBlendState(0).Get(), nullptr, 0xFFFFFFFF);

	//ビュー・プロジェクション行列作成
	D3D11_VIEWPORT viewport;
	UINT num_viewports{ 1 };
	context->RSGetViewports(&num_viewports, &viewport);

	// カメラの設定
	DirectX::XMVECTOR eye{ DirectX::XMVectorSet(
		camera.position.x,camera.position.y,camera.position.z,1) };//カメラの位置
	DirectX::XMVECTOR focus{ DirectX::XMVectorSet(0.0f,0.0f,0.0f,1.0f) };//見ている方向
	DirectX::XMVECTOR up{ DirectX::XMVectorSet(0.0f,1.0f,0.0f,0.0f) };//上方向（回転の基準）
	DirectX::XMMATRIX V{ DirectX::XMMatrixLookAtLH(eye,focus,up) };//ビュー行列を作成
	float aspect_ratio{ viewport.Width / viewport.Height };//アスペクト比（画面の横÷縦）
	DirectX::XMMATRIX P{ DirectX::XMMatrixPerspectiveFovLH(
		DirectX::XMConvertToRadians(30),
		aspect_ratio,0.1f,100.0f
	) };
	DirectX::XMStoreFloat4x4(&data.view_projection, V * P);
	data.light_direction = { 0, 0, -1, 0 };
	data.camera_position.x = camera.position.x;
	data.camera_position.y = camera.position.y;
	data.camera_position.z = camera.position.z;
	data.camera_position.w = 0;

#if true

#endif

	float x{ 0 };
	float y{ 0 };


#if 0
	for (size_t i = 0; i < 1092; i++)
	{
		sprites[1]->render(immediate_context,
			x, static_cast<float>(static_cast<int>(y) % 720), 64, 64,
			1, 1, 1, 1, 0, 140 * 0, 240 * 0, 140, 240);
		x += 32;
		if (x > 1280 - 64)
		{
			x = 0;
			y += 24;
		}
	}
#else

#endif

	framebuffers[0]->clear(context);
	framebuffers[0]->activate(context);

	// 深度テスト OFF
	context->OMSetDepthStencilState(states->GetDepthStenceilState(0).Get(), 1);

	// 裏面カリングなし（全部描画）
	context->RSSetState(states->GetRasterizerState(2).Get());

	//背景描画
	sprute_batches[0]->begin(context);
	sprute_batches[0]->render(context, 0, 0, 1280, 720);
	sprute_batches[0]->end(context);

	context->OMSetDepthStencilState(states->GetDepthStenceilState(1).Get(), 1);

	context->RSSetState(states->GetRasterizerState(0).Get());

	/*FBX SDKが読み込むモデルのZ軸がDirectXのY軸に対応し、
	Y軸がDirectXの-Z軸に対応するなど、
	軸の入れ替えと反転を行うための変換行列*/
	const DirectX::XMFLOAT4X4  coordinate_system_transforms[]
	{
		{ -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 },
		{ 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 },
		{ -1, 0, 0, 0, 0, 0, -1, 0, 0, 1, 0, 0, 0, 0, 0, 1 },
		{ 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1 },
	};

	/* To change the units from centimeters to meters,
	set 'scale_factor' to 0.01.*/
	/*モデル全体のスケール（拡大・縮小）を調整するための係数*/
	const float scale_factor = 0.05f;

	DirectX::XMMATRIX C
	{
		/*coordinate_system_transforms配列から、
		インデックス2番目の座標変換行列をDirectXMathのXMMATRIX形式にロード*/
		DirectX::XMLoadFloat4x4(&coordinate_system_transforms[0])
		*
		/*scale_factorを使って、すべての軸方向に均一なスケール変換行列を作成*/
		DirectX::XMMatrixScaling(scale_factor,scale_factor,scale_factor)
	};

	// 深度テスト ON
	context->OMSetDepthStencilState(states->GetDepthStenceilState(1).Get(), 1);

	// 面カリングあり
	context->RSSetState(states->GetRasterizerState(0).Get());

	// geometric_primitives[0] の 位置と回転と拡大を計算
	DirectX::XMMATRIX S{ DirectX::XMMatrixScaling(
		cube.scale.x,cube.scale.y,cube.scale.z) };
	DirectX::XMMATRIX R{ DirectX::XMMatrixRotationRollPitchYaw(
		cube.rotation.x,cube.rotation.y,cube.rotation.z) };
	DirectX::XMMATRIX T{ DirectX::XMMatrixTranslation(
		cube.position.x,cube.position.y,cube.position.z) };
	// で描画
	DirectX::XMFLOAT4X4 world;
	/*オブジェクトの最終的なワールド変換行列を計算し、保存する*/
	DirectX::XMStoreFloat4x4(&world, C * S * R * T);
#if 1
	int clip_index{ 0 };	//何番目のアニメーションクリップを使うか
	int frame_index{ 0 };	//現在再生中のフレーム番号
	static float animation_tick{ 0 };	//累積再生時間

	//最初のスキンメッシュの最初のアニメーションクリップを取得
	//skinned_mesh::animation& animation{ skinned_meshes[0]->animation_clips.at(clip_index) };

	//経過時間とサンプリングレートから 現在表示すべきフレーム番号を計算
	//frame_index = static_cast<int>(animation_tick * animation.sampling_rate);

	//アニメーションが終了したら最初にループ（ループ再生）
	//if (frame_index > animation.sequence.size() - 1)
	//{
	//	frame_index = 0;
	//	animation_tick = 0;
	//}
	//else
	//{
	//	//アニメーションを進める（1フレームあたりの経過時間を足す）
	//	animation_tick += elapsed_time;
	//}
	////現在のフレーム（ボーン姿勢データ）を取得
	//skinned_mesh::animation::keyframe& keyframe{ animation.sequence.at(frame_index) };
#else
	skinned_mesh::animation::keyframe keyframe;
	const skinned_mesh::animation::keyframe* keyframes[2]{
		&skinned_meshes[0]->animation_clips.at(0).sequence.at(40),
		&skinned_meshes[0]->animation_clips.at(0).sequence.at(80)
	};
	skinned_meshes[0]->blend_animations(keyframes, supplementation, keyframe);
	skinned_meshes[0]->update_animation(keyframe);

#endif


#if 0
	//回転を直接指定(ImGuiにする)
	XMStoreFloat4(&keyframe.nodes.at(30).rotation,
		DirectX::XMQuaternionRotationAxis(DirectX::XMVectorSet(1, 0, 0, 0), Rotation));

	//平行移動（X方向）を上書き(ImGuiにする)
	keyframe.nodes.at(30).translation.x = Long;

	//姿勢を再計算
	skinned_meshes[0]->update_animation(keyframe);
#endif

	//現在のボーン姿勢を使ってスキンメッシュを描画
	//skinned_meshes[0]->render
	//(context, world, material_color, &keyframe);

	context->RSSetState(states->GetRasterizerState(0).Get());



	S = { DirectX::XMMatrixScaling(
		w_cube.scale.x,w_cube.scale.y,w_cube.scale.z) };
	R = { DirectX::XMMatrixRotationRollPitchYaw(
		w_cube.rotation.x,w_cube.rotation.y,w_cube.rotation.z) };
	T = { DirectX::XMMatrixTranslation(
		w_cube.position.x,w_cube.position.y,w_cube.position.z) };

	DirectX::XMFLOAT4X4 W_world;
	/*オブジェクトの最終的なワールド変換行列を計算し、保存する*/
	DirectX::XMStoreFloat4x4(&W_world, C * S * R * T);
	geometric_primitives[1]->render(
		context,
		W_world,
		{ 0.5f,0.8f,0.2f,1.0f }
	);
	//immediate_context->RSSetState(reasterizer_states[0].Get());

	if (fbx_skinned_model)
	{
		float scale = 0.05f;
		cube.scale.x = cube.scale.y = cube.scale.z = 0.01;
		DirectX::XMMATRIX S{ DirectX::XMMatrixScaling(
			cube.scale.x,cube.scale.y,cube.scale.z) };

		DirectX::XMMATRIX R{ DirectX::XMMatrixRotationRollPitchYaw(
			cube.rotation.x,cube.rotation.y,cube.rotation.z) };

		DirectX::XMMATRIX T{ DirectX::XMMatrixTranslation(
			cube.position.x,cube.position.y,cube.position.z) };

		DirectX::XMFLOAT4X4 world;
		DirectX::XMStoreFloat4x4(&world, S* R* T);

		DirectX::XMFLOAT4 color = { 1.0f, 1.0f, 1.0f, 1.0f };

		fbx_skinned_model->Render(context, world, color);
	}

	// 深度テスト OFF
	context->OMSetDepthStencilState(states->GetDepthStenceilState(0).Get(), 1);

	// 面カリングなし（全部描画）
	context->RSSetState(states->GetRasterizerState(2).Get());


#if 1
	framebuffers[0]->deactivate(context);

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	context->Map(thresholdBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	memcpy(mappedResource.pData, &tb, sizeof(tb));
	context->Unmap(thresholdBuffer.Get(), 0);

	// ピクセルシェーダーへセット
	context->PSSetConstantBuffers(0, 1, thresholdBuffer.GetAddressOf());

	context->UpdateSubresource(bloomParamBuffer.Get(), 0, nullptr, &bloomParams, 0, 0);
	context->PSSetConstantBuffers(1, 1, bloomParamBuffer.GetAddressOf());



	framebuffers[1]->clear(context);

	framebuffers[1]->activate(context);

	bit_block_transfer->blit(context, framebuffers[0]->shader_resource_views[0].GetAddressOf(), 0, 1, pixel_shaders[0].Get());

	framebuffers[1]->deactivate(context);

#if 0
	bit_block_transfer->blit(immediate_context.Get(),
		framebuffers[1]->shader_resource_views[0].GetAddressOf(), 0, 1);
#endif

	ID3D11ShaderResourceView* shader_resource_view[2]
	{ framebuffers[0]->shader_resource_views[0].Get(),
	 framebuffers[1]->shader_resource_views[0].Get() };


	bit_block_transfer->blit(context, shader_resource_view, 0, 2, pixel_shaders[1].Get());


#endif

	// ここまでがオブジェクトの描画

	// ここが デバッグ用の表示である IMGUI の描画
#ifdef USE_IMGUI
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
#endif	// USE_IMGUI


	UINT sync_interval{ 0 };

	// これが描画の最終チェックでこれより下に書いても意味がない
	//FrontバッファとBackバッファ切り替え
	Graphics::Instance().EndFrame();

}
