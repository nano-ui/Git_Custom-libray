#include "BlurEffect.h"
#include "../Graphics/shader.h"
#include <windows.h>
#include <PostProcessConstantBuffers.h>

//==================
//コンストラクタ
//==================
BlurEffect::BlurEffect(ID3D11ShaderResourceView* original_scene_srv, float sigma, float intensity)
	:original_scene_srv(original_scene_srv)
	, gaussian_sigma(sigma)
	, bloom_intensity(intensity)
{
}

//=====================================
//シェーダーと定数バッファの初期化
//=====================================
void BlurEffect::Initialize(ID3D11Device* device)
{
	//-------------------------------
	//シェーダーとバッファの生成
	//-------------------------------
	create_ps_from_cso(device, "blur_ps.cso", pixel_shader.GetAddressOf());

	D3D11_BUFFER_DESC buffer_desc{};
	buffer_desc.ByteWidth = sizeof(BloomParams);
	buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	device->CreateBuffer(&buffer_desc, nullptr, constant_buffer.GetAddressOf());
}

//===============================================
//2枚の画像をセットし、パラメータを送って描画
//===============================================
void BlurEffect::Render(
	ID3D11DeviceContext* context,
	ID3D11ShaderResourceView** source_srv,
	ID3D11RenderTargetView* dest_rtv,
	fullscreen_quad* quad)
{
	//--------------------
	//定数バッファの更新
	//--------------------
	D3D11_MAPPED_SUBRESOURCE mapped_resource;									//マッピング用の構造体を宣言します
	if (SUCCEEDED(context->Map(constant_buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource)))
	{
		BloomParams* data = reinterpret_cast<BloomParams*>(mapped_resource.pData);	//バッファのポインタを構造体型にキャストします
		data->gaussian_sigme = gaussian_sigma;										//メンバ変数のシグマ値を書き込みます
		data->bloom_intensity = bloom_intensity;									//メンバ変数の強度を書き込みます
		context->Unmap(constant_buffer.Get(), 0);									//バッファのロックを解除してGPUに転送します
	}
	const UINT slot_b1 = 1;
	context->PSSetConstantBuffers(slot_b1, 1, constant_buffer.GetAddressOf());

	//=======================================
	//2枚のテクスチャのバインドと描画手順
	//=======================================
	ID3D11ShaderResourceView* srvs[2] = { original_scene_srv, *source_srv };
	context->OMSetRenderTargets(1, &dest_rtv, nullptr);
	const UINT start_slot_t0 = 0;
	const UINT texture_count = 2;
	quad->blit(context, srvs, start_slot_t0, texture_count, pixel_shader.Get());
}


