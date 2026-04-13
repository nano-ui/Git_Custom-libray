#pragma once
#include <vector>
#include <string>
#include <DirectXMath.h>

namespace tinygltf { class Model; struct Animation; }
class GltfBone;

class GltfAnimation
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
		DirectX::XMFLOAT4 value;	//格納値(位置/回転/スケール)
	};

	//チャンネル（どのボーンをどう動かすかの情報）
	struct Channel
	{
		int target_bone_index;	//動かすボーン番号
		TargetType target_type;	//変化させる項目
		std::vector<Keyframe> keyframes;	//時間ごとの変化データリスト
		mutable size_t last_key_index;	//前回参照したインデックス
	};

public:

	GltfAnimation();

	//アニメーションデータの初期化
	void Initialize(
		const tinygltf::Model& model,
		const tinygltf::Animation& anim);

	//アニメーションの更新
	void Update(float delta_time, std::vector<GltfBone>& bones);

	const std::string& GetAnimationName() const { return animation_name; }

private:
	std::string animation_name;		//アニメーション名
	std::vector<Channel> channels;	//全ボーンの変化チャンネルリスト
	float current_time;				//現在の再生時間
	float max_duration;				//アニメーションの最大長(秒)
};