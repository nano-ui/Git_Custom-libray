#pragma once

#include <vector>
#include <memory>
#include <DirectXMath.h>
#include <wrl.h>

class GltfModelData;
struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11Buffer;
class CustomShader;

class GltfInstancingRenderer
{
public:
	static constexpr UINT MAX_INSTANCE_COUNT = 10000;	//描画できる最大インスタンス数
	static constexpr UINT INSTANCE_SLOT = 1;			//インスタンスバッファを割り当てるスロット番号

	//インスタンスごとにGPUへ送るデータ構造体
	struct instance_data
	{
		DirectX::XMFLOAT4X4 world_matrix;	//各インスタンスのワールド行列
	};

public:
	//コンストラクタ
	GltfInstancingRenderer(ID3D11Device* device);

	//デストラクタ
	~GltfInstancingRenderer();

	//複数のインスタンスを一括で描画
	void RenderInstanced(ID3D11DeviceContext* immediate_context, const GltfModelData& data, const std::vector<instance_data>& instances);

private:
	Microsoft::WRL::ComPtr<ID3D11Buffer> instance_buffer;	// 各インスタンスの行列を転送するための動的バッファ
	Microsoft::WRL::ComPtr<ID3D11Buffer> primitive_cbuffer;	// プリミティブ情報を送る定数バッファ
	std::unique_ptr<CustomShader> custom_shader;			// インスタンシング対応のシェーダー
};

