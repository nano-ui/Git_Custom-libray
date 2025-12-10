#include "SceneConstantBuffers.h"
#include "misc.h"
#include <DirectXMath.h>

SceneConstantBuffers::SceneConstantBuffers(ID3D11Device* device)
	:device_ptr_(device)
{
}

//SceneConstantsバッファを作成
void SceneConstantBuffers::CreateConstanceBuffer()
{
	HRESULT hr{ S_OK };

	D3D11_BUFFER_DESC buffer_desc{};
	buffer_desc.ByteWidth = sizeof(SceneConstants);			//バッファサイズは構造体のサイズ
	buffer_desc.Usage = D3D11_USAGE_DYNAMIC;				//CPUから頻繁に更新
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;		//定数バッファとして使用
	buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;	//CPUが書き込みアクセスを持つ

	//シーン定数バッファの作成
	hr = device_ptr_->CreateBuffer(
		&buffer_desc, nullptr, scene_constants_buffer_.GetAddressOf()
	);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
}

//SceneConstantsのデータ更新とGPUへの転送
void SceneConstantBuffers::UpdateConstants(ID3D11DeviceContext* context, const SceneConstants& data)
{
	HRESULT hr{ S_OK };
	D3D11_MAPPED_SUBRESOURCE mapped_resource{};

	//GPUリソースをCPUがアクセスできるようにマップする
	hr = context->Map(
		scene_constants_buffer_.Get(),
		0,
		D3D11_MAP_WRITE_DISCARD,
		0,
		&mapped_resource
	);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	memcpy(mapped_resource.pData, &data, sizeof(SceneConstants));	//CPU側のデータをマップされたGPUメモリ領域にコピー

	context->Unmap(scene_constants_buffer_.Get(), 0);				//GPUリソースのマップを解除し、GPUがアクセスできるように戻す
}
