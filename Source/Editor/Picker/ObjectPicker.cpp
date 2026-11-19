#include "ObjectPicker.h"
#include "Gameplay\GameObjects\GameObject.h"
#include "Gameplay\GameObjects\ObjectManager.h"
#include "Gameplay\Components\Transform\TransformComponent.h"
#include "Engine\Camera\Camera.h"
#include "Engine\Collision\CollisionManager.h"
#include "Engine\Core\Log\Log.h"
#include "Engine\Core\Input.h"
#include "Utility\ConvertUtility.h"
#include "Utility\RayUtility.h"

#include <imgui.h>
#include <ImGuizmo.h>

//選択されたオブジェクトを取得
GameObject* ObjectPicker::PickObject(
	GameObject* selected_object,
	Camera* camera,
	CollisionManager* collision_manager,
	const DirectX::XMFLOAT2& preview_pos,
	const DirectX::XMFLOAT2& preview_size)
{
	//ガード処理 ・早期判定
	if (!camera || !collision_manager)
	{
		LOG_ERROR({ u8"設定" }, u8"設定されたポインタがnullです");
		return selected_object;
	}
	if (Input::Instance().IsKeyTrigger(VK_ESCAPE))return nullptr;
	if (ImGuizmo::IsOver() || ImGuizmo::IsUsing())return selected_object;

	//クリックとプレビュー領域の判定
	if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
	{
		ImVec2 mouse_pos = ImGui::GetIO().MousePos;
		DirectX::XMFLOAT2 preview_rect = { preview_pos.x + preview_size.x,preview_pos.y + preview_size.y };
		if (preview_pos.x <= mouse_pos.x && mouse_pos.x <= preview_rect.x && preview_pos.y <= mouse_pos.y && mouse_pos.y <= preview_rect.y)
		{
			//NDC（正規化デバイス座標）への変換
			DirectX::XMFLOAT2 preview_diff = { mouse_pos.x - preview_pos.x,mouse_pos.y - preview_pos.y };
			DirectX::XMFLOAT2 ndc = ConvertUtility::ConvertNDC(preview_size.x, preview_size.y, preview_diff);

			//レイキャストによる交差判定
			DirectX::XMFLOAT3 ray_start = { 0.0f,0.0f,0.0f };
			DirectX::XMFLOAT3 ray_end = { 0.0f,0.0f,0.0f };
			if (RayUtility::NdcToWorldRay(ndc, camera->GetView(), camera->GetProjection(), ray_start, ray_end))
			{
				GameObject* hit_object = nullptr;
				float closest_distance = FLT_MAX;
				DirectX::XMFLOAT3 ray_dir = { 0.0f,0.0f,0.0f };
				DirectX::XMVECTOR v_start = DirectX::XMLoadFloat3(&ray_start);
				DirectX::XMVECTOR v_end = DirectX::XMLoadFloat3(&ray_end);
				DirectX::XMVECTOR v_dir = DirectX::XMVectorSubtract(v_end, v_start);
				v_dir = DirectX::XMVector3Normalize(v_dir);
				DirectX::XMStoreFloat3(&ray_dir, v_dir);
				for (const auto& obj : ObjectManager::Instance().GetGameObjects())
				{
					if (!obj->IsActive())continue;
					std::shared_ptr<TransformComponent> transform = obj->GetComponent<TransformComponent>();
					if (!transform)continue;
					float pick_radius = 1.0f * transform->GetScale().x;

					float dist = 0.0f;
					if (collision_logic->RaySphere(ray_start, ray_dir, transform->GetPosition(), pick_radius, dist))
					{
						if (dist <= closest_distance)
						{
							closest_distance = dist;
							hit_object = obj.get();
						}
					}
				}
				if (!hit_object)return selected_object;
				return hit_object;
			}
			return selected_object;
		}
		return selected_object;
	}
	return selected_object;
}
