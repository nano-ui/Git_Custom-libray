#include "FbxAnimation.h"

#include <fbxsdk.h>

//==============
// 変換ヘルパー
//==============
namespace {
    DirectX::XMFLOAT4X4 ToXMFloat4x4(const FbxAMatrix& src) {
        DirectX::XMFLOAT4X4 dest;
        for (int r = 0; r < 4; r++) for (int c = 0; c < 4; c++) dest.m[r][c] = (float)src.Get(r, c);
        return dest;
    }
    DirectX::XMFLOAT3 ToXMFloat3(const FbxDouble3& src) {
        return DirectX::XMFLOAT3((float)src[0], (float)src[1], (float)src[2]);
    }
    DirectX::XMFLOAT4 ToXMFloat4(const FbxDouble4& src) {
        return DirectX::XMFLOAT4((float)src[0], (float)src[1], (float)src[2], (float)src[3]);
    }
}

//====================================
// キーフレームアニメーションを解析
//====================================
void FbxAnimation::Fetch(
	fbxsdk::FbxScene* scene,
	const std::vector<BoneData>& bones,
	std::unordered_map<std::string, AnimationClip>& out_animatins, 
	float sampling_rate)
{
	//-------------------------------------------
	//アニメーションスタック(クリップ)の数を取得
	//-------------------------------------------
	out_animatins.clear();
	int stack_count = scene->GetSrcObjectCount<FbxAnimStack>();
	for (int i = 0; i < stack_count; i++)
	{
		FbxAnimStack* stack = scene->GetSrcObject<FbxAnimStack>(i);
		scene->SetCurrentAnimationStack(stack);
		AnimationClip clip;
		clip.name = stack->GetName();

		//-------------------------------
		//アニメーションの時間範囲を取得
		//-------------------------------
		FbxTakeInfo* take_info = scene->GetTakeInfo(clip.name.c_str());
		FbxTime start = take_info->mLocalTimeSpan.GetStart();
		FbxTime end = take_info->mLocalTimeSpan.GetStop();

		//-------------------------
		//サンプリングレートの設定
		//-------------------------
		FbxTime duration = end - start;
		FbxTime step;
		float valid_sampling_rate = sampling_rate > 0 ? sampling_rate : 24.0f;
		FbxTime one_second;
		one_second.SetTime(0, 0, 1, 0, 0, scene->GetGlobalSettings().GetTimeMode());
		step = one_second / static_cast<double>(valid_sampling_rate);

		//------------------------------------------------
		//ボーン名とFBXノードの対応を事前にキャッシュする
		//------------------------------------------------
		std::unordered_map<std::string, FbxNode*> node_cache;
		int node_count = scene->GetSrcObjectCount<FbxNode>();
		for (int i = 0; i < node_count; i++)
		{
			FbxNode* node = scene->GetSrcObject<FbxNode>(i);
			node_cache[node->GetName()] = node;
		}

		//-------------------
		//フレームごとの処理(簡易実装)
		//-------------------
		for (FbxTime t = start; t < end; t += step)
		{
			std::vector<AnimationKeyframeNode> keyframe_nodes;
			keyframe_nodes.resize(bones.size());

			for (size_t b = 0; b < bones.size(); b++)
			{
				//---------------------------
				//キャッシュからノードを取得
				//---------------------------

				//アニメーションが存在するかを確認
				auto it = node_cache.find(bones[b].name);
				if (it != node_cache.end())
				{
					FbxNode* node = it->second;

					//---------------------
					//グローバル行列の取得
					//---------------------
					FbxAMatrix global_transform = node->EvaluateGlobalTransform(t);
					keyframe_nodes[b].global_transform = ToXMFloat4x4(global_transform);

					keyframe_nodes[b].scaling = ToXMFloat3(global_transform.GetS());
					keyframe_nodes[b].rotation = ToXMFloat4(global_transform.GetQ());
					keyframe_nodes[b].translation = ToXMFloat3(global_transform.GetT());
				}
			}
			clip.sequence.push_back(std::move(keyframe_nodes));
		}
		out_animatins[clip.name] = std::move(clip);
	}
}
