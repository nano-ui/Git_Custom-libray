#pragma once

#include <memory>
#include <functional>
#include <DirectXMath.h>

class JudgmentNode
{
public:
	virtual ~JudgmentNode() = default;
	virtual bool Check() = 0;
};

