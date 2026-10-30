#include "ObjectManager.h"
#include "GameObject.h"
#include "Character/Character.h"
#include "Engine/Graphics/Resources/GltfModel/ModelRenderQueue.h"
#include "Engine/Graphics/Resources/GltfModel/GltfModelRenderer.h"
#include "Gameplay\Components\Animation\AnimationComponent.h"
#include <Engine\Core\MacroTracy.h>
#include <Engine\Graphics\Resources\ModelManager.h>

//静的の実体定義
ObjectManager* ObjectManager::instance_ptr = nullptr;

//コンストラクタ
ObjectManager::ObjectManager()
{
	instance_ptr = this;
	model_render_queue = std::make_unique<ModelRenderQueue>();
}

//デストラクタ
ObjectManager::~ObjectManager()
{
	if (instance_ptr == this)
	{
		instance_ptr = nullptr;
	}

	Clear();
}

//オブジェクトの登録
void ObjectManager::Register(std::unique_ptr<GameObject> object)
{
	BS_PROFILE_FUNCTION();

	if (!object)
	{
		OutputDebugStringA("[ObjectManager エラー] Register: 登録対象の object が nullptr です。\n");
		return;
	}

	//プレハブシステムの完全自動化処理
	object->Initialize();
	object->SetupSerialization();
	object->LoadFromJson();

	//コライダーの登録
	if (collision_manager)
	{
		for (Collider* col : object->GetColliders())
		{
			if (!col)
			{
				continue;
			}
			collision_manager->Register(col);
		}
	}

	//リストへの登録
	game_objects.push_back(std::move(object));
}

//全オブジェクトの更新処理
void ObjectManager::Update(float elapsed_time)
{
	BS_PROFILE_FUNCTION();

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
	BS_PROFILE_FUNCTION();

	if (!context)
	{
		OutputDebugStringA("[ObjectManager エラー] Render: context が nullptr です。\n");
		return;
	}

	if (model_render_queue)
	{
		model_render_queue->Clear();
	}

	for (auto& object : game_objects)
	{
		if (object->IsActive())
		{
			object->Render(context);
		}
	}

	if (model_render_queue && model_render_queue->GetCommandCount() > 0)
	{
		model_render_queue->Sort();

		std::shared_ptr<GltfModelRenderer> renderer = ModelManager::Instance().GetRenderer();
		if (renderer)
		{
			renderer->RenderQueue(context, *model_render_queue);
		}
		else
		{
			OutputDebugStringA("[ObjectManager エラー] Render: GltfModelRenderer の取得に失敗しました。\n");
		}

		model_render_queue->Clear();
	}
}

//全オブジェクトのImGuiデバッグ描画処理
void ObjectManager::RenderGui()
{
	//生存している全オブジェクトのImGui描画処理
	for (auto& object : game_objects)
	{
		if (object->IsActive() && object->IsDebug())
		{
			object->RenderGui();
		}
	}
}

//全オブジェクトのデバッグ描画
void ObjectManager::RenderDebug(ShapeRenderer* renderer)
{
	BS_PROFILE_FUNCTION();

	//生存している全オブジェクトのデバッグ描画処理
	for (auto& object : game_objects)
	{
		if (object->IsActive())
		{
			object->RenderDebug(renderer);
		}
	}
}

//アニメーションマップの更新
void ObjectManager::RefreshAnimationMap(uint32_t target_model_hash, const std::unordered_map<uint32_t, std::string>& new_map)
{
	BS_PROFILE_FUNCTION();

	//全てのオブジェクトを走査
	for (auto iterator = game_objects.begin(); iterator != game_objects.end(); iterator++)
	{
		//モデルハッシュが一致するか確認
		if ((*iterator)->GetModelHash() == target_model_hash)
		{
			Character* character_obj = dynamic_cast<Character*>(iterator->get());	//キャラクターのモデル
		}
	}
}

//無効になったオブジェクトの削除処理
void ObjectManager::RemoveInactiveObject()
{
	BS_PROFILE_FUNCTION();

	for (auto iterator = game_objects.begin(); iterator != game_objects.end();)
	{
		if (!(*iterator)->IsActive())
		{
			if (collision_manager)
			{
				for (Collider* col : (*iterator)->GetColliders())
				{
					collision_manager->Remove(col);
				}
			}
			iterator = game_objects.erase(iterator);
		}
		else
		{
			iterator++;
		}
	}
}

void ObjectManager::Clear()
{
	BS_PROFILE_FUNCTION();

	if (collision_manager)
	{
		for (auto& object : game_objects)
		{
			for (Collider* col : object->GetColliders())
			{
				collision_manager->Remove(col);
			}
		}
	}
	game_objects.clear();
}
