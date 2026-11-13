#include "PlacementUtility.h"
#include "Engine\Camera\Camera.h"
#include "Engine\Collision\CollisionManager.h"
#include "Engine\Core\Log\Log.h"
#include "Utility\RayUtility.h"

//配置位置を取得
bool PlacementUtility::GetPlacementPositionFromNdc(
	DirectX::XMFLOAT2 ndc,
	Camera* camera,
	CollisionManager* collision_manager,
	DirectX::XMFLOAT3& out_hit_position)
{
	//ガード処理
	if (!camera || !collision_manager)
	{
		LOG_ERROR({ u8"設定" }, u8"設定されたポインタがnullです");
		return false;
	}
	
	DirectX::XMFLOAT3 ray_start = { 0.0f,0.0f,0.0f };
	DirectX::XMFLOAT3 ray_end = { 0.0f,0.0f,0.0f };
	if (!RayUtility::NdcToWorldRay(ndc, camera->GetView(), camera->GetProjection(), ray_start, ray_end))return false;
	return collision_manager->RayCastSpace(ray_start, ray_end, out_hit_position);
}
