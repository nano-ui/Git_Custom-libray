#include "CollisionManager.h"
#include "../Collision/SpaceDivisionCast.h"

#include <imgui.h>
#include <cmath>
#include <algorithm>

//コンストラクタ
CollisionManager::CollisionManager()
{
	is_enable_collision = true;
	constexpr float default_cell_size = 10.0f;
	cell_size = default_cell_size;
	collision_logic = std::make_unique<CollisionLogic>();
}

//デストラクタ
CollisionManager::~CollisionManager()
{
}

//コライダーの登録
void CollisionManager::Register(Collider* collider)
{
	//引数チェック
	if (!collider)return;

    //マスターリストへの追加
    if (collider->type != ColliderType::SpaceDivision)
    {
        dynamic_colliders.push_back(collider);
    }

	//形状ごとの専用リストへの振り分け
    switch (collider->type)
    {
    case ColliderType::Sphere:
        sphere_colliders.push_back(static_cast<SphereCollider*>(collider));
        break;

    case ColliderType::SpaceDivision:
        space_colliders.push_back(static_cast<SpaceDivisionCollider*>(collider));
        break;

    case ColliderType::Capsule:
        capsule_colliders.push_back(static_cast<CapsuleCollider*>(collider));
        break;

    default:
        // 未対応の形状が来た場合は何もしません。
        break;
    }
}

//コライダーの削除
void CollisionManager::Remove(Collider* collider)
{
    //引数チェック
    if (!collider)return;

    //マスターリストからの削除
    if (collider->type != ColliderType::SpaceDivision)
    {
        auto it_dyn = std::remove(dynamic_colliders.begin(), dynamic_colliders.end(), collider);
        dynamic_colliders.erase(it_dyn, dynamic_colliders.end());
    }


    //形状ごとの専用リストからの削除
    switch (collider->type)
    {
    case ColliderType::Sphere:
    {
        auto sphere_ptr = static_cast<SphereCollider*>(collider);
        auto it = std::remove(sphere_colliders.begin(), sphere_colliders.end(), sphere_ptr);
        sphere_colliders.erase(it, sphere_colliders.end());
        break;
    }
    case ColliderType::SpaceDivision:
    {
        auto space_ptr = static_cast<SpaceDivisionCollider*>(collider);
        auto it = std::remove(space_colliders.begin(), space_colliders.end(), space_ptr);
        space_colliders.erase(it, space_colliders.end());
        break;
    }
    case ColliderType::Capsule:
    {
        auto capsule = static_cast<CapsuleCollider*>(collider);
        auto it = std::remove(capsule_colliders.begin(), capsule_colliders.end(), capsule);
        capsule_colliders.erase(it, capsule_colliders.end());
        break;
    }
    default:
        break;
    }
}

//登録されたコライダーの一括判定
void CollisionManager::ExecuteCollision()
{
    //実行前チェック
    if (!is_enable_collision)return;

    //共通グリッドの構築
    spatial_grid.clear();
    for (size_t i = 0; i < dynamic_colliders.size(); i++)
    {
        Collider* collider = dynamic_colliders.at(i);
        if (!collider || !collider->is_active)continue;
        AddColluderToGrid(collider);
    }

    //各判定の呼び出し
    CheckDynamicVsSpace();
    CheckSphereVsSphere();
}

//ImGuiデバッグ描画
void CollisionManager::RenderGui()
{
#ifdef USE_IMGUI
    {
        ImGui::Checkbox("Enable Global Collision", &is_enable_collision);

        constexpr float min_cell = 1.0f;
        constexpr float max_cell = 100.0f;

        ImGui::SliderFloat("Grid Cell Size", &cell_size, min_cell, max_cell);

        ImGui::Text("Registered Spheres: %zu", sphere_colliders.size());
        ImGui::Text("Registered Spaces: %zu", space_colliders.size());

        ImGui::Text("Active Grid Cells: %zu", spatial_grid.size());
    }
#endif // USE_IMGUI
}

//動的コライダーと空間分割の判定
void CollisionManager::CheckDynamicVsSpace()
{
    //スフィアのループ処理
    for (size_t i = 0; i < dynamic_colliders.size(); i++)
    {
        Collider* collider = dynamic_colliders.at(i);
        if (!collider || !collider->is_active)continue;

        //空間分割のループ処理
        for (size_t j = 0; j < space_colliders.size(); j++)
        {
            SpaceDivisionCollider* space = space_colliders.at(j);
            if (!space || !space->space_cast)continue;
            DirectX::XMFLOAT3 temp_hit_pos = { 0.0f,0.0f,0.0f };
            DirectX::XMFLOAT3 temp_hit_normal = { 0.0f,0.0f,0.0f };
            bool is_hit = false;

            //形状に応じたキャスト方法の自動選択
            switch (collider->type)
            {
            case ColliderType::Sphere:
            {
                SphereCollider* sphere = static_cast<SphereCollider*>(collider);
                is_hit = space->space_cast->StaticSpheraCast(
                    sphere->center,
                    sphere->center,
                    sphere->radius,
                    temp_hit_pos,
                    temp_hit_normal
                );
                break;
            }
            case ColliderType::Capsule:
            {
                CapsuleCollider* capsule = static_cast<CapsuleCollider*>(collider);
                is_hit = space->space_cast->MultiSpheraCast(
                    capsule->old_start_center,
                    capsule->old_end_center,
                    capsule->radius,
                    temp_hit_pos,
                    temp_hit_normal
                );
                break;
            }
            default:
                break;
            }

            //衝突通知の実行
            if (is_hit && collider->listener)
            {
                CollisionResult result;
                result.hit_position = temp_hit_pos;
                result.hit_normal = temp_hit_normal;
                result.safe_position = temp_hit_pos;
                result.hit_attribute = space->attribute;
                collider->listener->OnCollisionHit(result);
            }
        }
    }
}

//スフィア同士の総当たり判定
void CollisionManager::CheckSphereVsSphere()
{
    //コライダーが2つ未満の場合は判定不要
    if (sphere_colliders.size() < 2)return;

    //セルベースの詳細判定
    for (auto it = spatial_grid.begin(); it != spatial_grid.end(); it++)
    {
        const GridKey& current_key = it->first;
        const std::vector<SphereCollider*> cell_spheres = it->second.spheres;
        if (cell_spheres.size() < 2)continue;

        //セルの内の総当たりループ
        for (size_t i = 0; i < cell_spheres.size(); i++)
        {
            SphereCollider* sphere_a = cell_spheres.at(i);
            if (!sphere_a->is_active)continue;
            GridRange range_a = CalculateGridRenge(sphere_a);
            
            for (size_t j = i + 1; j < cell_spheres.size(); j++)
            {
                SphereCollider* sphere_b = cell_spheres.at(j);
                if (!sphere_b->is_active || sphere_a == sphere_b)continue;
                GridRange range_b = CalculateGridRenge(sphere_b);

                //タイブレーク処理
                GridKey overlap_min_key;
                overlap_min_key.x = std::max(range_a.min_key.x, range_b.min_key.x);
                overlap_min_key.y = std::max(range_a.min_key.y, range_b.min_key.y);
                overlap_min_key.z = std::max(range_a.min_key.z, range_b.min_key.z);

                if (!(current_key == overlap_min_key))continue;

                //判定実行と相互通知
                CollisionResult result_a;
                bool is_hit = collision_logic->IsSphereSphereCollision(sphere_a, sphere_b, result_a);

                if (is_hit)
                {
                    if (sphere_a->listener)
                    {
                        result_a.hit_attribute = sphere_b->attribute;
                        sphere_a->listener->OnCollisionHit(result_a);
                    }
                    if (sphere_b->listener)
                    {
                        CollisionResult result_b = result_a;
                        result_b.hit_normal.x = -result_a.hit_normal.x;
                        result_b.hit_normal.y = -result_a.hit_normal.y;
                        result_b.hit_normal.z = -result_a.hit_normal.z;
                        result_b.hit_attribute = sphere_a->attribute;
                        sphere_b->listener->OnCollisionHit(result_b);
                    }
                }
            }
        }
    }
}

//グリッド登録用の補助関数
void CollisionManager::AddColluderToGrid(Collider* collider)
{
    //グリッド範囲の算出
    GridRange range = CalculateGridRenge(collider);

    //3重ループによる一括登録
    for (int x = range.min_key.x; x <= range.max_key.x; x++)
    {
        for (int y = range.min_key.y; y <= range.max_key.y; y++)
        {
            for (int z = range.min_key.z; z <= range.max_key.z; z++)
            {
                GridKey key = { x,y,z };

                switch (collider->type)
                {
                case ColliderType::Sphere:
                    spatial_grid[key].spheres.push_back(static_cast<SphereCollider*>(collider));
                    break;


                default:
                    break;
                }
            }
        }
    }
}

//座標からグリッド範囲を計算
GridRange CollisionManager::CalculateGridRenge(Collider* collider) const
{
    GridRange range;

    DirectX::XMFLOAT3 min_pos = { 0.0f,0.0f,0.0f };
    DirectX::XMFLOAT3 max_pos = { 0.0f,0.0f,0.0f };

    //形状ごとのAABB(境界線)の算出
    switch (collider->type)
    {
    case ColliderType::Sphere:
    {
        SphereCollider* sphere = static_cast<SphereCollider*>(collider);
        min_pos = { sphere->center.x - sphere->radius,sphere->center.y - sphere->radius,sphere->center.z - sphere->radius };
        max_pos = { sphere->center.x + sphere->radius,sphere->center.y + sphere->radius,sphere->center.z + sphere->radius };
        break;
    }
    default:
        break;
    }

    constexpr float min_safe_cell_size = 0.1f;
    float safe_cell = std::max(cell_size, min_safe_cell_size);

    //当たり判定の最小座標が含まれるセルインデックスを計算
    range.min_key.x = static_cast<int>(std::floor(min_pos.x / safe_cell));
    range.min_key.y = static_cast<int>(std::floor(min_pos.y / safe_cell));
    range.min_key.z = static_cast<int>(std::floor(min_pos.z / safe_cell));

    //当たり判定の最大座標が含まれるセルインデックスを計算
    range.max_key.x = static_cast<int>(std::floor(max_pos.x / safe_cell));
    range.max_key.y = static_cast<int>(std::floor(max_pos.y / safe_cell));
    range.max_key.z = static_cast<int>(std::floor(max_pos.z / safe_cell));

    return range;
}
