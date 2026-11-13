#pragma once
#include <DirectXMath.h>
#include <string>

class GameObject;

class ObjectSpawner
{
public:
	//オブジェクト出現
	static GameObject* ObjectSpawn(const std::string& model_path, const DirectX::XMFLOAT3& spawn_positon);
};

