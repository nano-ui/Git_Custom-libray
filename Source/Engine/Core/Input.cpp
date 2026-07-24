#include "Input.h"

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

	//マウス移動量の確定処理
	mouse_delta_x = raw_mouse_delta_x;
	mouse_delta_y = raw_mouse_delta_y;

	//次フレーム用に累積変数をクリア
	raw_mouse_delta_x = 0.0f;
	raw_mouse_delta_y = 0.0f;

	//スクリーン座標の更新
	prev_mouse_pos = current_mouse_pos;
	GetCursorPos(&current_mouse_pos);
}

//キーボードが押されているか判定
bool Input::IsKeyPress(int key_code) const
{
	return (current_key_state[key_code] & key_press_mask) != 0;
}

//キーボードが押された瞬間か判定
bool Input::IsKeyTrigger(int key_code) const
{
	if (key_code < 0 || key_code >= max_keys)return false;
	if (latched_key_trigger[key_code])
	{
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

//Windowsメッセージ処理
bool Input::ProcessMessage(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (msg == WM_MOUSEMOVE)
	{
		//メッセージパラメーターから現在のローカルマウス座標を抽出
		int current_x = static_cast<int>(LOWORD(lparam));
		int current_y = static_cast<int>(HIWORD(lparam));

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
	return false;
}
