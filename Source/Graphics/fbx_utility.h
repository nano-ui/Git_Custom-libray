#pragma once

#include <string>

namespace FbxUtility
{
	//指定されたFBXファイルにアニメーションデータが存在するかをチェック
	inline bool CheckForAnimation(const wchar_t* fbx_filename)
	{
		std::wstring filename_w(fbx_filename);
		return filename_w.find(L"Anim") != std::wstring::npos;
	}
}