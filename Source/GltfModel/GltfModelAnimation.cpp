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
	if (!model_data) return;	//モデルデータが有効か確認
	//--------------------------------------------------
	// ルートノードからの巡回処理
	//--------------------------------------------------
	std::stack<DirectX::XMFLOAT4X4> parent_global_transforms;							//親ノードの行列情報を順次保持するためのスタックを作成
	DirectX::XMFLOAT4X4 identity_matrix;												//処理の起点となる単位行列用の変数を宣言
	DirectX::XMStoreFloat4x4(&identity_matrix, DirectX::XMMatrixIdentity());			//DirectXの関数を利用して変数に単位行列を格納

	for (int node_index : model_data->scenes.at(model_data->default_scene).nodes)		//現在のデフォルトシーンに登録されている全てのルートノードをループ
	{
		parent_global_transforms.push(identity_matrix);									//一番根っこの親として単位行列をスタックに追加
		TraverseNodeForTransform(node_index, parent_global_transforms);					//ルートノードのインデックスを渡し階層の巡回処理を開始
		parent_global_transforms.pop();													//全探索が終了した後にスタックの単位行列を取り除きクリア
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

	using namespace DirectX;															// DirectXMathの名前空間を使用
	const size_t INDEX_OFFSET_NEXT = 1;													// 次の要素を参照するためのインデックスオフセット定数
	const GltfModelData::animation& animation = model_data->animations.at(animation_index);	// 引数で指定されたアニメーションデータを参照として取得

	//--------------------------------------------------
	// チャンネル（操作対象）ごとのアニメーション適用
	//--------------------------------------------------
	for (const GltfModelData::animation::channel& channel : animation.channels)                   // アニメーションが持つ全チャンネル（誰のどの部位を動かすか）をループ
	{
		const GltfModelData::animation::sampler& sampler = animation.samplers.at(channel.sampler);// チャンネルに対応するサンプラー（時間と値の紐付け情報）を取得
		const std::vector<float>& timeline = animation.timelines.at(sampler.input);               // サンプラーからタイムライン（時間軸）の配列データを取得

		if (timeline.empty())                                                                     // タイムラインのデータが空の場合
		{
			continue;                                                                             // 処理ができないためスキップし次のチャンネルへ進む
		}

		float interpolation_factor = 0.0f;                                                        // 関数から受け取るための補間係数変数を初期化
		size_t keyframe_index = GetAnimationKeyframeIndex(timeline, time, interpolation_factor);  // 現在の時間に対応するキーフレーム番号と補間割合を取得

		//--------------------------------------------------
		// 対象パラメータに応じた数式（補間処理）の実行
		//--------------------------------------------------
		if (channel.target_path == "scale")                                                       // 更新対象のパラメータがスケール（拡縮）の場合
		{
			const std::vector<XMFLOAT3>& scales = animation.scales.at(sampler.output);            // サンプラーからスケール値の配列を取得
			XMVECTOR scale_start = XMLoadFloat3(&scales.at(keyframe_index));                      // 現在のキーフレームのスケールをSIMDレジスタにロード
			XMVECTOR scale_end = XMLoadFloat3(&scales.at(keyframe_index + INDEX_OFFSET_NEXT));    // 次のキーフレームのスケールをSIMDレジスタにロード
			XMVECTOR lerped_scale = XMVectorLerp(scale_start, scale_end, interpolation_factor);   // 線形補間(Lerp)を用いて開始と終了の中間スケールを高速計算
			XMStoreFloat3(&animated_nodes.at(channel.target_node).scale, lerped_scale);           // 計算結果を対象ノードのスケール変数に直接保存
		}
		else if (channel.target_path == "rotation")                                               // 更新対象のパラメータが回転の場合
		{
			const std::vector<XMFLOAT4>& rotations = animation.rotations.at(sampler.output);      // サンプラーから回転値（クォータニオン）の配列を取得
			XMVECTOR rot_start = XMLoadFloat4(&rotations.at(keyframe_index));                     // 現在のキーフレームの回転情報をロード
			XMVECTOR rot_end = XMLoadFloat4(&rotations.at(keyframe_index + INDEX_OFFSET_NEXT));   // 次のキーフレームの回転情報をロード
			XMVECTOR slerped_rot = XMQuaternionSlerp(rot_start, rot_end, interpolation_factor);   // 球面線形補間(Slerp)を用いて歪みのない滑らかな中間回転を計算
			XMVECTOR normalized_rot = XMQuaternionNormalize(slerped_rot);                         // 丸め誤差によるクォータニオンの崩れを防ぐため正規化（長さを1にする）を実行
			XMStoreFloat4(&animated_nodes.at(channel.target_node).rotation, normalized_rot);      // 計算結果を対象ノードの回転変数に直接保存
		}
		else if (channel.target_path == "translation")                                            // 更新対象のパラメータが位置（座標移動）の場合
		{
			const std::vector<XMFLOAT3>& translations = animation.translations.at(sampler.output);		// サンプラーから位置座標の配列を取得
			XMVECTOR trans_start = XMLoadFloat3(&translations.at(keyframe_index));						// 現在のキーフレームの位置座標をロード
			XMVECTOR trans_end = XMLoadFloat3(&translations.at(keyframe_index + INDEX_OFFSET_NEXT));	// 次のキーフレームの位置座標をロード
			XMVECTOR lerped_trans = XMVectorLerp(trans_start, trans_end, interpolation_factor);			// 線形補間(Lerp)を用いて開始と終了の中間座標を計算
			XMStoreFloat3(&animated_nodes.at(channel.target_node).translation, lerped_trans);			// 計算結果を対象ノードの位置座標変数に直接保存
		}
	}

	//--------------------------------------------------
	// 更新されたノードのグローバル行列を再計算
	//--------------------------------------------------
	CumulateTransforms();
}

//===========================
//行列計算用の再帰呼び出し
//===========================
void GltfModelAnimation::TraverseNodeForTransform(int node_index, std::stack<DirectX::XMFLOAT4X4>& parent_global_transforms)
{
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
		parent_global_transforms.push(current_node.global_transform);			// 自身のグローバル行列を「親の行列」としてスタックの最上部に積む
		TraverseNodeForTransform(child_index, parent_global_transforms);		// 子ノードのインデックスを渡し自身を再帰呼び出し
		parent_global_transforms.pop();											// 子ノードの処理が終わったらスタックから自身の行列を取り除く
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
	float max_time = 0.0f;	//記録用の最大時間変数
	const GltfModelData::animation& animation = model_data->animations.at(animation_index);	//指定されたアニメーションデータを取得
	for (const GltfModelData::animation::channel& channel : animation.channels)			//アニメーションが持つ全チャンネルをループ
	{
		const GltfModelData::animation::sampler& sampler = animation.samplers.at(channel.sampler);	//サンプラーを取得
		const std::vector<float>& timeline = animation.timelines.at(sampler.input);					//サンプラーからの時間軸の配列を取得
		if (!timeline.empty())	//タイムラインにデータが存在する場合
		{
			max_time = std::max<float>(max_time, timeline.back());	//これまでの最大時間と、現在のタイムラインの最後の時間を比較し、大きい方を記録
		}
	}
	return max_time;	//アニメーション終了時間を返す
}