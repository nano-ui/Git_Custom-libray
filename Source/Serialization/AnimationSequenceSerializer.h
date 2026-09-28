#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include "Serialization\JsonSerializer.h"

//速度キーフレーム
struct SequenceKeyframe
{
	float sequencer_time;	//シーケンサ上の経過時間
	float speed_multiplier;	//速度倍率
};

struct SequenceColliderTrack
{
	std::string collider_name = "";	//コライダー名
	float start_time = 0.0f;		//開始時間
	float end_time = 0.0f;			//終了時間

	//JsonSerializerへの登録処理
	void SetupSerialization(JsonSerializer* serializer)
	{
		if (!serializer)return;
		serializer->RegisterVariable(u8"コライダー名", &collider_name);
		serializer->RegisterVariable(u8"開始時間", &start_time);
		serializer->RegisterVariable(u8"終了時間", &end_time);
	}
};

//1つのアニメーションのシーケンスデータ
struct AnimationSequenceData
{
	float animation_duration = 0.0f;	//元のアニメーション総時間
	float effective_duration = 0.0f;	//速度カーブ適用後の実行総時間
	std::vector<SequenceKeyframe> keyframes;			//速度キーフレーム配列
	std::vector<SequenceColliderTrack> collider_tracks;	//コライダー有効区間リスト
};

class AnimationSequenceSerializer
{
public:
	//指定モデルの全アニメーション設定をJSONファイルに一括保存
	static bool SaveToFile(const std::string& model_name,
		const std::unordered_map<std::string, AnimationSequenceData>& sequence_map);

	//指定モデルのJSONファイルから全アニメーション設定を一括読み込み
	static bool LoadFromFile(
		const std::string& model_name,
		std::unordered_map<std::string, AnimationSequenceData>& out_sequence_map);

	//保存・読み込み用のフルJSONファイルパスを取得
	static std::string GetFullFilePath(const std::string& clean_model_name);

private:
	//パスや拡張子から純粋なモデル名を抽出
	static std::string ExtractCleanModelName(const std::string& raw_model_name);

	//ディレクトリパスを取得し、存在しない場合は自動生成
	static std::string GetDirectoryPath(const std::string& clean_model_name);
};

