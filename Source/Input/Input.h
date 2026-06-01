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

	//マウスのX方向の移動量を取得
	float GetMouseDeltaX()const;

	//マウスのY方向の移動量を取得
	float GetMouseDeltaY()const;

private:
	//コンストラクタ
	Input();

	//デストラクタ
	~Input();

	//コピー生成と代入を禁止
	Input(const Input&) = delete;
	Input& operator = (const Input&) = delete;

private:
	static constexpr int max_keys = 256;	//キーボードの最大キー数
	BYTE current_key_state[max_keys];		//現在の全キー状態を保存する配列
	BYTE prev_key_state[max_keys];			//前回の全キー状態を保存する配列
	XINPUT_STATE current_pad_state;			//現在のコントローラーの状態を保存
	XINPUT_STATE prev_pad_state;			//前回のコントローラーの状態を保存
	bool is_pad_connected;					//コントローラーの接続フラグ
	POINT current_mouse_pos;				//現在のマウス座標
	POINT prev_mouse_pos;					//前回のマウス座標
};

