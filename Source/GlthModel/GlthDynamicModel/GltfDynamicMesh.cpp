#include "GltfDynamicMesh.h"
#include "GltfDynamicMaterial.h"

#include "tiny_gltf.h"

//=================================================
//アクセッサからポインタを取得
//=================================================
template<typename T>
inline const T* GltfDynamicMesh::GetElementPointer(const tinygltf::Model& model, int accessor_index)
{
	// インデックスが不正な場合はnullptrを返す
	if (accessor_index < 0 || accessor_index >= static_cast<int>(model.accessors.size()))
	{
		return nullptr;
	}

	// アクセッサを取得
	const auto& accessor = model.accessors[accessor_index];

	// bufferViewのインデックスチェック
	if (accessor.bufferView < 0 || accessor.bufferView >= static_cast<int>(model.bufferViews.size()))
	{
		return nullptr;
	}

	const auto& buffer_view = model.bufferViews[accessor.bufferView];
	const auto& buffer = model.buffers[buffer_view.buffer];

	// データのアドレスを計算して返す
	return reinterpret_cast<const T*>(&buffer.data[buffer_view.byteOffset + accessor.byteOffset]);
}

//=================================================
//コンストラクタ
//=================================================
GltfDynamicMesh::GltfDynamicMesh(
	ID3D11Device* device, 
	const std::vector<Vertex>& vertices,
	const std::vector<uint32_t>& indices)
{
	//バッファの作成
	CreateBuffers(device, vertices, indices);
}

//=================================================
//GLTFデータからメッシュを初期化
//=================================================
void GltfDynamicMesh::Initialize(
	ID3D11Device* device,
	const tinygltf::Model& model,
	const tinygltf::Primitive& primitive)
{
	//-----------------------------------------------
	//属性の存在確認
	//-----------------------------------------------
	
	//座標を探す
	auto it_position = primitive.attributes.find("POSITION"); 
	//座標がないメッシュは描画不能なので終了
	if (it_position == primitive.attributes.end()) return;
	//法線を探す
	auto it_normal = primitive.attributes.find("NORMAL"); 
	//UVを探す
	auto it_uv = primitive.attributes.find("TEXCOORD_0");
	//重みを探す
	auto it_weight = primitive.attributes.find("WEIGHTS_0"); 
	//ボーン番号を探す
	auto it_joint = primitive.attributes.find("JOINTS_0"); 
	//両方あればスキニング有効
	bool has_skin = (it_weight != primitive.attributes.end() && it_joint != primitive.attributes.end()); 

	//-----------------------------------------------
	//先頭ポインタの一括取得
	//-----------------------------------------------
	
	//頂点数の取得
	size_t vertex_count = model.accessors[it_position->second].count;
	//座標データのポインタ取得
	const float* position_ptr = GetElementPointer<float>(model, it_position->second);
	//法線データのポインタ取得
	const float* normal_ptr = (it_normal != primitive.attributes.end()) ? GetElementPointer<float>(model, it_normal->second) : nullptr;
	//UVデータのポインタ取得
	const float* uv_ptr = (it_uv != primitive.attributes.end()) ? GetElementPointer<float>(model, it_uv->second) : nullptr;
	//重みデータのポインタ取得
	const float* weight_ptr = has_skin ? GetElementPointer<float>(model, it_weight->second) : nullptr;
	// 1バイト型ボーン番号
	const uint8_t* joint_ptr_u8 = (has_skin && model.accessors[it_joint->second].componentType == 5121) ? GetElementPointer<uint8_t>(model, it_joint->second) : nullptr;	
	// 2バイト型ボーン番号
	const uint16_t* joint_ptr_u16 = (has_skin && model.accessors[it_joint->second].componentType == 5123) ? GetElementPointer<uint16_t>(model, it_joint->second) : nullptr;
	
	//-----------------------------------------------
	//頂点データの構築
	//-----------------------------------------------

	//頂点配列を宣言
	std::vector<Vertex> vertices;
	//メモリを事前確保
	vertices.reserve(vertex_count);

	//全頂点をループ
	for (size_t i = 0; i < vertex_count; i++)
	{
		//頂点構造体を初期化
		Vertex vertex = {};

		//座標データのコピー
		vertex.position = { position_ptr[i * 3], position_ptr[i * 3 + 1], position_ptr[i * 3 + 2] };

		//法線データがある場合はコピー
		if (normal_ptr)
		{
			vertex.normal = { normal_ptr[i * 3], normal_ptr[i * 3 + 1], normal_ptr[i * 3 + 2] };
		}

		//UVデータがある場合はコピー
		if (uv_ptr)
		{
			vertex.texcoord = { uv_ptr[i * 2], uv_ptr[i * 2 + 1] };
		}

		//スキニングデータがある場合はコピー
		if (has_skin && weight_ptr)
		{
			//重みデータをコピー
			vertex.bone_weights = { weight_ptr[i * 4], weight_ptr[i * 4 + 1], weight_ptr[i * 4 + 2], weight_ptr[i * 4 + 3] };
			//ボーン番号のアクセッサ情報を取得
			int joints_accessor_index = it_joint->second;
			//データ型（byteかshortか）を取得
			int joints_type = model.accessors[joints_accessor_index].componentType;

			//1バイト(unsigned byte)の場合
			if (joint_ptr_u8)
			{
				for (int j = 0; j < 4; ++j)
				{
					vertex.bone_indices[j] = joint_ptr_u8[i * 4 + j];
				}
			}
			//2バイト(unsigned short)の場合
			else if (joint_ptr_u16)
			{
				for (int j = 0; j < 4; ++j)
				{
					vertex.bone_indices[j] = joint_ptr_u16[i * 4 + j];
				}
			}
			else
			{
				//JOINTS_0の型が不明な場合はスキップ
				continue;
			}
		}
		//配列に追加
		vertices.push_back(vertex);
	}

	//------------------------------------------------
	//インデックスデータを構築
	//------------------------------------------------

	//インデックス配列を宣言
	std::vector<uint32_t> indices;
	//インデックスがある場合
	if (primitive.indices >= 0)
	{
		//インデックスのアクセッサ情報を取得
		const auto& index_accessor = model.accessors[primitive.indices];
		//メモリを事前確保
		indices.reserve(index_accessor.count);
		//2バイト型の場合
		if (index_accessor.componentType == 5123)
		{
			const uint16_t* index_ptr = GetElementPointer<uint16_t>(model, primitive.indices);
			for (size_t i = 0; i < index_accessor.count; ++i)
			{
				indices.push_back(static_cast<uint32_t>(index_ptr[i]));
			}
		}
		//4バイト型の場合
		else if (index_accessor.componentType == 5125)
		{
			const uint32_t* index_ptr = GetElementPointer<uint32_t>(model, primitive.indices);
			for (size_t i = 0; i < index_accessor.count; ++i)
			{
				indices.push_back(index_ptr[i]);
			}
		}
	}

	//------------------------------------------------
	//バッファの再構築
	//------------------------------------------------
	
	//新しい頂点とインデックスでバッファを再構築
	CreateBuffers(device, vertices, indices);
}

//=================================================
//メッシュ描画
//=================================================
void GltfDynamicMesh::Render(ID3D11DeviceContext* dc)
{
	//頂点バッファの設定
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	dc->IASetVertexBuffers(0, 1, vertex_buffer.GetAddressOf(), &stride, &offset);

	//インデックスバッファの設定
	dc->IASetIndexBuffer(index_buffer.Get(), DXGI_FORMAT_R32_UINT, 0);

	//プリミティブトポロジーの設定
	dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//マテリアルの設定
	if (material)
	{
		material->Bind(dc);
	}

	//描画処理
	dc->DrawIndexed(index_count, 0, 0);
}

//=================================================
//GPUリソースの作成
//=================================================
void GltfDynamicMesh::CreateBuffers(
	ID3D11Device* device,
	const std::vector<Vertex>& vertices,
	const std::vector<uint32_t>& indices)
{
	//インデックス数を保持
	index_count = static_cast<uint32_t>(indices.size());

	//-----------------------------------------------
	//頂点バッファの作成
	//-----------------------------------------------

	D3D11_BUFFER_DESC vertex_buffer_desc = {};
	vertex_buffer_desc.Usage = D3D11_USAGE_IMMUTABLE;									//高速化のため読み取り専用に設定
	vertex_buffer_desc.ByteWidth = sizeof(Vertex) * static_cast<UINT>(vertices.size());	//バッファの総バイト数
	vertex_buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;							//頂点バッファとして使用

	//初期データの設定
	D3D11_SUBRESOURCE_DATA vertex_data = {};
	vertex_data.pSysMem = vertices.data();	//配列の先頭アドレスを指定

	//デバイスを使用してバッファを作成
	device->CreateBuffer(&vertex_buffer_desc, &vertex_data, vertex_buffer.GetAddressOf());

	//-----------------------------------------------
	//インデックスバッファの作成
	//-----------------------------------------------

	D3D11_BUFFER_DESC index_buffer_desc = {};
	index_buffer_desc.Usage = D3D11_USAGE_IMMUTABLE;				//高速化のため読み取り専用に設定
	index_buffer_desc.ByteWidth = sizeof(uint32_t) * index_count;	//バッファの総バイト数
	index_buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;			//インデックスバッファとして使用

	//初期データの設定
	D3D11_SUBRESOURCE_DATA index_data = {};
	index_data.pSysMem = indices.data();	//配列の先頭アドレスを指定

	//デバイスを使用してバッファを作成
	device->CreateBuffer(&index_buffer_desc, &index_data, index_buffer.GetAddressOf());

}
