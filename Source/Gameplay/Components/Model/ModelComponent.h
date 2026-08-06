#pragma once

#include "Gameplay/Components/Base/Component.h"
#include "ThiedParty/json.hpp"
#include <DirectXMath.h>
#include <memory>
#include <string>

struct ID3D11DeviceContext;
class GltfModel;
class GltfModelData;
class GltfModelRenderer;
class TransformComponent;

// ModelManager を介して GLTF モデルを保持・更新・描画するコンポーネント
class ModelComponent : public Component
{
public:
	//コンストラクタ
	ModelComponent();

	//デストラクタ
	virtual ~ModelComponent();

	//初期化処理
	void Initialize() override;

	//モデルファイルのロードと初期化
	bool LoadModel(const std::string& file_path);

	//アニメーション・モデル更新処理
	void Update(float elapsed_time) override;

	//描画処理
	void Render(ID3D11DeviceContext* context) override;

	//ImGuiデバッグ描画
	void RenderGui() override;

	//トランスフォームコンポーネントの登録
	void SetTransformComponent(const std::shared_ptr<TransformComponent>& transform);

	//Jsonへのモデルパスデータ保存
	void SaveToObject(nlohmann::json& object_json) const;

	//Jsonへのモデルパスデータ復元とモデルロード
	void LoadFromJObject(const nlohmann::json& object_json);

	//描画表示フラグ設定
	void SetVisible(bool visible) { is_visible = visible; }

	//描画表示フラグ取得
	bool IsVisible() const { return is_visible; }

	//モデルパス取得
	const std::string& GetModelPath() const { return model_path; }

	//モデルデータ取得
	std::shared_ptr<GltfModelData> GetModelData() const { return model_data; }

	//モデル取得
	GltfModel* GetModel() const { return model.get(); }

private:
	//モデルの描画処理
	void RenderInternal(ID3D11DeviceContext* context);

private:
	std::shared_ptr<GltfModelData> model_data = nullptr;       //ModelManagerから共有キャッシュされるモデルデータ
	std::shared_ptr<GltfModelRenderer> renderer = nullptr;     //描画を司るレンダラー
	std::unique_ptr<GltfModel> model = nullptr;                //個別オブジェクトのモデル実体
	std::weak_ptr<TransformComponent> target_transform;        //描画用トランスフォームへの弱参照
	std::string model_path = "";                               //ロードしているモデルファイルパス
	bool is_visible = true;                                    //描画可否フラグ
};