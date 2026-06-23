#pragma once
#include "JudgmentNode.h"

#include <DirectXMath.h>
#include <functional>

//対象との相対方向
enum class RelativeDirection
{
	None,		//判定不可
	Front,		//前判定
	FrontRight,	//前方右判定
	Right,		//右判定
	BackRight,	//後方右判定
	Back,		//後ろ判定
	BackLeft,	//後方左判定
	Left,		//左判定
	FrontLeft	//前方左判定
};

class DirectionJudgment : public JudgmentNode
{
public:
	DirectionJudgment(
		const DirectX::XMFLOAT3& pos_ref,
		const DirectX::XMFLOAT3& front_ref,
		const DirectX::XMFLOAT3& target_pos_ref);

	//相対方向を取得する処理
	RelativeDirection GetRelativeDirection();

	//仮の判定処理
	bool Check()override;

private:
	std::reference_wrapper<const DirectX::XMFLOAT3> pos_ref;        //自身の位置への参照
	std::reference_wrapper<const DirectX::XMFLOAT3> front_ref;      //自身の前方ベクトルへの参照
	std::reference_wrapper<const DirectX::XMFLOAT3> target_pos_ref; //対象の位置への参照

	//---判定用定数---
	static constexpr float threshold_front = 0.924f;     //前方判定閾値(約22.5度)
	static constexpr float threshold_side = 0.383f;      //側面判定閾値(約67.5度)
	static constexpr float threshold_back = -0.383f;     //後方側面判定閾値(約112.5度)
	static constexpr float threshold_rear = -0.924f;     //真後ろ判定閾値(約157.5度)
	static constexpr float zero_tolerance = 1e-5f;       //ゼロベクトル許容誤差
};

