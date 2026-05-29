#include "SpaceDivisionCast.h"
#include <limits>
#include <cmath>

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

		Traiangle& new_triangle = triangles_list.emplace_back();	//新しい三角形データを配列の末尾に追加
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

//=====================
//静的球交差判定
//=====================
bool SpaceDivisionCast::StaticSpheraCast(const DirectX::XMFLOAT3& start_pos, const DirectX::XMFLOAT3& end_pos, float radius, DirectX::XMFLOAT3& hit_normal)
{
	if (areas_list.empty())return false;	//未構築の場合は処理を終了

	//-----------------------------
	//判定用の球体作成と初期化
	//-----------------------------
	DirectX::BoundingSphere sphere;										//移動先の座標に判定用の球体を作成
	sphere.Center = end_pos;											//球の中心座標を設定
	sphere.Radius = radius;												//球の半径を設定
	bool is_hit = false;												//ヒットしたかどうかを保存するフラグ
	float closest_dist = std::numeric_limits<float>::max();				//最短ヒット距離を浮動小数点の最大値で初期化
	DirectX::XMVECTOR start_vec = DirectX::XMLoadFloat3(&start_pos);	//始点の座標をベクトルとしてロード

	//-------------------------
	//エリアと球の交差判定
	//-------------------------
	for (const Area& area : areas_list)	//登録されたすべての空間分割エリアをループ
	{
		if (area.bounding_box.Intersects(sphere))	//エリアのバウンディングボックスと球が交差するか大まかに判定
		{

			//--------------------------------
			//三角形面と球の正確な交差判定
			//--------------------------------
			for (int tri_index : area.triangle_indices)	//エリアに含まれるすべての三角形に対して判定
			{
				const Traiangle& tri = triangles_list[tri_index];						//判定対象の三角形データを参照
				DirectX::XMVECTOR vertex_a = DirectX::XMLoadFloat3(&tri.position[0]);	//三角形の頂点Aをベクトルとしてロード
				DirectX::XMVECTOR vertex_b = DirectX::XMLoadFloat3(&tri.position[1]);	//三角形の頂点Bをベクトルとしてロード
				DirectX::XMVECTOR vertex_c = DirectX::XMLoadFloat3(&tri.position[2]);	//三角形の頂点Cをベクトルとしてロード
				
				if (sphere.Intersects(vertex_a, vertex_b, vertex_c))	//DirectXの公式関数を使用して、球と三角形（面）の正確な交差判定を行う
				{
					DirectX::XMVECTOR sum_ab = DirectX::XMVectorAdd(vertex_a, vertex_b);			//複数の面に当たった場合、一番近い面を優先するため、三角形の中心を求める
					DirectX::XMVECTOR sum_abc = DirectX::XMVectorAdd(sum_ab, vertex_c);				//3頂点の合計を計算
					const float ONE_THIRD = 1.0f / 3.0f;											//3で割るためのマジックナンバー回避用定数
					DirectX::XMVECTOR tri_center = DirectX::XMVectorScale(sum_abc, ONE_THIRD);		//合計を3で割って中心座標を計算
					DirectX::XMVECTOR dist_vec = DirectX::XMVectorSubtract(tri_center, start_vec);	//始点から三角形の中心までのベクトルを計算
					float dist = DirectX::XMVectorGetX(DirectX::XMVector3Length(dist_vec));			//ベクトルの長さを計算して距離を取得
					if (dist < closest_dist)	//より近い面に当たった場合のみ法線を更新
					{
						closest_dist = dist;		//最短距離を更新
						hit_normal = tri.normal;	//当たった面の法線ベクトルを保存
						is_hit = true;				//ヒットフラグを立てる
					}
				}
			}
		}
	}
	return is_hit;	//判定結果を返す
}

//=====================================
//複数球を用いた疑似カプセルキャスト
//=====================================
bool SpaceDivisionCast::MultiSpheraCast(const DirectX::XMFLOAT3& bottom_pos, const DirectX::XMFLOAT3& top_pos, float radius, DirectX::XMFLOAT3& hit_normal)
{
	//-----------------------------------------
	//カプセルの長さと配置する球の数の計算
	//-----------------------------------------
	DirectX::XMVECTOR bottom_vec = DirectX::XMLoadFloat3(&bottom_pos);					//下端の座標をベクトルとしてロード
	DirectX::XMVECTOR top_vec = DirectX::XMLoadFloat3(&top_pos);						//上端の座標をベクトルとしてロード
	DirectX::XMVECTOR dir_vec = DirectX::XMVectorSubtract(top_vec, bottom_vec);			//下端から上端へのベクトルを計算
	float capsule_length = DirectX::XMVectorGetX(DirectX::XMVector3Length(dir_vec));	//カプセルの長さ（下端から上端までの距離）を計算
	int sphere_count = CalculateSpheraCount(capsule_length, radius);					//配置する球の数を専用関数で計算
	bool is_any_hit = false;															//ヒットしたかどうかを保存する全体フラグ
	DirectX::XMVECTOR total_normal_vec = DirectX::XMVectorZero();						//複数の球が当たった場合に法線を合成するためのベクトルを初期化
	
	//----------------------------------
	//配置した球ごとに交差判定ループ
	//----------------------------------
	for (int i = 0; i < sphere_count; i++)	//計算した球の数だけループ処理を行う
	{
		float ratio = static_cast<float>(i) / static_cast<float>(sphere_count - 1);				//下端(0.0)から上端(1.0)までの割合（配置位置）を計算
		DirectX::XMFLOAT3 current_sphere_pos = GetLerpPosition(bottom_vec, top_vec, ratio);		//割合に応じた球の中心座標を専用関数で計算
		DirectX::XMFLOAT3 current_normal;														//球がヒットした際の法線を受け取る
		bool is_sphere_hit = StaticSpheraCast(current_sphere_pos, current_sphere_pos, radius, current_normal);	//静的球交差判定関数を呼び出して判定

		//---------------------------
		//ヒット時の法線合成処理
		//---------------------------
		if (is_sphere_hit)	//球が何かにヒットしたか確認
		{
			is_any_hit = true;	//全体ヒットフラグを立てる
			DirectX::XMVECTOR normal_vec = DirectX::XMLoadFloat3(&current_normal);	//取得した法線をベクトルとしてロード
			total_normal_vec = DirectX::XMVectorAdd(total_normal_vec, normal_vec);	//合成用ベクトルに加算して蓄積
		}
	}

	//--------------------------------
	//最終的な合成法線の正規化処理
	//--------------------------------
	if (is_any_hit)	//1つ以上の球がヒットしたか確認
	{
		float length_sq = DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(total_normal_vec));	//合成した法線ベクトルの長さの2乗を計算
		const float EPSILON_LENGTH = 0.0001f;	//計算誤差や相殺によるゼロベクトル化を防ぐための極小値
		if (length_sq > EPSILON_LENGTH)			//長さが十分にある場合のみ正規化
		{
			total_normal_vec = DirectX::XMVector3Normalize(total_normal_vec);	//合成法線ベクトルを正規化
			DirectX::XMStoreFloat3(&hit_normal, total_normal_vec);				//正規化した結果を出力用の変数に保存
		}
		else
		{
			hit_normal = { 0.0f,1.0f,0.0f };	//相殺されてゼロになった場合は、安全のため上方向（Y軸）を法線にする
		}
	}
	return is_any_hit;	//判定結果を返す
}

//=======================
//静的OBBキャスト
//=======================
bool SpaceDivisionCast::StaticObbCast(const DirectX::XMFLOAT3& center_pos, const DirectX::XMFLOAT3& extents, const DirectX::XMFLOAT4& orientation, DirectX::XMFLOAT3& hit_normal)
{
	if (areas_list.empty())return false;	//未構築（エリアデータがない）の場合は処理を終了

	//--------------------
	//判定用OBB作成
	//--------------------
	DirectX::BoundingOrientedBox obb;									//DirectX公式のOBB構造体を作成
	obb.Center = center_pos;											//ボックスの中心座標を設定
	obb.Extents = extents;												//ボックスの各軸の半分の長さ（サイズ）を設定
	obb.Orientation = orientation;										//ボックスの向き（クォータニオン）を設定
	bool is_hit = false;												//ヒットしたかどうかを保存
	float closest_dist = std::numeric_limits<float>::max();				//最短ヒット距離を浮動小数点の最大値で初期化
	DirectX::XMVECTOR center_vec = DirectX::XMLoadFloat3(&center_pos);	//距離計算のためにボックスの中心座標をベクトルとしてロード

	//---------------------------------------
	//空間分割エリアとボックスの交差判定
	//---------------------------------------
	for (const Area& area : areas_list)	//登録されたすべての空間分割エリアをループ
	{
		if (area.bounding_box.Intersects(obb))	//エリアの境界（AABB）と今回のボックス（OBB）が交差するか大まかに判定
		{
			//----------------------------
			//三角形面とOBBの交差判定
			//----------------------------
			for (int tri_index : area.triangle_indices)	//エリアに含まれるすべての三角形に対して判定
			{
				const Traiangle& tri = triangles_list[tri_index];						//判定対象の三角形データを参照
				DirectX::XMVECTOR vertex_a = DirectX::XMLoadFloat3(&tri.position[0]);	//三角形の頂点Aをベクトルとしてロード
				DirectX::XMVECTOR vertex_b = DirectX::XMLoadFloat3(&tri.position[1]);	//三角形の頂点Bをベクトルとしてロード
				DirectX::XMVECTOR vertex_c = DirectX::XMLoadFloat3(&tri.position[2]);	//三角形の頂点Cをベクトルとしてロード
				if (obb.Intersects(vertex_a, vertex_b, vertex_c))	//OBBと三角形の判定を実行
				{
					DirectX::XMVECTOR sum_ab = DirectX::XMVectorAdd(vertex_a, vertex_b);			//複数の面に当たった場合、一番近い面を優先するため、三角形の中心を求める
					DirectX::XMVECTOR sum_abc = DirectX::XMVectorAdd(sum_ab, vertex_c);				//3頂点の合計を計算
					const float ONE_THIRD = 1.0f / 3.0f;											//3で割るためのマジックナンバー回避用定数
					DirectX::XMVECTOR tri_center = DirectX::XMVectorScale(sum_abc, ONE_THIRD);		//合計を3で割って中心座標を計算
					DirectX::XMVECTOR dist_vec = DirectX::XMVectorSubtract(tri_center, center_vec);	//始点から三角形の中心までのベクトルを計算
					float dist = DirectX::XMVectorGetX(DirectX::XMVector3Length(dist_vec));			//ベクトルの長さを計算して距離を取得
					if (dist < closest_dist)	//より近い面に当たった場合のみ法線を更新
					{
						closest_dist = dist;		//最短距離を更新
						hit_normal = tri.normal;	//当たった面の法線ベクトルを保存
						is_hit = true;				//ヒットフラグを立てる
					}
				}
			}
		}
	}
	return is_hit;	//判定結果を返す
}

//===============
//エリアを作成
//===============
void SpaceDivisionCast::CreateAreas(const DirectX::XMFLOAT3& volume_min, const DirectX::XMFLOAT3& volume_max)
{
	int count_x = static_cast<int>(std::ceil((volume_max.x - volume_min.x) / CELL_SIZE));	//X軸方向のエリア数を計算
	int count_z = static_cast<int>(std::ceil((volume_max.z - volume_min.z) / CELL_SIZE));	//Z軸方向のエリア数を計算

	//エリアが0以下の場合は最低1つにする
	if (count_x <= 0)count_x = 1;
	if (count_z <= 0)count_z = 1;

	//X方向のループを開始
	for (int x = 0; x < count_x; ++x) 
	{
		//Z方向のループを開始
		for (int z = 0; z < count_z; z++)
		{
			Area new_area;	//新しいエリアを作成

			float center_x = volume_min.x + (x * CELL_SIZE) + (CELL_SIZE * 0.5f);	//エリアの中心座標Xを計算
			float center_y = (volume_max.y + volume_min.y) * 0.5f;					//エリアの中心座標Yは全体の中心をする
			float center_z = volume_min.z + (z * CELL_SIZE) + (CELL_SIZE * 0.5f);	//エリアの中心座標Zを計算

			//エリアの広さの半分を計算
			float extents_x = CELL_SIZE * 0.5f;							//X方向の広さの半分を計算
			float extents_y = (volume_max.y - volume_min.y) * 0.5f;		//Y方向は全体の高さをカバーする
			float extents_z = CELL_SIZE * 0.5f;							//Z方向の広さの半分を計算

			new_area.bounding_box.Center = DirectX::XMFLOAT3(center_x, center_y, center_z);		//バウンディングボックスの中心を設定
			new_area.bounding_box.Extents = DirectX::XMFLOAT3(extents_x, extents_y, extents_z);	//バウンディングボックスのサイズを設定

			//全ての三角形に対してエリアとの交差判定を行う
			for (int i = 0; i < static_cast<int>(triangles_list.size()); ++i)
			{
				DirectX::XMVECTOR tri_a = DirectX::XMLoadFloat3(&triangles_list[i].position[0]);	//頂点Aをロード
				DirectX::XMVECTOR tri_b = DirectX::XMLoadFloat3(&triangles_list[i].position[1]);	//頂点Bをロード
				DirectX::XMVECTOR tri_c = DirectX::XMLoadFloat3(&triangles_list[i].position[2]);	//頂点Cをロード

				//DirectXの公式関数を使用して、Boxと三角形が交差するか判定
				if (new_area.bounding_box.Intersects(tri_a, tri_b, tri_c))
				{
					new_area.triangle_indices.push_back(i);	//交差していれば、このエリアに三角形インデックスを追加
				}

				//三角形が1つ以上含まれるエリアのみ保存
				if (!new_area.triangle_indices.empty())
				{
					areas_list.push_back(new_area);	//エリアリストに追加
				}
			}
		}
	}
}

//==================================================
//カプセルの隙間を埋めるために必要な球の数を計算
//==================================================
int SpaceDivisionCast::CalculateSpheraCount(float capsule_length, float radius) const
{
	float interval = radius * SPHERE_INTERVAL_RATIO;					//配置間隔（半径 × 密度比率）を計算
	if (interval <= 0.0f) return 2;										//間隔が0以下になる異常事態を防ぐためのチェック
	int count = static_cast<int>(std::ceil(capsule_length / interval));	//長さを間隔で割り、切り上げて必要な数を計算
	count += 1;															//端の球を追加するため+1
	if (count < 2) count = 2;											//カプセルには最低でも下端と上端の2つの球が必要なため補正
	return count;														//計算した数を返す
}

//================================================
//2点間を指定した割合で線形補間した座標を計算
//================================================
DirectX::XMFLOAT3 SpaceDivisionCast::GetLerpPosition(const DirectX::XMVECTOR& start_vec, const DirectX::XMVECTOR& end_vec, float ratio) const
{
	DirectX::XMVECTOR lerp_vec = DirectX::XMVectorLerp(start_vec, end_vec, ratio);	//DirectXの公式関数を使用して線形補間
	DirectX::XMFLOAT3 result_pos;													//計算結果を保存
	DirectX::XMStoreFloat3(&result_pos, lerp_vec);									//ベクトルを構造体に書き込み
	return result_pos;																//補間した座標を返す
}
