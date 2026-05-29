#pragma once

#include <vector>
#include <memory>

class GameObject;
struct ID3D11DeviceContext;

class ObjectManager
{
public:
	//コンストラクタ
	ObjectManager();

	//デストラクタ
	~ObjectManager();

	//オブジェクトの登録
	void Register(std::unique_ptr<GameObject> object);

	//全オブジェクトの更新処理
	void Update(float elapsed_time);

	//全オブジェクトの描画処理
	void Render(ID3D11DeviceContext* context);

	//全オブジェクトのImGuiデバッグ描画処理
	void RenderGui();

	//全オブジェクトの強制削除処理
	void Clear() { game_objects.clear(); }

private:
	//無効になったオブジェクトの削除処理
	void RemoveInactiveObject();

private:
	std::vector<std::unique_ptr<GameObject>> game_objects;	//ゲームオブジェクトを管理する配列
};

