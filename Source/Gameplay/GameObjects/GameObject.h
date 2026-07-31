#pragma once

#include <DirectXMath.h>
#include <string>
#include <d3d11.h>

#include "Engine\Graphics\Renderers\ShapeRenderer.h"
#include "Engine\Graphics\Resources\Model.h"
#include "Serialization\JsonSerializer.h"
#include "Editor\GuiInspector.h"

struct Collider;

class GameObject
{
public:
	//コンストラクタ
	GameObject();

	//デストラクタ
	virtual ~GameObject();

	//初期化処理
	virtual void Initialize() = 0;

	//更新処理
	virtual void Update(float elapsed_time) = 0;

	//描画処理
	virtual void Render(ID3D11DeviceContext* context) = 0;

	//デバッグ描画処理
	virtual void RenderDebug(ShapeRenderer* renderer) = 0;

	//ImGuiデバッグ描画
	virtual void RenderGui();

	//をシリアライザに登録
	virtual void SetupSerialization();

	//指定されたJSONオブジェクトへ自身のデータを書き込む
	virtual void SaveToJObject(nlohmann::json& object_json);

	//指定されたJSONオブジェクトから自身のデータを復元
	virtual void LoadFromJObject(const nlohmann::json& object_json);

	//パラメータをJSONファイルへ保存
	void SaveToJson();

	//JSONファイルからパラメータを復元
	void LoadFromJson();

	//削除要求
	void Destory() { is_active = false; }

	//生存フラグを取得
	bool IsActive()const { return is_active; }

	//ワールド変換行列の合成、取得
	DirectX::XMMATRIX GetWorldMatrix()const;

	//座標取得
	DirectX::XMFLOAT3 GetPosition()const { return position; }

	//座標を設定
	void SetPosition(DirectX::XMFLOAT3 pos) { position = pos; }

	//回転取得
	DirectX::XMFLOAT4 GetRotation()const { return rotation; }

	//回転設定
	void SetRotation(DirectX::XMFLOAT4 rot) { rotation = rot; }

	//スケール取得
	DirectX::XMFLOAT3 GetScale()const { return scale; }

	//スケール設定
	void SetScale(DirectX::XMFLOAT3 scl) { scale = scl; }

	//コライダーを取得
	const std::vector<Collider*>GetColliders()const { return collideres; }

	//クラス名設定
	void SetClassName(const std::string& name) { class_name = name; }

	//クラス名取得
	const std::string& GetClassName()const { return class_name; }

	//モデル識別ハッシュ値を取得
	virtual uint32_t GetModelHash()const { return model_hash; }

	//モデル識別ハッシュ値の設定
	void SetModelHash(uint32_t hash) { model_hash = hash; }

	//モデル取得
	Model* GetModel()const { return model.get(); }

protected:
	//コライダーを登録
	void AddCollider(Collider* collider) { collideres.push_back(collider); }

protected:
	std::unique_ptr<Model> model;				//モデルインスタンス
	std::unique_ptr<JsonSerializer> serializer;	//自身専用のシリアライザ
	std::unique_ptr<GuiInspector> inspector;	//UI表示用インスペクター
	DirectX::XMFLOAT3 position;	//位置
	DirectX::XMFLOAT4 rotation;	//角度
	DirectX::XMFLOAT3 scale;	//スケール倍率
	DirectX::XMFLOAT4 color;	//色
	bool is_active;				//生存フラグ
	std::vector<Collider*> collideres;	//コライダーのリスト
	std::string class_name;				//クラス名
	uint32_t model_hash = 0;			//モデル識別ハッシュ値
};

