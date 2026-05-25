#include "LuminanceExtractionEffect.h"
#include "../Graphics/shader.h"

//====================
//コンストラクタ
//====================
LuminanceExtractionEffect::LuminanceExtractionEffect(float threshold)
	:threshold(threshold)
{
}

//============
//初期化
//============
void LuminanceExtractionEffect::Initialize(ID3D11Device* device)
{
	create_ps_from_cso(device, "luminance_extraction_ps.cso", pixel_shader_.GetAddressOf());	//シェーダー読み込み

	//----------------------
	//定数バッファの生成
	//----------------------
	D3D11_BUFFER_DESC buffer_desc{};
	buffer_desc.ByteWidth = sizeof(ThresholdBuffer);
	buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	device->CreateBuffer(&buffer_desc, nullptr, constant_buffer_.GetAddressOf());
}

//===========
//描画処理
//===========
void LuminanceExtractionEffect::Render(
	ID3D11DeviceContext* context,
	ID3D11ShaderResourceView** source_srv,
	ID3D11RenderTargetView* dest_rtv,
	fullscreen_quad* quad)
{
	//-------------------------------------------------------
	//定数バッファに最新のしきい値を書き込んでGPUへ送る
	//-------------------------------------------------------
	D3D11_MAPPED_SUBRESOURCE mapped_resource;
	if (SUCCEEDED(context->Map(constant_buffer_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource)))
	{
		ThresholdBuffer* data = reinterpret_cast<ThresholdBuffer*>(mapped_resource.pData);
		data->brightness_threshold = threshold;
		context->Unmap(constant_buffer_.Get(), 0);
	}

	context->PSSetConstantBuffers(0, 1, constant_buffer_.GetAddressOf());

	//-----------------------------------
	//描画先のセットと板ポリゴンの描画
	//-----------------------------------
	context->OMSetRenderTargets(1, &dest_rtv, nullptr);
	quad->blit(context, source_srv, 0, 1, pixel_shader_.Get());
}
