#pragma once

#include <cstdint>

class FLAUtils
{
public:
	static int32_t GetExtendedID(const void* ptr)
	{
		if ( GetExtendedIDFunc != nullptr )
			return GetExtendedIDFunc(ptr);
		
		uint16_t uID = *static_cast<const uint16_t*>(ptr);
		return uID > MAX_UINT16_ID ? *static_cast<const int16_t*>(ptr) : uID;
	}

	static void Init();

private:
	static const int32_t MAX_UINT16_ID = 0xFFFD;

	static int32_t (*GetExtendedIDFunc)(const void* ptr);
};