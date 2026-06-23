#include "CollisionLogic.h"
#include <cmath>
#include <algorithm>

//カプセルとスフィアの当たり判定
bool CollisionLogic::IsCapsuleSphereCollision(
    const DirectX::XMFLOAT3& capsule_start,
    const DirectX::XMFLOAT3& capsule_end,
    float capsule_radius,
    const DirectX::XMFLOAT3& sphere_center,
    float sphere_radius)
{
    //DirectXMathによるベクトルへの変換
    DirectX::XMVECTOR vector_start = DirectX::XMLoadFloat3(&capsule_start);     
    DirectX::XMVECTOR vector_end = DirectX::XMLoadFloat3(&capsule_end);         
    DirectX::XMVECTOR vector_center = DirectX::XMLoadFloat3(&sphere_center);    


    //始点から終点への線分ベクトルと距離の計算
    DirectX::XMVECTOR vector_line = DirectX::XMVectorSubtract(vector_end, vector_start); 
    DirectX::XMVECTOR dot_line_line = DirectX::XMVector3Dot(vector_line, vector_line);   
    float length_sq_line = DirectX::XMVectorGetX(dot_line_line);                         

    //線分上の最も近い点の割合を計算
    DirectX::XMVECTOR vector_to_center = DirectX::XMVectorSubtract(vector_center, vector_start);
    float t = 0.0f;                                                                             
    const float ZERO_LENGTH = 0.0f;                                                             
    if (length_sq_line > ZERO_LENGTH)   //線分の長さが0より大きい場合
    {
        DirectX::XMVECTOR dot_line_to_center = DirectX::XMVector3Dot(vector_line, vector_center); 
        t = DirectX::XMVectorGetX(dot_line_to_center) / length_sq_line;                           
        const float MIN_T = 0.0f;                                                                 
        if (t < MIN_T)t = MIN_T;   
        const float MAX_T = 1.0f;  
        if (t > MAX_T)t = MAX_T;   
    }

    //最も近い点の座標を算出し、スフィアとの距離を比較
    DirectX::XMVECTOR closest_point = DirectX::XMVectorAdd(vector_start, DirectX::XMVectorScale(vector_line, t)); 
    DirectX::XMVECTOR vector_diff = DirectX::XMVectorSubtract(vector_center, closest_point);                      
    DirectX::XMVECTOR dot_diff = DirectX::XMVector3Dot(vector_diff, vector_diff);                                 
    float distance_sq = DirectX::XMVectorGetX(dot_diff);                                                          

    //半径の合計と距離を比較して当たり判定を返す
    float sum_radius = capsule_radius + sphere_radius; 
    float sum_radius_sq = sum_radius * sum_radius;     
    if (distance_sq <= sum_radius_sq)                  
    {
        return true; 
    }
    return false; 
}

//スフィア同士の当たり判定
bool CollisionLogic::IsSphereSphereCollision(const SphereCollider* sphere_a, const SphereCollider* sphere_b, CollisionResult& out_result)
{
    //引数の有効判定
    if (!sphere_a || !sphere_a->is_active)return false;
    if (!sphere_b || !sphere_b->is_active)return false;

    //ベクトルへの変換
    DirectX::XMVECTOR center_a = DirectX::XMLoadFloat3(&sphere_a->center);
    DirectX::XMVECTOR center_b = DirectX::XMLoadFloat3(&sphere_b->center);

    //距離と半径を用いた交差判定
    DirectX::XMVECTOR diff_vec = DirectX::XMVectorSubtract(center_a, center_b);
    DirectX::XMVECTOR dist_sq_vec = DirectX::XMVector3Dot(diff_vec, diff_vec);
    float dist_sq = DirectX::XMVectorGetX(dist_sq_vec);
    float total_radius = sphere_a->radius + sphere_b->radius;
    float total_radius_sq = total_radius * total_radius;
    if (dist_sq > total_radius_sq)return false;

    //衝突時の詳細データの構築
    constexpr float epsilon = 0.0001f;
    float dist = std::sqrtf(dist_sq);
    DirectX::XMVECTOR normal_vec;
    if (dist < epsilon)
    {
        normal_vec = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    }
    else
    {
        normal_vec = DirectX::XMVectorScale(diff_vec, 1.0f / dist);
    }
    float penetration = total_radius - dist;
    DirectX::XMVECTOR push_vec = DirectX::XMVectorScale(normal_vec, penetration);
    DirectX::XMVECTOR safe_pos_vev = DirectX::XMVectorAdd(center_a, push_vec);

    //計算結果の格納
    DirectX::XMVECTOR hit_pos_vec = DirectX::XMVectorAdd(center_b, DirectX::XMVectorScale(normal_vec, sphere_b->radius));
    DirectX::XMStoreFloat3(&out_result.hit_normal, normal_vec);
    DirectX::XMStoreFloat3(&out_result.safe_position, safe_pos_vev);
    DirectX::XMStoreFloat3(&out_result.penetration_vector, push_vec);
    out_result.hit_collider = const_cast<SphereCollider*>(sphere_b);
    return true;
}

//カプセル同士の当たり判定
bool CollisionLogic::IsCapsuleCapsuleCollision(
    const CapsuleCollider* capsule_a,
    const CapsuleCollider* capsule_b,
    CollisionResult& out_result)
{
    //引数の有効判定
    if (!capsule_a || capsule_a->is_active) return false;
    if (!capsule_b || capsule_b->is_active) return false;

    //ベクトルの準備
    DirectX::XMVECTOR start_a = DirectX::XMLoadFloat3(&capsule_a->start_center);
    DirectX::XMVECTOR end_a = DirectX::XMLoadFloat3(&capsule_a->end_center);
    DirectX::XMVECTOR start_b = DirectX::XMLoadFloat3(&capsule_b->start_center);
    DirectX::XMVECTOR end_b = DirectX::XMLoadFloat3(&capsule_b->end_center);

    //2つの線分の最も近い点を計算
    DirectX::XMVECTOR closest_a;
    DirectX::XMVECTOR closest_b;
    CalculaterClosestPointsBetweenSegments(start_a, end_a, start_b, end_b, closest_a, closest_b);

    //距離と半径を用いた交差判定
    DirectX::XMVECTOR diff_vec = DirectX::XMVectorSubtract(closest_a, closest_b);
    DirectX::XMVECTOR dist_sq_vec = DirectX::XMVector3Dot(diff_vec, diff_vec);
    float dist_sq = DirectX::XMVectorGetX(dist_sq_vec);
    float total_radius = capsule_a->radius + capsule_b->radius;
    float total_radius_sq = total_radius * total_radius;
    if (dist_sq > total_radius_sq)return false;

    //衝突時の詳細データの構築
    constexpr float epsilon = 0.0001f;
    float dist = std::sqrtf(dist_sq);
    DirectX::XMVECTOR normal_vec;
    if (dist < epsilon)
    {
        normal_vec = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    }
    else
    {
        normal_vec = DirectX::XMVectorScale(diff_vec, 1.0f / dist);
    }
    float penetration = total_radius - dist;
    DirectX::XMVECTOR push_vec = DirectX::XMVectorScale(normal_vec, penetration);

    //計算結果の格納
    DirectX::XMVECTOR hit_pos_vec = DirectX::XMVectorAdd(closest_a, DirectX::XMVectorScale(normal_vec, capsule_b->radius));
    DirectX::XMStoreFloat3(&out_result.hit_position, hit_pos_vec);
    DirectX::XMStoreFloat3(&out_result.hit_normal, normal_vec);
    DirectX::XMVECTOR safe_pos_vec = DirectX::XMVectorAdd(start_a, push_vec);
    DirectX::XMStoreFloat3(&out_result.safe_position, safe_pos_vec);
    DirectX::XMStoreFloat3(&out_result.penetration_vector, push_vec);
    out_result.hit_collider = const_cast<CapsuleCollider*>(capsule_b);
    return true;
}

//2つの線分間の最近傍点を計算
void CollisionLogic::CalculaterClosestPointsBetweenSegments(
    const DirectX::XMVECTOR& start_a, const DirectX::XMVECTOR& end_a,
    const DirectX::XMVECTOR& start_b, const DirectX::XMVECTOR& end_b,
    DirectX::XMVECTOR& out_closest_a, DirectX::XMVECTOR& out_closest_b) const
{
    //総方向ベクトルの始点間ベクトルの計算
    DirectX::XMVECTOR dir_a = DirectX::XMVectorSubtract(end_a, start_a);
    DirectX::XMVECTOR dir_b = DirectX::XMVectorSubtract(end_b, start_b);
    DirectX::XMVECTOR start_diff = DirectX::XMVectorSubtract(start_a, start_b);

    //内積を用いた媒介変数用の係数を計算
    float len_sq_a = DirectX::XMVectorGetX(DirectX::XMVector3Dot(dir_a, dir_a));
    float len_sq_b = DirectX::XMVectorGetX(DirectX::XMVector3Dot(dir_b, dir_b));
    float dot_b_diff = DirectX::XMVectorGetX(DirectX::XMVector3Dot(dir_b, start_diff));
    float dot_a_b = DirectX::XMVectorGetX(DirectX::XMVector3Dot(dir_a, dir_b));
    float dot_a_diff = DirectX::XMVectorGetX(DirectX::XMVector3Dot(dir_a, start_diff));
    float ratio_a = 0.0f;
    float ratio_b = 0.0f;
    constexpr float epsilon = 0.0001f;

    //線分の長さによる条件分岐と媒介変数の算出
    if (len_sq_a <= epsilon && len_sq_b <= epsilon)
    {
        ratio_a = 0.0f;
        ratio_b = 0.0f;
    }
    else if (len_sq_a <= epsilon)
    {
        ratio_a = 0.0f;
        ratio_b = std::clamp(dot_b_diff / len_sq_b, 0.0f, 1.0f);
    }
    else if (len_sq_b <= epsilon)
    {
        ratio_b = 0.0f;
        ratio_a = std::clamp(-dot_a_diff / len_sq_a, 0.0f, 1.0f);
    }
    else
    {
        float denom = (len_sq_a * len_sq_b) - (dot_a_b * dot_a_b);
        if (denom != 0.0f)
        {
            ratio_a = std::clamp((dot_a_b * dot_b_diff - dot_a_diff * len_sq_b) / denom, 0.0f, 1.0f);
        }
        else
        {
            ratio_a = 0.0f;
        }
        ratio_b = (dot_a_b * ratio_a + dot_b_diff) / len_sq_b;
        if (ratio_b < 0.0f)
        {
            ratio_b = 0.0f;
            ratio_a = std::clamp(-dot_a_diff / len_sq_a, 0.0f, 1.0f);
        }
        else if (ratio_b > 1.0f)
        {
            ratio_b = 1.0f;
            ratio_a = std::clamp((dot_a_b - dot_a_diff) / len_sq_a, 0.0f, 1.0f);
        }
    }

    //最近傍点の座標を算出して出力
    out_closest_a = DirectX::XMVectorAdd(start_a, DirectX::XMVectorScale(dir_a, ratio_a));
    out_closest_b = DirectX::XMVectorAdd(start_b, DirectX::XMVectorScale(dir_b, ratio_b));
}
