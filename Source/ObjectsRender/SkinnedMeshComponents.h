#pragma once

#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <map>
#include <filesystem>
#include <algorithm>

#include "misc.h"

struct SceneConstants
{
	DirectX::XMFLOAT4X4 view_projection;//ビュー・プロジェクション行列
	DirectX::XMFLOAT4 light_direction;	//ライトの向き
	DirectX::XMFLOAT4 camera_position;	//カメラの位置
};

class SkinnedMeshSerializer;


namespace fbxsdk {
	class FbxScene;
	class FbxNode;
	class FbxMesh;
}

//メッシュの部品
struct MeshSubset
{
	uint64_t material_unique_id{ 0 };	//各マテリアルのID
	std::string material_name;			//各マテリアルの名前
	uint64_t start_index_location{ 0 };	//全体のインデックスバッファの描画開始位置
	uint32_t index_count{ 0 };			//描画する頂点数
};

//メッシュの頂点データ
struct MeshVertex
{
	DirectX::XMFLOAT3 position;							//頂点の座標
	DirectX::XMFLOAT3 normal;							//法線ベクトル
	DirectX::XMFLOAT4 tangent{ 1.0f,0.0f,0.0f,1.0f };	//接線ベクトル
	DirectX::XMFLOAT2 texcoord;							//テクスチャ座標
	float bone_weights[4]{ 1.0f,0.0f,0.0f,0.0f };		//頂点のボーンの影響度
	uint32_t bone_indices[4]{ 0,0,0,0 };				//影響を受けるボーンの番号
};

//モデルの形状データ（頂点の集合体）
struct MeshData
{
	uint64_t unique_id{ 0 };							//メッシュの固有ID
	std::string name;									//メッシュの名前
	int64_t node_index{ 0 };							//シーン内のどのノード(ボーン)に紐づいているか
	DirectX::XMFLOAT4X4 default_global_transform;		//初期の配置行列
	std::vector<MeshSubset> subsets;					//マテリアル分けされたサブセットのリスト

	//シリアライズ用にCPU側にデータを一時保持するコンテナ
	std::vector<MeshVertex> vertices;
	std::vector<uint32_t> indices;

	Microsoft::WRL::ComPtr<ID3D11Buffer> vertex_buffer;	//頂点データをGPUに送るバッファ
	Microsoft::WRL::ComPtr<ID3D11Buffer> index_buffer;	//頂点の順番(インデックス)をGPUに送るバッファ
	DirectX::XMFLOAT3 bounding_box[2]
	{
		{+D3D11_FLOAT32_MAX,+D3D11_FLOAT32_MAX,+D3D11_FLOAT32_MAX},	//Min
		{-D3D11_FLOAT32_MAX,-D3D11_FLOAT32_MAX,-D3D11_FLOAT32_MAX}	//Max
	};
};

//アニメーションするための骨の情報
struct BoneData
{
	std::string name;						//ボーンの名前
	int64_t parent_index{ -1 };				//親のボーン番号
	DirectX::XMFLOAT4X4 offset_transform;	//逆バインドポーズ行列
};

//アニメーションの1フレームにおけるボーンの状態
struct AnimationKeyframeNode
{
	DirectX::XMFLOAT4X4 global_transform;	//計算済みの最終的な行列
	DirectX::XMFLOAT3 scaling;				//拡大縮小
	DirectX::XMFLOAT4 rotation;				//回転
	DirectX::XMFLOAT3 translation;			//位置移動
};

//アニメーション全体
struct AnimationClip
{
	std::string name;											//クリップ名
	float sampling_rate{ 0 };									//1秒間のフレーム数
	std::vector<std::vector<AnimationKeyframeNode>> sequence;	//キーフレームの配列(時間経過ごとの全ボーンの姿勢リスト)
};

//モデルの色やテクスチャの情報
struct MaterialData
{
	uint64_t unique_id{ 0 };													//マテリアル固有のID
	std::string name;															//マテリアル名
	DirectX::XMFLOAT4 color{ 1.0f,1.0f,1.0f,1.0f };								//基本色
	std::string texture_filenames[2];											//テクスチャのファイル名
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shader_resource_views[2];	//GPUでテクスチャを読むためのビュー
};

//モデルデータの読み込みと保持（読み取り専用）
class SkinnedMeshResource
{
	friend class SkinnedMeshSerializer;

public:
	//ファイル名を指定してモデルの読み込みを開始
	SkinnedMeshResource(ID3D11Device* device, const std::string& filename);
	~SkinnedMeshResource() = default;

	//別のFBXファイルからアニメーションのみを追加で読み込む
	bool AppendAnimations(const std::string& animation_filenama, float sampling_rate = 0.0f);

	//メッシュデータを取得
	const std::vector<MeshData>& GetMeshes()const { return meshes; }

	//ボーンデータを取得
	const std::vector<BoneData>& GetBones()const { return bones; }

	//アニメーションデータの取得
	const std::unordered_map<std::string, AnimationClip>& GetAnimations()const { return animations; }

	//マテリアルデータの取得
	const MaterialData* GetMaterial(uint64_t unique_id)const;

	//頂点シェーダーを取得
	ID3D11VertexShader* GetVertexShader()const { return vertex_shader.Get(); }

	//ピクセルシェーダーを取得
	ID3D11PixelShader* GetPixelShader()const { return pixel_shader.Get(); }

	//インプットレイアウトを取得
	ID3D11InputLayout* GetInputLayout()const { return input_layout.Get(); }

	//定数バッファを取得
	ID3D11Buffer* GetConstantBuffer()const { return constant_buffer.Get(); }

private:
	//内部計算用のヘルパー構造体（頂点ごとのボーン影響情報）
	struct BoneInfluence
	{
		uint32_t bone_index;
		float weight;
	};
	using BoneInfluencePerControlPoint = std::vector<BoneInfluence>;

	//FBXファイルを読み込み、メモリ上のデータに変換
	void LoadFbx(ID3D11Device* device, const std::string& filename);

	//マテリアルデータの抽出
	void FetchMaterials(ID3D11Device* device, fbxsdk::FbxScene* scene, const std::string& fbx_filename);

	//スケルトンの抽出
	void FetchSkeleton(fbxsdk::FbxScene* scene);

	//メッシュデータの抽出
	void FetchMeshes(ID3D11Device* device, fbxsdk::FbxScene* sceneconst);

	//アニメーションデータの抽出
	void FetchAnimations(fbxsdk::FbxScene* scene, float sampling_rate = 0.0f);

	//特定のメッシュの頂点に対するボーンの影響度を抽出
	void FetchBoneInfluences(const fbxsdk::FbxMesh* fbx_mesh, std::vector<BoneInfluencePerControlPoint>& influences);

	//読み込んだ頂点データを、DirectX（GPU）が使えるバッファに変換
	void CreateComBuffers(ID3D11Device* device, MeshData& mesh);

private:
	std::vector<MeshData> meshes;								//読み込んだメッシュデータを格納するリスト
	std::vector<BoneData> bones;								//読み込んだボーンデータを格納するリスト
	std::unordered_map<std::string, AnimationClip> animations;	//読み込んだアニメーションデータを格納するリスト
	std::unordered_map<uint64_t, MaterialData> materials;		//読み込んだマテリアルデータを格納するリスト

	//シェーダーにデータを送るための定数バッファ
	struct Constants
	{
		DirectX::XMFLOAT4X4 world;
		DirectX::XMFLOAT4 material_color;
		DirectX::XMFLOAT4X4 bone_transforms[256];
	};
	Microsoft::WRL::ComPtr<ID3D11Buffer> constant_buffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> scene_constant_buffer;

	//描画に使うシェーダープログラム
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertex_shader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixel_shader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> input_layout;
};

//個別のモデルのアニメーション状態管理と描画指示
class SkinnedMeshComponent
{
public:
	//Resource(モデル)を受け取って初期化
	SkinnedMeshComponent(std::shared_ptr<SkinnedMeshResource> resource);
	~SkinnedMeshComponent() = default;

	//アニメーションを更新
	void Update(float delta_time);

	//GPUに現在の姿勢データを渡して描画指示を出す
	void Render(ID3D11DeviceContext* context, const DirectX::XMFLOAT4X4 world_transform, const DirectX::XMFLOAT4 color = { 1.0f,1.0f,1.0f,1.0f });

	//再生するアニメーションを名前で切り替え
	void PlayAnimation(const std::string& clip_name, bool loop = true);

private:
	//アニメーション計算用ヘルパー
	void CalculateBoneTransforms();

private:
	std::shared_ptr<SkinnedMeshResource> resource;				//共有しているモデルデータへのポインタ
	std::string current_clip_name;								//現在再生中のアニメーション名
	float current_time{ 0.0f };									//現在のアニメーションの再生時間
	bool is_loop{ true };										//ループ再生するかどうか
	std::vector<DirectX::XMFLOAT4X4> current_bone_transforms;	//計算結果のボーン行列リスト
};
