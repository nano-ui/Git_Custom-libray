#pragma once
#include "Engine\Core\Log\Log.h"

#include <DirectXMath.h>
#include <algorithm>

class ConvertUtility
{
public:
	//NDC座標に変換
	 static DirectX::XMFLOAT2 ConvertNDC(float width, float height, DirectX::XMFLOAT2 target_pos);

	 //0.0f～1.0fの間に変換
	 template<typename T>
	 static float ConvertNormalized(const T& current_value, const T& min_value, const T& max_value)
	 {
		 float value = static_cast<float>(current_value);
		 float max_val = static_cast<float>(max_value);
		 float min_val = static_cast<float>(min_value);
		 if (max_val <= min_val)
		 {
			 LOG_WARNING({ u8"設定" }, u8"最大値が最小値以下です。(最大値:%f 最小値:%f)", max_val, min_val);
			 return 0.0f;
		 }
		 float ratio = (value - min_val) / (max_val - min_val);
		 ratio = std::clamp(ratio, 0.0f, 1.0f);
		 return ratio;
	 }

	 template<typename T>
	 static float ConvertInverseNormalized(const T& current_value, const T& min_value, const T& max_value)
	 {
		 float max_val = static_cast<float>(max_value);
		 float min_val = static_cast<float>(min_value);
		 if (max_val <= min_val)
		 {
			 LOG_WARNING({ u8"設定" }, u8"最大値が最小値以下です。(最大値:%f 最小値:%f)", max_val, min_val);
			 return 0.0f;
		 }
		 return 1.0f - ConvertNormalized(current_value, min_value, max_value);
	 }

private:

};

