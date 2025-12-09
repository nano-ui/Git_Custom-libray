#pragma once

#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <vector>
#include <string>
#include <fbxsdk.h> 
#include <map>
#include <unordered_map>
#include <filesystem>
#include <cereal/archives/binary.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/set.hpp>
#include <cereal/types/unordered_map.hpp>

#include "texture.h"

/*DirectX のベクトルや行列型を JSON や XML に
シリアライズ／デシリアライズできるようにする「cereal との橋渡し」 を行っている*/
namespace DirectX
{
	template<class T>
	void serialize(T& archive, DirectX::XMFLOAT2& v)
	{
		archive(
			cereal::make_nvp("x", v.x),
			cereal::make_nvp("y", v.y)
		);
	}

	template<class T>
	void serialize(T& archive, DirectX::XMFLOAT3& v)
	{
		archive(
			cereal::make_nvp("x", v.x),
			cereal::make_nvp("y", v.y),
			cereal::make_nvp("z", v.z)
		);
	}

	template<class T>
	void serialize(T& archive, DirectX::XMFLOAT4& v)
	{
		archive(
			cereal::make_nvp("x", v.x),
			cereal::make_nvp("y", v.y),
			cereal::make_nvp("z", v.z),
			cereal::make_nvp("w", v.w)
		);
	}

	template<class T>
	void serialize(T& archive, DirectX::XMFLOAT4X4& m)
	{
		archive(
			cereal::make_nvp("_11", m._11), cereal::make_nvp("_12", m._12),
			cereal::make_nvp("_13", m._13), cereal::make_nvp("_14", m._14),
			cereal::make_nvp("_21", m._21), cereal::make_nvp("_22", m._22),
			cereal::make_nvp("_23", m._23), cereal::make_nvp("_24", m._24),
			cereal::make_nvp("_31", m._31), cereal::make_nvp("_32", m._32),
			cereal::make_nvp("_33", m._33), cereal::make_nvp("_34", m._34),
			cereal::make_nvp("_41", m._41), cereal::make_nvp("_42", m._42),
			cereal::make_nvp("_43", m._43), cereal::make_nvp("_44", m._44)
		);
	}
}




struct scene
{
	//3Dシーンを構成する個々の要素(ノード)
	struct node 
	{
		uint64_t unique_id{ 0 };	//他のノードと区別するID
		std::string name;	//ノードの名前	
		FbxNodeAttribute::EType attribute{ FbxNodeAttribute::EType::eUnknown };	//ノードが持つ属性のタイプ
		int64_t parent_index{ -1 };	//親のインデックス(-1なので親を持たない)

		template<class T>
		void serialize(T& archive)
		{
			archive(unique_id, name, attribute, parent_index);
		}

	};
	std::vector<node> nodes;	//node型のオブジェクトを格納する動的な配列

	template<class T>
	void serialize(T& archive)
	{
		archive(nodes);
	}


	//探しているnode(unique_id)がnodesのどこに格納されているかを検索
	int64_t indexof(uint64_t unique_id)const
	{
		int64_t index{ 0 };		//現在のノードのインデックスを追跡

		//nodes内のすべてのnodeを検索
		for (const node& node : nodes)
		{
			//探しているnodeが見つかったか?
			if (node.unique_id == unique_id)
			{
				//一致したindexを返す
				return index;
			}
			//次のノードのインデックスに移行
			index++;
		}
		//最後まで見つからなかった
		return -1;
	}
};

class skinned_mesh
{
public:

	//スキンメッシュアニメーション用のデータ構造を定義
	struct animation
	{
		//アニメーションの名前
		std::string name;

		//1秒間に何回サンプリングされているか
		float sampling_rate{ 0 };

		//ある時点のアニメーション状態
		struct keyframe
		{
			//そのポーズにおける各ボーンのグローバル行列
			struct node
			{
				//そのボーンのワールド空間における姿勢(位置＋回転＋スケール)
				DirectX::XMFLOAT4X4 global_transform{ 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
			
				DirectX::XMFLOAT3 scaling{ 1,1,1 };//スケール
				DirectX::XMFLOAT4 rotation{ 0,0,0,1 };//回転
				DirectX::XMFLOAT3 translation{ 0,0,0 };//位置
				template<class T>
				void serialize(T& archive)
				{
					archive(global_transform, scaling, rotation, translation);
				}
			};
			//各キーフレームでの各ボーンの姿勢
			std::vector<node> nodes;

			template<class T>
			void serialize(T& archive)
			{
				archive(nodes);
			}

		};
		//そのアニメーションの全キーフレーム（時間軸）
		std::vector<keyframe> sequence;

		template<class T>
		void serialize(T& archive)
		{
			archive(name, sampling_rate, sequence);
		}


	};
	//複数のアニメーションクリップ（走る・歩くなど）
	std::vector<animation> animation_clips;

	struct skeleton
	{
		struct bone
		{

			//FBXなどから読み取った一意なID
			uint64_t unique_id{ 0 };

			//ボーンの名前
			std::string name;

			//親ボーンのインデックス
			int64_t parent_index{ -1 };

			//FBXのノード順などに対応するインデックス
			int64_t node_index{ 0 };

			//バインドポーズの逆変換行列
			DirectX::XMFLOAT4X4 offset_transform{ 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };

			template<class T>
			void serialize(T& archive)
			{
				archive(unique_id, name, parent_index, node_index, offset_transform);
			}

			//親を持たないか
			bool is_orphan() const { return parent_index < 0; };
		};
		//全てのボーンのリスト
		std::vector<bone> bones;

		template<class T>
		void serialize(T& archive)
		{
			archive(bones);
		}

		//特定のunique_idを持つボーンのインデックスを検索
		int64_t indexof(uint64_t unique_id) const
		{
			int64_t index{ 0 };
			for (const bone& bone : bones)
			{
				if (bone.unique_id == unique_id)
				{
					return index;
				}
				index++;
			}
			return -1;
		}
	};

	//各頂点が最大でいくつのボーンの影響を受けることができるか
	static const int MAX_BONE_INFLUENCES{ 4 };
	//各頂点のデータ
	struct vertex
	{
		DirectX::XMFLOAT3 position;			//位置
		DirectX::XMFLOAT3 normal;	//法線ベクトル
		DirectX::XMFLOAT4 tangent{ 1,0,0,1 };//接線ベクトル
		DirectX::XMFLOAT2 texcoord;	//テクスチャ座標

		//各ボーンがこの頂点にどれくらい影響を与えるかを示す重み（割合）
		float bone_weights[MAX_BONE_INFLUENCES]{1,0,0,0};

		//頂点に影響を与えるボーンのID（インデックス）
		uint32_t bone_indices[MAX_BONE_INFLUENCES]{};

		template<class T>
		void serialize(T& archive)
		{
			archive(position, normal, tangent, texcoord, bone_weights, bone_indices);
		}
	};

	//1つのスキンメッシュで使用可能なボーン（骨）の最大数
	static const int MAX_BONES{ 256 };

	//GPUに送られるConstant Bufferのデータ
	struct constants
	{
		DirectX::XMFLOAT4X4 world;	//3D空間内のメッシュの位置、角度、サイズが入る変換行列
		DirectX::XMFLOAT4 material_color;	//オブジェクトの色情報

		//ボーンアニメーション用の変換行列
		DirectX::XMFLOAT4X4 bone_transforms[MAX_BONES]{
			{ 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 }
		};
	};

	struct mesh
	{
		uint64_t unique_id{ 0 };	//他ノードと区別する番号
		std::string name;			//メッシュの名前
		// 'node_index' is an index that refers to the node array of the scene. 
		int64_t node_index{ 0 };	//メッシュが所属する「シーンノード」のインデックス

		uint64_t material_id{ 0 };	//メッシュが使用するマテリアルのID

		std::vector<vertex> vertices;		//頂点データ
		std::vector<uint32_t> indices;		//ポリゴンを形成するかを指定する番号

		struct subset
		{
			//サブセットに適用されるマテリアルの一意なID
			uint64_t material_unique_id{ 0 };

			//サブセットに適用されるマテリアルの名前
			std::string material_name;

			/*サブセットを構成するインデックスデータが、
			全体のインデックスバッファのどこから始まるかを示すオフセット*/
			uint32_t start_index_location{ 0 };

			//サブセットを構成するインデックスの数
			uint32_t index_count{ 0 };

			template<class T>
			void serialize(T& archive)
			{
				archive(material_unique_id, material_name, start_index_location, index_count);
			}


		};
		/*1つのメッシュが複数のマテリアル部分を持つ場合に、
		それらのサブセット情報をリストとして保持*/
		std::vector<subset> subsets;

		template<class T>
		void serialize(T& archive)
		{
			archive(unique_id, name, node_index, subsets, default_global_transform,
				bind_pose, bounding_box, vertices, indices);
		}

	private:
		Microsoft::WRL::ComPtr<ID3D11Buffer> vertex_buffer;	//メッシュを構成する全ての頂点データを格納
		Microsoft::WRL::ComPtr<ID3D11Buffer> index_buffer;	//メッシュを構成する全てのインデックスデータを格納
		friend class skinned_mesh;

		//スケルトンに含まれる全てのボーンのリスト
		skeleton bind_pose;

		//メッシュのデフォルトのワールド変換行列
		DirectX::XMFLOAT4X4 default_global_transform
		{ 1.0f,0.0f,0.0f,0.0f,
		  0.0f,1.0f,0.0f,0.0f,
		  0.0f,0.0f,1.0f,0.0f,
		  0.0f,0.0f,0.0f,1.0f };

		DirectX::XMFLOAT3 bounding_box[2]
		{
			{+D3D11_FLOAT32_MAX,+D3D11_FLOAT32_MAX,+D3D11_FLOAT32_MAX},
			{ -D3D11_FLOAT32_MAX, -D3D11_FLOAT32_MAX, -D3D11_FLOAT32_MAX}
		};
	};
	std::vector<mesh> meshes;

	struct material
	{
		uint64_t unique_id{ 0 };	////他ノードと区別する番号
		std::string name;			//マテリアルの名前

		//環境光に対するマテリアルの色を定義
		DirectX::XMFLOAT4 ka{ 0.2f,0.2f,0.2f,1.0f };

		//拡散光に対するマテリアルの色を定義
		DirectX::XMFLOAT4 kd{ 0.8f,0.8f,0.8f,1.0f };

		//鏡面光に対するマテリアルの色を定義
		DirectX::XMFLOAT4 ks{ 1.0f,1.0f,1.0f,1.0f };

		//マテリアルが使用するテクスチャ画像のファイル名を最大4つまで格納できる配列
		std::string texture_filenames[4];
		//シェーダーからテクスチャにアクセスして描画に利用
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shader_resource_views[4];
	
		template<class T>
		void serialize(T& archive)
		{
			archive(unique_id, name, ka, kd, ks, texture_filenames);
		}


	};
	//シーン内で使用されるすべてのマテリアルを管理するためのコンテナ
	std::unordered_map<uint64_t, material>materials;


private:
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertex_shader;	//3Dモデルの各頂点データを格納
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixel_shader;		//3Dモデルの各色情報を格納
	Microsoft::WRL::ComPtr<ID3D11InputLayout> input_layout;		//CPU側のvertex_shaderのデータをGPU側に伝える
	Microsoft::WRL::ComPtr<ID3D11Buffer> constant_buffer;		//定数バッファ(固定されたデータ情報)を格納

	//ダミーマテリアル
	material dummy_material;

	//ダミーマテリアルのSRVを保持
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> dummy_texture_srv;

public:
	skinned_mesh(ID3D11Device* device, const char* fbx_filename, bool triangulate = false, float sampling_rate = 0);

	//スマートポイントタのようなメンバ変数のみを持つ場合、自動でメモリやリソースを開放
	virtual ~skinned_mesh() = default;

	//FBX SDKを使ってFBXシーンからメッシュデータを抽出し、std::vector<mesh>に格納する
	void fetch_meshes(FbxScene* fbx_scene, std::vector<mesh>& meshes);
	
	void fetch_materials(
		FbxScene* fbx_scene,
		std::unordered_map<uint64_t, material>& materials);

	//FBXモデルのスケルトン構造を読み取り、bind_pose にボーンとして格納
	void fetch_skeleton(FbxMesh* fbx_mesh, skeleton& bind_pose);

	//FBXファイルからアニメーションデータ（ボーンの動き）を読み取って、animation_clips に保存する処理
	void fetch_animations(FbxScene* fbx_scene, std::vector<animation>& animation_clips, float sampling_rate);

	//アニメーション更新
	void update_animation(animation::keyframe& keyframe);

	//DirectXのレンダリングパイプラインで利用できるようにGPU側のリソースに変換・準備する
	void create_com_objects(ID3D11Device* device, const char* fbx_filename);


	/*異なる数学ライブラリ間で行列データを正しく受け渡すための橋渡し関数*/
	inline XMFLOAT4X4 to_xmfloat4x4(const FbxAMatrix& fbxamatrix);

	/*FBX SDKが扱う3次元ベクトル（または点）のデータ形式を、
	DirectXが使う3次元ベクトルのデータ形式に変換*/
	inline XMFLOAT3 to_xmfloat3(const FbxDouble3& fbxdouble3);

	inline XMFLOAT4 to_xmfloat4(const FbxDouble4& fbxdouble4);
	 
	//描画処理
	void render(ID3D11DeviceContext* immediate_context,
		const DirectX::XMFLOAT4X4& world,
		const DirectX::XMFLOAT4& material_color,
		const animation::keyframe* keyframe);

	//FBXファイルからアニメーションクリップを読み込み、skinned_mesh に追加する処理
	bool append_animations(const char* animation_filename, float sampling_rate);

	void blend_animations(const animation::keyframe* keyframes[2], float factor, animation::keyframe& keyframe);
	template<class T>
	void serialize(T& archive)
	{
		archive(
			scene_view,
			meshes,
			materials,
			animation_clips
		);
	}
protected:
	scene scene_view;	//FBXファイルからロードされたモデルの情報を保持
};

/*3Dモデルの各頂点（点）が、
「どの骨（ボーン）によって、どれくらいの割合で動かされるか」という情報を、
FBXファイルから読み取って、プログラムで使える形にまとめる処理*/
//void fetch_bone_influences(
//	const FbxMesh* fbx_mesh,
//	std::vector<bone_influence_per_control_point>& bone_influences);
