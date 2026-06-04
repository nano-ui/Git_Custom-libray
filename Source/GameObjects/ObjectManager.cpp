#include "ObjectManager.h"
#include "GameObject.h"

//コンストラクタ
ObjectManager::ObjectManager()
{
}

//デストラクタ
ObjectManager::~ObjectManager()
{
	Clear();
}

//オブジェクトの登録
void ObjectManager::Register(std::unique_ptr<GameObject> object)
{
	//オブジェクトの初期化とリストに追加
	object->Initialize();
	game_objects.push_back(std::move(object));
}

//全オブジェクトの更新処理
void ObjectManager::Update(float elapsed_time)
{
	//生存している全オブジェクトの更新処理
	for (auto& object : game_objects)
	{
		if (object->IsActive())
		{
			object->Update(elapsed_time);
		}
	}

	//不要なオブジェクトの削除
	RemoveInactiveObject();
}

//全オブジェクトの描画処理
void ObjectManager::Render(ID3D11DeviceContext* context)
{
	//生存している全オブジェクトの描画処理
	for (auto& object : game_objects)
	{
		if (object->IsActive())
		{
			object->Render(context);
		}
	}
}

//全オブジェクトのImGuiデバッグ描画処理
void ObjectManager::RenderGui()
{
	//生存している全オブジェクトのImGui描画処理
	for (auto& object : game_objects)
	{
		if (object->IsActive())
		{
			object->RenderGui();
		}
	}
}

//全オブジェクトのデバッグ描画
void ObjectManager::RenderDebug(ShapeRenderer* renderer)
{
	//生存している全オブジェクトのデバッグ描画処理
	for (auto& object : game_objects)
	{
		if (object->IsActive())
		{
			object->RenderDebug(renderer);
		}
	}
}

//無効になったオブジェクトの削除処理
void ObjectManager::RemoveInactiveObject()
{
	for (auto iterator = game_objects.begin(); iterator != game_objects.end();)
	{
		if (!(*iterator)->IsActive())
		{
			iterator = game_objects.erase(iterator);
		}
		else
		{
			iterator++;
		}
	}
}
