#pragma once
#include "RotationBase.h"

class LockAtRotation : public RotationBase
{
public:
	//‘ÎÛ‚ÉŒü‚«‘±‚¯‚é
	DirectX::XMVECTOR Update(const DirectX::XMVECTOR& current_rotation, const DirectX::XMFLOAT3& target_dir) override;

	//‘ÎÛ‚ÉŒü‚«‘±‚¯‚é
	float UpdateAngle(float elapsed_time, float current_angle, const DirectX::XMFLOAT3& target_dir);
};

