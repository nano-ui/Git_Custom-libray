#include "GltfStaticMaterial.h"
#include <cstring>
#include <fstream>
#include <vector>

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

//シェーダーの初期化
bool GltfStaticMaterial::InitializeShader(
	ID3D11Device* device,
	const std::wstring& vertex_shader_path,
	const std::wstring& pixel_shader_path)
{
	//頂点シェーダー読み込み
	std::ifstream vs_file(vertex_shader_path,std::ios::binary);
	if (!vs_file.is_open()) return false;
	std::vector<char> vs_data((std::istreambuf_iterator<char>(vs_file)), std::istreambuf_iterator<char>());

	HRESULT hr = device->CreateVertexShader(vs_data.data(), vs_data.size(), nullptr, vertex_shader.GetAddressOf());
	if (FAILED(hr)) return false;

	//入力レイアウトの作成
	D3D11_INPUT_ELEMENT_DESC input_desc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TANGENT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	hr = device->CreateInputLayout(input_desc, ARRAYSIZE(input_desc), vs_data.data(), vs_data.size(), input_layout.GetAddressOf());
	if (FAILED(hr)) return false;

	//ピクセルシェーダー読み込み
	std::ifstream ps_file(pixel_shader_path, std::ios::binary);
	if (!ps_file.is_open()) return false;
	std::vector<char> ps_data((std::istreambuf_iterator<char>(ps_file)), std::istreambuf_iterator<char>());

	hr = device->CreatePixelShader(ps_data.data(), ps_data.size(), nullptr, pixel_shader.GetAddressOf());
	return SUCCEEDED(hr);
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
	Microsoft::WRL::ComPtr<ID3D11Buffer> constant_buffer,
	unsigned int slot)
{
	if (!context || !constant_buffer) return;

	//シェーダーと入力レイアウトのセット
	if (vertex_shader) context->VSSetShader(vertex_shader.Get(), nullptr, 0);
	if (pixel_shader)  context->PSSetShader(pixel_shader.Get(), nullptr, 0);
	if (input_layout)  context->IASetInputLayout(input_layout.Get());

	//コンスタントバッファの更新
	D3D11_MAPPED_SUBRESOURCE mapped_resource;
	HRESULT hr = context->Map(constant_buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource);
	if (SUCCEEDED(hr))
	{
		std::memcpy(mapped_resource.pData, &material_data, sizeof(GltfStaticMaterialData));
		context->Unmap(constant_buffer.Get(), 0);
	}

	//各種スロットへのセット
	context->PSSetConstantBuffers(slot, 1, constant_buffer.GetAddressOf());

	//テクスチャをピクセルシェーダーにセット
	ID3D11ShaderResourceView* srvs[] = {
		textures.color_texture.Get(),
		textures.normal_texture.Get(),
		textures.metallic_roughness_texture.Get(),
		textures.occlusion_texture.Get(),
		textures.emissive_texture.Get()
	};
	context->PSSetShaderResources(0, 5, srvs);
}
