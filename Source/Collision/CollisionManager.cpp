#include "CollisionManager.h"

#include "../Collision/SpaceDivisionCast.h"
#include "../Graphics/ShapeRenderer.h"

#include <imgui.h>
#include <cmath>
#include <algorithm>
#include <chrono>

#undef min
#undef max

//コンストラクタ
CollisionManager::CollisionManager()
{
	is_enable_collision = true;
    is_draw_grid = false;
    is_auto_optimize_grid = true;
    constexpr float default_margin = 3.0f;
    grid_margin_multiplier = default_margin;
	constexpr float default_cell_size = 5.0f;
	cell_size = default_cell_size;
	collision_logic = std::make_unique<CollisionLogic>();

    test_multipliers[0] = 1.5f;
    test_multipliers[1] = 3.0f;
    test_multipliers[2] = 5.0f;

    accumulated_times[0] = 0.0f;
    accumulated_times[1] = 0.0f;
    accumulated_times[2] = 0.0f;

    current_test_index = 0;
    evaluation_frame_counter = 0;
    interval_frame_counter = 0;
    is_evaluating_multipliers = false;
    is_use_spatial_hash = true;
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
    if (!is_enable_collision)
    {
        execution_time_ms = 0.0f;
        return;
    }

    //計測開始
    auto start_time = std::chrono::high_resolution_clock::now();

    //各判定の呼び出し
    if (is_use_spatial_hash)
    {
        OptimizeCellSize();

        //グリッドのクリアと構築
        grid_heads.clear();
        grid_elements.clear();
        grid_elements.reserve(dynamic_colliders.size() * 4);
        spatial_grid.clear();

        for (size_t i = 0; i < dynamic_colliders.size(); i++)
        {
            Collider* col = dynamic_colliders.at(i);
            if (!col || !col->is_active) continue;
            AddColluderToGrid(col);
        }

        CheckSphereVsSphere();
    }
    else
    {
        CheckSphereVsSphereBruteForce();
    }
    CheckDynamicVsSpace();

    //計測終了と時間算出
    auto end_time = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    execution_time_ms = static_cast<float>(elapsed.count()) / 1000.0f;

    //自動評価の呼び出し
    EvaluateMarginMultiplier();
}

//ImGuiデバッグ描画
void CollisionManager::RenderGui()
{
#ifdef USE_IMGUI
    {
        if (ImGui::CollapsingHeader("CollisionManagerInfo", ImGuiTreeNodeFlags_DefaultOpen))
        {
            //判定方式の切り替えUI
            ImGui::Text("Collision Algorithm:");
            //空間ハッシュを選択するボタン
            if (ImGui::RadioButton("Spatial Hash (O(N))", is_use_spatial_hash == true))
            {
                is_use_spatial_hash = true;
            }
            ImGui::SameLine(); 
            //総当たりを選択するボタン
            if (ImGui::RadioButton("Brute Force (O(N^2))", is_use_spatial_hash == false))
            {
                is_use_spatial_hash = false;
            }

            ImGui::Checkbox("Enable Global Collision", &is_enable_collision);
            ImGui::Checkbox("Draw Spatial Grid", &is_draw_grid);
            ImGui::Checkbox("Auto Optimize Grid Size", &is_auto_optimize_grid);

            constexpr float min_cell = 1.0f;
            constexpr float max_cell = 100.0f;

            if (ImGui::RadioButton("Grid Show", is_draw_grid == true))
            {
                is_draw_grid = true;
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("Grid Hide", is_draw_grid == false))
            {
                is_draw_grid = false;
            }

            if (is_auto_optimize_grid)
            {
                ImGui::SliderFloat("Optimization Multiplier", &grid_margin_multiplier, 1.0f, 5.0f);
                ImGui::Text("Current Optimal Cell Size: %.3f", cell_size);

                //評価ステータスの描画
                if (is_evaluating_multipliers)
                {
                    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Evaluating... (Testing: %.1f)", test_multipliers[current_test_index]);
                }
                else
                {
                    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Best Multiplier Applied: %.1f", grid_margin_multiplier);
                }
            }
            else
            {
                // 手動モードの場合は通常のスライダーでサイズを変更できるようにします
                constexpr float min_cell = 1.0f;
                constexpr float max_cell = 100.0f;
                ImGui::SliderFloat("Grid Cell Size", &cell_size, min_cell, max_cell);
            }

            ImGui::Text("Collision Execution Time: %.3f ms", execution_time_ms);
            ImGui::ProgressBar(execution_time_ms / 16.667f, ImVec2(0.0f, 0.0f));

            ImGui::Text("Registered Spheres: %zu", sphere_colliders.size());
            ImGui::Text("Registered Capsules:%zu", capsule_colliders.size());
            ImGui::Text("Registered Spaces: %zu", space_colliders.size());

            ImGui::Text("Active Grid Cells: %zu", grid_heads.size());
        }
    }
#endif // USE_IMGUI
}

//デバッグ描画
void CollisionManager::RenderDebug(ShapeRenderer* renderer)
{
    if (!is_draw_grid || !renderer)return;

    DirectX::XMFLOAT4 color = { 1.0f,1.0f,0.0f,1.0f };
    DirectX::XMFLOAT4 rotation = { 0.0f,0.0f,0.0f,1.0f };
    DirectX::XMFLOAT3 size = { cell_size,cell_size,cell_size };

    //ハッシュマップに登録されている全てのアクティブなセルをループ
    for (auto it = grid_heads.begin(); it != grid_heads.end(); it++)
    {
        const GridKey& key = it->first;
        DirectX::XMFLOAT3 center = {
            (key.x * cell_size) + (cell_size * 0.5f),
            (key.y * cell_size) + (cell_size * 0.5f),
            (key.z * cell_size) + (cell_size * 0.5f),
        };
        renderer->DrawBox(center, rotation, size, color, ShapeDrawMode::Wireframe);
    }

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
                    capsule->start_center,
                    capsule->end_center,
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

                DirectX::XMVECTOR v_hit_pos = DirectX::XMLoadFloat3(&temp_hit_pos);
                DirectX::XMVECTOR v_hit_normal = DirectX::XMLoadFloat3(&temp_hit_normal);
                DirectX::XMVECTOR v_push = DirectX::XMVectorZero();
                DirectX::XMFLOAT3 current_collider_base = { 0.0f,0.0f,0.0f };

                //押し戻し座標の計算
                switch (collider->type)
                {
                case ColliderType::Sphere:
                {
                    SphereCollider* sphere = static_cast<SphereCollider*>(collider);
                    current_collider_base = sphere->center;
                    DirectX::XMVECTOR v_sphere_center = DirectX::XMLoadFloat3(&sphere->center);
                    DirectX::XMVECTOR v_safe_center = DirectX::XMVectorAdd(v_hit_pos, DirectX::XMVectorScale(v_hit_normal, sphere->radius));
                    v_push = DirectX::XMVectorSubtract(v_safe_center, v_sphere_center);
                    break;
                }
                case ColliderType::Capsule:
                {
                    CapsuleCollider* capsule = static_cast<CapsuleCollider*>(collider);
                    current_collider_base = capsule->start_center;
                    DirectX::XMVECTOR v_start = DirectX::XMLoadFloat3(&capsule->start_center);
                    DirectX::XMVECTOR v_end = DirectX::XMLoadFloat3(&capsule->end_center);
                    DirectX::XMVECTOR v_dir = DirectX::XMVectorSubtract(v_end, v_start);
                    DirectX::XMVECTOR v_to_hit = DirectX::XMVectorSubtract(v_hit_pos, v_start);
                    float len_sq = DirectX::XMVectorGetX(DirectX::XMVector3Dot(v_dir, v_dir));
                    float t = 0.0f;
                    const float ZERO_THRESHOLD = 0.001f;
                    if (len_sq > ZERO_THRESHOLD)
                    {
                        t = DirectX::XMVectorGetX(DirectX::XMVector3Dot(v_to_hit, v_dir)) / len_sq;
                        t = std::clamp(t, 0.0f, 1.0f);
                    }
                    DirectX::XMVECTOR v_closest = DirectX::XMVectorAdd(v_start, DirectX::XMVectorScale(v_dir, t));
                    DirectX::XMVECTOR v_safe_center = DirectX::XMVectorAdd(v_hit_pos, DirectX::XMVectorScale(v_hit_normal, capsule->radius));
                    v_push = DirectX::XMVectorSubtract(v_safe_center, v_closest);

                    break;
                }
                default:
                    break;
                }
                DirectX::XMVECTOR v_base = DirectX::XMLoadFloat3(&current_collider_base);
                DirectX::XMVECTOR v_safe_pos = DirectX::XMVectorAdd(v_base, v_push);
                DirectX::XMStoreFloat3(&result.safe_position, v_safe_pos);
                result.hit_attribute = space->attribute;
                collider->listener->OnCollisionHit(result);
            }
        }
    }
}

//動的コライダーとレイの判定
bool CollisionManager::RayCastSpace(
    const DirectX::XMFLOAT3& start_pos,
    const DirectX::XMFLOAT3& end_pos,
    DirectX::XMFLOAT3& hit_position) const
{
    bool is_hit = false;
    float closest_distance = std::numeric_limits<float>::max();
    DirectX::XMFLOAT3 temp_hit_pos = { 0.0f,0.0f,0.0f };
    DirectX::XMFLOAT3 temp_hit_normal = { 0.0f,0.0f,0.0f };

    constexpr float max_ray_distance = 10000.0f;

    DirectX::XMVECTOR v_start = DirectX::XMLoadFloat3(&start_pos);

    //登録されているすべての空間分割コライダーに対して判定
    for (size_t i = 0; i < space_colliders.size(); i++)
    {
        SpaceDivisionCollider* space = space_colliders[i];
        if (!space) continue;
        if (!space->space_cast)continue;
        if (!space->is_active)continue;

        if (space->space_cast->Raycast(start_pos, end_pos, temp_hit_pos, temp_hit_normal))
        {
            DirectX::XMVECTOR v_hit = DirectX::XMLoadFloat3(&temp_hit_pos);
            DirectX::XMVECTOR v_dist = DirectX::XMVectorSubtract(v_hit, v_start);
            float dist = DirectX::XMVectorGetX(DirectX::XMVector3Length(v_dist));

            if (dist < closest_distance)
            {
                closest_distance = dist;
                hit_position = temp_hit_pos;
                is_hit = true;
            }
        }

    }
    return is_hit;
}

//スフィア同士の総当たり判定
void CollisionManager::CheckSphereVsSphere()
{
    //コライダーが2つ未満の場合は判定不要
    if (sphere_colliders.size() < 2)return;

    //セルベースの詳細判定
    for (auto it = grid_heads.begin(); it != grid_heads.end(); it++)
    {
        const GridKey& current_key = it->first;
        int head_index = it->second;

        if (head_index == -1 || grid_elements[head_index].next_index == -1)continue;

        int idx_a = head_index;
        while (idx_a != -1)
        {
            SphereCollider* sphere_a = grid_elements[idx_a].sphere;
            int idx_b = grid_elements[idx_a].next_index;
            if (sphere_a->is_active)
            {
                GridRange range_a = CalculateGridRenge(sphere_a);
                while (idx_b != -1)
                {
                    SphereCollider* sphere_b = grid_elements[idx_b].sphere;
                    if (sphere_b->is_active && sphere_a != sphere_b)
                    {
                        GridRange range_b = CalculateGridRenge(sphere_b);

                        //タイブレーク処理
                        GridKey overlap_min_key;
                        overlap_min_key.x = std::max(range_a.min_key.x, range_b.min_key.x);
                        overlap_min_key.y = std::max(range_a.min_key.y, range_b.min_key.y);
                        overlap_min_key.z = std::max(range_a.min_key.z, range_b.min_key.z);

                        if (current_key == overlap_min_key)
                        {
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
                                    result_b.penetration_vector.x = -result_a.penetration_vector.x;
                                    result_b.penetration_vector.y = -result_a.penetration_vector.y;
                                    result_b.penetration_vector.z = -result_a.penetration_vector.z;
                                    result_b.hit_attribute = sphere_a->attribute;
                                    sphere_b->listener->OnCollisionHit(result_b);
                                }
                            }
                        }
                        idx_b = grid_elements[idx_b].next_index;
                    }
                }
                idx_a = grid_elements[idx_a].next_index;
            }
        }
    }
}

//スフィア同士の総当たり判定（比較用）
void CollisionManager::CheckSphereVsSphereBruteForce()
{
    if (sphere_colliders.size() < 2)return;

    //総当たりループの開始
    for (size_t i = 0; i < sphere_colliders.size(); i++)
    {
        SphereCollider* sphere_a = sphere_colliders[i];

        if (!sphere_a || !sphere_a->is_active)continue;

        //重複判定を防ぐための i+1 からの内部ループ
        for (size_t j = i + 1; j < sphere_colliders.size(); j++)
        {
            SphereCollider* sphere_b = sphere_colliders[j];
            if (!sphere_b || !sphere_b->is_active) continue;

            //実際の当たり判定ロジックの呼び出し
            CollisionResult result_a;
            bool is_hit = collision_logic->IsSphereSphereCollision(sphere_a, sphere_b, result_a);

            //衝突時の通知処理
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
                    // B向けの押し戻し方向と法線を反転させる
                    result_b.hit_normal.x = -result_a.hit_normal.x;
                    result_b.hit_normal.y = -result_a.hit_normal.y;
                    result_b.hit_normal.z = -result_a.hit_normal.z;
                    result_b.penetration_vector.x = -result_a.penetration_vector.x;
                    result_b.penetration_vector.y = -result_a.penetration_vector.y;
                    result_b.penetration_vector.z = -result_a.penetration_vector.z;
                    result_b.hit_attribute = sphere_a->attribute;
                    sphere_b->listener->OnCollisionHit(result_b);
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

                //チェインマップ登録
                GridElement elem;
                elem.sphere = static_cast<SphereCollider*>(collider);
                auto it = grid_heads.find(key);
                elem.next_index = (it != grid_heads.end()) ? it->second : -1;
                int new_elem_index = static_cast<int>(grid_elements.size());
                grid_elements.push_back(elem);
                grid_heads[key] = new_elem_index;
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
    case ColliderType::Capsule:
    {
        CapsuleCollider* capsule = static_cast<CapsuleCollider*>(collider);

        //点と終点の2つの位置から各軸の最小値を求め、さらに半径を引き算して最小位置を計算
        min_pos.x = std::min(capsule->start_center.x, capsule->end_center.x) - capsule->radius;
        min_pos.y = std::min(capsule->start_center.y, capsule->end_center.y) - capsule->radius;
        min_pos.z = std::min(capsule->start_center.z, capsule->end_center.z) - capsule->radius;

        //始点と終点の2つの位置から各軸の最大値を求め、さらに半径を足し算して最大位置を計算
        max_pos.x = std::max(capsule->start_center.x, capsule->end_center.x) + capsule->radius;
        max_pos.y = std::max(capsule->start_center.y, capsule->end_center.y) + capsule->radius;
        max_pos.z = std::max(capsule->start_center.z, capsule->end_center.z) + capsule->radius;

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

//現在のコライダー密度から最適なセルサイズを計算して適用
void CollisionManager::OptimizeCellSize()
{
    if (!is_auto_optimize_grid || dynamic_colliders.empty())return;

    const size_t current_collider_count = dynamic_colliders.size();
    //if (current_collider_count == previous_collider_count)return;

    float total_diameter = 0.0f;
    size_t active_count = 0;

    //平均サイズの算出ループ
    for (size_t i = 0; i < dynamic_colliders.size(); i++)
    {
        Collider* col = dynamic_colliders[i];
        if (!col || !col->is_active)continue;
        switch (col->type)
        {
        case ColliderType::Sphere:
        {
            SphereCollider* sphere = static_cast<SphereCollider*>(col);
            total_diameter += sphere->radius * 2.0f;
            break;
        }
        case ColliderType::Capsule:
        {
            CapsuleCollider* capsule = static_cast<CapsuleCollider*>(col);
            DirectX::XMVECTOR v_start = DirectX::XMLoadFloat3(&capsule->start_center);
            DirectX::XMVECTOR v_end = DirectX::XMLoadFloat3(&capsule->end_center);
            DirectX::XMVECTOR v_length = DirectX::XMVector3Length(DirectX::XMVectorSubtract(v_end, v_start));
            total_diameter += DirectX::XMVectorGetX(v_length) + (capsule->radius * 2.0f);
            break;
        }
        default:
            break;
        }
        active_count++;
    }

    //最終セルサイズの適用
    if (active_count > 0)
    {
        float average_diameter = total_diameter / static_cast<float>(active_count);
        float optimal_size = average_diameter * grid_margin_multiplier;
        constexpr float min_limit = 1.0f;
        constexpr float max_limit = 100.0f;
        cell_size = std::clamp(optimal_size, min_limit, max_limit);
    }
    previous_collider_count = dynamic_colliders.size();
}

//処理負荷が最も軽いマージン倍率を自動評価
void CollisionManager::EvaluateMarginMultiplier()
{
    if (!is_auto_optimize_grid)return;

    //評価待機フェーズ
    if (!is_evaluating_multipliers)
    {
        interval_frame_counter++;
        if (interval_frame_counter >= evaluation_interval_frames)
        {
            //インターバルを満たしたら評価モードへ移行
            is_evaluating_multipliers = true;
            interval_frame_counter = 0;
            current_test_index = 0;
            evaluation_frame_counter = 0;

            //過去の計測時間をリセット
            for (int i = 0; i < max_multiplier_patterns; i++)
            {
                accumulated_times[i] = 0.0f;
            }

            //最初のテスト倍率を適用
            grid_margin_multiplier = test_multipliers[current_test_index];
        }
        return;
    }

    //評価実行フェーズ

    //前の行で計測された今フレームの処理時間を累積
    accumulated_times[current_test_index] += execution_time_ms;
    evaluation_frame_counter++;

    //指定フレーム数計測したら次の倍率へ
    if (evaluation_frame_counter >= evaluation_frames_per_pattern)
    {
        current_test_index++;
        evaluation_frame_counter = 0;

        //全パターンの計測が完了したか判定
        if (current_test_index >= max_multiplier_patterns)
        {
            //最適な倍率の決定フェーズ
            int best_index = 0;
            float min_time = accumulated_times[0];

            //累積時間が最も短い(処理が軽い)インデックスを探索
            for (int i = 0; i < max_multiplier_patterns; i++)
            {
                if (accumulated_times[i] < min_time)
                {
                    min_time = accumulated_times[i];
                    best_index = i;
                }
            }
            //最も軽かった倍率を最終決定して適用
            grid_margin_multiplier = test_multipliers[best_index];
            is_evaluating_multipliers = false;

            //意図しない挙動がないか確認用のデバッグ
            float average_best_time = min_time / static_cast<float>(evaluation_frames_per_pattern);
            if (average_best_time > 16.0f)
            {
                
            }
        }
    }



}
