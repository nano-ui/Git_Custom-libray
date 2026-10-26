#include "VisionSensorComponent.h"
#include "Editor\GuiInspector.h"
#include "Serialization\JsonSerializer.h"
#include "Engine\Core\Log\Log.h"
#include "Engine\Graphics\Renderers\ShapeRenderer.h"
#include "Gameplay\Components\Transform\TransformComponent.h"
#include <Engine\Core\MacroTracy.h>

//ÉRÉìÉXÉgÉâÉNÉ^
VisionSensorComponent::VisionSensorComponent()
{
	Initialize();
    SetComponentName(u8"éãäE");
}

//âºëzÉfÉXÉgÉâÉNÉ^
VisionSensorComponent::~VisionSensorComponent() = default;

//èâä˙âªèàóù
void VisionSensorComponent::Initialize()
{
	view_angle = DEFAULT_VIEW_ANGLE;
	view_distance = DEFAULT_VIEW_DISTANCE;
    is_sight = false;
}

//çXêVèàóù
void VisionSensorComponent::Update(float elapsed_time)
{

}

//ï`âÊèàóù
void VisionSensorComponent::Render(ID3D11DeviceContext* context)
{

}

//ÉfÉoÉbÉOï`âÊ
void VisionSensorComponent::RenderDebug(ShapeRenderer* renderer)
{
    BS_PROFILE_FUNCTION();

    if (!renderer)
    {
        LOG_WARNING({ u8"éãäE", u8"ï`âÊ" }, u8"ShapeRenderer Ç™ null Ç≈Ç∑");
        return;
    }

    auto transform = target_transform.lock();
    if (!transform)
    {
        LOG_WARNING({ u8"éãäE", u8"ï`âÊ" }, u8"éùÇøéÂÇÃ TransformComponent Ç™ñ≥å¯");
        return;
    }

    static constexpr DirectX::XMFLOAT4 NO_SIGHT_COLOR = { 0.0f, 0.6f, 0.2f, 1.0f };
    static constexpr DirectX::XMFLOAT4 SIGHT_COLOR = { 0.7f,0.0f,0.0f,1.0f };
    DirectX::XMFLOAT4 color = { 1.0f,1.0f,1.0f,1.0f };
    if (is_sight) color = SIGHT_COLOR;
    else color = NO_SIGHT_COLOR;
    
    DirectX::XMFLOAT3 position = transform->GetPosition();
    DirectX::XMFLOAT3 rotation = transform->GetRotation();

    position.y += 0.2f;

    renderer->DrawFan(
        position,
        rotation.y,
        view_distance,
        view_angle,
        color,
        ShapeDrawMode::Solid);
}

//Guiï`âÊ
void VisionSensorComponent::RenderGui()
{
}

//JSONï€ë∂ópïœêîìoò^
void VisionSensorComponent::SetupSerialization(JsonSerializer* serializer)
{
    serializer->RegisterVariable(u8"ç≈ëÂãóó£", &view_distance, SerializeTarget::Detail);
    serializer->RegisterVariable(u8"éãñÏäp", &view_angle, SerializeTarget::Detail);
}

//GuiInspectorêÍópÇÃÉCÉìÉXÉyÉNÉ^Å[çÄñ⁄ìoò^
void VisionSensorComponent::SetupInspector(GuiInspector* inspector)
{
    inspector->RegisterVariable(u8"ç≈ëÂãóó£", &view_distance, GetComponentName());
    inspector->RegisterVariable(u8"éãñÏäp", &view_angle, GetComponentName());
}

//éãäEì‡Ç…ì¸Ç¡ÇΩÇ©îªíË
bool VisionSensorComponent::IsTargetInSight(const DirectX::XMFLOAT3& target_position)
{
    BS_PROFILE_FUNCTION();

    auto transform = target_transform.lock();
    if (!transform)
    {
        LOG_WARNING({ u8"éãäE", u8"îªíË" }, u8"éùÇøéÂÇÃ TransformComponent Ç™ñ≥å¯");
        is_sight = false;
        return is_sight;
    }

    //ëŒè€Ç∆ÇÃãóó£ÇéZèo
    DirectX::XMFLOAT3 my_pos = transform->GetPosition();
    DirectX::XMVECTOR v_my_pos = DirectX::XMLoadFloat3(&my_pos);
    DirectX::XMVECTOR v_target_pos = DirectX::XMLoadFloat3(&target_position);
    DirectX::XMVECTOR v_distance = DirectX::XMVectorSubtract(v_target_pos, v_my_pos);
    float distance_x = DirectX::XMVectorGetX(v_distance);
    float distance_z = DirectX::XMVectorGetZ(v_distance);
    float distance = (distance_x * distance_x) + (distance_z * distance_z);

    if (distance <= view_distance * view_distance)
    {
        //é©êgÇÃê≥ñ ÉxÉNÉgÉãÇÃéZèo
        DirectX::XMFLOAT3 rotation = transform->GetRotation();
        DirectX::XMVECTOR v_front = DirectX::XMVectorSet(sinf(rotation.y), 0.0f, cosf(rotation.y), 0.0f);

        //É[ÉçèúéZ
        static constexpr float EPSILON = 1e-6f;
        if (distance <= EPSILON)
        {
            is_sight = true;
            return is_sight;
        }

        //ëŒè€Ç∆ÇÃãóó£ÇÃê≥ãKâª
        DirectX::XMVECTOR v_diff = DirectX::XMVectorSet(distance_x, 0.0f, distance_z, 0.0f);
        DirectX::XMVECTOR v_to_target = DirectX::XMVector3Normalize(v_diff);

        //ì‡êœÇÃéZèoÇ∆ÉXÉJÉâÅ[ílÇÃíäèo
        DirectX::XMVECTOR v_dot = DirectX::XMVector3Dot(v_front, v_to_target);
        float dot = DirectX::XMVectorGetX(v_dot);

        //éãñÏäpÇÃËáílílÇ∆ÇÃî‰ärîªíË
        float cos_half_angle = cosf(view_angle * 0.5f);
        if (dot >= cos_half_angle)
        {
            is_sight = true;
            return is_sight;
        }
        else
        {
            is_sight = false;
            return is_sight;
        }
    }
    else
    {
        is_sight = false;
        return is_sight;
    }
}
