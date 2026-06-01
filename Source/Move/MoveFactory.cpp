#include "MoveFactory.h"

#include "LinearMove.h"
#include "CircularMove.h"
#include "RepulsiveMove.h"

//=====================
//ˆÚ“®ƒNƒ‰ƒX‚ğ¶¬
//=====================
std::unique_ptr<MoveBase> MoveFactory::Create(MoveType move_type)
{
	switch (move_type)
	{
	case MoveType::Linear:
		return std::make_unique<LinearMove>();
	case MoveType::Circular:
		return std::make_unique<CircularMove>();
	case MoveType::Repulsive:
		return std::make_unique<RepulsiveMove>();
	default:
		return nullptr;
	}
}
