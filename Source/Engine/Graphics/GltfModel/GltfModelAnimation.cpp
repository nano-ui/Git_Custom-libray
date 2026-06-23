#include "GltfModelAnimation.h"

#include <algorithm>


//============================================
//アニメーションクラスをモデルデータで初期化
//============================================
void GltfModelAnimation::Initialize(const std::shared_ptr<const GltfModelData>& data)
{
	//----------------------------------------
	//データの保持と作業用ノード配列の初期化
	//----------------------------------------
	model_data = data;	//モデルデータを保存
	if (model_data)		//モデルデータが有効か確認
	{
		animated_nodes = model_data->nodes;	//初期状態のノード構造をアニメーション用の可変配列にコピー
		CumulateTransforms();				//初期姿勢のグローバル行列を一度計算して確定
	}
}

//====================================================
//親子関係をたどって各ノードのグローバル行列を計算
//====================================================
void GltfModelAnimation::CumulateTransforms()
{
	//モデルデータの有効性チェック
	if (!model_data) return;

	std::vector<int> root_nodes;

	//デフォルトシーン番号が配列の範囲内にあるか存在確認
	if (model_data->default_scene >= 0 && static_cast<size_t>(model_data->default_scene) < model_data->scenes.size())
	{
		root_nodes = model_data->scenes.at(model_data->default_scene).nodes;
	}
	else
	{
		//OutputDebugStringA("[GltfModelAnimation Error] model_data->default_scene is OUT OF RANGE!\n");

		if (!model_data->scenes.empty())
		{
			//OutputDebugStringA("[GltfModelAnimation Warning] Safety fallback: Using the first scene (0) instead.\n");
			root_nodes = model_data->scenes.at(0).nodes;
		}
		else
		{
			//OutputDebugStringA("[GltfModelAnimation Warning] Safety fallback: Scenes container is completely empty!\n");
		}
	}

	//--------------------------------------------------
	// ルートノードからの巡回処理
	//--------------------------------------------------
	std::stack<DirectX::XMFLOAT4X4> parent_global_transforms;							//親ノードの行列情報を順次保持するためのスタックを作成
	DirectX::XMFLOAT4X4 identity_matrix;												//処理の起点となる単位行列用の変数を宣言
	DirectX::XMStoreFloat4x4(&identity_matrix, DirectX::XMMatrixIdentity());			//DirectXの関数を利用して変数に単位行列を格納

	for (int node_index : root_nodes)		//現在のデフォルトシーンに登録されている全てのルートノードをループ
	{
		if (node_index >= 0 && static_cast<size_t>(node_index) < animated_nodes.size())
		{
			parent_global_transforms.push(identity_matrix);									//一番根っこの親として単位行列をスタックに追加
			TraverseNodeForTransform(node_index, parent_global_transforms);					//ルートノードのインデックスを渡し階層の巡回処理を開始
			parent_global_transforms.pop();													//全探索が終了した後にスタックの単位行列を取り除きクリア
		}
	}
}

//===========================================
//名前を指定してアニメーションの再生を開始
//===========================================
void GltfModelAnimation::PlayAniamtion(const std::string& animation_name, bool is_loop)
{
	if (!model_data) return;	//モデルデータが有効か確認
	//-------------------------------------
	//マップからアニメーション番号の検索
	//--------------------------------------
	auto iterator = model_data->animation_index_map.find(animation_name);	//指定されたアニメーションを検索
	if (iterator == model_data->animation_index_map.end())					//名前が見つからない場合
	{
		return;	//処理を終了
	}

	//--------------------
	//再生状態の初期化
	//--------------------
	current_animation_index = iterator->second;	//検索結果からインデックス番号を取得
	is_loop_enabled = is_loop;					//ループフラグを設定
	current_animation_time = 0.0f;				//再生時間をリセット
	current_animation_duration = CalculateAnimationDuration(current_animation_index);	//アニメーション終了時間を取得
	is_playing = true;							//再生フラグを起動
}

//============================
//アニメーション更新処理
//============================
void GltfModelAnimation::UpdateAnimation(float delta_time)
{
	if (!model_data || !is_playing || model_data->animations.empty())return;

	//------------------------
	//時間の進行とループ判定
	//------------------------
	current_animation_time += delta_time;						//経過時間を加算してアニメーションを進める
	if (current_animation_time > current_animation_duration)	//現在の時間がアニメーションの終了時間を超過した場合
	{
		if (is_loop_enabled)	//ループ再生が有効な場合
		{
			current_animation_time = fmod(current_animation_time, current_animation_duration);	//超過した余りの時間を計算し、最初からループ
		}
		else
		{
			current_animation_time = current_animation_duration;	//終了時間でピッタリ止めるため数値を固定
			is_playing = false;										//再生完了としてフラグをオフに設定
		}
	}
	Animate(current_animation_index, current_animation_time);	//実際の計算処理
}

//========================================================
//指定した時間のアニメーションを適用しノード情報を更新
//========================================================
void GltfModelAnimation::Animate(size_t animation_index, float time)
{
	//--------------------------------------------------
	// アニメーションデータの有無の確認
	//--------------------------------------------------
	if (model_data->animations.empty())	// GltfModelData内にアニメーションデータが存在しない場合
	{
		return;						// 更新する対象がないため何も処理せずに安全に終了
	}

	if (animation_index >= model_data->animations.size())
	{
		OutputDebugStringA("[GltfModelAnimation Error] animation_index out of range!/n");
	}

	using namespace DirectX;															// DirectXMathの名前空間を使用
	const size_t INDEX_OFFSET_NEXT = 1;													// 次の要素を参照するためのインデックスオフセット定数
	const GltfModelData::animation& animation = model_data->animations.at(animation_index);	// 引数で指定されたアニメーションデータを参照として取得

	//--------------------------------------------------
	// チャンネル（操作対象）ごとのアニメーション適用
	//--------------------------------------------------
	for (const GltfModelData::animation::channel& channel : animation.channels)                   // アニメーションが持つ全チャンネル（誰のどの部位を動かすか）をループ
	{
		//サンプラーインデックスの安全な存在確認
		if (channel.sampler < 0 || static_cast<size_t>(channel.sampler) >= animation.samplers.size())
		{
			OutputDebugStringA("[GltfModelAnimation Error] Animate: channel.sampler is out of range!\n");
			continue;
		}

		const GltfModelData::animation::sampler& sampler = animation.samplers.at(channel.sampler);// チャンネルに対応するサンプラー（時間と値の紐付け情報）を取得
		
		//タイムラインマップのキーの安全な存在確
		auto timeline_it = animation.timelines.find(sampler.input);
		if (timeline_it == animation.timelines.end())
		{
			OutputDebugStringA("[GltfModelAnimation Error] Animate: sampler.input key not found in timelines map!\n");
			continue;
		}
		const std::vector<float>& timeline = timeline_it->second;

		if (timeline.empty())                                                                     // タイムラインのデータが空の場合
		{
			continue;                                                                             // 処理ができないためスキップし次のチャンネルへ進む
		}

		float interpolation_factor = 0.0f;                                                        // 関数から受け取るための補間係数変数を初期化
		size_t keyframe_index = GetAnimationKeyframeIndex(timeline, time, interpolation_factor);  // 現在の時間に対応するキーフレーム番号と補間割合を取得

		//--------------------------------------------------
		// 対象パラメータに応じた数式（補間処理）の実行
		//--------------------------------------------------
		if (channel.target_path == "scale")                                                      
		{
			auto scale_it = animation.scales.find(sampler.output);
			if (scale_it == animation.scales.end() || (keyframe_index + INDEX_OFFSET_NEXT) >= scale_it->second.size())
			{
				OutputDebugStringA("[GltfModelAnimation Error] Animate: scales key or keyframe index error!\n");
				continue;
			}
			const std::vector<XMFLOAT3>& scales = scale_it->second;								
			XMVECTOR scale_start = XMLoadFloat3(&scales.at(keyframe_index));                    
			XMVECTOR scale_end = XMLoadFloat3(&scales.at(keyframe_index + INDEX_OFFSET_NEXT));  
			XMVECTOR lerped_scale = XMVectorLerp(scale_start, scale_end, interpolation_factor); 

			if (static_cast<size_t>(channel.target_node) < animated_nodes.size())
			{
				XMStoreFloat3(&animated_nodes.at(channel.target_node).scale, lerped_scale);           
			}
		}
		else if (channel.target_path == "rotation")                                               
		{
			auto rot_it = animation.rotations.find(sampler.output);
			if (rot_it == animation.rotations.end() || (keyframe_index + INDEX_OFFSET_NEXT) >= rot_it->second.size())
			{
				OutputDebugStringA("[GltfModelAnimation Error] Animate: rotations key or keyframe index error!\n");
				continue;
			}
			const std::vector<XMFLOAT4>& rotations = rot_it->second;      
			XMVECTOR rot_start = XMLoadFloat4(&rotations.at(keyframe_index));                     
			XMVECTOR rot_end = XMLoadFloat4(&rotations.at(keyframe_index + INDEX_OFFSET_NEXT));   
			XMVECTOR slerped_rot = XMQuaternionSlerp(rot_start, rot_end, interpolation_factor);   
			XMVECTOR normalized_rot = XMQuaternionNormalize(slerped_rot);                         

			if (static_cast<size_t>(channel.target_node) < animated_nodes.size())
			{
				XMStoreFloat4(&animated_nodes.at(channel.target_node).rotation, normalized_rot);     
			}
		}
		else if (channel.target_path == "translation")                                           
		{
			auto trans_it = animation.translations.find(sampler.output);
			if (trans_it == animation.translations.end() || (keyframe_index + INDEX_OFFSET_NEXT) >= trans_it->second.size())
			{
				OutputDebugStringA("[GltfModelAnimation Error] Animate: translations key or keyframe index error!\n");
				continue;
			}
			const std::vector<XMFLOAT3>& translations = trans_it->second;		
			XMVECTOR trans_start = XMLoadFloat3(&translations.at(keyframe_index));						
			XMVECTOR trans_end = XMLoadFloat3(&translations.at(keyframe_index + INDEX_OFFSET_NEXT));	
			XMVECTOR lerped_trans = XMVectorLerp(trans_start, trans_end, interpolation_factor);			

			if (static_cast<size_t>(channel.target_node) < animated_nodes.size())
			{
				XMStoreFloat3(&animated_nodes.at(channel.target_node).translation, lerped_trans);			
			}
		}
	}

	// 更新されたノードのグローバル行列を再計算
	CumulateTransforms();
}

//===========================
//行列計算用の再帰呼び出し
//===========================
void GltfModelAnimation::TraverseNodeForTransform(int node_index, std::stack<DirectX::XMFLOAT4X4>& parent_global_transforms)
{
	if (node_index < 0 || static_cast<size_t>(node_index) >= animated_nodes.size())
	{
		return;
	}

	//--------------------------------------------------
	// ローカル変換行列の構築とグローバル行列の計算
	//--------------------------------------------------
	using namespace DirectX;                                                                                                    // DirectXMathの名前空間を使用し記述を省略
	GltfModelData::node& current_node = animated_nodes.at(node_index);                                                                   // 対象のノードを参照で取得し更新可能にする
	XMMATRIX scale_matrix = XMMatrixScaling(current_node.scale.x, current_node.scale.y, current_node.scale.z);                  // XYZのスケール値からスケール行列を作成
	XMMATRIX rotation_matrix = XMMatrixRotationQuaternion(XMVectorSet(current_node.rotation.x, current_node.rotation.y, current_node.rotation.z, current_node.rotation.w)); // クォータニオン情報から回転行列を作成
	XMMATRIX translation_matrix = XMMatrixTranslation(current_node.translation.x, current_node.translation.y, current_node.translation.z); // XYZの位置座標から移動行列を作成

	XMMATRIX local_matrix = scale_matrix * rotation_matrix * translation_matrix;                                                // スケール、回転、移動を掛け合わせて自身のローカル行列を作成
	XMMATRIX global_matrix = local_matrix * XMLoadFloat4x4(&parent_global_transforms.top());                                    // スタック最上部にある親の行列を掛けて自身のグローバル行列を算出
	XMStoreFloat4x4(&current_node.global_transform, global_matrix);                                                             // 算出したグローバル行列をノード構造体に保存

	//--------------------------------------------------
	// 子ノードに対する再帰処理
	//--------------------------------------------------
	for (int child_index : current_node.children)								// ノードが持つ全ての子ノードに対してループ処理
	{
		if (child_index >= 0 && static_cast<size_t>(child_index) < animated_nodes.size())
		{
			parent_global_transforms.push(current_node.global_transform);			// 自身のグローバル行列を「親の行列」としてスタックの最上部に積む
			TraverseNodeForTransform(child_index, parent_global_transforms);		// 子ノードのインデックスを渡し自身を再帰呼び出し
			parent_global_transforms.pop();											// 子ノードの処理が終わったらスタックから自身の行列を取り除く
		}
	}
}

//========================================================
//アニメーションのキーフレームを取得し補間係数を計算
//========================================================
size_t GltfModelAnimation::GetAnimationKeyframeIndex(const std::vector<float>& timelines, float time, float& interpolation_factor)
{
	//--------------------------------------------------
	// 定数の定義
	//--------------------------------------------------
	const size_t keyframe_count = timelines.size();		// タイムラインに存在するキーフレームの総数を取得
	const size_t INDEX_OFFSET_NEXT = 1;					// 次の要素やインデックス計算用のオフセット定数
	const size_t INDEX_OFFSET_PREV = 2;					// 末尾から2番目を取得するためのオフセット定数
	const float INTERPOLATION_MAX = 1.0f;				// 補間係数の最大値（100%）
	const float INTERPOLATION_MIN = 0.0f;				// 補間係数の最小値（0%）
	const size_t START_INDEX = 0;						// 配列の開始インデックス

	//--------------------------------------------------
	// 時間外の境界値判定
	//--------------------------------------------------
	if (time > timelines.at(keyframe_count - INDEX_OFFSET_NEXT))	// 指定された時間が最後のキーフレーム時間を超過している場合
	{
		interpolation_factor = INTERPOLATION_MAX;					// 補間係数を最大値に固定してアニメーションを末尾で止める
		return keyframe_count - INDEX_OFFSET_PREV;					// 最後の補間区間の開始インデックスを返す
	}
	else if (time < timelines.at(START_INDEX))						// 指定された時間が最初のキーフレーム時間より前の場合
	{
		interpolation_factor = INTERPOLATION_MIN;					// 補間係数を最小値に固定して初期ポーズを維持
		return START_INDEX;											// 一番最初のインデックスを返す
	}

	//--------------------------------------------------
	//適切なキーフレームの探索と補間係数の計算
	//--------------------------------------------------
	size_t keyframe_index = START_INDEX;																		// 該当するキーフレームのインデックスを初期化
	for (size_t time_index = INDEX_OFFSET_NEXT; time_index < keyframe_count; time_index++)						// 2番目のキーフレームから最後まで順番にチェック
	{
		if (time < timelines.at(time_index))																	// 現在の時間がチェック中のキーフレームの時間を下回った場合
		{
			keyframe_index = std::max<size_t>(START_INDEX, time_index - INDEX_OFFSET_NEXT);						// 配列外アクセスを防ぎつつ1つ前のインデックスを現在のキーフレームとして設定
			break;																								// 目的の区間が見つかったのでループを強制終了
		}
	}

	float time_difference = timelines.at(keyframe_index + INDEX_OFFSET_NEXT) - timelines.at(keyframe_index);	// 次のキーフレームまでの全体時間を計算
	time_difference = std::max<float>(time_difference, 0.00001f);												// 差分が0になるのを防ぐためのセーフガード
	interpolation_factor = (time - timelines.at(keyframe_index)) / time_difference;								// 現在の時間がその区間でどの位置にあるかの割合を計算

	return keyframe_index;
}

//====================================
//アニメーションの全体の長さを計算
//====================================
float GltfModelAnimation::CalculateAnimationDuration(size_t animation_index)
{
	if (!model_data || model_data->animations.empty() || animation_index >= model_data->animations.size())
	{
		OutputDebugStringA("[GltfModelAnimation Error] CalculateAnimationDuration: index out of range!\n");
		return 0.0f;
	}

	float max_time = 0.0f;	//記録用の最大時間変数
	const GltfModelData::animation& animation = model_data->animations.at(animation_index);	//指定されたアニメーションデータを取得
	for (const GltfModelData::animation::channel& channel : animation.channels)			//アニメーションが持つ全チャンネルをループ
	{
		//サンプラーインデックスの安全確認
		if (channel.sampler < 0 || static_cast<size_t>(channel.sampler) >= animation.samplers.size())
		{
			OutputDebugStringA("[GltfModelAnimation Error] CalculateAnimationDuration : channel.sampler is out of range!\n");
			continue;
		}

		const GltfModelData::animation::sampler& sampler = animation.samplers.at(channel.sampler);	//サンプラーを取得
		
		//タイムラインマップのキーの安全な存在確認
		auto timeline_it = animation.timelines.find(sampler.input);
		if (timeline_it == animation.timelines.end())
		{
			OutputDebugStringA("[GltfModelAnimation Error] CalculateAnimationDuration: sampler.input key not found in timelines map!\n");
			continue;
		}
		
		const std::vector<float>& timeline = timeline_it->second;					//サンプラーからの時間軸の配列を取得
		if (!timeline.empty())	//タイムラインにデータが存在する場合
		{
			max_time = std::max<float>(max_time, timeline.back());	//これまでの最大時間と、現在のタイムラインの最後の時間を比較し、大きい方を記録
		}
	}
	return max_time;	//アニメーション終了時間を返す
}