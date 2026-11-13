#pragma once
#include <DirectXMath.h>

class Camera;
class CollisionManager;

class PlacementUtility
{
public:
	//”z’uˆÊ’u‚ðŽæ“¾
	static bool GetPlacementPositionFromNdc(DirectX::XMFLOAT2 ndc, Camera* camera, CollisionManager* collision_manager, DirectX::XMFLOAT3& out_hit_position);
};

