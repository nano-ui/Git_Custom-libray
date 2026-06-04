#pragma once

#include <vector>
#include <memory>
#include <unordered_map>
#include <DirectXMath.h>

#include "Collider.h"
#include "CollisionLogic.h"

//グリッドキー
struct GridKye
{
	int x;	//X軸のインデックス
	int y;	//Y軸のインデックス
	int z;	//Z軸のインデックス

	//ハッシュマップに必要な比較演算子を定義
	bool operator == (const GridKye& other)const
	{
		//全ての軸が一致しているか判定
		return x == other.x && y == other.y && z == other.z;
	}
};

class CollisionManager
{
};

