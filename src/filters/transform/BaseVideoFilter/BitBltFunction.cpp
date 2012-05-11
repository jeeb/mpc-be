#include "stdafx.h"
#include <memory.h>
#include "BitBltFunction.h"

bool BitBltFromP016ToP016(size_t w, size_t h, BYTE* dstY, BYTE* dstUV, int dstPitch, BYTE* srcY, BYTE* srcUV, int srcPitch)
{
	// Copy Y plane
	for (size_t row = 0; row < h; row++)
	{
		BYTE* src = srcY + row * srcPitch;
		BYTE* dst = dstY + row * dstPitch;
		
		memcpy(dst, src, dstPitch);
	}

	// Copy UV plane. UV plane is half height.
	for (size_t row = 0; row < h/2; row++)
	{
		BYTE* src = srcUV + row * srcPitch;
		BYTE* dst = dstUV + row * dstPitch;

		memcpy(dst, src, dstPitch);
	}

	return true;
}