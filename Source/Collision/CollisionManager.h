#pragma once

#include <vector>
#include <memory>
#include <unordered_map>
#include <DirectXMath.h>

#include "Collider.h"
#include "CollisionLogic.h"

class ShapeRenderer;

struct CellData
{
	std::vector<SphereCollider*> spheres;			//スフィアコライダー登録リスト
	std::vector<CapsuleCollider*> capsules;			//カプセルコライダー登録リスト
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
		constexpr std::size_t prime_x = 73856093;
		constexpr std::size_t prime_y = 19349663;
		constexpr std::size_t prime_z = 83492791;

		//各座標成分に素数を掛けてXORで結合し、一意に近いハッシュ値を生成
		return (static_cast<std::size_t>(key.x) * prime_x) ^
			(static_cast<std::size_t>(key.y) * prime_y) ^
			(static_cast<std::size_t>(key.z) * prime_z);
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

	//デバッグ描画
	void RenderDebug(ShapeRenderer* renderer);

private:
	//動的コライダーと空間分割の判定
	void CheckDynamicVsSpace();

	//スフィア同士の総当たり判定
	void CheckSphereVsSphere();

	//グリッド登録用の補助関数
	void AddColluderToGrid(Collider* collider);

	//座標からグリッド範囲を計算
	GridRange CalculateGridRenge(Collider* collider)const;

	//現在のコライダー密度から最適なセルサイズを計算して適用
	void OptimizeCellSize();

private:
	struct GridElement
	{
		SphereCollider* sphere;	//登録されたスフィアへのポインタ
		int next_index;			//同じセルに属する次の要素へのリンクインデックス
	};

private:
	std::unique_ptr<CollisionLogic> collision_logic;					//判定ロジッククラス
	std::vector<SphereCollider*> sphere_colliders;						//スフィアコライダーの登録リスト
	std::vector<CapsuleCollider*> capsule_colliders;					//カプセルコライダーの登録リスト
	std::vector<SpaceDivisionCollider*> space_colliders;				//空間分割コライダー登録リスト
	std::vector<Collider*> dynamic_colliders;							//動的コライダーの登録リスト
	std::unordered_map<GridKey, CellData, GridKeyHasher > spatial_grid;	//グリッド分割のハッシュマップ
	std::vector<GridElement> grid_elements;								//全セルの要素を1つにまとめた配列
	std::unordered_map<GridKey, int, GridKeyHasher> grid_heads;			//各セルの先頭要素へのインデックスマップ
	float cell_size;			//グリッド1セルの一辺の長さ
	bool is_enable_collision;	//当たり判定システムの有効フラグ
	bool is_draw_grid;			//グリッド描画有効フラグ
	float execution_time_ms = 0.0f;	//衝突判定全体の処理負荷時間
	bool is_auto_optimize_grid;		//オートリサイズ機能を有効にするかどうかのフラグ	
	float grid_margin_multiplier;	//セルサイズを決定する際の、平均直径に対する余裕を持たせるための倍率
	size_t previous_collider_count = 0;	//前フレームのコライダー総数を記憶
};