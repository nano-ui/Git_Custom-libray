#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_MSC_SECURE_CRT

#include "GlthStaticModel.h"
#include "GltfStaticMaterial.h"

#include <filesystem>
#include <fstream>
#include <algorithm>

// シェーダー作成ヘルパー
#include "../../Graphics/shader.h"
#include <WICTextureLoader.h>

// GLTF ファイル読み込み
std::shared_ptr<GltfModel> GlthStaticModel::LoadModel(
ID3D11Device* device,
const std::string& filename)
{
// ファイルパスの確認
std::filesystem::path filepath(filename);
if (!std::filesystem::exists(filepath))
{
OutputDebugStringA("ERROR: GLTF file not found\n");
return nullptr;
}

// GLTF ファイル読み込み
tinygltf::Model gltf_model;
if (!LoadGltfFile(filepath.string(), gltf_model))
{
OutputDebugStringA("ERROR: Failed to load GLTF file\n");
return nullptr;
}

// モデルデータ作成
auto model = std::make_shared<GltfModel>();
DirectX::XMStoreFloat4x4(&model->transform, DirectX::XMMatrixIdentity());

// メッシュデータ抽出
ExtractMeshData(device, gltf_model, model);

if (model->meshes.empty())
{
OutputDebugStringA("WARNING: No meshes extracted from GLTF file\n");
}

// マテリアルデータ抽出
std::string base_path = filepath.parent_path().string();
ExtractMaterialData(device, gltf_model, base_path, model);

// gltf_static_mesh シェーダー読み込み
D3D11_INPUT_ELEMENT_DESC input_element_desc[] =
{
{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
  D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
  D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0,
  D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

HRESULT hr = create_vs_from_cso(device,
"gltf_static_mesh_vs.cso",
model->vertex_shader.GetAddressOf(),
model->input_layout.GetAddressOf(),
input_element_desc,
ARRAYSIZE(input_element_desc));
if (FAILED(hr))
{
OutputDebugStringA("ERROR: Failed to create gltf_static_mesh vertex shader\n");
}

hr = create_ps_from_cso(device,
"gltf_static_mesh_ps.cso",
model->pixel_shader.GetAddressOf());
if (FAILED(hr))
{
OutputDebugStringA("ERROR: Failed to create gltf_static_mesh pixel shader\n");
}

// オブジェクト定数バッファ作成 (b0: world matrix)
D3D11_BUFFER_DESC cb_desc = {};
cb_desc.ByteWidth      = sizeof(DirectX::XMFLOAT4X4);
cb_desc.Usage          = D3D11_USAGE_DEFAULT;
cb_desc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
cb_desc.CPUAccessFlags = 0;

hr = device->CreateBuffer(&cb_desc, nullptr, model->object_constant_buffer.GetAddressOf());
if (FAILED(hr))
{
OutputDebugStringA("ERROR: Failed to create object constant buffer\n");
}

return model;
}

// GLTF ファイル読み込み
bool GlthStaticModel::LoadGltfFile(
const std::string& filepath,
tinygltf::Model& out_model)
{
tinygltf::TinyGLTF loader;
std::string error, warning;

std::string extension = std::filesystem::path(filepath).extension().string();

// 小文字に統一して拡張子確認
std::transform(extension.begin(), extension.end(), extension.begin(),
[](unsigned char c) { return std::tolower(c); });

bool result = false;

if (extension == ".glb")
{
result = loader.LoadBinaryFromFile(&out_model, &error, &warning, filepath);
}
else if (extension == ".gltf")
{
result = loader.LoadASCIIFromFile(&out_model, &error, &warning, filepath);
}
else
{
OutputDebugStringA("ERROR: Unsupported file format. Only .glb and .gltf are supported.\n");
return false;
}

if (!warning.empty())
{
OutputDebugStringA("WARNING: ");
OutputDebugStringA(warning.c_str());
OutputDebugStringA("\n");
}

if (!result)
{
OutputDebugStringA("ERROR: ");
OutputDebugStringA(error.c_str());
OutputDebugStringA("\n");
return false;
}

return true;
}

// POSITION データ抽出
void GlthStaticModel::ExtractPositionData(
const tinygltf::Model& model,
const tinygltf::Primitive& primitive,
std::vector<GlthVertex>& vertices)
{
auto positionIt = primitive.attributes.find("POSITION");
if (positionIt == primitive.attributes.end())
{
return;
}

const tinygltf::Accessor&   posAccessor   = model.accessors[positionIt->second];
const tinygltf::BufferView& posBufferView = model.bufferViews[posAccessor.bufferView];
const tinygltf::Buffer&     posBuffer     = model.buffers[posBufferView.buffer];

const float* posData = reinterpret_cast<const float*>(
posBuffer.data.data() + posBufferView.byteOffset + posAccessor.byteOffset);

for (size_t i = 0; i < posAccessor.count; i++)
{
vertices[i].position.x = posData[i * 3];
vertices[i].position.y = posData[i * 3 + 1];
vertices[i].position.z = posData[i * 3 + 2];
}
}

// NORMAL データ抽出
void GlthStaticModel::ExtractNormalData(
const tinygltf::Model& model,
const tinygltf::Primitive& primitive,
std::vector<GlthVertex>& vertices)
{
auto normalIt = primitive.attributes.find("NORMAL");
if (normalIt == primitive.attributes.end())
{
// NORMAL がない場合はデフォルト値を設定
for (auto& vertex : vertices)
{
vertex.normal = { 0.0f, 1.0f, 0.0f };
}
return;
}

const tinygltf::Accessor&   normalAccessor   = model.accessors[normalIt->second];
const tinygltf::BufferView& normalBufferView = model.bufferViews[normalAccessor.bufferView];
const tinygltf::Buffer&     normalBuffer     = model.buffers[normalBufferView.buffer];

const float* normalData = reinterpret_cast<const float*>(
normalBuffer.data.data() + normalBufferView.byteOffset + normalAccessor.byteOffset);

for (size_t i = 0; i < normalAccessor.count; i++)
{
vertices[i].normal.x = normalData[i * 3];
vertices[i].normal.y = normalData[i * 3 + 1];
vertices[i].normal.z = normalData[i * 3 + 2];
}
}

// TEXCOORD データ抽出
void GlthStaticModel::ExtractTexCoordData(
const tinygltf::Model& model,
const tinygltf::Primitive& primitive,
std::vector<GlthVertex>& vertices)
{
auto texcoordIt = primitive.attributes.find("TEXCOORD_0");
if (texcoordIt == primitive.attributes.end())
{
// TEXCOORD がない場合はデフォルト値を設定
for (auto& vertex : vertices)
{
vertex.texcoord = { 0.0f, 0.0f };
}
return;
}

const tinygltf::Accessor&   texAccessor   = model.accessors[texcoordIt->second];
const tinygltf::BufferView& texBufferView = model.bufferViews[texAccessor.bufferView];
const tinygltf::Buffer&     texBuffer     = model.buffers[texBufferView.buffer];

const float* texData = reinterpret_cast<const float*>(
texBuffer.data.data() + texBufferView.byteOffset + texAccessor.byteOffset);

for (size_t i = 0; i < texAccessor.count; i++)
{
vertices[i].texcoord.x = texData[i * 2];
vertices[i].texcoord.y = texData[i * 2 + 1];
}
}

// メッシュデータ抽出
void GlthStaticModel::ExtractMeshData(
ID3D11Device* device,
const tinygltf::Model& gltf_model,
std::shared_ptr<GltfModel> model)
{
if (gltf_model.meshes.size() == 0)
{
OutputDebugStringA("WARNING: No meshes found in GLTF model\n");
return;
}

for (const auto& gltfMesh : gltf_model.meshes)
{
for (const auto& primitive : gltfMesh.primitives)
{
GlthMesh mesh;

// マテリアルインデックスを記録
mesh.material_index = primitive.material;

// インデックスデータ抽出
if (primitive.indices >= 0)
{
const tinygltf::Accessor&   indexAccessor   = gltf_model.accessors[primitive.indices];
const tinygltf::BufferView& indexBufferView = gltf_model.bufferViews[indexAccessor.bufferView];
const tinygltf::Buffer&     indexBuffer     = gltf_model.buffers[indexBufferView.buffer];

const void* indexData = indexBuffer.data.data() +
indexBufferView.byteOffset +
indexAccessor.byteOffset;

mesh.indices.resize(indexAccessor.count);

if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
{
memcpy(mesh.indices.data(), indexData,
indexAccessor.count * sizeof(uint32_t));
}
else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
{
// uint16 -> uint32 に変換
const uint16_t* indexData16 = static_cast<const uint16_t*>(indexData);
for (size_t i = 0; i < indexAccessor.count; i++)
{
mesh.indices[i] = indexData16[i];
}
}
}

// 頂点データ抽出
auto positionIt = primitive.attributes.find("POSITION");
if (positionIt != primitive.attributes.end())
{
const tinygltf::Accessor& posAccessor = gltf_model.accessors[positionIt->second];
mesh.vertices.resize(posAccessor.count);

// 各頂点情報を抽出
ExtractPositionData(gltf_model, primitive, mesh.vertices);
ExtractNormalData(gltf_model, primitive, mesh.vertices);
ExtractTexCoordData(gltf_model, primitive, mesh.vertices);
}

// バッファ作成
if (!mesh.vertices.empty())
{
mesh.vertex_buffer = CreateVertexBuffer(device,
mesh.vertices.data(),
mesh.vertices.size() * sizeof(GlthVertex));

if (!mesh.vertex_buffer)
{
OutputDebugStringA("ERROR: Failed to create vertex buffer\n");
continue;
}
}

if (!mesh.indices.empty())
{
mesh.index_buffer = CreateIndexBuffer(device,
mesh.indices.data(),
mesh.indices.size() * sizeof(uint32_t));

if (!mesh.index_buffer)
{
OutputDebugStringA("ERROR: Failed to create index buffer\n");
continue;
}
}

model->meshes.push_back(mesh);
}
}
}

// マテリアルデータ抽出
void GlthStaticModel::ExtractMaterialData(
ID3D11Device* device,
const tinygltf::Model& gltf_model,
const std::string& base_path,
std::shared_ptr<GltfModel> model)
{
for (const auto& gltf_material : gltf_model.materials)
{
auto material = std::make_shared<GltfStaticMaterial>();

// PBR Metallic Roughness パラメータ
const auto& pbr = gltf_material.pbrMetallicRoughness;

if (pbr.baseColorFactor.size() >= 4)
{
material->data.color_factor = {
static_cast<float>(pbr.baseColorFactor[0]),
static_cast<float>(pbr.baseColorFactor[1]),
static_cast<float>(pbr.baseColorFactor[2]),
static_cast<float>(pbr.baseColorFactor[3])
};
}

material->data.metallic_factor  = static_cast<float>(pbr.metallicFactor);
material->data.roughness_factor = static_cast<float>(pbr.roughnessFactor);

// エミッシブファクター
if (gltf_material.emissiveFactor.size() >= 3)
{
material->data.emissive_factor = {
static_cast<float>(gltf_material.emissiveFactor[0]),
static_cast<float>(gltf_material.emissiveFactor[1]),
static_cast<float>(gltf_material.emissiveFactor[2])
};
}

// ノーマルスケール
material->data.normal_scale = static_cast<float>(gltf_material.normalTexture.scale);

// オクルージョン強度
material->data.occlusion_strength = static_cast<float>(gltf_material.occlusionTexture.strength);

// テクスチャ読み込み
LoadTextureFromGltf(device, gltf_model, pbr.baseColorTexture.index,
base_path, material->color_texture);

LoadTextureFromGltf(device, gltf_model, gltf_material.normalTexture.index,
base_path, material->normal_texture);

LoadTextureFromGltf(device, gltf_model, pbr.metallicRoughnessTexture.index,
base_path, material->metallic_roughness_texture);

LoadTextureFromGltf(device, gltf_model, gltf_material.occlusionTexture.index,
base_path, material->occlusion_texture);

LoadTextureFromGltf(device, gltf_model, gltf_material.emissiveTexture.index,
base_path, material->emissive_texture);

// 定数バッファ作成
material->CreateConstantBuffer(device);

model->materials.push_back(material);
}
}

// GLTF テクスチャ読み込みヘルパー
bool GlthStaticModel::LoadTextureFromGltf(
ID3D11Device* device,
const tinygltf::Model& gltf_model,
int texture_index,
const std::string& base_path,
Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& out_srv)
{
if (texture_index < 0 ||
texture_index >= static_cast<int>(gltf_model.textures.size()))
{
return false;
}

const tinygltf::Texture& tex = gltf_model.textures[texture_index];
if (tex.source < 0 || tex.source >= static_cast<int>(gltf_model.images.size()))
{
return false;
}

const tinygltf::Image& image = gltf_model.images[tex.source];

// 外部ファイル参照の場合
if (!image.uri.empty() && image.uri.find("data:") == std::string::npos)
{
std::filesystem::path tex_path =
std::filesystem::path(base_path) / image.uri;

std::wstring wpath(tex_path.wstring());

Microsoft::WRL::ComPtr<ID3D11Resource> resource;
HRESULT hr = DirectX::CreateWICTextureFromFile(
device,
wpath.c_str(),
resource.GetAddressOf(),
out_srv.GetAddressOf());

if (FAILED(hr))
{
OutputDebugStringA("WARNING: Failed to load texture: ");
OutputDebugStringA(image.uri.c_str());
OutputDebugStringA("\n");
return false;
}
return true;
}

// 埋め込み画像データから作成
if (!image.image.empty() && image.width > 0 && image.height > 0)
{
D3D11_TEXTURE2D_DESC tex_desc = {};
tex_desc.Width            = static_cast<UINT>(image.width);
tex_desc.Height           = static_cast<UINT>(image.height);
tex_desc.MipLevels        = 1;
tex_desc.ArraySize        = 1;
tex_desc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
tex_desc.SampleDesc.Count = 1;
tex_desc.Usage            = D3D11_USAGE_DEFAULT;
tex_desc.BindFlags        = D3D11_BIND_SHADER_RESOURCE;

// 4チャネルに変換 (RGBA)
std::vector<uint8_t> rgba_data;
if (image.component == 4)
{
rgba_data = image.image;
}
else if (image.component == 3)
{
rgba_data.resize(image.width * image.height * 4);
for (int i = 0; i < image.width * image.height; ++i)
{
rgba_data[i * 4 + 0] = image.image[i * 3 + 0];
rgba_data[i * 4 + 1] = image.image[i * 3 + 1];
rgba_data[i * 4 + 2] = image.image[i * 3 + 2];
rgba_data[i * 4 + 3] = 255;
}
}
else
{
return false;
}

D3D11_SUBRESOURCE_DATA subresource = {};
subresource.pSysMem     = rgba_data.data();
subresource.SysMemPitch = static_cast<UINT>(image.width) * 4;

Microsoft::WRL::ComPtr<ID3D11Texture2D> tex2d;
HRESULT hr = device->CreateTexture2D(&tex_desc, &subresource, tex2d.GetAddressOf());
if (FAILED(hr))
{
return false;
}

D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
srv_desc.Format              = tex_desc.Format;
srv_desc.ViewDimension       = D3D11_SRV_DIMENSION_TEXTURE2D;
srv_desc.Texture2D.MipLevels = 1;

hr = device->CreateShaderResourceView(tex2d.Get(), &srv_desc, out_srv.GetAddressOf());
if (FAILED(hr))
{
return false;
}

return true;
}

return false;
}

// メッシュ描画 (頂点/インデックスバッファ設定 + DrawIndexed のみ)
void GlthStaticModel::DrawMesh(ID3D11DeviceContext* context, const GlthMesh& mesh)
{
if (!context || !mesh.vertex_buffer || !mesh.index_buffer)
{
return;
}

// 頂点バッファ設定
UINT stride = sizeof(GlthVertex);
UINT offset = 0;
context->IASetVertexBuffers(0, 1, mesh.vertex_buffer.GetAddressOf(), &stride, &offset);

// インデックスバッファ設定
context->IASetIndexBuffer(mesh.index_buffer.Get(), DXGI_FORMAT_R32_UINT, 0);

// プリミティブトポロジー設定
context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

// 描画
context->DrawIndexed(static_cast<UINT>(mesh.indices.size()), 0, 0);
}

// モデル全体をマテリアルを適用して描画
void GlthStaticModel::Render(
ID3D11DeviceContext* context,
const GltfModel& model,
const DirectX::XMFLOAT4X4& world)
{
if (!context)
{
return;
}

// シェーダーとインプットレイアウトを設定
if (model.vertex_shader)
{
context->VSSetShader(model.vertex_shader.Get(), nullptr, 0);
}
if (model.pixel_shader)
{
context->PSSetShader(model.pixel_shader.Get(), nullptr, 0);
}
if (model.input_layout)
{
context->IASetInputLayout(model.input_layout.Get());
}

// オブジェクト定数バッファ (b0: world matrix) を更新
if (model.object_constant_buffer)
{
context->UpdateSubresource(model.object_constant_buffer.Get(), 0, nullptr, &world, 0, 0);
context->VSSetConstantBuffers(0, 1, model.object_constant_buffer.GetAddressOf());
}

// 各メッシュを描画
for (const auto& mesh : model.meshes)
{
// マテリアルを適用 (material_index が有効な場合)
if (mesh.material_index >= 0 &&
mesh.material_index < static_cast<int>(model.materials.size()) &&
model.materials[mesh.material_index])
{
model.materials[mesh.material_index]->ApplyToShader(context);
}

DrawMesh(context, mesh);
}
}

// 頂点バッファ作成
Microsoft::WRL::ComPtr<ID3D11Buffer> GlthStaticModel::CreateVertexBuffer(
ID3D11Device* device,
const void* data,
size_t size)
{
D3D11_BUFFER_DESC desc = {};
desc.ByteWidth      = static_cast<UINT>(size);
desc.Usage          = D3D11_USAGE_IMMUTABLE;
desc.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
desc.CPUAccessFlags = 0;

D3D11_SUBRESOURCE_DATA subresource = {};
subresource.pSysMem = data;

Microsoft::WRL::ComPtr<ID3D11Buffer> buffer;
HRESULT hr = device->CreateBuffer(&desc, &subresource, buffer.GetAddressOf());

if (FAILED(hr))
{
OutputDebugStringA("ERROR: Failed to create vertex buffer\n");
return nullptr;
}

return buffer;
}

// インデックスバッファ作成
Microsoft::WRL::ComPtr<ID3D11Buffer> GlthStaticModel::CreateIndexBuffer(
ID3D11Device* device,
const void* data,
size_t size)
{
D3D11_BUFFER_DESC desc = {};
desc.ByteWidth      = static_cast<UINT>(size);
desc.Usage          = D3D11_USAGE_IMMUTABLE;
desc.BindFlags      = D3D11_BIND_INDEX_BUFFER;
desc.CPUAccessFlags = 0;

D3D11_SUBRESOURCE_DATA subresource = {};
subresource.pSysMem = data;

Microsoft::WRL::ComPtr<ID3D11Buffer> buffer;
HRESULT hr = device->CreateBuffer(&desc, &subresource, buffer.GetAddressOf());

if (FAILED(hr))
{
OutputDebugStringA("ERROR: Failed to create index buffer\n");
return nullptr;
}

return buffer;
}
