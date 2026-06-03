#pragma once

#include <memory>
#include "RotationBase.h"

enum class RotationType
{
	LookAt,	//対象の方向に向く回転
};

class RotationFactory
{
public:
	//指定された種類の回転クラスをインスタンス化
	static std::unique_ptr<RotationBase>Create(RotationType rotation_type);
};

