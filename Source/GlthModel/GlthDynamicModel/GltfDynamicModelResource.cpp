#include <filesystem>


#include "tiny_gltf.h"
#include "GltfDynamicModelResource.h"


///==============================================================
//アクセッサからデータを安全に取得するためのヘルパー関数
//===============================================================
template<typename T>
const T* GetElementPointer(const tinygltf::Model& model, int accessor_index) 
{
	//インデックスが不正な場合はnullptrを返す
	if (accessor_index < 0 || accessor_index >= static_cast<int>(model.accessors.size())) return nullptr;    const auto& accessor = model.accessors[accessor_index]; //
	
	const auto& accessor = model.accessors[accessor_index];

	//bufferViewのインデックスチェック
	if (accessor.bufferView < 0 || accessor.bufferView >= static_cast<int>(model.bufferViews.size())) return nullptr;

	const auto& buffer_view = model.bufferViews[accessor.bufferView];
	const auto& buffer = model.buffers[buffer_view.buffer];

	//データのアドレスを計算して返す
	return reinterpret_cast<const T*>(&buffer.data[buffer_view.byteOffset + accessor.byteOffset]);
}

//=================================================
//コンストラクタ
//=================================================
GltfDynamicModelResource::GltfDynamicModelResource(ID3D11Device* device, const char* filename)
{
    tinygltf::Model model;					//GLTFの全データを保持する構造体
	tinygltf::TinyGLTF loader;				//GLTFのローダーオブジェクト
    std::string err, warn;					//エラーと警告メッセージ格納用
	std::filesystem::path path(filename);   //ファイルパスを解析するためのオブジェクト
    bool ret = false;						//ロード成否のフラグ

    //拡張子に応じてロード関数を切り替える
	if (path.extension() == ".glb") {
		ret = loader.LoadBinaryFromFile(&model, &err, &warn, filename);
	}
    else {
		ret = loader.LoadASCIIFromFile(&model, &err, &warn, filename);
	}

	if (!ret)
	{
#ifdef _DEBUG
		OutputDebugStringA(err.c_str()); // デバッグ出力にエラーを表示
#endif
		return;
	}

	//----------------------------------------------
	//マテリアルの読み込み
	//----------------------------------------------
	materials.reserve(model.materials.size());	//メモリ領域の確保
	for (const auto& gltf_material : model.materials)
	{
		auto material = std::make_shared<GltfDynamicMaterial>();	//マテリアルオブジェクトの作成
		materials.push_back(material);	//リストに追加
	}

	//----------------------------------------------
	//メッシュの読み込み
	//----------------------------------------------

	//メッシュの数だけループ
	for (const auto& gltf_mesh : model.meshes)
	{
		//メッシュ内の全プリミティブ（描画単位）をループ
		for (const auto& primitive : gltf_mesh.primitives)
		{
			//------------------------------
			//属性の存在チェック
			//------------------------------
			if (primitive.attributes.find("POSITION") == primitive.attributes.end()) continue;
			if (primitive.attributes.find("NORMAL") == primitive.attributes.end()) continue;
			if (primitive.attributes.find("TEXCOORD_0") == primitive.attributes.end()) continue;
			bool has_skinning = (primitive.attributes.find("WEIGHTS_0") != primitive.attributes.end() &&
				primitive.attributes.find("JOINTS_0") != primitive.attributes.end());

			std::vector<GltfDynamicMesh::Vertex> vertices;	//頂点データの格納用
			std::vector<uint32_t> indices;					//インデックスデータの格納用

			//属性位置の取得
			const float* position_ptr = GetElementPointer<float>(model, primitive.attributes.at("POSITION"));	//座標データへのポインタ取得
			const float* normal_ptr = GetElementPointer<float>(model, primitive.attributes.at("NORMAL"));		//法線データへのポインタ取得
			const float* uv_ptr = GetElementPointer<float>(model, primitive.attributes.at("TEXCOORD_0"));		//UVデータへのポインタ取得
			
			//必須属性が取得できない場合はスキップ
			if (!position_ptr || !normal_ptr || !uv_ptr) continue;
			
			const float* weight_ptr = GetElementPointer<float>(model, primitive.attributes.at("WEIGHTS_0"));	//重みデータへのポインタ取得
			
			//JOINTSのデータ型はモデルにより unsigned byte (5121) か unsigned short (5123) のため、適切なキャストが必要
			int joints_accessor_idx = has_skinning ? primitive.attributes.at("JOINTS_0") : -1;
			int joints_type = (joints_accessor_idx >= 0) ? model.accessors[joints_accessor_idx].componentType : -1;
			
			size_t vertex_count = model.accessors[primitive.attributes.at("POSITION")].count;	//頂点数の取得
			vertices.reserve(vertex_count);	//メモリ領域の確保

			//全頂点分データをコピー
			for (size_t i = 0; i < vertex_count; i++)
			{
				GltfDynamicMesh::Vertex vertex = {};	//頂点構造体を初期化
				vertex.position = { position_ptr[i * 3], position_ptr[i * 3 + 1], position_ptr[i * 3 + 2] };	//座標データのコピー
				vertex.normal = { normal_ptr[i * 3], normal_ptr[i * 3 + 1], normal_ptr[i * 3 + 2] };			//法線データのコピー
				vertex.texcoord = { uv_ptr[i * 2], uv_ptr[i * 2 + 1] };											//UVデータのコピー
				if (has_skinning && weight_ptr)
				{
					vertex.bone_weights = { weight_ptr[i * 4], weight_ptr[i * 4 + 1], weight_ptr[i * 4 + 2], weight_ptr[i * 4 + 3] };	//重みデータのコピー
				}

				//JOINTSの型に応じた取得処理
				if (joints_type == 5121)
				{ // TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE
					const uint8_t* p = GetElementPointer<uint8_t>(model, joints_accessor_idx);
					for (int j = 0; j < 4; ++j) vertex.bone_indices[j] = p[i * 4 + j];
				}
				else if(joints_type == 5123) 
				{ // 5123: TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT
					const uint16_t* p = GetElementPointer<uint16_t>(model, joints_accessor_idx);
					for (int j = 0; j < 4; ++j) vertex.bone_indices[j] = p[i * 4 + j];
				}
				else
				{
					continue; // JOINTS_0の型が不明な場合はスキップ
				}

				vertices.push_back(vertex);                    //配列に追加
			}
			//インデックスデータの取得
			if (primitive.indices >= 0) 
			{
				const auto& accessor = model.accessors[primitive.indices];
				indices.reserve(accessor.count);

				//インデックスの型（16bit or 32bit）を判別
				if (accessor.componentType == 5123) { // UNSIGNED_SHORT
					const uint16_t* p = GetElementPointer<uint16_t>(model, primitive.indices);
					for (size_t i = 0; i < accessor.count; i++)
					{
						indices.push_back(static_cast<uint32_t>(p[i]));
					}
				}
				else if (accessor.componentType == 5125) { // UNSIGNED_INT
					const uint32_t* p = GetElementPointer<uint32_t>(model, primitive.indices);
					for (size_t i = 0; i < accessor.count; i++)
					{
						indices.push_back(p[i]);
					}
				}
			}

			auto mesh = std::make_shared<GltfDynamicMesh>(device, vertices, indices);	//メッシュリソースを作成
			if (primitive.material >= 0 && primitive.material < static_cast<int>(materials.size()))
			{
				mesh->SetMaterial(materials[primitive.material]);	//生成したマテリアルを紐付け
			}
			meshes.push_back(mesh);	//リストに追加
		}
	}
}
