#pragma once

#include <cstdint>
#include "Utility\BitmaskOperators.h"


//オブジェクトタグ
enum class GameObjectTag : uint32_t
{
	None = 0,			//無し
	Player = 1 << 0,	//プレイヤー
	Allies = 1 << 1,	//味方
	Enemy = 1 << 2		//敵
};
template<>struct EnableBitmaskOperators<GameObjectTag> : std::true_type {};