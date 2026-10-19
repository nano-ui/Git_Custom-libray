#pragma once

#include <DirectXMath.h>
#include <string>
#include <d3d11.h>

#include "Engine\Graphics\Renderers\ShapeRenderer.h"
#include "Engine\Graphics\Resources\Model.h"
#include "Serialization\JsonSerializer.h"
#include "Editor\GuiInspector.h"
#include "Gameplay\Components\Base\Component.h"

struct Collider;

class GameObject
{
public:
	//コンストラクタ
	GameObject();

	//デストラクタ
	virtual ~GameObject();

	//初期化処理
	virtual void Initialize();

	//更新処理
	virtual void Update(float elapsed_time);

	//描画処理
	virtual void Render(ID3D11DeviceContext* context);

	//デバッグ描画処理
	virtual void RenderDebug(ShapeRenderer* renderer) = 0;

	//ImGuiデバッグ描画
	virtual void RenderGui();

	//コンポーネントの追加
	template<typename T,typename... Args>
	std::shared_ptr<T> AddComponent(Args&&... args)
	{
		static_assert(std::is_base_of<Component, T>::value, "Tは Component の派生クラスである必要があります");
		std::shared_ptr<T> new_component = std::make_shared<T>(std::forward<Args>(args)...);
		if (!new_component)
		{
			OutputDebugStringA("[GameObject エラー] AddComponent: コンポーネントの生成に失敗しました。\n");
			return nullptr;
		}
		components.push_back(new_component);
		return new_component;
	}

	//コンポーネントの取得
	template<typename T>
	std::shared_ptr<T> GetComponent()const
	{
		static_assert(std::is_base_of<Component, T>::value, "T は Component の派生クラスである必要があります。");

		//アタッチされているコンポーネントを走査して型判定
		for (const auto& component : components)
		{
			if (!component)continue;

			//型チェック
			std::shared_ptr<T> casted_component = std::dynamic_pointer_cast<T>(component);
			if (casted_component)return casted_component;
		}
		return nullptr;
	}

	//コンポーネントの削除
	template <typename T>
	bool RemoveComponent()
	{
		static_assert(std::is_base_of<Component, T>::value, "T は Component の派生クラスである必要があります。");

		for (auto iterator = components.begin(); iterator != components.end(); iterator++)
		{
			if (!(*iterator))continue;
			std::shared_ptr<T>casted_component = std::dynamic_pointer_cast<T>(*iterator);
			if (casted_component)
			{
				components.erase(iterator);
				return true;
			}
		}
		OutputDebugStringA("[GameObject 警告] RemoveComponent: 削除対象のコンポーネントが見つかりませんでした。\n");
		return false;
	}
	
	// シリアライズ・インスペクター登録
	virtual void SetupSerialization();
	virtual void SetupInspector();

	//指定されたJSONオブジェクトへ自身のデータを書き込む
	virtual void SaveToJObject(nlohmann::json& object_json, SerializeTarget target = SerializeTarget::All);

	//指定されたJSONオブジェクトから自身のデータを復元
	virtual void LoadFromJObject(const nlohmann::json& object_json, SerializeTarget target = SerializeTarget::All);

	//パラメータをJSONファイルへ保存
	void SaveToJson(SerializeTarget target = SerializeTarget::Base);

	//JSONファイルからパラメータを復元
	void LoadFromJson(SerializeTarget target = SerializeTarget::Base);

	//クラス名とモデル個別情報のみを保存
	bool SaveToDetailFile(const std::string& file_path);

	//モデル個別情報のみを復元
	bool LoadFromDetailFile(const std::string& file_path);

	//オブジェクト生成前に、個別 JSON ファイルから class_nameを読み取る
	static std::string ReadClassNameFromDetailFile(const std::string& file_path);

	//削除要求
	void Destory() { is_active = false; }

	//生存フラグを取得
	bool IsActive()const { return is_active; }

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

	//オブジェクトIDを取得
	uint32_t GetObjectId()const { return object_id; }

protected:
	//コライダーを登録
	void AddCollider(Collider* collider) { collideres.push_back(collider); }

protected:
	std::unique_ptr<JsonSerializer> serializer;	//自身専用のシリアライザ
	std::unique_ptr<GuiInspector> inspector;	//UI表示用インスペクター
	std::vector<std::shared_ptr<Component>> components;	//コンポーネント群のリスト
	DirectX::XMFLOAT4 color;	//色
	bool is_active;				//生存フラグ
	std::vector<Collider*> collideres;	//コライダーのリスト
	std::string class_name;				//クラス名
	uint32_t model_hash = 0;			//モデル識別ハッシュ値
	uint32_t object_id;					//オブジェクトID
	static constexpr uint32_t INVALID_OBJECT_ID = 0;	//ID未設定時の値
};

