#pragma once

#include "GltfModelData.h"
#include <stack>
#include <vector>
#include <memory>

class GltfModelAnimation
{
public:
	//アニメーションクラスをモデルデータで初期化
	void Initialize(const std::shared_ptr<const GltfModelData>& data);

	//親子関係をたどって各ノードのグローバル行列を計算
	void CumulateTransforms(); 

	//名前を指定してアニメーションの再生を開始
	void PlayAniamtion(const std::string& animation_name, bool is_loop);

	//アニメーション更新処理
	void UpdateAnimation(float delta_time);

	//計算済みノード配列を取得
	const std::vector<GltfModelData::node>& GetAnimationNodes()const { return animated_nodes; }

	//アニメーションの終了したか取得
	bool IsFinished()const { return is_animation_finished; }

	//現在のアニメーション経過時間を取得
	float GetCurrentAnimationTime()const;

	//アニメーションの再生時間を設定
	void SetCurrentAnimationTime(float time);

	//アニメーションの総時間を取得
	float GetAnimationDuration()const;

	//指定したノードの移動成分を外部から上書き
	void SetNodeTranslation(int node_index, const DirectX::XMFLOAT3& translation);

	//二つのアニメーション姿勢の合成
	void AnimationBlend(size_t animation_index_a, float time_a, size_t animation_index_b, float time_b, float blend_factor);

	int ComputeTargetRootNodeIndex(size_t animation_index) const;

private:

	//ノードのローカル変換成分（TRS）のみを保持する軽量構造体
	struct NodePose
	{
		DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };
		DirectX::XMFLOAT4 rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
		DirectX::XMFLOAT3 translation = { 0.0f, 0.0f, 0.0f };
	};

	//アニメーション適用対象のパラメータ種類
	enum class TargetPath
	{
		Scale,
		Rotation,
		Translation,
		Unknown
	};

	//毎フレームの検索負荷をなくすための高速アクセス用キャッシュ
	struct AnimationChannelCache
	{
		int target_node = -1;
		TargetPath path_type = TargetPath::Unknown;
		const std::vector<float>* timeline = nullptr;
		const std::vector<DirectX::XMFLOAT3>* scales = nullptr;
		const std::vector<DirectX::XMFLOAT4>* rotations = nullptr;
		const std::vector<DirectX::XMFLOAT3>* translations = nullptr;
	};

	//指定した時間のアニメーションからTRS成分のみをサンプリング
	void SampleAnimationPose(size_t animation_index, float time, std::vector<NodePose>& out_poses);

	//指定した時間のアニメーションを適用しノード情報を更新
	void Animate(size_t animation_index, float time);

	//対象のノードがシーンの最上位ルートノードが判定
	bool IsRootNode(int node_index)const;

	//指定アニメーション内で最も移動量の大きいルートモーション対象ノードのインデックスを取得
	int GetTargetRootNodeIndex(size_t animation_index) const;

	//アニメーションのキーフレームを取得し補間係数を計算
	size_t GetAnimationKeyframeIndex(const std::vector<float>& timelines, float time, float& interpolation_factor);

	//アニメーションの全体の長さを計算
	float CalculateAnimationDuration(size_t animation_index);

	//アニメーションキャッシュの構築
	void BuildAnimationCache();

private:
	std::shared_ptr<const GltfModelData> model_data;	//モデルデータの保持
	std::vector<GltfModelData::node> animated_nodes;	//アニメーション計算結果を格納
	std::vector<int> target_root_node_indices;
	std::vector<std::vector<AnimationChannelCache>> animation_caches; // 事前バインドされたキャッシュ

	std::vector<NodePose> work_poses;		//単一アニメーション計算用バッファ
	std::vector<NodePose> blend_poses_a;	//ブレンド用バッファA
	std::vector<NodePose> blend_poses_b;	//ブレンド用バッファB

	size_t current_animation_index = 0;			//現在再生中のアニメーション番号
	float current_animation_time = 0.0f;		//現在の再生時間
	float current_animation_duration = 0.0f;	//アニメーション終了時間
	bool is_loop_enabled = true;				//ループ再生の有効フラグ
	bool is_playing = false;					//アニメーション再生中判定フラグ
	bool is_animation_finished = false;			//アニメーションの終了フラグ
};

