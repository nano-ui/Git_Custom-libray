#pragma once
#include <vector>
#include <string>
#include <DirectXMath.h>

namespace tinygltf { class Model; struct Animation; }
class GltfBone;

class GltfAnimaton
{
public:
	//アニメーションのターゲット
	enum class TargetType
	{
		Translation,
		Rotation,
		Scale
	};

	//キーフレームのデータ構造
	struct Keyframe
	{
		float time;	//キーフレームの時間
		DirectX::XMFLOAT4 value;
	};

	//チャンネル（どのボーンをどう動かすかの情報）
	struct Channel
	{
		int target_bone_index;	//動かすボーン番号
		TargetType target_type;	//変化させる項目
		std::vector<Keyframe> keyframes;	//時間ごとの変化データリスト
	};

public:



private:
	std::string animation_name;	//アニメーション名
	std::vector<Channel> channels;	//全ボーンの変化チャンネルリスト
	float current_time;				//現在の再生時間
	float max_duration;				//アニメーションの最大長(秒)
};