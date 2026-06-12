#pragma once

#include "GltfModelData.h"
#include <wrl.h>
#include <memory>
#include "../Shaders/CustomShader.h"

class GltfModel;

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

private:
	//1回の描画に必要な情報
	struct RenderCommand
	{
		DirectX::XMFLOAT4X4 world_matrix;					//プリミティブごとのワールド行列
		const GltfModelData::mesh::primitive* primitive;	//描画するプリミティブのポインタ
		int skin_index;										//スキン情報の番号
		DirectX::XMFLOAT4X4 node_global_matrix;				//パーツ自身の行列
	};

	//2つの描画コマンドのマテリアルIDを比較
	struct CompartMaterial
	{
		bool operator()(const RenderCommand& a, RenderCommand& b)const
		{
			return a.primitive->material < b.primitive->material;
		}
	};

public:
	//デバイスを受け取り描画リソースを初期化
	GltfModelRenderer(ID3D11Device* device);

	//GPUに命令を送りモデルを描画
	void Render(
		ID3D11DeviceContext* immediate_context,
		const GltfModel& model,
		const DirectX::XMFLOAT4X4& world,
		ID3D11SamplerState* const* sampler_states);

private:
	//描画対象のノードを階層順に巡回
	void TraverseNodeForRender(int node_index, ID3D11DeviceContext* immediate_context, const GltfModelData& data, const std::vector<GltfModelData::node>& nodes, const DirectX::XMFLOAT4X4& world, ID3D11SamplerState** sampler_states);

	//描画対象のノードを巡回し、プリミティブ情報をフラットな配列に集める
	void GetherRenderCommands(
		int node_index,
		const GltfModelData& data,
		const std::vector<GltfModelData::node>& nodes,
		const DirectX::XMFLOAT4X4& world,
		std::vector<RenderCommand>& commands);

private:
	Microsoft::WRL::ComPtr<ID3D11Buffer> primitive_cbuffer;				//プリミティブの情報を送るための定数バッファ
	Microsoft::WRL::ComPtr<ID3D11Buffer> primitive_joint_cbuffer;		//スキン（ボーン）行列を送るための定数バッファ
	std::unique_ptr<CustomShader> custom_shader;
};

