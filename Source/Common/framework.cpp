#include "framework.h"
#include <d3d.h>

framework::framework(HWND hwnd) : hwnd(hwnd)
{
	cube.position = { 0.0f,-1.5f,0.0f };
	cube.scale = { 0.4f,0.4f,0.4f };
	w_cube.position = { 3,0,0 };
	camera.position = { 0.0f,0.0f,10.0f };
}

bool framework::initialize()
{

	//①デバイス・デバイスコンテキスト・スワップチェーンの作成

	HRESULT hr{ S_OK };

	UINT create_device_flags{ 0 };
#ifdef _DEBUG
	create_device_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif // _DEBUG

	D3D_FEATURE_LEVEL feature_levels{ D3D_FEATURE_LEVEL_11_0 };

	//下記のプログラム変数はスワップチェーンの設定

	DXGI_SWAP_CHAIN_DESC swap_chain_desc{};
	//スワップチェーンのバファーの数
	swap_chain_desc.BufferCount = 2;
	//スワップチェーンウィンドウの縦の長さ
	swap_chain_desc.BufferDesc.Width = SCREEN_WIDTH;
	//スワップチェーンウィンドウの横の長さ
	swap_chain_desc.BufferDesc.Height = SCREEN_HEIGHT;
	//スワップチェーンの色と透明度の設定
	swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	//フレームレートの分子を設定
	swap_chain_desc.BufferDesc.RefreshRate.Numerator = 60;
	//フレームレートの分母を設定
	swap_chain_desc.BufferDesc.RefreshRate.Denominator = 1;
	//バックバッファをレンダーターゲット(描画結果)の出力先に設定
	swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	//描画結果を指定したウィンドウに表示する
	swap_chain_desc.OutputWindow = hwnd;
	//描画された画像の粗さを軽減する処理回数(1は実行しないという意味)
	swap_chain_desc.SampleDesc.Count = 1;
	//画像の粗さを軽減する処理能力の高さ
	swap_chain_desc.SampleDesc.Quality = 0;
	//ウィンドウをフルスクリーンにするかどうか
	swap_chain_desc.Windowed = !FULLSCREEN;

	swap_chain_desc.Flags = 0;
	//デバイスとスワップチェーンの作成
	hr = D3D11CreateDeviceAndSwapChain(
		NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		create_device_flags,
		&feature_levels,
		1,
		D3D11_SDK_VERSION,
		&swap_chain_desc,
		swap_chain.GetAddressOf(), device.GetAddressOf(),
		NULL,
		immediate_context.GetAddressOf());

	//プログラムがエラーを起こした場合に通知する
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	//②レンダーターゲットビューの作成
	/*レンダーターゲットビューとはレンダーターゲット(描画結果)
	　として使用する画像へとアクセスするオブジェクト*/

	 //2次元のデータコンテナを用意
	ID3D11Texture2D* back_buffer{};
	/*描画先のバックバッファのテクスチャオブジェクトの
	　アクセス権限を取得*/
	hr = swap_chain->GetBuffer(
		0, __uuidof(ID3D11Texture2D),
		reinterpret_cast<LPVOID*>(&back_buffer));

	//プログラムがエラーを起こした場合に通知する
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	//レンダーターゲットビューを作成
	hr = device->CreateRenderTargetView(
		back_buffer, NULL, &render_target_view);

	//プログラムがエラーを起こした場合に通知する
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	back_buffer->Release();

	//2次元のデータコンテナを用意
	ID3D11Texture2D* depth_stencil_buffer{};
	/*テクスチャの様々な属性（
	　幅、高さ、フォーマット、ミップマップの数、バインドフラグなど）
	　を記述*/
	D3D11_TEXTURE2D_DESC texture2d_desc{};
	//作成するテクスチャの縦の幅を指定
	texture2d_desc.Width = SCREEN_WIDTH;
	//作成するテクスチャの横の幅を指定
	texture2d_desc.Height = SCREEN_HEIGHT;
	//1つのテクスチャに格納する解像度の数
	texture2d_desc.MipLevels = 1;
	//複数の関連するテクスチャを扱う数
	texture2d_desc.ArraySize = 1;
	//作成するテクスチャのピクセルフォーマットを表す列挙値
	texture2d_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	//1ピクセル当たりの画像の粗さを軽減する処理の回数
	texture2d_desc.SampleDesc.Count = 1;
	//1ピクセルあたりの画像の粗さを軽減する処理能力の高さ
	texture2d_desc.SampleDesc.Quality = 0;
	//2DテクスチャがGPUによって読み書きをする
	texture2d_desc.Usage = D3D11_USAGE_DEFAULT;
	//2Dテクスチャを3Dシーンの深度とステンシルを記憶するためのバッファとして使用する
	texture2d_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	//CPUがテクスチャのデータに直接アクセスすることを許可しない
	texture2d_desc.CPUAccessFlags = 0;
	//2Dテクスチャに特別な特性や使用方法を指定しない
	texture2d_desc.MiscFlags = 0;

	hr = device->CreateTexture2D(&texture2d_desc, NULL, &depth_stencil_buffer);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	D3D11_DEPTH_STENCIL_VIEW_DESC depth_stancil_view_desc{};

	//深度・ステンシルビューが2Dテクスチャリソースと同じピクセルフォーマットでデータを解釈する
	depth_stancil_view_desc.Format = texture2d_desc.Format;
	//標準的な1枚の2Dテクスチャとして扱う
	depth_stancil_view_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	//最も高解像度のミップマップレベルにアクセスする
	depth_stancil_view_desc.Texture2D.MipSlice = 0;
	hr = device->CreateDepthStencilView(
		depth_stencil_buffer,
		&depth_stancil_view_desc,
		&depth_stencil_view
	);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	depth_stencil_buffer->Release();

	//④ビューピートの設定
	D3D11_VIEWPORT viewport{};
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = static_cast<float>(SCREEN_WIDTH);
	viewport.Height = static_cast<float>(SCREEN_HEIGHT);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	//サンプラーステイトオブジェクト生成
	D3D11_SAMPLER_DESC sampler_desc;
	sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.MipLODBias = 0;
	sampler_desc.MaxAnisotropy = 16;
	sampler_desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	sampler_desc.BorderColor[0] = 0;
	sampler_desc.BorderColor[1] = 0;
	sampler_desc.BorderColor[2] = 0;
	sampler_desc.BorderColor[3] = 0;
	sampler_desc.MinLOD = 0;
	sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;
	hr = device->CreateSamplerState(&sampler_desc, &sampler_state[0]);
	 _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	 sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	 hr = device->CreateSamplerState(&sampler_desc, &sampler_state[1]);
	 _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	 sampler_desc.Filter = D3D11_FILTER_ANISOTROPIC;
	 hr = device->CreateSamplerState(&sampler_desc, &sampler_state[2]);
	 _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	 //ステンシルステートオブジェクト生成
	 D3D11_DEPTH_STENCIL_DESC depth_stencil_desc{};

	 //地面やモデルなど、手前にあるものが奥を隠す
	 depth_stencil_desc.DepthEnable = TRUE;//深度テストON
	 depth_stencil_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;//深度書き込みON
	 depth_stencil_desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;//カメラに近いピクセルのみ描画
	 hr = device->CreateDepthStencilState(
		 &depth_stencil_desc, &depth_stencil_state[0]);
	 _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	 //半透明オブジェクトやポストエフェクトに使う設定
	 depth_stencil_desc.DepthEnable = TRUE;//深度テストON
	 depth_stencil_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;//深度書き込みOFF
	 depth_stencil_desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;//カメラに近いピクセルのみ描画
	 hr = device->CreateDepthStencilState(
		 &depth_stencil_desc, &depth_stencil_state[1]);
	 _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	 //深度テストを無効にした特殊用途
	 depth_stencil_desc.DepthEnable = FALSE;//深度テストOFF
	 depth_stencil_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;//深度書き込みON
	 depth_stencil_desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;//カメラに近いピクセルのみ描画
	 hr = device->CreateDepthStencilState(
		 &depth_stencil_desc, &depth_stencil_state[2]);
	 _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	 //完全に深度を無効化した状態
	 depth_stencil_desc.DepthEnable = FALSE;//深度テストOFF
	 depth_stencil_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;//深度書き込みOFF
	 depth_stencil_desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;//カメラに近いピクセルのみ描画
	 hr = device->CreateDepthStencilState(
		 &depth_stencil_desc, &depth_stencil_state[3]);
	 _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	 //ブレンディングステートオブジェクト作成
	 D3D11_BLEND_DESC blend_desc{};
	 blend_desc.AlphaToCoverageEnable = FALSE;
	 blend_desc.IndependentBlendEnable = FALSE;
	 blend_desc.RenderTarget[0].BlendEnable = TRUE;
	 blend_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	 blend_desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	 blend_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	 blend_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	 blend_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	 blend_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	 blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	 hr = device->CreateBlendState(&blend_desc, &blend_states[0]);
	 _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

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

	 create_ps_from_cso(device.Get(), "luminance_extraction_ps.cso", pixel_shaders[0].GetAddressOf());

	 geometric_primitives[0] = 
		 std::make_unique<geometric_primitive>(device.Get());

	 geometric_primitives[1] =
		 std::make_unique<geometric_primitive>(device.Get());

	 framebuffers[1] = std::make_unique<framebuffer>(device.Get(), 1280 / 2, 720 / 2);

	 framebuffers[2] = std::make_unique<framebuffer>(device.Get(), 1280 / 2, 720 / 2);

	 create_ps_from_cso(device.Get(), "blur_ps.cso", pixel_shaders[1].GetAddressOf());
	 create_ps_from_cso(device.Get(), "blur_ps.cso", pixel_shaders[2].GetAddressOf());

	 D3D11_BUFFER_DESC desc = {};
	 desc.Usage = D3D11_USAGE_DEFAULT;
	 desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	 desc.ByteWidth = sizeof(BloomParams);
	 desc.CPUAccessFlags = 0;

	 D3D11_SUBRESOURCE_DATA initData = {};
	 initData.pSysMem = &bloomParams;

	 device->CreateBuffer(&desc, &initData, &bloomParamBuffer);

	 //ラスタライザステートオブジェクト作成
	 D3D11_RASTERIZER_DESC rasterizer_desc{};

	 //ポリゴンの内部を塗りつぶして描画する
	 rasterizer_desc.FillMode = D3D11_FILL_SOLID;
	 //カメラから見て背面を向いているポリゴンを描画しない
	 rasterizer_desc.CullMode = D3D11_CULL_BACK;
	 //時計回りに定義された頂点を持つポリゴンを前面として扱う
	 rasterizer_desc.FrontCounterClockwise = TRUE;
	 //描画するポリゴンの深度値にバイアスを加えない
	 rasterizer_desc.DepthBias = 0;
	 //深度バイアスのクランプ(Z値の上限・下限)を適用しない
	 rasterizer_desc.DepthBiasClamp = 0;
	 //カメラから見た傾斜に基づいて深度オフセットを適用しない
	 rasterizer_desc.SlopeScaledDepthBias = 0;
	 //カメラより手前のオブジェクトとカメラから一定距離離れているオブジェクトを描画しない
	 rasterizer_desc.DepthClipEnable = TRUE;
	 //描画するピクセルが、シザー短形によって制限されないようにする
	 rasterizer_desc.ScissorEnable = FALSE;
	 //マルチサンプリング(ジャギー)を無効にする
	 rasterizer_desc.MultisampleEnable = FALSE;
	 //ラインのアンチエイジングを無効にする
	 rasterizer_desc.AntialiasedLineEnable = FALSE;

	 hr = device->CreateRasterizerState(
		 &rasterizer_desc,
		 reasterizer_states[0].GetAddressOf()
	 );
	 _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	 //ポリゴンの辺のみを描画する
	 rasterizer_desc.FillMode = D3D11_FILL_WIREFRAME;

	 hr = device->CreateRasterizerState(
		 &rasterizer_desc,
		 reasterizer_states[1].GetAddressOf()
	 );
	 _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));


	 //上記までがDirectX11の初期設定処理

	 //下記からはゲームで必要なオブジェクトの初期化


	//描画する場所を設定
	immediate_context->RSSetViewports(1, &viewport);

	sprites[0] = std::make_unique<sprite>
		(device.Get(), L"./resources/cyberpunk.jpg");
	sprites[1] = std::make_unique<sprite>
		(device.Get(), L"./resources/player-sprites.png");
	sprites[2] = std::make_unique<sprite>
		(device.Get(), L"./resources/fonts/font0.png");
	//sprute_batches[0] = std::make_unique<sprite_batch>
	//	(device.Get(), L"./resources/player-sprites.png", 2048);
	sprute_batches[0] = std::make_unique<sprite_batch>
		(device.Get(), L"./resources/screenshot.jpg", 1);
	static_meshes[0] = std::make_unique<static_mesh>
		(device.Get(), L"./resources/Rock/Rock.obj");

	skinned_meshes[0] = std::make_unique<skinned_mesh>
		(device.Get(), "./resources/nico.fbx", true);
	//skinned_meshes[0] = make_unique<skinned_mesh>(device.Get(), "./resources/AimTest/MNK_Mesh.fbx");
	//skinned_meshes[0]->append_animations("./resources/AimTest/Aim_Space.fbx", 0);

	framebuffers[0] = std::make_unique<framebuffer>(device.Get(), 1280, 720);

	bit_block_transfer = std::make_unique<fullscreen_quad>(device.Get());

	return true;
}

void framework::update(float elapsed_time/*Elapsed seconds from last frame*/)
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
		ImGui::SliderFloat("gaussian_sigma", &bloomParams.gaussian_sigma, 0.0f, 1.0f);
	}

	if (ImGui::CollapsingHeader("W_Cube", ImGuiTreeNodeFlags_None))
	{
		ImGui::SliderFloat3("W_CubeScale", &W_Cubuposcale.x, 0, 10);
		ImGui::SliderFloat3("W_CubeRotation", &W_Cuburotation.x, 0, 0.01745f * 360);
		ImGui::SliderFloat3("W_CubePosion", &W_Cubuposition.x, 10, 10);
	}
	if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_None))
	{
		ImGui::SliderFloat3("Camera", &camera.position.x, -100,100);
	}
	if (ImGui::CollapsingHeader("light", ImGuiTreeNodeFlags_None))
	{
		ImGui::SliderFloat4("light_direction", &data.light_direction.x, -1, 1);
	}


	//ImGui::End();
#endif
}
void framework::render(
	float elapsed_time/*Elapsed seconds from last frame*/)
{
	HRESULT hr{ S_OK };

	//GPU が同じテクスチャを「描画先」としても「読み取り元」としても使わないように、一旦すべてのバインドを解除
	ID3D11RenderTargetView* null_render_target_views[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT]{};
	immediate_context->OMSetRenderTargets(_countof(null_render_target_views), null_render_target_views, 0);//現在の「描画ターゲット」を全部解除して、何にも描かない状態に戻す
	ID3D11ShaderResourceView* null_shader_resource_views[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT]{};
	immediate_context->VSSetShaderResources(0, _countof(null_shader_resource_views), null_shader_resource_views);//頂点シェーダーで使っていた読み取り用テクスチャを全部解除
	immediate_context->PSSetShaderResources(0, _countof(null_shader_resource_views), null_shader_resource_views);//ピクセルシェーダーで使っていた読み取り用テクスチャを全部解除

	//定数バッファ更新
	immediate_context->UpdateSubresource(
		constnt_buffer[0].Get(),
		0, 0,
		&data,
		0, 0);
	immediate_context->VSSetConstantBuffers(
		1, 1, constnt_buffer[0].GetAddressOf()
	);

	immediate_context->PSSetConstantBuffers(
		1, 1,
		constnt_buffer[0].GetAddressOf()
	);

	// ここからオブジェクトの描画

	immediate_context->RSSetState(reasterizer_states[0].Get());

	// ここが毎フレームの描画の準備なのでこれより上に書いても意味が無い

	//レンダーターゲットと深度クリア
	FLOAT color[]{ 0.2f,0.2f,0.2f,1.0f };
	immediate_context->ClearRenderTargetView(
		render_target_view.Get(), color);

	immediate_context->ClearDepthStencilView(
		depth_stencil_view.Get(),
		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
		1.0f,
		0);

	immediate_context->OMSetRenderTargets(
		1,
		render_target_view.GetAddressOf(),
		depth_stencil_view.Get());

	// これがシェーダーに利用されるであろう設定
	immediate_context->PSSetSamplers(0, 1, sampler_state[0].GetAddressOf());
	immediate_context->PSSetSamplers(1, 1, sampler_state[1].GetAddressOf());
	immediate_context->PSSetSamplers(2, 1, sampler_state[2].GetAddressOf());
	
	// ここが毎オブジェクトごとの都度の設定
	immediate_context->OMSetDepthStencilState(
		depth_stencil_state[3].Get(), 1);
	
	immediate_context->OMSetBlendState(
		blend_states[0].Get(),
		nullptr,
		0xFFFFFFFF);

	//ビュー・プロジェクション行列作成
	D3D11_VIEWPORT viewport;
	UINT num_viewports{ 1 };
	immediate_context->RSGetViewports(&num_viewports, &viewport);

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
	data.camera_position.y= camera.position.y;
	data.camera_position.z = camera.position.z;
	data.camera_position.w = 0;

#if true
	//sprites[0]->render(
	//	immediate_context.Get(),
	//	position.x,
	//	size.x,
	//	position.y,
	//	size.y,
	//	render_color.x,
	//	render_color.y,
	//	render_color.z,
	//	render_color.w,
	//	angle
	//);

	//sprites[1]->render(
	//	immediate_context.Get(),
	//	700,
	//	200,
	//	200,
	//	200,
	//	1, 1, 1, 1,
	//	45,
	//	0, 0, 140, 240
	//);
	
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

	//sprute_batches[0]->begin(immediate_context.Get());
	//for (size_t i = 0; i < 1092; i++)
	//{
	//	sprute_batches[0]->render(immediate_context.Get(),
	//		x, static_cast<float>(static_cast<int>(y) % 720), 64, 64,
	//		1, 1, 1, 1, 0, 140 * 0, 240 * 0, 140, 240);
	//	x += 32;
	//	if (x > 1280 - 64)
	//	{
	//		x = 0;
	//		y += 24;
	//	}
	//}
	//sprute_batches[0]->end(immediate_context.Get());
#endif

	framebuffers[0]->clear(immediate_context.Get());
	framebuffers[0]->activate(immediate_context.Get());

	// 深度テスト OFF
	immediate_context->OMSetDepthStencilState(
		depth_stencil_state[3].Get(), 1);

	// 面カリングなし（全部描画）
	immediate_context->RSSetState(reasterizer_states[2].Get());

	//背景描画
	sprute_batches[0]->begin(immediate_context.Get());
	sprute_batches[0]->render(immediate_context.Get(),0, 0, 1280, 720);
	sprute_batches[0]->end(immediate_context.Get());

	//sprites[2]->textout(
	//	immediate_context.Get(),
	//	"ECC TEST",
	//	100, 100, 32, 32,
	//	1, 1, 1, 1);

	immediate_context->OMSetDepthStencilState(
		depth_stencil_state[0].Get(), 1);


	immediate_context->RSSetState(reasterizer_states[0].Get());

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
	immediate_context->OMSetDepthStencilState(
		depth_stencil_state[0].Get(), 1);

	// 面カリングあり
	immediate_context->RSSetState(reasterizer_states[0].Get());

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
	skinned_mesh::animation&  animation{ skinned_meshes[0]->animation_clips.at(clip_index) };
	
	//経過時間とサンプリングレートから 現在表示すべきフレーム番号を計算
	frame_index = static_cast<int>(animation_tick * animation.sampling_rate);
	
	//アニメーションが終了したら最初にループ（ループ再生）
	if (frame_index > animation.sequence.size() - 1)
	{
		frame_index = 0;
		animation_tick = 0;
	}
	else
	{
		//アニメーションを進める（1フレームあたりの経過時間を足す）
		animation_tick += elapsed_time;
	}
	//現在のフレーム（ボーン姿勢データ）を取得
	skinned_mesh::animation::keyframe& keyframe{ animation.sequence.at(frame_index) };
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
	skinned_meshes[0]->render
	(immediate_context.Get(), world, material_color, &keyframe);
		

	//geometric_primitives[0]->render(
	//	immediate_context.Get(),
	//	world,
	//	{ 0.5f,0.8f,0.2f,1.0f }
	//);

	//static_meshes[0]->render(
	//	immediate_context.Get(),
	//	world,
	//	{ 1.0f,1.0f,1.0f,1.0f });



	immediate_context->RSSetState(reasterizer_states[1].Get());



	S = { DirectX::XMMatrixScaling(
		w_cube.position.x,w_cube.position.y,w_cube.position.z) };
	R = { DirectX::XMMatrixRotationRollPitchYaw(
		w_cube.rotation.x,w_cube.rotation.y,w_cube.rotation.z) };
	T = { DirectX::XMMatrixTranslation(
		w_cube.scale.x,w_cube.scale.y,w_cube.scale.z) };

	DirectX::XMFLOAT4X4 W_world;
	/*オブジェクトの最終的なワールド変換行列を計算し、保存する*/
	DirectX::XMStoreFloat4x4(&W_world, C* S* R* T);
	geometric_primitives[1]->render(
		immediate_context.Get(),
		W_world,
		{ 0.5f,0.8f,0.2f,1.0f }
	);
	//immediate_context->RSSetState(reasterizer_states[0].Get());

	// 深度テスト OFF
	immediate_context->OMSetDepthStencilState(
		depth_stencil_state[3].Get(), 1);

	// 面カリングなし（全部描画）
	immediate_context->RSSetState(reasterizer_states[2].Get());


#if 1
	framebuffers[0]->deactivate(immediate_context.Get());

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	immediate_context->Map(thresholdBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	memcpy(mappedResource.pData, &tb, sizeof(tb));
	immediate_context->Unmap(thresholdBuffer.Get(), 0);

	// ピクセルシェーダーへセット
	immediate_context->PSSetConstantBuffers(0, 1, thresholdBuffer.GetAddressOf());

	immediate_context->UpdateSubresource(
		bloomParamBuffer.Get(), 0, nullptr, &bloomParams, 0, 0);
	immediate_context->PSSetConstantBuffers(1, 1, bloomParamBuffer.GetAddressOf());



	framebuffers[1]->clear(immediate_context.Get());

	framebuffers[1]->activate(immediate_context.Get());

	bit_block_transfer->blit(immediate_context.Get(),
		framebuffers[0]->shader_resource_views[0].GetAddressOf(),
		0, 1, pixel_shaders[0].Get());

	framebuffers[1]->deactivate(immediate_context.Get());

#if 0
	bit_block_transfer->blit(immediate_context.Get(),
		framebuffers[1]->shader_resource_views[0].GetAddressOf(), 0, 1);
#endif

	ID3D11ShaderResourceView* shader_resource_view[2]
	{ framebuffers[0]->shader_resource_views[0].Get(),
	 framebuffers[1]->shader_resource_views[0].Get() };


	bit_block_transfer->blit(
		immediate_context.Get(),
		shader_resource_view,
		0, 2,
		pixel_shaders[1].Get());


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
	swap_chain->Present(sync_interval, 0);



}

void framework::render(
	ID3D11DeviceContext* immediate_context,
	float dx, float dy, float dw, float dh,
	float r, float g, float b, float a,
	float angle)
{
}

bool framework::uninitialize()
{
	//device->Release();
	//immediate_context->Release();
	//swap_chain->Release();
	//render_target_view->Release();
	//depth_stencil_view->Release();

	//for (int i = 0; i < 3; i++)
	//{
	//	sampler_state[i]->Release();
	//}

	//for (int i = 0; i < 4; i++)
	//{
	//	depth_stencil_state[i]->Release();
	//}

	//for (int i = 0; i < 1; i++)
	//{
	//	blend_states[i]->Release();
	//}

	//for (sprite* p : sprites)delete p;

	return true;
}

void framework::editInformation(
	std::string label,
	Transform& t,
	float posRange,
	float scaleRange)
{
	{
		ImGui::Begin("ImGUI");
		if (ImGui::CollapsingHeader(label.c_str()))
		{
			std::string pos = "Position ";
			std::string scale = "Scale ";
			std::string rotation = "Rotation ";
			std::string color = "Color ";

			ImGui::SliderFloat3((pos + label).c_str(), &t.position.x, -posRange, posRange);
			ImGui::SliderFloat3((scale + label).c_str(), &t.scale.x, -scaleRange, scaleRange);
			ImGui::SliderFloat3((rotation + label).c_str(), &t.rotation.x, 0.0f, XM_2PI);
			ImGui::ColorEdit4((color + label).c_str(), &t.color.x);
		}
		ImGui::End();
	}
}

framework::~framework()
{
}