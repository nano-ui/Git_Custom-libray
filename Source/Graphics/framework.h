#pragma once
#include <windows.h>
#include <memory>
#include "../Common/high_resolution_timer.h"
#include "../Scene/Scene.h"

class framework
{
public:
	CONST HWND hwnd;

	//コンストラクタ
	framework(HWND hwnd);

	//デストラクタ
	~framework();

	int run();
	LRESULT CALLBACK handle_message(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

private:
	void calculate_frame_stats();

	//フルスクリーンとウィンドウモードの切り替え
	void toggle_fullscreen();

	high_resolution_timer tictoc;
	uint32_t frames_per_second{ 0 };
	float count_by_seconds{ 0.0f };
};