#pragma once
#include <memory>
#include "GltfDynamicModelResource.h"
#include "../Graphics/Graphics.h"

class GltfDynamicModel
{
public:
	GltfDynamicModel(std::shared_ptr<GltfDynamicModelResource> resource_);

	//描画処理
	void Draw(ID3D11DeviceContext* dc);

	void SetWorldMaterix(const DirectX::XMFLOAT4X4& world) { wordl_matrix = world; }

private:
	std::shared_ptr<GltfDynamicModelResource> resource;	//共有データ
	DirectX::XMFLOAT4X4 wordl_matrix;	//ワールド行列
	Microsoft::WRL::ComPtr<ID3D11Buffer> object_cb;	//オブジェクト定数バッファ

	//定数バッファの更新
	void UpdateConstantBuffer(ID3D11DeviceContext* dc, const DirectX::XMFLOAT4& color);
};

