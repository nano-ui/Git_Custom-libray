#pragma once

#include <cstdint>

//オブジェクトタグ
enum class GameObjectTag : uint32_t
{
	None = 0,			//無し
	Player = 1 << 0,	//プレイヤー
	Allies = 1 << 1,	//味方
	Enemy = 1 << 2		//敵
};

//OR演算
constexpr GameObjectTag operator|(GameObjectTag a, GameObjectTag b)
{
	return static_cast<GameObjectTag>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

//AND演算
constexpr GameObjectTag operator&(GameObjectTag a, GameObjectTag b)
{
	return static_cast<GameObjectTag>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

//NOT演算
constexpr GameObjectTag operator~(GameObjectTag a)
{
	return static_cast<GameObjectTag>(~static_cast<uint32_t>(a));
}

//追加
constexpr GameObjectTag& operator|=(GameObjectTag& a, GameObjectTag b) noexcept
{
	a = a | b;
	return a;
}

//削除
constexpr GameObjectTag& operator&=(GameObjectTag& a, GameObjectTag b) noexcept
{
	a = a & b;
	return a;
}