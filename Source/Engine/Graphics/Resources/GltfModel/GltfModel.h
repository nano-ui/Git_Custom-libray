#pragma once

#include <memory>
#include <string>
#include <vector>
#include <DirectXMath.h>

#include "Engine\Graphics\Resources\GltfModel\GltfModelData.h"
#include "Engine\Graphics\Device\Frustum.h"

class GltfModelAnimation;
class GltfModelRenderer;
class ModelRenderQueue;
struct ID3D11DeviceContext;

class GltfModel
{
public:
	//コンストラクタ
	GltfModel(const std::shared_ptr<GltfModelData>& model_data, const std::shared_ptr<GltfModelRenderer> model_renderer);

	//デストラクタ
	~GltfModel();

	//更新処理
	void Update(float delta_time);

	//描画処理
	void Render(ID3D11DeviceContext* immediate_context, const DirectX::XMFLOAT4X4& world_matrix);

	//レンダーキューに登録
	void SubmitRenderCommands(ModelRenderQueue& queue, const DirectX::XMFLOAT4X4& world_matrix);

	//視錐台カリング内外判定の更新
	void UpdateFrustumVisibility(const Frustum& frustum, const DirectX::XMFLOAT4X4& world_matrix);

	//視錐台内に存在するか取得
	bool IsInFrustum() const { return is_in_frustum; }

	//バウンディングスフィアの手動設定
	void SetBoundingSphere(const DirectX::XMFLOAT3& center, float radius)
	{
		bounding_sphere.center = center;
		bounding_sphere.radius = radius;
	}

	//バウンディングスフィアの取得
	const BoundingSphere& GetBoundingSphere() const { return bounding_sphere; }

	//アニメーション切り替え
	void PlayAnimation(const std::string& animation_name, bool is_loop);

	//アニメーションの終了したか取得
	bool IsAnimationFinished()const;

	//モデル情報取得
	std::shared_ptr<GltfModelData> GetData()const { return data; }

	//アニメーション情報取得
	const std::vector<GltfModelData::node>& GetAnimatedNodes() const;

	//現在の再生経過時間を取得
	float GetAnimationCurrentTime() const;

	//再生時間を設定
	void SetAnimationTime(float time);

	//アニメーションの総時間を取得
	float GetAnimationDuration() const;

	//指定したノードの移動成分を外部から上書き
	void SetNodeTranslation(int node_index, const DirectX::XMFLOAT3& translation);

	//変更されたノード情報をもとに、グローバル行列を再計算
	void RecalculateTransforms();

	//アニメーションブレンド
	void AnimateBlend(const std::string& anim_a, float time_a, const std::string& anim_b, float time_b, float blend_factor);

	//ノード名からインデックスを検索
	int FindNodeIndex(const std::string& node_name)const;

	//ノード名からモデル空間のグローバル行列を取得
	bool GetNodeGlobalTransform(const std::string& node_name, DirectX::XMFLOAT4X4& out_transform)const;

	//ノードインデックスからモデル空間のグローバル行列を取得
	bool GetNodeGlobalTransform(int node_index, DirectX::XMFLOAT4X4& out_transform)const;

private:
	//モデル頂点データからバウンディングスフィアを算出
	void ComputeBoundingSphere();

private:
	std::shared_ptr<GltfModelData> data;			//リソースデータ
	std::shared_ptr<GltfModelRenderer> renderer;	//描画命令クラス
	std::unique_ptr<GltfModelAnimation> animation;	//アニメーション計算クラス
	BoundingSphere bounding_sphere;					//ローカル空間のバウンディングスフィア
	bool is_in_frustum = true;						//視錐台内存在フラグ
};

