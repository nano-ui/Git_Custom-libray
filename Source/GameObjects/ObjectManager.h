#pragma once

#include <vector>
#include <memory>

class ShapeRenderer;
class GameObject;
struct ID3D11DeviceContext;

class ObjectManager
{
public:
	//コンストラクタ
	ObjectManager();

	//デストラクタ
	~ObjectManager();

	//オブジェクトの生成・初期化・自動登録
	template <class T, class... Args>
	T* Instantiate(Args&&... args) 
	{
		//オブジェクトの生成
		std::unique_ptr<T> new_object = std::make_unique<T>(std::forward<Args>(args)...);
		T* raw_pointer = new_object.get();

		//派生クラスの構築完了を待ってから初期化を実行
		new_object->Initialize();

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
	void Clear() { game_objects.clear(); }

private:
	//無効になったオブジェクトの削除処理
	void RemoveInactiveObject();

private:
	std::vector<std::unique_ptr<GameObject>> game_objects;	//ゲームオブジェクトを管理する配列
};

