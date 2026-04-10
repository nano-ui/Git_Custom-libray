#include "GltfDynamicMaterial.h"

//=================================================================
//コンストラクタ
//=================================================================
GltfDynamicMaterial::GltfDynamicMaterial()
{
	base_color = { 1.0f, 1.0f, 1.0f, 1.0f };
}

//=================================================================
//GLTFデータからマテリアルを初期化
//=================================================================
void GltfDynamicMaterial::Initialize(
	ID3D11Device* device,
	const tinygltf::Model& model,
	const tinygltf::Material& gltf_mat,
	const std::string& directory)
{
}

//=================================================================
//シェーダーへのバインド
//=================================================================
void GltfDynamicMaterial::Bind(ID3D11DeviceContext* dc)
{
	//シェーダーリソースをピクセルシェーダーにセット
	ID3D11ShaderResourceView* srvs[] = {
		base_map.Get(),
		normal_map.Get(),
	};

	dc->PSSetShaderResources(0, _countof(srvs), srvs);
}
