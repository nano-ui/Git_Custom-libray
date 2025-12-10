#pragma once

#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>

//ブルーム処理における、輝度抽出の閾値を格納
struct ThresholdBuffer
{
	float brightness_threshold;
	float padding[3];
};

//ブルーム処理における、ガウスぼかしの強さやブルームの強度を格納
struct BloomParams
{
	float gaussian_sigme;	//ぼかしの広がり
	float bloom_intensity;	//ブルームの最終的な強度
	DirectX::XMFLOAT2 paddings;
};

class PostProcessConstantBuffers
{
public:
	PostProcessConstantBuffers(ID3D11Device* device);

	//ThresholdBufferとBloomParamsの各定数バッファオブジェクトを作成
	void CreateConstantBuffers();

	//ブルーム関連のパラメータをCPU側で更新し、GPUに転送
	void UpdateConstants(ID3D11DeviceContext* context, const ThresholdBuffer& threshold_data, const BloomParams& bloom_data);

	//輝度閾値バッファオブジェクトを取得
	Microsoft::WRL::ComPtr<ID3D11Buffer>& GetThresholdBuffer() { return threshold_buffer_; }

	//ブルームパラメータバッファオブジェクトを取得
	Microsoft::WRL::ComPtr<ID3D11Buffer>& GetBloomParamsBuffer() { return bloom_params_buffer_; }
private:
	ID3D11Device* device_ptr_;
	Microsoft::WRL::ComPtr<ID3D11Buffer> threshold_buffer_;
	Microsoft::WRL::ComPtr<ID3D11Buffer> bloom_params_buffer_;
};

