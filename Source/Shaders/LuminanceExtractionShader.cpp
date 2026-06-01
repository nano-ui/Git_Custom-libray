#include "LuminanceExtractionShader.h"
#include "Graphics.h"

//マジックナンバーを避けるための定数定義群
static constexpr UINT constant_buffer_slot_threshold = 0; //register(b0) に対応するスロット番号
static constexpr UINT buffer_offset = 0;                  //バッファ更新時の先頭オフセット値
static constexpr UINT buffer_depth = 0;                   //一般バッファ更新用の深度値

//シェーダーの読み込みと定数バッファの初期生成
bool LuminanceExtractionShader::Initialize()
{
	//シェーダーバイナリのロードと自動解析
	bool is_shader_initialized = costom_shader.Initialize("fullscreen_quad_vs.cso", "luminance_extraction_ps.cso");
	if (!is_shader_initialized)return false;

	//専用定数バッファの生成
	bool is_buffer_created = CreateConstantBuffer();
	if (!is_buffer_created)return false;

	return true;
}

//定数バッファをパイプラインにバインド
void LuminanceExtractionShader::Apply()
{
	//GPUデータの更新とパイプラインへの設定
	auto context = Graphics::Instance().GetContext();
	context->UpdateSubresource(constant_buffer.Get(), buffer_offset, nullptr, &threshold_data, buffer_offset, buffer_depth);
	context->PSSetConstantBuffers(constant_buffer_slot_threshold, 1, constant_buffer.GetAddressOf());
	costom_shader.Apply();
}

//DirectXデバイスを使用して定数バッファを生成
bool LuminanceExtractionShader::CreateConstantBuffer()
{
	//バッファの生成に必要な設定構造体を初期化
	D3D11_BUFFER_DESC buffer_desc = {};
	buffer_desc.ByteWidth = sizeof(threshold_constant_data);
	buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	buffer_desc.CPUAccessFlags = 0;
	buffer_desc.MiscFlags = 0;
	buffer_desc.StructureByteStride = 0;

	//GraphicsクラスからDirectXデバイスを取得
	auto device = Graphics::Instance().GetDevice();

	//デバイスを使って定数バッファを生成
	HRESULT hr = device->CreateBuffer(&buffer_desc, nullptr, constant_buffer.GetAddressOf());
	if (FAILED(hr))return false;

	return true;
}