#pragma once

#include "../Shaders/PostProcess.h"
#include <DirectXMath.h>

//ブルーム処理における、ガウスぼかしの強さやブルームの強度を格納
//struct BloomParams
//{
//	float gaussian_sigme;	//ぼかしの広がり
//	float bloom_intensity;	//ブルームの最終的な強度
//	DirectX::XMFLOAT2 paddings;
//};

class BlurEffect :public PostProcess
{
public:
	//コンストラクタ
	BlurEffect(ID3D11ShaderResourceView* original_scene_srv, float sigma, float intensity);

	//デストラクタ
	~BlurEffect() override = default;

	//シェーダーと定数バッファの初期化
	void Initialize(ID3D11Device* device)override;

	//2枚の画像をセットし、パラメータを送って描画
	void Render(
		ID3D11DeviceContext* context,
		ID3D11ShaderResourceView** source_srv,
		ID3D11RenderTargetView* dest_rtv,
		fullscreen_quad* quad) override;

private:
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixel_shader;
	Microsoft::WRL::ComPtr<ID3D11Buffer> constant_buffer;
	ID3D11ShaderResourceView* original_scene_srv;
	float gaussian_sigma;
	float bloom_intensity;
};

