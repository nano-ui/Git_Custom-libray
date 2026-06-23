#include "FbxAnimation.h"

#include <fbxsdk.h>

//==============
// 変換ヘルパー
//==============
namespace {
	DirectX::XMFLOAT4X4 ToXMFloat4x4(const FbxAMatrix& src) {
		DirectX::XMFLOAT4X4 dest;
		for (int r = 0; r < 4; r++)
		{
			for (int c = 0; c < 4; c++)
			{
				dest.m[r][c] = (float)src.Get(r, c);
			}
		}

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
	std::unordered_map<std::string, AnimationClip>& out_animations,
	float sampling_rate)
{
	//出力用マップをクリア
	out_animations.clear();

	//シーンに含まれるアニメーションスタックの数を取得
	int stack_count = scene->GetSrcObjectCount<FbxAnimStack>();

	//全スタックについてループ
	for (int i = 0; i < stack_count; i++)
	{
		//アニメーションスタックを取得
		FbxAnimStack* stack = scene->GetSrcObject<FbxAnimStack>(i);

		//現在のスタックをシーンに適用
		scene->SetCurrentAnimationStack(stack);

		//クリップデータを作成
		AnimationClip clip;

		//スタック名をクリップ名にする
		clip.name = stack->GetName();

		//アニメーションの時間範囲(開始・終了)を取得
		FbxTakeInfo* take_info = scene->GetTakeInfo(clip.name.c_str());

		//情報があれば取得、無ければ0とする
		FbxTime start = take_info ? take_info->mLocalTimeSpan.GetStart() : FbxTime(0);
		FbxTime end = take_info ? take_info->mLocalTimeSpan.GetStop() : FbxTime(0);

		//サンプリングレートを決定(指定が無ければ24fps)
		float rate = sampling_rate > 0 ? sampling_rate : 24.0f;
		clip.sampling_rate = rate;

		//1フレーム当たりの時間を計算
		FbxTime step;
		FbxTime one_second;

		//1秒を表すFbxTimeを設定
		one_second.SetTime(0, 0, 1, 0, 0, scene->GetGlobalSettings().GetTimeMode());

		//1秒をレートで割ってステップ時間を算出
		step = one_second / static_cast<double>(rate);

		//ボーン名からノードを検索するためのキャッシュを作成
		std::unordered_map<std::string, FbxNode*> node_cache;
		int node_count = scene->GetSrcObjectCount<FbxNode>();
		for (int i = 0; i < node_count; i++)
		{
			FbxNode* node = scene->GetSrcObject<FbxNode>(i);
			node_cache[node->GetName()] = node;
		}

		//開始時間から終了時間までステップごとにループ
		for (FbxTime t = start; t < end; t += step)
		{
			//全ボーンの姿勢リストを作成
			std::vector<AnimationKeyframeNode> keyframe_nodes;

			//ボーンの数に合わせてリサイズ
			keyframe_nodes.resize(bones.size());

			//全ボーンについて処理
			for (size_t b = 0; b < bones.size(); b++)
			{
				//ボーン名に対応するFBXノードをキャッシュから探す
				auto it = node_cache.find(bones[b].name);
				if (it != node_cache.end())
				{
					FbxNode* node = it->second;

					//時間におけるグローバル行列を計算
					FbxAMatrix global_transform = node->EvaluateGlobalTransform(t);
					FbxAMatrix local = global_transform;

					if (bones[b].parent_index >= 0)
					{
						FbxNode* parent_node = node_cache[bones[bones[b].parent_index].name];
						FbxAMatrix parent_global = parent_node->EvaluateGlobalTransform(t);

						local = parent_global.Inverse() * global_transform;

					}

					//行列、スケール、回転、位置を抽出して変換
					keyframe_nodes[b].global_transform = ToXMFloat4x4(local);
					keyframe_nodes[b].scaling = ToXMFloat3(local.GetS());
					keyframe_nodes[b].rotation = ToXMFloat4(local.GetQ());
					keyframe_nodes[b].translation = ToXMFloat3(local.GetT());
				}
			}

			//1フレーム分のデータをシーケンスに追加
			clip.sequence.push_back(std::move(keyframe_nodes));
		}

		//解析したクリップをマップに登録
		out_animations[clip.name] = std::move(clip);
	}
}
