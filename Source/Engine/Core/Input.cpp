#include "Input.h"
#include "Log\Log.h"

#include <cmath>

//マジックナンバーを排除するための定数
static constexpr int pad_user_index = 0;			//読み取るコントローラーのプレイヤー番号（1P）
static constexpr int key_press_mask = 0x80;			//GetKeyboardStateでキーが押されているかを判定する最上位ビットマスク
static constexpr float stick_max_value = 32767.0f;	//アナログスティックの最大値（short型の最大値）
static constexpr float dead_zone = 0.2f;			//アナログスティックの遊び（デッドゾーン）の割合
static constexpr float max_abnormal_mouse_delta = 1000.0f; //異常なマウス移動量の検知閾値

//シングルトンインスタンスの取得
Input& Input::Instance()
{
	static Input instance;
	return instance;
}

//コンストラクタ
Input::Input()
{
	Initialize();
}

//デストラクタ
Input::~Input()
{

}

//初期化処理
void Input::Initialize()
{
	//キーとパッドの初期化
	ZeroMemory(current_key_state, sizeof(current_key_state));
	ZeroMemory(prev_key_state, sizeof(prev_key_state));
	ZeroMemory(latched_key_trigger, sizeof(latched_key_trigger));

	ZeroMemory(&current_pad_state, sizeof(XINPUT_STATE));
	ZeroMemory(&prev_pad_state, sizeof(XINPUT_STATE));
	is_pad_connected = false;

	//マウス座標の初期化
	current_mouse_pos = { 0,0 };
	GetCursorPos(&current_mouse_pos);
	prev_mouse_pos = current_mouse_pos;

	mouse_delta_x = 0.0f;
	mouse_delta_y = 0.0f;
	raw_mouse_delta_x = 0.0f;
	raw_mouse_delta_y = 0.0f;

	ZeroMemory(current_mouse_button_state, sizeof(current_mouse_button_state));
	ZeroMemory(prev_mouse_button_state, sizeof(prev_mouse_button_state));
	ZeroMemory(latched_mouse_trigger, sizeof(latched_mouse_trigger));

	mouse_wheel_delta = 0;
	raw_mouse_wheel_delta = 0;
}

//更新処理
void Input::Update()
{
	//キーボードの状態更新
	memcpy(prev_key_state, current_key_state, sizeof(current_key_state));
	bool success_keyboard = GetKeyboardState(current_key_state);
	if (!success_keyboard)
	{
		OutputDebugStringA("[Input] エラー: GetKeyboardStateの取得に失敗しました。\n");
		ZeroMemory(current_key_state, sizeof(current_key_state));
	}

	//各キーのトリガー発生をチェックし、ラッチフラグを更新
	for (int key_idx = 0; key_idx < max_keys; key_idx++)
	{
		bool is_current_press = (current_key_state[key_idx] & key_press_mask) != 0;
		bool is_prev_press = (prev_key_state[key_idx] & key_press_mask) != 0;

		//押された瞬間を検知
		if (is_current_press && !is_prev_press)latched_key_trigger[key_idx] = true;
		else if (!is_current_press)latched_key_trigger[key_idx] = false;
	}

	//コントローラーの状態更新
	prev_pad_state = current_pad_state;

	DWORD result = XInputGetState(pad_user_index, &current_pad_state);
	if (result == ERROR_SUCCESS)
	{
		is_pad_connected = true;
	}
	else
	{
		is_pad_connected = false;
		ZeroMemory(&current_pad_state, sizeof(XINPUT_STATE));
	}

	//スクリーン座標の更新とマウス差分の計算
	prev_mouse_pos = current_mouse_pos;
	if (!GetCursorPos(&current_mouse_pos))
	{
		LOG_WARNING({ u8"インプット" }, u8"GetCursorPos の取得に失敗");
	}

	//メッセージハンドラで累積された移動差分を採用
	mouse_delta_x = static_cast<float>(current_mouse_pos.x - prev_mouse_pos.x);
	mouse_delta_y = static_cast<float>(current_mouse_pos.y - prev_mouse_pos.y);

	//次フレーム用に累積変数をクリア
	raw_mouse_delta_x = 0.0f;
	raw_mouse_delta_y = 0.0f;

	//現在のマウスの状態を検知
	for (int btn_idx = 0; btn_idx < mouse_button_count; btn_idx++)
	{
		bool is_current_press = current_mouse_button_state[btn_idx];
		bool is_prev_press = prev_mouse_button_state[btn_idx];

		if (is_current_press && is_prev_press)
		{
			latched_mouse_trigger[btn_idx] = false;
		}
	}

	memcpy(prev_mouse_button_state, current_mouse_button_state, sizeof(current_mouse_button_state));

	mouse_wheel_delta = raw_mouse_wheel_delta;
	raw_mouse_wheel_delta = 0;
}

//キーボードが押されているか判定
bool Input::IsKeyPress(int key_code) const
{
	return (current_key_state[key_code] & key_press_mask) != 0;
}

//キーボードが押された瞬間か判定
bool Input::IsKeyTrigger(int key_code) const
{
	if (key_code < 0 || key_code >= max_keys)
	{
		OutputDebugStringA("[Input] エラー: 範囲外のキーコードがIsKeyTriggerに渡されました。\n");
		return false;
	}

	if (latched_key_trigger[key_code])
	{
		latched_key_trigger[key_code] = false;
		return true;
	}

	bool is_current_press = (current_key_state[key_code] & key_press_mask) != 0;
	bool is_prev_press = (prev_key_state[key_code] & key_press_mask) != 0;
	return  is_current_press && !is_prev_press;
}

//キーボードが離された瞬間か判定
bool Input::IsKeyRelease(int key_code) const
{
	bool is_current_press = (current_key_state[key_code] & key_press_mask) != 0;
	bool is_prev_press = (prev_key_state[key_code] & key_press_mask) != 0;

	return !is_current_press && is_prev_press;
}

//ボタンが押されているか判定
bool Input::IsMousePress(int button) const
{
	if (button < 0 || button >= mouse_button_count)
	{
		OutputDebugStringA("[Input] エラー: 範囲外のマウスボタン番号がIsMousePressに渡されました。\n");
		return false;
	}
	return current_mouse_button_state[button];
}

//ボタンが押された瞬間か判定
bool Input::IsMouseTrigger(int button) const
{
	if (button < 0 || button >= mouse_button_count)
	{
		OutputDebugStringA("[Input] エラー: 範囲外のマウスボタン番号がIsMouseTriggerに渡されました。\n");
		return false;
	}

	if (latched_mouse_trigger[button])
	{
		latched_mouse_trigger[button] = false;
		return true;
	}

	return current_mouse_button_state[button] && !prev_mouse_button_state[button];
}

//ボタンが離された瞬間
bool Input::IsMouseRelease(int button) const
{
	if (button < 0 || button >= mouse_button_count)
	{
		OutputDebugStringA("[Input] エラー: 範囲外のマウスボタン番号がIsMouseReleaseに渡されました。\n");
		return false;
	}
	return !current_mouse_button_state[button] && prev_mouse_button_state[button];
}

//ホイールが奥へ回転したかを判定
bool Input::IsMouseWheelUp()const
{
	return mouse_wheel_delta > 0;
}

//ホイールが手前へ回転したかを判定
bool Input::IsMouseWheelDown()const
{
	return mouse_wheel_delta < 0;
}

//コントローラーのボタンが押されているか判定
bool Input::IsButtonPress(WORD button) const
{
	if (!is_pad_connected)return false;
	return (current_pad_state.Gamepad.wButtons & button) != 0;
}

//コントローラーのボタンが押された瞬間か判定
bool Input::IsButtonTrigger(WORD button) const
{
	if (!is_pad_connected)return false;

	bool is_current_press = (current_pad_state.Gamepad.wButtons & button) != 0;
	bool is_prev_press = (prev_pad_state.Gamepad.wButtons & button) != 0;

	return is_current_press && !is_prev_press;
}

//コントローラーのボタンが離された瞬間か判定
bool Input::IsButtonRelease(WORD button) const
{
	if (!is_pad_connected)return false;

	bool is_current_press = (current_pad_state.Gamepad.wButtons & button) != 0;
	bool is_prev_press = (prev_pad_state.Gamepad.wButtons & button) != 0;

	return !is_current_press && is_prev_press;
}

//コントローラーの左スティックのX軸入力を取得
float Input::GetLeftStickX() const
{
	if (!is_pad_connected)return 0.0f;

	//アナログ値を正規化してデッドゾーンを処理
	float value = static_cast<float>(current_pad_state.Gamepad.sThumbLX) / stick_max_value;
	if (value > -dead_zone && value < dead_zone)
	{
		return 0.0f;
	}
	return value;
}

//コントローラーの左スティックのY軸入力を取得
float Input::GetLeftSticeY() const
{
	if (!is_pad_connected)return 0.0f;

	//アナログ値を正規化してデッドゾーンを処理
	float value = static_cast<float>(current_pad_state.Gamepad.sThumbLY) / stick_max_value;
	if (value > -dead_zone && value < dead_zone)
	{
		return 0.0f;
	}
	return value;
}

//全てのキートリガーラッチ状態をクリア
void Input::ClearKeyTriggers()
{
	ZeroMemory(latched_key_trigger, sizeof(latched_key_trigger));
}

//マウスのX方向の移動量を取得
float Input::GetMouseDeltaX() const
{
	return mouse_delta_x;
}

//マウスのY方向の移動量を取得
float Input::GetMouseDeltaY() const
{
	return mouse_delta_y;
}

//ホイール移動量取得
int Input::GetMouseWheelDelta() const
{
	return mouse_wheel_delta;
}

//Windowsメッセージ処理
bool Input::ProcessMessage(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (msg == WM_MOUSEMOVE)
	{
		//メッセージパラメーターから現在のローカルマウス座標を抽出
		short current_x = static_cast<short>(LOWORD(lparam));
		short current_y = static_cast<short>(HIWORD(lparam));

		static int last_msg_x = current_x;
		static int last_msg_y = current_y;

		float diff_x = static_cast<float>(current_x - last_msg_x);
		float diff_y = static_cast<float>(current_y - last_msg_y);

		//異常に大きな飛び値を検出した場合のデバッグ出力
		if (std::abs(diff_x) > max_abnormal_mouse_delta || std::abs(diff_y) > max_abnormal_mouse_delta)
		{
			OutputDebugStringA("[Input] 警告: WM_MOUSEMOVEで異常なマウス移動差分を検出しました。\n");
		}
		else
		{
			raw_mouse_delta_x += diff_x;
			raw_mouse_delta_y += diff_y;
		}

		last_msg_x = current_x;
		last_msg_y = current_y;
		return true;
	}
	else if (msg == WM_LBUTTONDOWN)
	{
		current_mouse_button_state[mouse_left] = true;
		latched_mouse_trigger[mouse_left] = true;
		return true;
	}
	else if (msg == WM_LBUTTONUP)
	{
		current_mouse_button_state[mouse_left] = false;
		return true;
	}
	else if (msg == WM_RBUTTONDOWN)
	{
		current_mouse_button_state[mouse_right] = true;
		latched_mouse_trigger[mouse_right] = true;
		return true;
	}
	else if (msg == WM_RBUTTONUP)
	{
		current_mouse_button_state[mouse_right] = false;
		return true;
	}
	else if (msg == WM_MBUTTONDOWN)
	{
		current_mouse_button_state[mouse_middle] = true;
		latched_mouse_trigger[mouse_middle] = true;
		return true;
	}
	else if (msg == WM_MBUTTONUP)
	{
		current_mouse_button_state[mouse_middle] = false;
		return true;
	}
	else if (msg == WM_MOUSEWHEEL)
	{
		short delta = static_cast<short>(HIWORD(wparam));
		raw_mouse_wheel_delta += delta;
		return true;
	}
	return false;
}

//フレームの終わりに呼び出し、更新ロックを解除する
void Input::EndFrame()
{
	is_pad_connected = false;
}