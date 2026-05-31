#include "Input.h"

//マジックナンバーを排除するための定数
static constexpr int pad_user_index = 0;			//読み取るコントローラーのプレイヤー番号（1P）
static constexpr int key_press_mask = 0x80;			//GetKeyboardStateでキーが押されているかを判定する最上位ビットマスク
static constexpr float stick_max_value = 32767.0f;	//アナログスティックの最大値（short型の最大値）
static constexpr float dead_zone = 0.2f;			//アナログスティックの遊び（デッドゾーン）の割合

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

	ZeroMemory(&current_pad_state, sizeof(XINPUT_STATE));
	ZeroMemory(&prev_pad_state, sizeof(XINPUT_STATE));
	is_pad_connected = false;

	//マウス座標の初期化
	current_mouse_pos = { 0,0 };
	GetCursorPos(&current_mouse_pos);
	prev_mouse_pos = current_mouse_pos;
}

//更新処理
void Input::Update()
{
	//キーボードの状態更新
	memcpy(prev_key_state, current_key_state, sizeof(current_key_state));
	bool success_keyboard = GetKeyboardState(current_key_state);
	if (success_keyboard)
	{
		ZeroMemory(current_key_state, sizeof(current_key_state));
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

	//マウス座標の更新
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
	bool is_current_press = (current_key_state[key_code] & key_press_mask) != 0;
	bool is_prev_press = (prev_key_state[key_code] & key_press_mask) != 0;
	return  is_current_press && !is_prev_press;
}

//キーボードが離された瞬間か判定
bool Input::IsKeyRelease(int key_code) const
{
	bool is_current_press = (current_key_state[key_code] & key_press_mask) != 0;
	bool is_prev_press = (prev_key_state[key_code] & key_press_mask) != 0;

	return is_current_press && is_prev_press;
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
	float value = static_cast<float>(current_pad_state.Gamepad.sThumbLY) / stick_max_value;
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
	return static_cast<float>(current_mouse_pos.x - prev_mouse_pos.x);
}

//マウスのY方向の移動量を取得
float Input::GetMouseDeltaY() const
{
	return static_cast<float>(current_mouse_pos.y - prev_mouse_pos.y);
}
