#pragma once

#include <windows.h>
#include <Xinput.h>

#pragma comment(lib, "xinput.lib")

class Input
{
public:
	//シングルトンの取得
	static Input& Instance();

	//初期化処理
	void Initialize();

	//更新処理
	void Update();

	//キーボードが押されているか判定
	bool IsKeyPress(int key_code)const;

	//キーボードが押された瞬間か判定
	bool IsKeyTrigger(int key_code)const;

	//キーボードが離された瞬間か判定
	bool IsKeyRelease(int key_code)const;

	//ボタンが押されているか判定
	bool IsMousePress(int button)const;

	//ボタンが押された瞬間か判定
	bool IsMouseTrigger(int button)const;

	//ボタンが離された瞬間
	bool IsMouseRelease(int button)const;

	//ホイールが奥へ回転したかを判定
	bool IsMouseWheelUp()const;

	//ホイールが手前へ回転したかを判定
	bool IsMouseWheelDown()const;

	//コントローラーのボタンが押されているか判定
	bool IsButtonPress(WORD button)const;

	//コントローラーのボタンが押された瞬間か判定
	bool IsButtonTrigger(WORD button)const;

	//コントローラーのボタンが離された瞬間か判定
	bool IsButtonRelease(WORD button)const;

	//コントローラーの左スティックのX軸入力を取得
	float GetLeftStickX()const;

	//コントローラーの左スティックのY軸入力を取得
	float GetLeftSticeY()const;

	//全てのキートリガーラッチ状態をクリア
	void ClearKeyTriggers();

	//マウスのX方向の移動量を取得
	float GetMouseDeltaX()const;

	//マウスのY方向の移動量を取得
	float GetMouseDeltaY()const;

	//ホイール移動量取得
	int GetMouseWheelDelta()const;

	//Windowsメッセージ処理
	bool ProcessMessage(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

	//フレームの終わりに呼び出し、更新ロックを解除する
	void EndFrame();

private:
	//コンストラクタ
	Input();

	//デストラクタ
	~Input();

	//コピー生成と代入を禁止
	Input(const Input&) = delete;
	Input& operator = (const Input&) = delete;

public:
	static constexpr int mouse_left = 0;		//左クリック
	static constexpr int mouse_right = 1;		//右クリック
	static constexpr int mouse_middle = 2;		//マウスホイール

private:
	static constexpr int max_keys = 256;		//キーボードの最大キー数
	BYTE current_key_state[max_keys];			//現在の全キー状態を保存する配列
	BYTE prev_key_state[max_keys];				//前回の全キー状態を保存する配列
	mutable bool latched_key_trigger[max_keys]; //トリガー判定の取りこぼしを防ぐ保持用配列
	XINPUT_STATE current_pad_state;				//現在のコントローラーの状態を保存
	XINPUT_STATE prev_pad_state;				//前回のコントローラーの状態を保存
	bool is_pad_connected;						//コントローラーの接続フラグ
	POINT current_mouse_pos;					//現在のマウス座標
	POINT prev_mouse_pos;						//前回のマウス座標

	float mouse_delta_x = 0.0f;					//確定した1フレーム間のマウスX移動量
	float mouse_delta_y = 0.0f;					//確定した1フレーム間のマウスY移動量
	float raw_mouse_delta_x = 0.0f;				//メッセージハンドラで累積蓄積するマウスX移動量
	float raw_mouse_delta_y = 0.0f;				//メッセージハンドラで累積蓄積するマウスY移動量

	static constexpr int mouse_button_count = 3;			//判定対象とするマウスボタン数
	bool current_mouse_button_state[mouse_button_count];	//各ボタンの押下状態
	bool prev_mouse_button_state[mouse_button_count];		//前回の各ボタンの押下状態
	mutable bool latched_mouse_trigger[mouse_button_count];	//メッセージ受信時に押下瞬間

	int mouse_wheel_delta = 0;								//ホイールの回転量
	int raw_mouse_wheel_delta = 0;							//ホイールの一時蓄積

	bool is_updated_this_frame = false;
};

