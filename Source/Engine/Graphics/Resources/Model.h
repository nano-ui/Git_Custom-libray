#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <memory>
#include <string>
#include <vector>

#include "ThiedParty/json.hpp"
#include "Engine/Graphics/Resources/GltfModel/GltfModelData.h"

class GltfModel;
class GltfModelRenderer;
class RootMotionComponent;

class Model
{
public:
	//コンストラクタ・デストラクタ
	Model();
	~Model();

	//初期化処理（ModelManagerを利用して共有ロード）
	bool Initialize(const std::string& file_path);

	//更新処理（アニメーションおよびルートモーション）
	void Update(float elapsed_time);

	//描画処理
	void Render(ID3D11DeviceContext* context, const DirectX::XMFLOAT4X4& world, const DirectX::XMFLOAT4 color = { 1.0f, 1.0f, 1.0f, 1.0f });

	//ImGuiでのデバッグ用パラメータ描画
	void DrawImGui();

	//JSONへのモデルデータ書き出し
	void SaveToObject(nlohmann::json& object_json) const;

	//JSONからのモデルデータ復元と自動読み込み
	void LoadFromJObject(const nlohmann::json& object_json);

	//アニメーション再生
	void PlayAnimation(const std::string& animation_name, bool is_loop = true);

	//アニメーション時間の取得・設定
	float GetAnimationTime() const;
	void SetAnimationTime(float time);

	//アニメーション総時間の取得
	float GetAnimationDuration() const;

	//アニメーション終了判定
	bool IsAnimationFinished() const;

	//アニメーション名からインデックス取得
	int GetAnimationIndex(const char* name) const;

	//ルートモーション差分の取得
	DirectX::XMFLOAT3 GetDeltaPosition() const;
	DirectX::XMFLOAT4 GetDeltaRotation() const;

	//描画表示フラグ設定・取得
	void SetVisible(bool visible) { is_visible = visible; }
	bool IsVisible() const { return is_visible; }

	//モデルパス取得
	const std::string& GetModelPath() const { return model_path; }

	//glTF共有データ構造体の取得
	std::shared_ptr<const GltfModelData> GetGltfModelData() const { return data; }

	//アニメーション計算適用後の動的ノード配列取得
	const std::vector<GltfModelData::node>& GetAnimatedNodes() const;

	//指定ノードの位置の上書き
	void SetNodeTranslation(int node_index, const DirectX::XMFLOAT3& translation);

	//姿勢行列の再計算
	void RecalculateTransforms();

private:
	//使いまわさない内部専用のモデルロード処理
	bool LoadModelInternal(const std::string& file_path);

private:
	std::shared_ptr<GltfModelData> data = nullptr;				//ModelManagerから共有キャッシュされるモデルデータ
	std::shared_ptr<GltfModelRenderer> renderer = nullptr;		//描画を司るレンダラー
	std::unique_ptr<GltfModel> model = nullptr;					//個別オブジェクトのノード姿勢制御用実体
	std::unique_ptr<RootMotionComponent> root_motion_component = nullptr; //ルートモーション計算コンポーネント
	std::string model_path = "";								//ロード中のモデルファイルパス
	bool is_visible = true;										//描画表示可否フラグ
};