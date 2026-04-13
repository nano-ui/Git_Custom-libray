#pragma once
#include <memory>
#include "GltfDynamicModelResource.h"
#include "GltfDynamicModelData.h"
#include "../Graphics/Graphics.h"

class GltfDynamicModel
{
public:
	GltfDynamicModel(ID3D11Device* device, std::shared_ptr<GltfDynamicModelResource> resource_);

	//アニメーション更新処理
	void Update(float delta_time);

	//アニメーション切り替え処理
	void ChangeAnimation(const std::string& name);

	//描画処理
	void Draw(ID3D11DeviceContext* dc);

	void SetWorldMaterix(const DirectX::XMFLOAT4X4& world) { wordl_matrix = world; }
private:
	//定数バッファの更新
	void UpdateConstantBuffer(ID3D11DeviceContext* dc, const DirectX::XMFLOAT4& color);

private:
	std::shared_ptr<GltfDynamicModelResource> resource;	//共有データ
	std::vector<GltfBone> bones;	//インスタンス専用のボーン状態
	DirectX::XMFLOAT4X4 wordl_matrix;	//ワールド行列
	Microsoft::WRL::ComPtr<ID3D11Buffer> object_cb;	//オブジェクト定数バッファ
	int current_animation_index;	//現在再生中のアニメーション番号
};

