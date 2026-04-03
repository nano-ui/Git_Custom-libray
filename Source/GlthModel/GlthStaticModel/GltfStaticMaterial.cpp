#include "GltfStaticMaterial.h"
#include <cstring>

//コンストラクタ
GltfStaticMaterial::GltfStaticMaterial()
{
	//デフォルト値の設定
	material_data.color = { 1.0f, 1.0f, 1.0f, 1.0f };
	material_data.emissive = { 0.0f,0.0f,0.0f };
	material_data.metallic = 0.0f;
	material_data.roughness = 0.5f;
	material_data.normal_scale = 1.0f;
	material_data.occlusion_strength = 1.0f;
	material_data.padding[0] = 0;
	material_data.padding[1] = 0;
}

//マテリアルデータの設定
void GltfStaticMaterial::SetMaterialData(const GltfStaticMaterialData& material_data_in)
{
	material_data = material_data_in;
}

//マテリアルデータの取得
const GltfStaticMaterialData& GltfStaticMaterial::GetMaterialData() const
{
	return material_data;
}

//テクスチャの設定
void GltfStaticMaterial::SetTextures(const GltfStaticMaterialTextures& textures_in)
{
	textures = textures_in;
}

//テクスチャの取得
const GltfStaticMaterialTextures& GltfStaticMaterial::GetTextures() const
{
	return textures;
}

//マテリアル名の設定
void GltfStaticMaterial::SetMaterialName(const std::string& name)
{
	material_name = name;
}

//マテリアル名の取得
const std::string& GltfStaticMaterial::GetMaterialName() const
{
	return material_name;
}

//コンスタントバッファの作成
Microsoft::WRL::ComPtr<ID3D11Buffer> GltfStaticMaterial::CreateConstantBuffer(ID3D11Device* device)
{
	if (!device)
	{
		return nullptr;
	}

	D3D11_BUFFER_DESC buffer_desc = {};
	buffer_desc.ByteWidth = sizeof(GltfStaticMaterialData);
	buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	D3D11_SUBRESOURCE_DATA subresource_data = {};
	subresource_data.pSysMem = &material_data;

	Microsoft::WRL::ComPtr<ID3D11Buffer> buffer;
	HRESULT hr = device->CreateBuffer(&buffer_desc, &subresource_data, buffer.GetAddressOf());

	if (FAILED(hr))
	{
		return nullptr;
	}

	return buffer;
}

//マテリアルをシェーダーに適用
void GltfStaticMaterial::ApplyToShader(
	ID3D11DeviceContext* context,
	Microsoft::WRL::ComPtr<ID3D11Buffer> constnt_buffer,
	unsigned int slot)
{
	if (!context || !constnt_buffer)
	{
		return;
	}

	//コンスタントバッファにマテリアルデータを更新
	D3D11_MAPPED_SUBRESOURCE mapped_resource;
	HRESULT hr = context->Map(constnt_buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource);

	if (FAILED(hr))
	{
		return;
	}

	std::memcpy(mapped_resource.pData, &material_data, sizeof(GltfStaticMaterialData));
	context->Unmap(constnt_buffer.Get(), 0);

	//コンスタントバッファをピクセルシェーダーにセット
	context->PSSetConstantBuffers(slot, 1, constnt_buffer.GetAddressOf());

	//テクスチャをピクセルシェーダーにセット
	if (textures.color_texture)
	{
		context->PSSetShaderResources(slot, 1, textures.color_texture.GetAddressOf());
	}
	if (textures.normal_texture)
	{
		context->PSSetShaderResources(slot + 1, 1, textures.normal_texture.GetAddressOf());
	}
	if (textures.metallic_roughness_texture)
	{
		context->PSSetShaderResources(slot + 2, 1, textures.metallic_roughness_texture.GetAddressOf());
	}
	if (textures.occlusion_texture)
	{
		context->PSSetShaderResources(slot + 3, 1, textures.occlusion_texture.GetAddressOf());
	}
	if (textures.emissive_texture)
	{
		context->PSSetShaderResources(slot + 4, 1, textures.emissive_texture.GetAddressOf());
	}	
}
