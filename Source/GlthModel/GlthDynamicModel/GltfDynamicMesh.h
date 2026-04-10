#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <vector>
#include <memory>
#include <DirectXMath.h>

namespace tinygltf { class Model; struct Primitive; }
class GltfDynamicMaterial;

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

	//GLTFデータからメッシュを初期化
	void Initialize(
		ID3D11Device* device,
		const tinygltf::Model& model,
		const tinygltf::Primitive& primitive);

	//メッシュ描画
	void Render(ID3D11DeviceContext* dc);

	//マテリアルの設定
	void SetMaterial(std::shared_ptr<GltfDynamicMaterial> mat) { material = mat; }

	//マテリアルの取得
	std::shared_ptr<GltfDynamicMaterial> GetMaterial() const { return material; }

private:
	//GPUリソースの作成
	void CreateBuffers(
		ID3D11Device* device,
		const std::vector<Vertex>& vertices,
		const std::vector<uint32_t>& indices);

	//アクセッサからポインタを取得
	template<typename T>
	const T* GetElementPointer(const tinygltf::Model& model, int accessor_index);

private:
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertex_buffer;	//頂点バッファ
	Microsoft::WRL::ComPtr<ID3D11Buffer> index_buffer;	//インデックスバッファ
	uint32_t index_count;								//インデックスの数
	std::shared_ptr<GltfDynamicMaterial> material;		//マテリアル
};