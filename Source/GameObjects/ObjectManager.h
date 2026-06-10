#pragma once

#include <vector>
#include <memory>

#include "../Collision/CollisionManager.h"

class ShapeRenderer;
class GameObject;
struct ID3D11DeviceContext;
struct Collider;

class ObjectManager
{
public:
	//コンストラクタ
	ObjectManager();

	//デストラクタ
	~ObjectManager();

	//シングルトンの取得
	static ObjectManager& Instance() { return *instance_ptr; }

	//オブジェクトの生成・初期化・自動登録
	template <class T, class... Args>
	T* Instantiate(Args&&... args) 
	{
		//オブジェクトの生成
		std::unique_ptr<T> new_object = std::make_unique<T>(std::forward<Args>(args)...);
		T* raw_pointer = new_object.get();

		//プレハブシステムの完全自動化処理
		new_object->SetupSerialization();
		new_object->LoadFromJson();
		new_object->Initialize();

		//コライダーの自動登録処理
		if (collision_manager)
		{
			auto colliders = new_object->GetColliders();
			for (size_t i = 0; i < colliders.size(); i++) 
			{
				Collider* col = colliders[i];
				if (!col) continue;
				collision_manager->Register(col);
			}
		}

		//マネージャーのリストへ所有権を移動して登録
		game_objects.push_back(std::move(new_object));
		return raw_pointer;
	}

	//オブジェクトの登録
	void Register(std::unique_ptr<GameObject> object);

	//全オブジェクトの更新処理
	void Update(float elapsed_time);

	//全オブジェクトの描画処理
	void Render(ID3D11DeviceContext* context);

	//全オブジェクトのImGuiデバッグ描画処理
	void RenderGui();

	//全オブジェクトのデバッグ描画
	void RenderDebug(ShapeRenderer* renderer);

	//全オブジェクトの強制削除処理
	void Clear();

	//当たり判定マネージャーの連携
	void SetCollisionManager(CollisionManager* manager) { collision_manager = manager; }

	//全ゲームオブジェクトのリストへの参照を取得
	const std::vector<std::unique_ptr<GameObject>>& GetGameObjects()const { return game_objects; }

private:
	//無効になったオブジェクトの削除処理
	void RemoveInactiveObject();

private:
	std::vector<std::unique_ptr<GameObject>> game_objects;	//ゲームオブジェクトを管理する配列
	CollisionManager* collision_manager = nullptr;			//CollisionManagerのポインタ
	static ObjectManager* instance_ptr;						//静的インスタンスポインタ
};

