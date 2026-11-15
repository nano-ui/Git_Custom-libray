#pragma once

#include <DirectXMath.h>
#include <memory>

class GameObject;
class Camera;
class CollisionManager;
class CollisionLogic;

class ObjectPicker
{
public:
	//選択されたオブジェクトを取得
	GameObject* PickObject(
		GameObject* selected_object,
		Camera* camera,
		CollisionManager* collision_manager,
		const DirectX::XMFLOAT2& preview_pos,
		const DirectX::XMFLOAT2& preview_size
	);
private:
	std::unique_ptr<CollisionLogic> collision_logic;	//当たり判定
};

