#include "ShadowMap.h"

#include "../Common/misc.h"

#include <memory>
#include <cstdio>
#include <cmath>

static constexpr UINT shadow_map_width = 1024;			//シャドウマップの横幅の解像度
static constexpr UINT shadow_map_height = 1024;			//シャドウマップの立幅の解像度
static constexpr float shadow_draw_rect = 30.0f;		//シャドウマップに描画する正射影の範囲サイズ
static constexpr float default_shadow_bias = 0.008f;	//影の縞模様を防ぐ初期バイアス値
static constexpr float light_distance_scale = -50.0f;	//ライトの視点をどこまで後方に引くかのスケール値
static constexpr float up_vector_threshold = 0.99f;     // 上方向ベクトルが平行か判定する閾値

//コンストラクタ
ShadowMap::ShadowMap()
{
	shadow_color = { 0.3f,0.3f,0.3f };
	shadow_bias = default_shadow_bias;
	DirectX::XMStoreFloat4x4(&light_view_projection, DirectX::XMMatrixIdentity());
}

//デストラクタ
ShadowMap::~ShadowMap()
{

}

//初期化
void ShadowMap::Initialize(ID3D11Device* device)
{
	HRESULT hr = S_OK;

	//深度描画用の2Dテクスチャバッファの生成
	Microsoft::WRL::ComPtr<ID3D11Texture2D> depth_buffer = {};
	D3D11_TEXTURE2D_DESC texture2d_desc = {};
	texture2d_desc.Width = shadow_map_width;
	texture2d_desc.Height = shadow_map_height;
	texture2d_desc.MipLevels = 1;
	texture2d_desc.ArraySize = 1;
	texture2d_desc.Format = DXGI_FORMAT_R32_TYPELESS;
	texture2d_desc.SampleDesc.Count = 1;
	texture2d_desc.SampleDesc.Quality = 0;
	texture2d_desc.Usage = D3D11_USAGE_DEFAULT;
	texture2d_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	texture2d_desc.CPUAccessFlags = 0;
	texture2d_desc.MiscFlags = 0;
	hr = device->CreateTexture2D(&texture2d_desc, NULL, depth_buffer.GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	//深度ステンシルビューの生成
	D3D11_DEPTH_STENCIL_VIEW_DESC depth_stencil_view_desc = {};
	depth_stencil_view_desc.Format = DXGI_FORMAT_D32_FLOAT;
	depth_stencil_view_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depth_stencil_view_desc.Texture2D.MipSlice = 0;
	hr = device->CreateDepthStencilView(depth_buffer.Get(), &depth_stencil_view_desc, depth_stencil_view.GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	//シェーダーリソースビューの生成
	D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_view_desc = {};
	shader_resource_view_desc.Format = DXGI_FORMAT_R32_FLOAT;
	shader_resource_view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shader_resource_view_desc.Texture2D.MostDetailedMip = 0;
	shader_resource_view_desc.Texture2D.MipLevels = 1;
	hr = device->CreateShaderResourceView(depth_buffer.Get(), &shader_resource_view_desc, shader_resource_view.GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	//定数バッファの生成
	D3D11_BUFFER_DESC buffer_desc = {};
	buffer_desc.ByteWidth = sizeof(shadowmap_constants);
	buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	buffer_desc.CPUAccessFlags = 0;
	hr = device->CreateBuffer(&buffer_desc, nullptr, constant_buffer.GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	//サンプラステートの生成
	D3D11_SAMPLER_DESC sampler_desc = {};
	sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	sampler_desc.MipLODBias = 0;
	sampler_desc.MaxAnisotropy = 16;
	sampler_desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	sampler_desc.BorderColor[0] = FLT_MAX;
	sampler_desc.BorderColor[1] = FLT_MAX;
	sampler_desc.BorderColor[2] = FLT_MAX;
	sampler_desc.BorderColor[3] = FLT_MAX;
	sampler_desc.MinLOD = 0;
	sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;
	hr = device->CreateSamplerState(&sampler_desc, sampler_state.GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	//シャドウマップ描画用シェーダーの読み込み
	D3D11_INPUT_ELEMENT_DESC input_element_desc[]
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	bool success = shadow_caster_shader.Initialize(
		"shadowmap_caster_vs.cso",
		input_element_desc,
		_countof(input_element_desc)
	);
	_ASSERT_EXPR(success, L"ShadowMap Shader Initialization Failed");

}

//影の書き込みパス開始処理
void ShadowMap::BeginCasterPass(ID3D11DeviceContext* context, const DirectX::XMFLOAT3& light_direction)
{
	//レンダリングターゲットをシャドウマップの深度バッファのみに切り替え
	context->ClearDepthStencilView(depth_stencil_view.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	context->OMSetRenderTargets(0, nullptr, depth_stencil_view.Get());

	//シャドウマップ用のビューポート設定
	D3D11_VIEWPORT viewport = {};
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = static_cast<float>(shadow_map_width);
	viewport.Height = static_cast<float>(shadow_map_height);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	context->RSSetViewports(1, &viewport);

	shadowmap_constants shadowmap = {};
	shadowmap.light_view_projection = light_view_projection;
	context->UpdateSubresource(constant_buffer.Get(), 0, 0, &shadowmap, 0, 0);
	context->VSSetConstantBuffers(6, 1, constant_buffer.GetAddressOf());

	//シャドウマップ専用シェーダーのバインド
	shadow_caster_shader.Apply();

	//ライトの視点に基づく行列計算
	DirectX::XMVECTOR light_dir_vec = DirectX::XMLoadFloat3(&light_direction);
	light_dir_vec = DirectX::XMVector3Normalize(light_dir_vec);
	DirectX::XMVECTOR light_pos_vec = DirectX::XMVectorScale(light_dir_vec, light_distance_scale);
	DirectX::XMVECTOR up_vec = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	if (std::fabs(light_direction.y) > up_vector_threshold)
	{
		up_vec = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	}

	DirectX::XMMATRIX view_matrix = DirectX::XMMatrixLookAtLH(
		light_pos_vec,
		DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f),
		up_vec
	);


	DirectX::XMMATRIX proj_matrix = DirectX::XMMatrixOrthographicLH(
		shadow_draw_rect, shadow_draw_rect,
		0.1f, 200.0f
	);
	DirectX::XMStoreFloat4x4(&light_view_projection, view_matrix * proj_matrix);

	shadowmap.light_view_projection = light_view_projection;
	context->UpdateSubresource(constant_buffer.Get(), 0, 0, &shadowmap, 0, 0);
	context->VSSetConstantBuffers(6, 1, constant_buffer.GetAddressOf());
}

//影の読み込みパス開始処理（通常のオブジェクト描画へのバインド）
void ShadowMap::BindReceiverPass(ID3D11DeviceContext* context, UINT texture_slot, UINT sampler_slot, UINT constant_buffer_slot)
{
	//定数バッファの更新と転送
	shadowmap_constants shadowmap = {};
	shadowmap.light_view_projection = light_view_projection;
	shadowmap.shadow_color = shadow_color;
	shadowmap.shadow_bias = shadow_bias;
	
	context->UpdateSubresource(constant_buffer.Get(), 0, 0, &shadowmap, 0, 0);
	context->VSSetConstantBuffers(constant_buffer_slot, 1, constant_buffer.GetAddressOf());
	context->PSSetConstantBuffers(constant_buffer_slot, 1, constant_buffer.GetAddressOf());

	//テクスチャとサンプラのバインド
	context->PSSetShaderResources(texture_slot, 1, shader_resource_view.GetAddressOf());
	context->PSSetSamplers(sampler_slot, 1, sampler_state.GetAddressOf());
}

//影の読み込みパス終了処理（テクスチャの強制バインド解除）
void ShadowMap::UnbindReceiverPass(ID3D11DeviceContext* context, UINT texture_slot)
{
	//テクスチャの紐付けを解除
	ID3D11ShaderResourceView* clear_shader_resource_view[] = { nullptr };
	context->PSSetShaderResources(texture_slot, 1, clear_shader_resource_view);
}
