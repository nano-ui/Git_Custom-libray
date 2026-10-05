#pragma once

#include <cstdint>

//ノードエディタのカメラ・ビューポート制御クラス
class StateGraphCameraController
{
public:
	//コンストラクタ
	StateGraphCameraController();

	//デストラクタ
	~StateGraphCameraController() = default;

	//特定のノードへのフォーカス移動を要求
	void RequestFocusNode(
		uint32_t node_id	//フォーカス対象のノードID
	);

	//カメラのフォーカス更新処理
	void UpdateCameraFocus();

	//ズーム補正の有効・無効設定
	void SetZoomCorrectionEnabled(bool is_enable) { is_zoom_correction_enabled = is_enable; }

	//フォーカス補完アニメーション時間の設定
	void SetFocusDurationTime(float duration) { focus_duration_time = duration; }

private:
	//指定ノードが現在の画面内に収まっているか判定
	bool IsNodeInScreen(
		uint32_t node_id	//判定対象ノードのID
	);

private:
	uint32_t pending_focus_node_id = 0;			//フォーカル移動を待機しているノードID
	float focus_margin = 50.0f;					//画面内判定に用いるマージン余白
	float focus_duration_time = 0.0f;			//カメラ補完移動のアニメーション時間
	bool is_zoom_correction_enabled = false;	//カメラ移動時のズーム倍率自動補正フラグ
};

