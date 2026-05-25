#include "BasicShader.h"
#include "../Graphics/shader.h"

//==================
//シェーダー生成
//==================
void BasicShader::Initialize(ID3D11Device* device)
{
	create_ps_from_cso(device, filename.c_str(), pixel_shader.GetAddressOf());	//シェーダーファイルの読み込み
}

//==========================
//シェーダーを適用して描画
//==========================
void BasicShader::Render(ID3D11DeviceContext* context, ID3D11ShaderResourceView** source_srv, ID3D11RenderTargetView* dest_rtv, fullscreen_quad* quad)
{
	context->OMSetRenderTargets(1, &dest_rtv, nullptr);			//描画先のレンダーターゲットを一つだけセット
	quad->blit(context, source_srv, 0, 1, pixel_shader.Get());	//ソーステクスチャにシェーダーを適用し、dest_rtvへ描画
}
