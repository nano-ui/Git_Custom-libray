#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>

class GltfDynamicMaterial
{
public:
	GltfDynamicMaterial();

	//シェーダーへのバインド
	void Bind(ID3D11DeviceContext* dc);

private:
	DirectX::XMFLOAT4 base_color;	//マテリアルの基本色
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> base_map;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> normal_map;
};
