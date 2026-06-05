#include "SkyBox.h"
#include "../Shaders/CustomShader.h"
#include "../ObjectsRender/texture.h"
#include <crtdbg.h>
#include <misc.h>

//コンストラクタ
SkyBox::SkyBox()
{
}

//デストラクタ
SkyBox::~SkyBox()
{
}

//スカイボックスとIBL環境色テクスチャを読み込む
bool SkyBox::Initialize(
	ID3D11Device* device,
	const std::wstring& skybox_tex,
	std::wstring& diffuse_tex,
	const std::wstring& specular_tex,
	const std::wstring& lut_tex)
{
	HRESULT hr;

	//各種環境テクスチャの読み込み
	hr = load_texture_from_file(device, skybox_tex.c_str(), skybox_srv.ReleaseAndGetAddressOf(), nullptr);			
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));																		
	hr = load_texture_from_file(device, diffuse_tex.c_str(), diffuse_iem_srv.ReleaseAndGetAddressOf(), nullptr);	
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));																		
	hr = load_texture_from_file(device, specular_tex.c_str(), specular_pmrem_srv.ReleaseAndGetAddressOf(), nullptr);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));																		
	hr = load_texture_from_file(device, lut_tex.c_str(), lut_ggx_srv.ReleaseAndGetAddressOf(), nullptr);			
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	//シェーダーオブジェクトの初期化
	D3D11_INPUT_ELEMENT_DESC input_element_desc[] = {																	
		{ 
			"POSITION", OFFSET_ZERO, DXGI_FORMAT_R32G32B32_FLOAT, OFFSET_ZERO, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, OFFSET_ZERO }
		};
	skybox_shader = std::make_unique<CustomShader>();																	
	bool is_success = skybox_shader->Initialize("sky_box_vs.cso", "sky_box_ps.cso", input_element_desc, _countof(input_element_desc)); 
	_ASSERT_EXPR(is_success, L"Failed to initialize skybox shader");

	//立方体を描画するための頂点・インデックス生成
	DirectX::XMFLOAT3 vertices[] = {
		{-1.0f,1.0f,-1.0f},{1.0f,1.0f,-1.0f},{1.0f,-1.0f,-1.0f},{-1.0f,-1.0f,-1.0f},
		{-1.0f,1.0f,1.0f},{1.0f,1.0f,1.0f},{1.0f,-1.0f,1.0f},{-1.0f,-1.0f,1.0f}
	};
	D3D11_BUFFER_DESC buffer_desc = {};
	buffer_desc.ByteWidth = sizeof(vertices);
	buffer_desc.Usage = D3D11_USAGE_IMMUTABLE;
	buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	D3D11_SUBRESOURCE_DATA init_data = { vertices };
	device->CreateBuffer(&buffer_desc, &init_data, vertex_buffer.ReleaseAndGetAddressOf());
	WORD indices[] = {																									
		0, 1, 2, 0,
		2, 3, 4, 6,
		5, 4, 7, 6,
		4, 5, 1, 4,
		1, 0, 3, 2,
		6, 3, 6, 7,
		1, 5, 6, 1,
		6, 2, 4, 0,
		3, 4, 3, 7
	};
	buffer_desc.ByteWidth = sizeof(indices);
	buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	init_data.pSysMem = indices;
	device->CreateBuffer(&buffer_desc, &init_data, index_buffer.ReleaseAndGetAddressOf());

	//定数バッファの生成
	buffer_desc.ByteWidth = sizeof(DirectX::XMFLOAT4X4);
	buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	device->CreateBuffer(&buffer_desc, nullptr, constant_buffer.ReleaseAndGetAddressOf());

	return true;
}

//スカイボックスをシーンの背景として描画
void SkyBox::Render(ID3D11DeviceContext* immediate_context)
{
	//シェーダーとリソースのバインド
	skybox_shader->Apply();
	immediate_context->PSSetShaderResources(OFFSET_ZERO, RESOURCE_COUNT_1, skybox_srv.GetAddressOf());

	//ワールド行列の更新と送信
	DirectX::XMFLOAT4X4 identity_matrix;
	DirectX::XMStoreFloat4x4(&identity_matrix, DirectX::XMMatrixIdentity());
	immediate_context->VSSetConstantBuffers(OFFSET_ZERO, RESOURCE_COUNT_1, constant_buffer.GetAddressOf());

	//描画命令の発行
	UINT stride = STRIDE_SIZE;
	UINT offset = OFFSET_ZERO;
	immediate_context->IASetVertexBuffers(OFFSET_ZERO, RESOURCE_COUNT_1, vertex_buffer.GetAddressOf(), &stride, &offset);
	immediate_context->IASetIndexBuffer(index_buffer.Get(), DXGI_FORMAT_R16_UINT, OFFSET_ZERO);
	immediate_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	immediate_context->DrawIndexed(36, OFFSET_ZERO, OFFSET_ZERO);
}

//全モデル向けにIBL色情報を一括バインド
void SkyBox::BindIblTextures(ID3D11DeviceContext* immediate_context) const
{
	//IBL用テクスチャのバインド処理
	immediate_context->PSSetShaderResources(IBL_DIFFUSE_SLOT, RESOURCE_COUNT_1, diffuse_iem_srv.GetAddressOf());
	immediate_context->PSSetShaderResources(IBL_SPECULAR_SLOT, RESOURCE_COUNT_1, specular_pmrem_srv.GetAddressOf());
	immediate_context->PSSetShaderResources(IBL_LUT_SLOT, RESOURCE_COUNT_1, lut_ggx_srv.GetAddressOf());
}
