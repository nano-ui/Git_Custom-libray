#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <vector>
#include <memory>
#include <DirectXMath.h>
#include "GltfDynamicMaterial.h"

class GltfDynamicMesh
{
public:
	struct Vertex
	{
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT3 normal;
		DirectX::XMFLOAT4 tangent;
		DirectX::XMFLOAT2 texcoord;
		DirectX::XMFLOAT4 bone_weights;
		uint32_t bone_indices[4];
	};

	//コンストラクタ
	GltfDynamicMesh(
		ID3D11Device* device,
		const std::vector<Vertex>& vertices,
		const std::vector<uint32_t>& indices
	);

	//メッシュ描画
	void Render(ID3D11DeviceContext* dc);

	void SetMaterial(std::shared_ptr<GltfDynamicMaterial> mat) { material = mat; }
	std::shared_ptr<GltfDynamicMaterial> GetMaterial() const { return material; }

private:
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertex_buffer;	//頂点バッファ
	Microsoft::WRL::ComPtr<ID3D11Buffer> index_buffer;	//インデックスバッファ
	uint32_t index_count;	//インデックスの数
	std::shared_ptr<GltfDynamicMaterial> material;	//マテリアル
};
