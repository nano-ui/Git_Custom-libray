#pragma once

#include <unordered_map>
#include <string>
#include <vector>

#include "../Common/ModelData.h"

namespace fbxsdk
{
	class FbxScene;
}

class FbxAnimation
{
public:
	//キーフレームアニメーションを解析
	static void Fetch(fbxsdk::FbxScene* scene, const std::vector<BoneData>& bones, std::unordered_map<std::string, AnimationClip>& out_animatins, float sampling_rate = 0.0f);
};