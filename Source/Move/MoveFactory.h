#pragma once

#include <memory>
#include "MoveBase.h"

//ˆÚ“®‚Ìí—Ş
enum class MoveType
{
	Linear,		//’¼üˆÚ“®
	Circular,	//‰~ˆÚ“®
	Repulsive	//Ë—ÍˆÚ“®
};

class MoveFactory
{
public:
	//ˆÚ“®ƒNƒ‰ƒX‚ğ¶¬
	static std::unique_ptr<MoveBase> Create(MoveType move_type);
};

