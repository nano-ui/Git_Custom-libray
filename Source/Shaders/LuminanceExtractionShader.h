#pragma once

#include"CustomShader.h"

#include <wrl.h>
#include <d3d11.h>
#include <DirectXMath.h>

struct threshold_constant_data
{
	float brightness_threshold;	//明るさ
	DirectX::XMFLOAT3 padding;	//16バイトに合わせるための詰め物
};

class LuminanceExtractionShader
{
public:
	//シェーダーの読み込みと定数バッファの初期生成
	bool Initialize();

	//定数バッファをパイプラインにバインド
	void Apply();

	//高輝度抽出の閾値を設定
	void SetThreshold(float threshodl) { threshold_data.brightness_threshold = threshodl; }

	//ピクセルシェーダーを取得
	ID3D11PixelShader* GetPixelShader()const { return costom_shader.GetPixelShader(); }

private:
	//DirectXデバイスを使用して定数バッファを生成
	bool CreateConstantBuffer();

private:
	CustomShader costom_shader;								//リフレクション付きの既存クラス
	Microsoft::WRL::ComPtr<ID3D11Buffer> constant_buffer;	//GPU上に配置する定数バッファのメモリを管理
	threshold_constant_data threshold_data = {};			//CPU側で保持しておく明るさのパラメータデータ
};	

