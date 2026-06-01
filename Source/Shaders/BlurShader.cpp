#include "BlurShader.h"
#include "Graphics.h"

//マジックナンバーを避けるための定数定義群
static constexpr UINT constant_buffer_slot_bloom = 1; //register(b1) に対応するスロット番号
static constexpr UINT buffer_offset = 0;              //バッファ更新時の先頭オフセット値
static constexpr UINT buffer_depth = 0;               //バッファ更新時の深度値

//==============================================
//シェーダーの読み込みと定数バッファの初期生成
//==============================================
bool BlurShader::Initialize()
{
	//シェーダーの初期化
	bool is_shader_initialized = custom_shader.Initialize("fullscreen_quad_vs.cso", "blur_ps.cso");
	if (!is_shader_initialized)return false;

	//定数バッファの生成
	bool is_buffer_created = CreateConstantBuffer();
	if (!is_buffer_created)return false;

	return true;
}

//=====================================
//シェーダーをパイプラインにバインド
//=====================================
void BlurShader::Apply()
{
	//GPUバッファの更新
	auto context = Graphics::Instance().GetContext();
	context->UpdateSubresource(constant_buffer.Get(), buffer_offset, nullptr, &bloom_params, buffer_offset, buffer_depth);

	//パイプラインへのバインド
	context->PSSetConstantBuffers(constant_buffer_slot_bloom, 1, constant_buffer.GetAddressOf());
	custom_shader.Apply();
}

//===============================================
//DirectXデバイスを使用して定数バッファを生成
//===============================================
bool BlurShader::CreateConstantBuffer()
{
	//バッファ設定の構築
	D3D11_BUFFER_DESC buffer_desc = {};
	buffer_desc.ByteWidth = sizeof(bloom_params_data);
	buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	buffer_desc.CPUAccessFlags = 0;

	//バッファの生成
	auto device = Graphics::Instance().GetDevice();
	HRESULT hr = device->CreateBuffer(&buffer_desc, nullptr, constant_buffer.GetAddressOf());
	if (FAILED(hr))return false;

	return true;
}
