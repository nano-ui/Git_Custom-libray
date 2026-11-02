#pragma once

#include <DirectXMath.h>
#include <algorithm>

struct BoundingSphere
{
	DirectX::XMFLOAT3 center = { 0.0f, 0.0f, 0.0f };
	float radius = 1.0f;
};

class Frustum
{
public:
	static constexpr size_t PLANE_COUNT = 6;
	static constexpr size_t PLANE_LEFT = 0;
	static constexpr size_t PLANE_RIGHT = 1;
	static constexpr size_t PLANE_BOTTOM = 2;
	static constexpr size_t PLANE_TOP = 3;
	static constexpr size_t PLANE_NEAR = 4;
	static constexpr size_t PLANE_FAR = 5;

	void Update(const DirectX::XMFLOAT4X4& view_projection)
	{
		DirectX::XMMATRIX vp = DirectX::XMLoadFloat4x4(&view_projection);

		DirectX::XMFLOAT4X4 m;
		DirectX::XMStoreFloat4x4(&m, vp);

		planes[PLANE_LEFT] = DirectX::XMVectorSet(m._14 + m._11, m._24 + m._21, m._34 + m._31, m._44 + m._41);
		planes[PLANE_RIGHT] = DirectX::XMVectorSet(m._14 - m._11, m._24 - m._21, m._34 - m._31, m._44 - m._41);
		planes[PLANE_BOTTOM] = DirectX::XMVectorSet(m._14 + m._12, m._24 + m._22, m._34 + m._32, m._44 + m._42);
		planes[PLANE_TOP] = DirectX::XMVectorSet(m._14 - m._12, m._24 - m._22, m._34 - m._32, m._44 - m._42);
		planes[PLANE_NEAR] = DirectX::XMVectorSet(m._13, m._23, m._33, m._43);
		planes[PLANE_FAR] = DirectX::XMVectorSet(m._14 - m._13, m._24 - m._23, m._34 - m._33, m._44 - m._43);

		for (size_t i = 0; i < PLANE_COUNT; ++i)
		{
			DirectX::XMVECTOR normal = planes[i];
			DirectX::XMVECTOR length = DirectX::XMVector3Length(normal);
			planes[i] = DirectX::XMVectorDivide(planes[i], length);
		}
	}

	bool Intersects(const DirectX::XMFLOAT3& center, float radius) const
	{
		DirectX::XMVECTOR sphere_center = DirectX::XMLoadFloat3(&center);

		for (size_t i = 0; i < PLANE_COUNT; ++i)
		{
			DirectX::XMVECTOR plane = planes[i];
			DirectX::XMVECTOR dot = DirectX::XMVector3Dot(plane, sphere_center);
			float distance = DirectX::XMVectorGetX(dot) + DirectX::XMVectorGetW(plane);

			if (distance < -radius)
			{
				return false;
			}
		}

		return true;
	}

private:
	DirectX::XMVECTOR planes[PLANE_COUNT];
};