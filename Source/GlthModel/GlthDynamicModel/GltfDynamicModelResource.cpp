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

	LoadBones(model);

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
				DirectX::XMFLOAT4X4 ibm(
					ibm_ptr[i * 16 + 0], ibm_ptr[i * 16 + 4], ibm_ptr[i * 16 + 8], ibm_ptr[i * 16 + 12],
					ibm_ptr[i * 16 + 1], ibm_ptr[i * 16 + 5], ibm_ptr[i * 16 + 9], ibm_ptr[i * 16 + 13],
					ibm_ptr[i * 16 + 2], ibm_ptr[i * 16 + 6], ibm_ptr[i * 16 + 10], ibm_ptr[i * 16 + 14],
					ibm_ptr[i * 16 + 3], ibm_ptr[i * 16 + 7], ibm_ptr[i * 16 + 11], ibm_ptr[i * 16 + 15]
				);

				//左手座標系への変換
				// == 左手座標系への変換 (ConvertMatrixAxisSystem) ==
				ibm._12 = -ibm._12;
				ibm._13 = -ibm._13;
				ibm._14 = -ibm._14;
				ibm._21 = -ibm._21;
				ibm._31 = -ibm._31;
				ibm._41 = -ibm._41;

				bones[bone_index].SetOffsetMatrix(ibm);
			}
		}
		joints = skin.joints;
	}

	//----------------------------------------------
	//アニメーションの読み込み
	//----------------------------------------------

	//メモリ領域の確保
	animations.resize(model.animations.size());

	//全てのアニメーションデータを解析
	for (size_t i = 0; i < model.animations.size(); i++)
	{
		//アニメーションの初期化
		animations[i].Initialize(model, model.animations[i]);
	}
}

//============================
//ボーン構造の読み込み処理
//============================
void GltfDynamicModelResource::LoadBones(const tinygltf::Model& model)
{
	//------------------------
	//全てのボーンを実体化
	//------------------------

	bones.resize(model.nodes.size());	//全ノード分のメモリ領域の確保
	//全ノードをループ
	for (size_t i = 0; i < model.nodes.size(); i++)
	{
		//親無しとして初期化
		bones[i].Initalize(model.nodes[i], -1);
	}

	//----------------------------------------
	//親子関係をインデックスでリンク
	//----------------------------------------

	//再度全ノードをループ
	for (int i = 0; i < static_cast<int>(model.nodes.size()); i++)
	{
		//親を持っている子の番号リストを確認
		for (int child_index : model.nodes[i].children)
		{
			//番号が正常範囲内かチェック
			if (child_index >= 0 && child_index < bones.size())
			{
				bones[child_index].SetParentIndex(i);
			}
		}
	}
}