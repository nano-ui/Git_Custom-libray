#include <filesystem>
#include "tiny_gltf.h"

#include "GltfDynamicModelResource.h"

//=================================================
//アクセッサからポインタを取得
//=================================================
template<typename T>
inline const T* GltfDynamicModelResource::GetElementPointer(const tinygltf::Model& model, int accessor_index)
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
GltfDynamicModelResource::GltfDynamicModelResource(ID3D11Device* device, const char* filename)
{
	//----------------------------------------------
	//Gltfファイルの読み込み
	//----------------------------------------------

    tinygltf::Model model;					//GLTFの全データを保持する構造体
	tinygltf::TinyGLTF loader;				//GLTFのローダーオブジェクト
    std::string err, warn;					//エラーと警告メッセージ格納用
	std::filesystem::path path(filename);   //ファイルパスを解析するためのオブジェクト
    bool ret = false;						//ロード成否のフラグ

    //拡張子に応じてロード関数を切り替える
	if (path.extension() == ".glb") 
	{
		ret = loader.LoadBinaryFromFile(&model, &err, &warn, filename);
	}
    else 
	{
		ret = loader.LoadASCIIFromFile(&model, &err, &warn, filename);
	}

	if (!ret)
	{
#ifdef _DEBUG
		OutputDebugStringA(err.c_str()); // デバッグ出力にエラーを表示
#endif
		return;
	}

	//ディレクトリパス取得
	std::string directory = path.parent_path().string() + "/";

	//----------------------------------------------
	//マテリアルの読み込み
	//----------------------------------------------

	//メモリ領域の確保
	materials.reserve(model.materials.size());	
	//マテリアルの数だけループ
	for (const auto& gltf_mat : model.materials)
	{
		auto material = std::make_shared<GltfDynamicMaterial>();
		//マテリアルの初期化
		material->Initialize(device, model, gltf_mat, directory);
		//リストに追加
		materials.push_back(material);
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
			auto mesh = std::make_shared<GltfDynamicMesh>(device, std::vector<GltfDynamicMesh::Vertex>{}, std::vector<uint32_t>{});
			mesh->Initialize(device, model, primitive);
			// マテリアルが設定されていれば紐付ける
			if (primitive.material >= 0 && primitive.material < static_cast<int>(materials.size()))
			{
				mesh->SetMaterial(materials[primitive.material]); //読み込み済みのものをセット
			}
			meshes.push_back(mesh); //リストに追加		
		}
	}

	//----------------------------------------------
	//ボーンの読み込み
	//----------------------------------------------

	//メモリ領域の確保
	bones.reserve(model.nodes.size());
	//ノードの数だけループ
	for (size_t i = 0; i < model.nodes.size(); i++)
	{
		bones[i].Initalize(model.nodes[i], -1); //ノードの情報を元に初期化
	}

	//ボーンの親子関係構築
	for (size_t i = 0; i < model.nodes.size(); i++)
	{
		//現在のノードが持つ子の番号をチェック
		for (int child_index : model.nodes[i].children)
		{
			//子から親へインデックスでリンクを張る
			bones[child_index].SetParentIndex(static_cast<int>(i));
		}
	}

	//----------------------------------------------
	//スキンのロード
	//----------------------------------------------

	//スキンデータが存在する場合
	if (!model.skins.empty())
	{
		const auto& skin = model.skins[0];
		//逆バインド行列の先頭取得
		const float* ibm_ptr = GetElementPointer<float>(model, skin.inverseBindMatrices);
		//ポインタが取得できた場合
		if (ibm_ptr)
		{
			//スキンに関連付けられたジョイント分ループ
			for (size_t i = 0; i < skin.joints.size(); i++)
			{
				int bone_index = skin.joints[i];
				DirectX::XMFLOAT4X4 ibm;
				memcpy(&ibm, &ibm_ptr[i * 16], sizeof(DirectX::XMFLOAT4X4));
				bones[bone_index].SetOffsetMatrix(ibm);
			}
		}
	}

	//----------------------------------------------
	//アニメーションの読み込み
	//----------------------------------------------

	//メモリ確保
	animations.reserve(model.animations.size());

	//全てのアニメーションデータを解析
	for (size_t i = 0; i < model.animations.size(); i++)
	{
		//アニメーションの初期化
		animations[i].Initialize(model, model.animations[i]);
	}
}