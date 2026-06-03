#include "RotationFactory.h"
#include "LockAtRotation.h"

//===============================================
//指定された種類の回転クラスをインスタンス化
//===============================================
std::unique_ptr<RotationBase> RotationFactory::Create(RotationType rotation_type)
{
	switch (rotation_type)
	{
	case RotationType::LookAt:
		return std::make_unique<LockAtRotation>();
	default:
		return nullptr;
	}
}
