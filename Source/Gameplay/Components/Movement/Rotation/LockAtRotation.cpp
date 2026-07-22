#include "LockAtRotation.h"

//=====================
//対象に向き続ける
//=====================
DirectX::XMVECTOR LockAtRotation::Update(const DirectX::XMVECTOR& current_rotation, const DirectX::XMFLOAT3& target_dir)
{
    DirectX::XMVECTOR target_vector = DirectX::XMLoadFloat3(&target_dir);   //対象の方向ベクトル
    float length_sq = DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(target_vector));
    if (length_sq <= 0.0f)return current_rotation;  //ベクトルの長さが0出ないか確認
    target_vector = DirectX::XMVector3Normalize(target_vector);     //ベクトルを正規化

    DirectX::XMMATRIX look_at_matrix = DirectX::XMMatrixLookToLH(   //行れるを作成し、クォータニオンに変換
        DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f),
        target_vector,
        DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)
    );

    DirectX::XMVECTOR target_rotation = DirectX::XMQuaternionRotationMatrix(look_at_matrix);    //行列からクォータニオンを生成

    target_rotation = DirectX::XMQuaternionSlerp(current_rotation, target_rotation, 0.1f);  //球面線形補間

    return target_rotation;
}

//=====================
//対象に向き続ける
//=====================
float LockAtRotation::UpdateAngle(float elapsed_time, float current_angle, const DirectX::XMFLOAT3& target_dir)
{
    float length_sq = (target_dir.x * target_dir.x) + (target_dir.z * target_dir.z);
    if (length_sq < 0.001f)
    {
        return current_angle;
    }

    float target_angle = atan2f(target_dir.x, target_dir.z);    //目標角度
    float angle_diff = target_angle - current_angle;    //現在の角度を目標角度の差
    while (angle_diff > DirectX::XM_PI) angle_diff -= DirectX::XM_2PI;
    while (angle_diff < -DirectX::XM_PI) angle_diff += DirectX::XM_2PI;

    float rotate_speed = DirectX::XM_2PI * 0.5f;
    float rotate_rate = rotate_speed * elapsed_time;

    if (rotate_rate > fabsf(angle_diff))
    {
        rotate_rate = fabsf(angle_diff);
    }

    float sign = (angle_diff > 0) ? 1.0f : -1.0f;
    float new_angle = current_angle + (sign * rotate_rate);

    while (new_angle > DirectX::XM_PI) new_angle -= DirectX::XM_2PI;
    while (new_angle < -DirectX::XM_PI) new_angle += DirectX::XM_2PI;

    return new_angle;
}
