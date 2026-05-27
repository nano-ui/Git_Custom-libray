#pragma once

#include "CustomShader.h"
#include <wrl.h>
#include <d3d11.h>
#include <DirectXMath.h>

struct bloom_params_data
{
	float gaussian_sigma;	//ガウスブラーの広がり具合
	float bloom_intensity;	//ブラー結果の強度
	DirectX::XMFLOAT2 padding;
};

class BlurShader
{
public:
	//シェーダーの読み込みと定数バッファの初期生成
	bool Initialize();

	//シェーダーをパイプラインにバインド
	void Apply();

	//ブラーパラメータを設定
	void SetBloomParams(const bloom_params_data& params) { bloom_params = params; }

	//ピクセルシェーダーを取得
	ID3D11PixelShader* GetPixelShader() const { return custom_shader.GetPixelShader(); }

private:
	//DirectXデバイスを使用して定数バッファを生成
	bool CreateConstantBuffer();

private:
	CustomShader custom_shader;								//リフレクション付きの既存クラス
	Microsoft::WRL::ComPtr<ID3D11Buffer> constant_buffer;	//GPU上に配置する定数バッファのメモリを管理
	bloom_params_data bloom_params = {};					//CPU側で保持しておくブラーのパラメータデータ
};

