#include "GltfStaticMaterial.h"

// 定数バッファ作成
bool GltfStaticMaterial::CreateConstantBuffer(ID3D11Device* device)
{
	if (!device)
	{
		return false;
	}

	D3D11_BUFFER_DESC desc = {};
	desc.ByteWidth      = sizeof(MaterialConstants);
	desc.Usage          = D3D11_USAGE_DEFAULT;
	desc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
	desc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA subresource = {};
	subresource.pSysMem = &data;

	HRESULT hr = device->CreateBuffer(&desc, &subresource, constant_buffer.GetAddressOf());
	if (FAILED(hr))
	{
		OutputDebugStringA("ERROR: GltfStaticMaterial: Failed to create constant buffer\n");
		return false;
	}

	return true;
}

// マテリアルデータとテクスチャをピクセルシェーダーへ適用
void GltfStaticMaterial::ApplyToShader(ID3D11DeviceContext* context) const
{
	if (!context)
	{
		return;
	}

	// 定数バッファを更新して b2 にバインド
	if (constant_buffer)
	{
		context->UpdateSubresource(constant_buffer.Get(), 0, nullptr, &data, 0, 0);
		context->PSSetConstantBuffers(2, 1, constant_buffer.GetAddressOf());
	}

	// テクスチャを t0〜t4 に設定
	ID3D11ShaderResourceView* srvs[5] =
	{
		color_texture.Get(),             // t0
		normal_texture.Get(),            // t1
		metallic_roughness_texture.Get(),// t2
		occlusion_texture.Get(),         // t3
		emissive_texture.Get(),          // t4
	};
	context->PSSetShaderResources(0, 5, srvs);
}
