#include "PostProcessConstantBuffers.h"
#include "misc.h"

PostProcessConstantBuffers::PostProcessConstantBuffers(ID3D11Device* device)
	:device_ptr_(device)
{
}

//ThresholdBufferとBloomParamsBufferを作成
void PostProcessConstantBuffers::CreateConstantBuffers()
{
	HRESULT hr{ S_OK };

	D3D11_BUFFER_DESC buffer_desc{};
	buffer_desc.Usage = D3D11_USAGE_DYNAMIC;			//リソース使用方法の設定(CPUから頻繁にデータを更新)
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;	//リソースの用途の設定(定数バッファとしてGPUに使用)
	buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;//CPUがこのバッファに書き込みアクセスを行うことを許可

	//----------------------
	//ThresholdBufferの作成
	//----------------------
	buffer_desc.ByteWidth = sizeof(ThresholdBuffer);//ThresholdBufferのサイズ設定
	hr = device_ptr_->CreateBuffer(&buffer_desc, nullptr, threshold_buffer_.GetAddressOf());//ThresholdBufferの作成
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));		//エラーチェック

	//------------------------
	//BloomParamsBufferの作成
	//------------------------
	buffer_desc.ByteWidth = sizeof(BloomParams);//BloomParamsBufferのサイズ設定
	hr = device_ptr_->CreateBuffer(&buffer_desc, nullptr, bloom_params_buffer_.GetAddressOf());//BloomParamsBufferの作成
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));	//エラーチェック
}

//ブルーム関連のデータ更新とGPUへの転送
void PostProcessConstantBuffers::UpdateConstants(ID3D11DeviceContext* context, const ThresholdBuffer& threshold_data, const BloomParams& bloom_data)
{
	HRESULT hr{ S_OK };
	D3D11_MAPPED_SUBRESOURCE mapped_resource{};

	//-----------------------
	//ThresholdBufferの更新
	//-----------------------

	//ThresholdBufferのマップ(CPUからアクセス可能なメモリ領域にマップ(マッピング))
	hr = context->Map(
		threshold_buffer_.Get(),
		0,
		D3D11_MAP_WRITE_DISCARD,
		0,
		&mapped_resource
	);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));//エラーチェック
	memcpy(mapped_resource.pData, &threshold_data, sizeof(ThresholdBuffer));//データ転送
	context->Unmap(threshold_buffer_.Get(), 0);//ThresholdBufferのアンマップ(CPUによるアクセスを終了し、GPUがバッファの新しいデータを使用できるようにする)

	//------------------------
	//BloomParamsBufferの更新
	//------------------------

	//BloomParamsBufferのマップ（ロック）(CPUからアクセス可能なメモリ領域にマップ(マッピング))
	hr = context->Map(
		bloom_params_buffer_.Get(),
		0,
		D3D11_MAP_WRITE_DISCARD,
		0,
		&mapped_resource
	);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));//エラーチェック
	memcpy(mapped_resource.pData, &bloom_data, sizeof(BloomParams));//データ転送
	context->Unmap(bloom_params_buffer_.Get(), 0);//BloomParamsBufferのアンマップ(CPUによるアクセスを終了し、GPUがバッファの新しいデータを使用できるようにする)
}
