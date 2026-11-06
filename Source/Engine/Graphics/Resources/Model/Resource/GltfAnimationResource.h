#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <DirectXMath.h>

#include "Engine\Core\Log\Log.h"

static constexpr int INVALID_ANIMATION_INDEX = -1;	//アニメーション無効値

//対象の要素
enum class AnimaitonTargetPath
{
	Translation,	//移動
	Rotation,		//回転
	Scale,			//拡縮
	Unknown			//不明
};

//アニメーション補完方法
enum class AnimationInterpolation
{
	Linear,			//線形補完
	Step,			//
	CubicSpline
};

//アニメーションチャンネル情報
struct GltfAnimationChannel
{
	int target_node;								//対象のノードインデックス
	AnimaitonTargetPath target_path;				//対象の要素
	AnimationInterpolation interpolation;			//補完方法
	std::vector<float> timelines;					//キーフレーム時刻配列
	std::vector<DirectX::XMFLOAT3> translations;	//移動キーフレーム配列
	std::vector<DirectX::XMFLOAT4> rotations;		//回転キーフレーム配列
	std::vector<DirectX::XMFLOAT3> scales;			//拡縮キーフレーム配列

	GltfAnimationChannel()
	{
		target_node = -1;
		target_path = AnimaitonTargetPath::Unknown;
		interpolation = AnimationInterpolation::Linear;
	}
};

//アニメーションクリップ情報
struct GltfAnimationClip
{
	std::string name;	//アニメーションクリップ名
	float duration;		//アニメーションの総時間
	std::vector<GltfAnimationChannel> channels;	//アニメーションチャンネル配列

	GltfAnimationClip()
	{
		name = "";
		duration = 0.0f;
	}
};


class GltfAnimationResource
{
public:
	GltfAnimationResource() = default;
	~GltfAnimationResource() = default;

public:
	//クリップの設定
	void SetClips(std::vector<GltfAnimationClip>&& in_clip)
	{
		clips = std::move(in_clip);
		animation_index_map.clear();
		for (size_t i = 0; i < clips.size(); ++i)
		{
			if (!clips[i].name.empty())animation_index_map[clips[i].name] = i;
		}
	}

	//クリップの取得
	const std::vector<GltfAnimationClip>& GetClips()const { return clips; }

	//クリップの取得
	std::vector<GltfAnimationClip>& GetClips() { return clips; }

	//特定のクリップの取得
	const GltfAnimationClip* GetClip(size_t index)const
	{
		if (index >= clips.size())
		{
			LOG_WARNING({ u8"設定" }, u8"指定したインデックス番号が範囲外です", index);
			return nullptr;
		}
		return &clips[index];
	}

	//クリップ名からインデックス番号を検索
	int FindClipIndex(const std::string& clip_name)const
	{
		if (clip_name.empty())
		{
			LOG_WARNING({ u8"設定" }, u8"検索対象の名前が設定されていません");
			return INVALID_ANIMATION_INDEX;
		}

		auto it = animation_index_map.find(clip_name);
		if (it != animation_index_map.end())return static_cast<int>(it->second);

		LOG_WARNING({ u8"検索" }, u8"アニメーションが見つかりませんでした", clip_name);
		return INVALID_ANIMATION_INDEX;
	}

	//クリップ数を取得
	size_t GetClipCount()const { return clips.size(); }

private:
	std::vector<GltfAnimationClip> clips;							//アニメーションクリップ配列
	std::unordered_map<std::string, size_t> animation_index_map;	//クリップ名とインデック番号のハッシュマップ
};