#pragma once

#include "GltfModelData.h"
#include <wrl.h>
#include <memory>
#include "../Shaders/CustomShader.h"

class GltfModelRenderer
{
public:
	static constexpr UINT SHADER_SLOT_0 = 0;	//シェーダースロット0番
	static constexpr UINT SHADER_SLOT_1 = 1;	//シェーダースロット1番
	static constexpr UINT SHADER_SLOT_2 = 2;	//シェーダースロット2番
	static constexpr UINT RESOURCE_COUNT_1 = 1;	//一度にバインドするリソースの数
	static constexpr UINT OFFSET_ZERO = 0;		//バッファ等のオフセット初期値
	static constexpr UINT TEXTURE_COUNT_5 = 5;	//マテリアルが保持するテクスチャの総数
	static constexpr UINT SAMPLER_COUNT_3 = 3;	//使用するサンプラーステートの総数

public:
	//デバイスを受け取り描画リソースを初期化
	GltfModelRenderer(ID3D11Device* device);

	//GPUに命令を送りモデルを描画
	void Render(ID3D11DeviceContext* immediate_context, const GltfModelData& data, const DirectX::XMFLOAT4X4& world, const std::vector<GltfModelData::node>& animated_nodes);

private:
	//1次元配列によるフラットループ描画
	void RenderFlatLoop(ID3D11DeviceContext* immediate_context, const GltfModelData& data, const std::vector<GltfModelData::node>& nodes_to_render, const DirectX::XMFLOAT4X4& world);

private:
	Microsoft::WRL::ComPtr<ID3D11Buffer> primitive_cbuffer;				//プリミティブの情報を送るための定数バッファ
	Microsoft::WRL::ComPtr<ID3D11Buffer> primitive_joint_cbuffer;		//スキン（ボーン）行列を送るための定数バッファ
	std::unique_ptr<CustomShader> custom_shader;

};

