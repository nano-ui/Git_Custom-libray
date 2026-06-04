#pragma once

#include <vector>
#include <memory>
#include <unordered_map>
#include <DirectXMath.h>

#include "Collider.h"
#include "CollisionLogic.h"

struct CellData
{
	std::vector<SphereCollider*> spheres;			//スフィアコライダー登録リスト
};

//グリッドキー構造体
struct GridKey
{
	int x;	//X軸のインデックス
	int y;	//Y軸のインデックス
	int z;	//Z軸のインデックス

	//ハッシュマップに必要な比較演算子を定義
	bool operator == (const GridKey& other)const
	{
		//全ての軸が一致しているか判定
		return x == other.x && y == other.y && z == other.z;
	}
};

//グリッドキーのハッシュ計算構造体
struct GridKeyHasher
{
	//関数呼び出し演算子をオーバーロード
	std::size_t operator()(const GridKey& key) const 
	{
		//ビットシフト用の定数
		constexpr int bit_shift = 1;

		//各要素の標準ハッシュ値を計算
		std::size_t hash_x = std::hash<int>()(key.x);
		std::size_t hash_y = std::hash<int>()(key.y);
		std::size_t hash_z = std::hash<int>()(key.z);

		//排他的論理和とビットシフトを組み合わせて一意に近い値を生成
		return ((hash_x ^ (hash_y << bit_shift)) >> bit_shift) ^ (hash_z << bit_shift);
	}
};

//グリッドの範囲構造体
struct GridRange
{
	GridKey min_key;	//最小のセル座標
	GridKey max_key;	//最大のセル座標
};

class CollisionManager
{
public:
	//コンストラクタ
	CollisionManager();

	//デストラクタ
	~CollisionManager();

	//コライダーの登録
	void Register(Collider* collider);

	//コライダーの削除
	void Remove(Collider* collider);

	//登録されたコライダーの一括判定
	void ExecuteCollision();

	//ImGuiデバッグ描画
	void RenderGui();

private:
	//スフィアと空間分割の判定
	void CheckSphereVsSpace();

	//スフィア同士の総当たり判定
	void CheckSphereVsSphere();

	//グリッド登録用の補助関数
	void AddColluderToGrid(Collider* collider);

	//座標からグリッド範囲を計算
	GridRange CalculateGridRenge(Collider* collider)const;

private:
	std::unique_ptr<CollisionLogic> collision_logic;					//判定ロジッククラス
	std::vector<SphereCollider*> sphere_colliders;						//スフィアコライダーの登録リスト
	std::vector<SpaceDivisionCollider*> space_colliders;				//空間分割コライダー登録リスト
	std::vector<Collider*> dynamic_colliders;							//動的コライダーの登録リスト
	std::unordered_map<GridKey, CellData, GridKeyHasher > spatial_grid;	//グリッド分割のハッシュマップ
	float cell_size;			//グリッド1セルの一辺の長さ
	bool is_enable_collision;	//当たり判定システムの有効フラグ

};

