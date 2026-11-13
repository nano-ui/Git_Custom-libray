#include "ConvertUtility.h"
#include "Engine\Core\Log\Log.h"

//NDCç¿ïWÇ…ïœä∑
DirectX::XMFLOAT2 ConvertUtility::ConvertNDC(float width, float height, DirectX::XMFLOAT2 target_pos)
{
	if (height <= 0.0f || width <= 0.0f)
	{
		LOG_WARNING({ u8"ê›íË" }, u8"â°ïùÇ‹ÇΩÇÕècïùÇ™0à»â∫Ç≈Ç∑(â°ïù:%f.2 ècïù:%f.2)", height, width);
		return { 0.0f,0.0f };
	}

	DirectX::XMFLOAT2 ndc = { 0.0f,0.0f };
	DirectX::XMFLOAT2 ratio = { 0.0f,0.0f };

	ratio.x = target_pos.x / width;			//âÊñ ÇÃî‰ó¶Åi0.0 Å` 1.0ÅjÇ÷ÇÃê≥ãKâª
	ratio.x *= 2.0f;						//îÕàÕÇ2î{Ç…ägí£Åi0.0 Å` 2.0Åj
	ndc.x = ratio.x - 1.0f;					//íÜâõÇå¥ì_Ç…ÉVÉtÉgÅi-1.0 Å` +1.0Åj

	ratio.y = target_pos.y / height;
	ratio.y *= 2.0f;
	ndc.y = 1.0f - ratio.y;

	return ndc;
}
