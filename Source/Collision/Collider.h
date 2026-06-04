#pragma once

#include <cfloat>
#include <DirectXMath.h>
#include <cstdint>

class SpaceDivisionCast;

//当たり判定の属性
enum class ColliderAttribute
{
	None,       // 属性なし
	Stage,      // ステージ（壁・床）
	Player,     // プレイヤー
	Enemy,      // 敵
	Attack      // 攻撃判定
};

//判定結果
struct CollisionResult
{
	DirectX::XMFLOAT3 hit_position;		//ヒットした座標
	DirectX::XMFLOAT3 hit_normal;		//ヒットした面の法線ベクトル
	DirectX::XMFLOAT3 safe_position;	//押し出し後の座標
	uint32_t hit_layer;					//衝突した相手のレイヤー情報
	ColliderAttribute hit_attribute;	//衝突した当たり判定の属性
};

//衝突イベント受け取り用インターフェース
class ICollisionListener
{
public:
	virtual ~ICollisionListener() = default;
	virtual void OnCollisionHit(const CollisionResult& result) = 0;
};

enum class ColliderType
{
	Sphere,
	Box,
	Cylinder,
	OBB,
	Capsule,
	SpaceDivision
};

//コライダー基底構造体
struct Collider
{
	ColliderType type;                                      //コライダーの形状
	DirectX::XMFLOAT3 mtd;                                  //押し出しベクトル
	ColliderAttribute attribute = ColliderAttribute::None;  //自身の属性
	bool is_active = true;                                  //有効フラグ
	ICollisionListener* listener = nullptr;                 //衝突の通知先
	virtual ~Collider() = default;
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

//カプセルコライダー
struct CapsuleCollider:public Collider
{
	DirectX::XMFLOAT3 start_center;		//カプセルの始点の中心座標
	DirectX::XMFLOAT3 end_center;		//カプセルの終点の中心座標
	DirectX::XMFLOAT3 old_start_center;	//移動前の始点座標
	DirectX::XMFLOAT3 old_end_center;	//移動前の終点座標
	float radius;						//カプセルの半径

	CapsuleCollider()
	{
		type = ColliderType::Capsule;
		radius = 0.0f;
		start_center = { 0.0f,0.0f,0.0f };
		end_center = { 0.0f,0.0f,0.0f };
		old_start_center = { 0.0f,0.0f,0.0f };
		old_end_center = { 0.0f,0.0f,0.0f };
	}
};

//空間分割コライダー
struct SpaceDivisionCollider :public Collider
{
	SpaceDivisionCast* space_cast;	//空間分割データ
	SpaceDivisionCollider()
	{
		type = ColliderType::SpaceDivision;
		space_cast = nullptr;
	}
};