#pragma once

#include <cfloat>
#include <DirectXMath.h>
#include <cstdint>


namespace CollisionLayer
{
	constexpr uint32_t none = 0;			//0000 0000
	constexpr uint32_t cast = 1 << 0;		//0000 0001 (キャスト)
	constexpr uint32_t collision = 1 << 1;  //0000 0010 (押し出し)
	constexpr uint32_t attack = 1 << 3;     //0000 1000 (攻撃判定)
	constexpr uint32_t all = 0xFFFFFFFF;	//全てのビットが立つ
}

//判定結果
struct CollisionResult
{
	DirectX::XMFLOAT3 hit_position;		//ヒットした座標
	DirectX::XMFLOAT3 hit_normal;		//ヒットした面の法線ベクトル
	DirectX::XMFLOAT3 safe_position;	//押し出し後の座標
	uint32_t hit_layer;					//衝突した相手のレイヤー情報
};

//衝突イベント受け取り用インターフェース
class ICollisionListener
{
public:
	virtual ~ICollisionListener() = default;
	virtual void OnCollisionHit(const CollisionResult& result) = 0;
};

enum class ColliderType { Sphere, Box, Cylinder, OBB, Capsule };

//コライダー基底構造体
struct Collider
{
	ColliderType type;								//形状
	DirectX::XMFLOAT3 mtd;							//押し出しベクトル
	uint32_t layer = CollisionLayer::none;			//所属するレイヤー
	uint32_t target_mask = CollisionLayer::none;	//相手のレイヤー
	bool is_active = true;							//有効フラグ
	ICollisionListener* listener = nullptr;			//衝突を通知する相手
	virtual  ~Collider() = default;
};

//スフィアコライダー
struct SphereCollider :public Collider
{
	DirectX::XMFLOAT3 center;		//中心座標
	DirectX::XMFLOAT3 old_center;	//前回の中心座標
	float radius;					//半径
	SphereCollider()
	{
		type = ColliderType::Sphere;
		radius = 0.0f;
		center = { 0.0f,0.0f,0.0f };
		old_center = { 0.0f,0.0f,0.0f };
	}
};

//ボックスコライダー
struct BoxCollider :public Collider
{
	DirectX::XMFLOAT3 box_min;      //最小座標
	DirectX::XMFLOAT3 box_max;      //最大座標
	DirectX::XMFLOAT3 old_box_min;  //移動前の最小座標
	DirectX::XMFLOAT3 old_box_max;  //移動前の最大座標

	BoxCollider()
	{
		box_min = { 0.0f,0.0f,0.0f };
		box_max = { 0.0f,0.0f,0.0f };
		old_box_min = { 0.0f,0.0f,0.0f };
		old_box_max = { 0.0f,0.0f,0.0f };
		type = ColliderType::Box;
	}
};

//シリンダーコライダー
struct CylinderCollider :public Collider
{
	DirectX::XMFLOAT3 center;       //中心座標
	DirectX::XMFLOAT3 old_center;   //移動前の中心座標
	float radius;                   //半径
	float height;                   //高さ

	CylinderCollider()
	{
		center = { 0.0f,0.0f,0.0f };
		old_center = { 0.0f,0.0f,0.0f };
		radius = 0.0f;
		height = 0.0f;
		type = ColliderType::Cylinder;
	}
};

//OBBコライダー
struct OBB :public Collider
{
	DirectX::XMFLOAT3 center;       //中心座標
	DirectX::XMFLOAT3 old_center;   //移動前の中心座標
	DirectX::XMFLOAT3 half;         //ハーフサイズ（X,Y,Z）
	DirectX::XMFLOAT3 axis[3];      //X,Y,Z方向ベクトル

	OBB()
	{
		center = { 0.0f,0.0f,0.0f };
		old_center = { 0.0f,0.0f,0.0f };
		half = { 0.0f,0.0f,0.0f };
		axis[0] = { 0.0f,0.0f,0.0f };
		axis[1] = { 0.0f,0.0f,0.0f };
		axis[2] = { 0.0f,0.0f,0.0f };
		type = ColliderType::OBB;
	}
};