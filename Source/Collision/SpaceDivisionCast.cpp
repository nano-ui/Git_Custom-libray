#include "SpaceDivisionCast.h"
#include <limits>
#include <cmath>
#include <algorithm>

//===========================
//空間分割データを構築
//===========================
void SpaceDivisionCast::Build(const std::vector<DirectX::XMFLOAT3>& vertices_data, const std::vector<uint32_t>& indices_data)
{
	DirectX::XMVECTOR vol_min = DirectX::XMVectorReplicate(std::numeric_limits<float>::max());		//最小値を最大値で初期化
	DirectX::XMVECTOR vol_max = DirectX::XMVectorReplicate(std::numeric_limits<float>::lowest());	//最大値を最小値で初期化

	//------------------------------------
	//三角形データの生成と全体AABBの計算
	//------------------------------------
	for (size_t i = 0; i < indices_data.size(); i += 3)
	{
		uint32_t index_a = indices_data.at(i + 0);	//頂点インデックスAを取得
		uint32_t index_b = indices_data.at(i + 1);	//頂点インデックスBを取得
		uint32_t index_c = indices_data.at(i + 2);	//頂点インデックスCを取得

		DirectX::XMVECTOR vec_a = DirectX::XMLoadFloat3(&vertices_data.at(index_a));	//頂点Aの座標をXMVECTORにロード
		DirectX::XMVECTOR vec_b = DirectX::XMLoadFloat3(&vertices_data.at(index_b));	//頂点Bの座標をXMVECTORにロード
		DirectX::XMVECTOR vec_c = DirectX::XMLoadFloat3(&vertices_data.at(index_c));	//頂点Cの座標をXMVECTORにロード

		DirectX::XMVECTOR sub_ab = DirectX::XMVectorSubtract(vec_b, vec_a);	//ベクトルABを計算
		DirectX::XMVECTOR sub_ac = DirectX::XMVectorSubtract(vec_c, vec_a);	//ベクトルACを計算

		DirectX::XMVECTOR vec_n = DirectX::XMVector3Cross(sub_ab, sub_ac);	//法線ベクトル算出
		DirectX::XMVECTOR length_n = DirectX::XMVector3Length(vec_n);		//法線ベクトルの長さを算出

		if (DirectX::XMVectorGetX(length_n) < EPSILON_NORMAL)continue;	//法線の長さが閾値以下の場合は三角形を構成できないのでスキップ

		vec_n = DirectX::XMVector3Normalize(vec_n);	//法線ベクトルを正規化

		triangles_list.emplace_back();
		Traiangle& new_triangle = triangles_list.back();	//新しい三角形データを配列の末尾に追加
		DirectX::XMStoreFloat3(&new_triangle.position[0], vec_a);	//頂点Aを保存
		DirectX::XMStoreFloat3(&new_triangle.position[1], vec_b);	//頂点Bを保存
		DirectX::XMStoreFloat3(&new_triangle.position[2], vec_c);	//頂点Cを保存
		DirectX::XMStoreFloat3(&new_triangle.normal, vec_n);		//法線ベクトルを保存

		vol_min = DirectX::XMVectorMin(vol_min, vec_a);	//頂点Aで最小座標を更新
		vol_min = DirectX::XMVectorMin(vol_min, vec_b);	//頂点Bで最小座標を更新
		vol_min = DirectX::XMVectorMin(vol_min, vec_c);	//頂点Cで最小座標を更新

		vol_max = DirectX::XMVectorMax(vol_max, vec_a);	//頂点Aで最大座標を更新
		vol_max = DirectX::XMVectorMax(vol_max, vec_b);	//頂点Bで最大座標を更新
		vol_max = DirectX::XMVectorMax(vol_max, vec_c);	//頂点Cで最大座標を更新
	}

	//---------------------
	//エリア生成処理
	//---------------------
	
	//計算した最小値をXMFLOAT3に変換
	DirectX::XMFLOAT3 float_min;
	DirectX::XMStoreFloat3(&float_min, vol_min);

	//計算した最大値をXMFLOAT3に変換
	DirectX::XMFLOAT3 float_max;
	DirectX::XMStoreFloat3(&float_max, vol_max);

	CreateAreas(float_min, float_max);	//エリア生成関数を呼び出し
	is_built = true;	//構築完了フラグを立てる
}

//=========================
//レイキャストを実行
//=========================
bool SpaceDivisionCast::Raycast(const DirectX::XMFLOAT3& start_pos, const DirectX::XMFLOAT3& end_pos, DirectX::XMFLOAT3& hit_position, DirectX::XMFLOAT3& hit_normal)
{
	if (!is_built)return false;	//構築されていない場合はfalseを返す

	DirectX::XMVECTOR vec_start = DirectX::XMLoadFloat3(&start_pos);				//始点をXMVECTORにロード
	DirectX::XMVECTOR vec_end = DirectX::XMLoadFloat3(&end_pos);					//終点をXMVECTORにロード
	DirectX::XMVECTOR vec_dir = DirectX::XMVectorSubtract(vec_end, vec_start);		//レイのベクトルを計算
	DirectX::XMVECTOR vec_norm_dir = DirectX::XMVector3Normalize(vec_dir);			//レイの方向を正規化
	float max_distance = DirectX::XMVectorGetX(DirectX::XMVector3Length(vec_dir));	//レイの最大長さを計算

	bool is_hit = false;					//ヒットしたかのフラグ
	float closest_distance = max_distance;	//最短ヒット距離をレイの長さに初期化

	//-------------------------
	//エリアごとの交差判定
	//-------------------------
	//登録されたすべてのエリアに対してループ処理を行う
	for (const Area& area : areas_list)	
	{
		float area_dist = 0.0f;	//エリアまでの距離を保存

		//レイがエリアのバウンディングボックスと交差するか判定
		if (area.bounding_box.Intersects(vec_start, vec_norm_dir, area_dist))
		{	
			if (closest_distance < area_dist)continue;	//エリアまでの距離が現在の最短ヒット距離より遠い場合はスキップ

			//エリアに含まれるすべての三角形に対して判定を行う
			for (int tri_index : area.triangle_indices)
			{
				const Traiangle& tri = triangles_list[tri_index];	//三角形データを取得

				DirectX::XMVECTOR a = DirectX::XMLoadFloat3(&tri.position[0]);	//頂点Aをロード
				DirectX::XMVECTOR b = DirectX::XMLoadFloat3(&tri.position[1]);	//頂点Bをロード
				DirectX::XMVECTOR c = DirectX::XMLoadFloat3(&tri.position[2]);	//頂点Cをロード

				float tri_dist = 0.0f;	//三角形までの距離を保存

				//DirectXの公式関数を使用して、レイと三角形の交差判定を行う
				if (DirectX::TriangleTests::Intersects(vec_start, vec_norm_dir, a, b, c, tri_dist))
				{
					//ヒットした距離が現在の最短ヒット距離より近いか確認
					if (tri_dist < closest_distance)
					{
						closest_distance = tri_dist;	//最短距離を更新
						hit_normal = tri.normal;		//法線情報を更新
						is_hit = true;					//ヒットフラグを立てる
					}
				}
			}
		}
	}
	return is_hit;	//判定結果を返す
}

//-------------------------------
//疑似スフィアキャストを実行
//-------------------------------
bool SpaceDivisionCast::PsseudoSpheraCast(const DirectX::XMFLOAT3& start_pos, const DirectX::XMFLOAT3& end_pos, float radius, DirectX::XMFLOAT3& hit_position, DirectX::XMFLOAT3& hit_normal)
{
	if (!is_built) return false;	//未構築の場合は処理を中断

	DirectX::XMVECTOR v_start = DirectX::XMLoadFloat3(&start_pos);					//始点をベクトルにロード
	DirectX::XMVECTOR v_end = DirectX::XMLoadFloat3(&end_pos);						//終点をベクトルにロード
	DirectX::XMVECTOR v_dir = DirectX::XMVectorSubtract(v_end, v_start);			//レイのベクトルを計算
	float max_distance = DirectX::XMVectorGetX(DirectX::XMVector3Length(v_dir));	//レイの最大長さを計算
	if (max_distance <= 0.0001f)return false;										//長さが0の場合は移動していないためヒットしないとみなし終了
	DirectX::XMVECTOR v_norm_dir = DirectX::XMVector3Normalize(v_dir);				//レイの方向を正規化

	//-----------------------------
	//基準となる軸ベクトルの算出
	//-----------------------------
	DirectX::XMVECTOR v_world_up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);	//進行方向に対する上方向の基準ベクトルを仮設定

	//進行方向が真上や真下に近い場合は、基準ベクトルをX軸に変更
	if (std::abs(DirectX::XMVectorGetY(v_norm_dir)) > 0.99f)
	{
		v_world_up = DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);	//真上・真下を向いている時の計算破綻を防止
	}
	DirectX::XMVECTOR v_right = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(v_world_up, v_norm_dir));	//右方向ベクトルを外積で計算
	DirectX::XMVECTOR v_up = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(v_norm_dir, v_right));			//正確な上方向ベクトルを外積で再計算
	bool is_any_hit = false;			//全体のヒット判定フラグ
	float closest_dist = max_distance;	//最短距離を最大距離で初期化
	constexpr int CIRCLE_RAY_COUNT = 8;	//円周上に配置するレイの本数
	constexpr float ANGLE_STEP = DirectX::XM_2PI / CIRCLE_RAY_COUNT;	//1本あたりの角度間隔を計算

	//------------------------------
	//複数レイによるキャスト処理
	//------------------------------
	//中心1本 ＋ 周囲のレイ で判定
	for (int i = 0; i <= CIRCLE_RAY_COUNT; i++)
	{
		DirectX::XMVECTOR v_offset;	//オフセットベクトルを保存
		if (i == 0)v_offset = DirectX::XMVectorZero();	//中心なのでオフセットはゼロに設定
		else
		{
			float angle = ANGLE_STEP * (i - 1);	//周囲のレイの角度を計算
			float cos_a = std::cosf(angle);		//X方向の比率を計算
			float sin_a = std::sinf(angle);		//Y方向の比率を計算
			v_offset = DirectX::XMVectorAdd(
				DirectX::XMVectorScale(v_right, cos_a * radius),
				DirectX::XMVectorScale(v_up, sin_a * radius)
			);
		}

		DirectX::XMVECTOR v_ray_start = DirectX::XMVectorAdd(v_start, v_offset);	//オフセットを加算した新しい始点
		DirectX::XMVECTOR v_ray_end = DirectX::XMVectorAdd(v_end, v_offset);		//オフセットを加算した新しい終点
		DirectX::XMFLOAT3 ray_start_f3;	//既存関数呼び出し用に始点を構造体へ変換
		DirectX::XMStoreFloat3(&ray_start_f3, v_ray_start);	
		DirectX::XMFLOAT3 ray_end_f3;	//既存関数呼び出し用に終点を構造体へ変換
		DirectX::XMStoreFloat3(&ray_end_f3, v_ray_end);
		DirectX::XMFLOAT3 temp_hit_pos;		//一時的なHit位置を保持
		DirectX::XMFLOAT3 temp_hit_normal;	//一時的なHit法線を保持

		//既存のRaycast関数を呼び出して判定
		if (Raycast(ray_start_f3, ray_end_f3, temp_hit_pos, temp_hit_normal))
		{
			DirectX::XMVECTOR v_hit = DirectX::XMLoadFloat3(&temp_hit_pos);				//始点からヒット位置までのベクトルを計算
			DirectX::XMVECTOR v_dist = DirectX::XMVectorSubtract(v_hit, v_ray_start);	//ベクトルを引き算
			float dist = DirectX::XMVectorGetX(DirectX::XMVector3Length(v_dist));		//ヒット距離を算出

			//最短距離を更新したか確認
			if (dist < closest_dist)
			{
				closest_dist = dist;			//最短距離を上書き
				hit_position = temp_hit_pos;	//ヒット位置を上書き
				hit_normal = temp_hit_normal;	//法線情報を上書き
				is_any_hit = true;				//ヒットフラグを立てる
			}
		}

	}
	return is_any_hit;	//判定結果を返す
}

//静的球交差判定
bool SpaceDivisionCast::StaticSpheraCast(const DirectX::XMFLOAT3& start_pos, const DirectX::XMFLOAT3& end_pos, float radius, DirectX::XMFLOAT3& hit_position, DirectX::XMFLOAT3& hit_normal)
{
	if (areas_list.empty())return false;	

	//判定用の球体作成と初期化
	DirectX::BoundingSphere sphere;										
	sphere.Center = end_pos;											
	sphere.Radius = radius;												
	bool is_hit = false;												
	float closest_dist = std::numeric_limits<float>::max();				
	DirectX::XMVECTOR start_vec = DirectX::XMLoadFloat3(&start_pos);

	//グリッドインデックスによる探索範囲の計算
	float min_x = end_pos.x - radius;
	float max_x = end_pos.x + radius;
	float min_z = end_pos.z - radius;
	float max_z = end_pos.z + radius;

	int start_x = static_cast<int>(std::floor((min_x - volume_min_pos.x) / CELL_SIZE));
	int end_x = static_cast<int>(std::floor((max_x - volume_min_pos.x) / CELL_SIZE));
	int start_z = static_cast<int>(std::floor((min_z - volume_min_pos.z) / CELL_SIZE));
	int end_z = static_cast<int>(std::floor((max_z - volume_min_pos.z) / CELL_SIZE));

	start_x = std::clamp(start_x, 0, grid_count_x - 1);
	end_x = std::clamp(end_x, 0, grid_count_x - 1);
	start_z = std::clamp(start_z, 0, grid_count_z - 1);
	end_z = std::clamp(end_z, 0, grid_count_z - 1);

	//対象エリアのみの限定ループ処理
	for (int x = start_x; x <= end_x; ++x)
	{
		for (int z = start_z; z <= end_z; ++z)
		{
			int index = x + z * grid_count_x;
			const Area& area = areas_list[index];
			if (area.triangle_indices.empty())continue;

			//エリアのBoundingBoxと球が交差しているか大まかに判定
			if (area.bounding_box.Intersects(sphere))
			{
				//エリアに含まれる三角形のインデックスリストでループ
				for (int tri_index : area.triangle_indices)
				{
					const Traiangle& tri = triangles_list[tri_index];
					DirectX::XMVECTOR vertex_a = DirectX::XMLoadFloat3(&tri.position[0]);
					DirectX::XMVECTOR vertex_b = DirectX::XMLoadFloat3(&tri.position[1]);
					DirectX::XMVECTOR vertex_c = DirectX::XMLoadFloat3(&tri.position[2]);

					if (sphere.Intersects(vertex_a, vertex_b, vertex_c))
					{
						DirectX::XMVECTOR vec_center = DirectX::XMLoadFloat3(&end_pos);
						DirectX::XMVECTOR vec_normal = DirectX::XMLoadFloat3(&tri.normal);
						DirectX::XMVECTOR vec_c_to_a = DirectX::XMVectorSubtract(vec_center, vertex_a);
						float plane_dist = DirectX::XMVectorGetX(DirectX::XMVector3Dot(vec_c_to_a, vec_normal));
						float abs_dist = std::abs(plane_dist);

						if (abs_dist < closest_dist)
						{
							closest_dist = abs_dist;
							hit_normal = tri.normal;
							DirectX::XMVECTOR vec_hit_pos = DirectX::XMVectorSubtract(vec_center, DirectX::XMVectorScale(vec_normal, plane_dist));
							DirectX::XMStoreFloat3(&hit_position, vec_hit_pos);
							is_hit = true;
						}
					}

				}
			}
		}
	}
	return is_hit;
}

//複数球を用いた疑似カプセルキャスト
bool SpaceDivisionCast::MultiSpheraCast(
	const DirectX::XMFLOAT3& bottom_pos, const DirectX::XMFLOAT3& top_pos,
	float radius,
	DirectX::XMFLOAT3& hit_position, DirectX::XMFLOAT3& hit_normal)
{
	//カプセルの長さと配置する球の数の計算
	DirectX::XMVECTOR bottom_vec = DirectX::XMLoadFloat3(&bottom_pos);					
	DirectX::XMVECTOR top_vec = DirectX::XMLoadFloat3(&top_pos);						
	DirectX::XMVECTOR dir_vec = DirectX::XMVectorSubtract(top_vec, bottom_vec);			
	float capsule_length = DirectX::XMVectorGetX(DirectX::XMVector3Length(dir_vec));	
	int sphere_count = CalculateSpheraCount(capsule_length, radius);					

	//配置割合のステップ幅の事前計算
	float step_ratio = 0.0f;
	if (sphere_count > 1)
	{
		step_ratio = 1.0f / static_cast<float>(sphere_count - 1);
	}

	//配置した球ごとに交差判定ループ
	for (int i = 0; i < sphere_count; i++)
	{
		float ratio = static_cast<float>(i) * step_ratio;										
		DirectX::XMFLOAT3 current_sphere_pos = GetLerpPosition(bottom_vec, top_vec, ratio);	
		DirectX::XMFLOAT3 current_position = { 0.0f,0.0f,0.0f };
		DirectX::XMFLOAT3 current_normal = { 0.0f,0.0f,0.0f };
		bool is_sphere_hit = StaticSpheraCast(current_sphere_pos, current_sphere_pos, radius, current_position, current_normal);

		//ヒット時の処理
		if (is_sphere_hit)
		{
			hit_position = current_position;
			hit_normal = current_normal;
			return true;
		}
	}

	return false;
}

//=======================
//静的OBBキャスト
//=======================
bool SpaceDivisionCast::StaticObbCast(const DirectX::XMFLOAT3& center_pos, const DirectX::XMFLOAT3& extents, const DirectX::XMFLOAT4& orientation, DirectX::XMFLOAT3& hit_position, DirectX::XMFLOAT3& hit_normal)
{
	if (areas_list.empty())return false;

	//判定用OBB作成
	DirectX::BoundingOrientedBox obb;									
	obb.Center = center_pos;											
	obb.Extents = extents;												
	obb.Orientation = orientation;										
	bool is_hit = false;												
	float closest_dist = std::numeric_limits<float>::max();				
	DirectX::XMVECTOR center_vec = DirectX::XMLoadFloat3(&center_pos);

	//空間分割エリアとボックスの交差判定
	for (const Area& area : areas_list)
	{
		if (area.bounding_box.Intersects(obb))
		{
			//三角形面とOBBの交差判定
			for (int tri_index : area.triangle_indices)
			{
				const Traiangle& tri = triangles_list[tri_index];						
				DirectX::XMVECTOR vertex_a = DirectX::XMLoadFloat3(&tri.position[0]);
				DirectX::XMVECTOR vertex_b = DirectX::XMLoadFloat3(&tri.position[1]);
				DirectX::XMVECTOR vertex_c = DirectX::XMLoadFloat3(&tri.position[2]);
				if (obb.Intersects(vertex_a, vertex_b, vertex_c))
				{
					DirectX::XMVECTOR sum_ab = DirectX::XMVectorAdd(vertex_a, vertex_b);			
					DirectX::XMVECTOR sum_abc = DirectX::XMVectorAdd(sum_ab, vertex_c);				
					const float ONE_THIRD = 1.0f / 3.0f;											
					DirectX::XMVECTOR tri_center = DirectX::XMVectorScale(sum_abc, ONE_THIRD);		
					DirectX::XMVECTOR dist_vec = DirectX::XMVectorSubtract(tri_center, center_vec);
					float dist = DirectX::XMVectorGetX(DirectX::XMVector3Length(dist_vec));			
					if (dist < closest_dist)	
					{
						closest_dist = dist;		
						hit_normal = tri.normal;
						DirectX::XMStoreFloat3(&hit_position, tri_center);
						is_hit = true;			
					}
				}
			}
		}
	}
	return is_hit;
}

//全ての境界線のリストを取得
std::vector<DirectX::BoundingBox> SpaceDivisionCast::GetAreaBoundingBoxes() const
{
	//戻り値用のコンテナを準備
	std::vector<DirectX::BoundingBox> bboxes;
	bboxes.reserve(areas_list.size());

	//エリアリストから境界線データを抽出
	for (size_t i = 0; i < areas_list.size(); i++)
	{
		bboxes.push_back(areas_list.at(i).bounding_box);
	}

	return bboxes;
}

//エリアの総数を取得
size_t SpaceDivisionCast::GetAreaCount()
{
	return areas_list.size();
}

//エリアを作成
void SpaceDivisionCast::CreateAreas(const DirectX::XMFLOAT3& volume_min, const DirectX::XMFLOAT3& volume_max)
{
	//分割数の計算と保存
	grid_count_x = static_cast<int>(std::ceil((volume_max.x - volume_min.x) / CELL_SIZE));
	grid_count_z = static_cast<int>(std::ceil((volume_max.z - volume_min.z) / CELL_SIZE));

	if (grid_count_x <= 0) grid_count_x = 1;
	if (grid_count_z <= 0) grid_count_z = 1;

	volume_min_pos = volume_min;
	volume_max_pos = volume_max;

	areas_list.clear();
	areas_list.resize(grid_count_x * grid_count_z);

	//X方向のループを開始
	for (int x = 0; x < grid_count_x; ++x)
	{
		//Z方向のループを開始
		for (int z = 0; z < grid_count_z; z++)
		{
			int index = x + z * grid_count_x;
			Area& new_area = areas_list[index];

			float center_x = volume_min.x + (x * CELL_SIZE) + (CELL_SIZE * 0.5f);	
			float center_y = (volume_max.y + volume_min.y) * 0.5f;					
			float center_z = volume_min.z + (z * CELL_SIZE) + (CELL_SIZE * 0.5f);	

			//エリアの広さの半分を計算
			float extents_x = CELL_SIZE * 0.5f;							
			float extents_y = (volume_max.y - volume_min.y) * 0.5f;		
			float extents_z = CELL_SIZE * 0.5f;							

			new_area.bounding_box.Center = DirectX::XMFLOAT3(center_x, center_y, center_z);		
			new_area.bounding_box.Extents = DirectX::XMFLOAT3(extents_x, extents_y, extents_z);	

			//全ての三角形に対してエリアとの交差判定を行う
			for (int i = 0; i < static_cast<int>(triangles_list.size()); ++i)
			{
				DirectX::XMVECTOR tri_a = DirectX::XMLoadFloat3(&triangles_list[i].position[0]);
				DirectX::XMVECTOR tri_b = DirectX::XMLoadFloat3(&triangles_list[i].position[1]);
				DirectX::XMVECTOR tri_c = DirectX::XMLoadFloat3(&triangles_list[i].position[2]);

				//DirectXの公式関数を使用して、Boxと三角形が交差するか判定
				if (new_area.bounding_box.Intersects(tri_a, tri_b, tri_c))
				{
					new_area.triangle_indices.push_back(i);
				}
			}
		}
	}
}

//カプセルの隙間を埋めるために必要な球の数を計算
int SpaceDivisionCast::CalculateSpheraCount(float capsule_length, float radius) const
{
	float interval = radius * SPHERE_INTERVAL_RATIO;					
	if (interval <= 0.0f) return 2;										
	int count = static_cast<int>(std::ceil(capsule_length / interval));	
	count += 1;															
	if (count < 2) count = 2;											
	return count;														
}

//2点間を指定した割合で線形補間した座標を計算
DirectX::XMFLOAT3 SpaceDivisionCast::GetLerpPosition(const DirectX::XMVECTOR& start_vec, const DirectX::XMVECTOR& end_vec, float ratio) const
{
	DirectX::XMVECTOR lerp_vec = DirectX::XMVectorLerp(start_vec, end_vec, ratio);	
	DirectX::XMFLOAT3 result_pos;													
	DirectX::XMStoreFloat3(&result_pos, lerp_vec);									
	return result_pos;																
}
