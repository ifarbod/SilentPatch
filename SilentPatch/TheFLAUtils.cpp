#include "TheFLAUtils.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "ASIModuleHandle.h"

int32_t (*FLAUtils::GetExtendedID8Func)(const uint8_t* ptr) = FLAUtils::GetExtendedID8_Stock;
int32_t (*FLAUtils::GetExtendedID16Func)(const uint16_t* ptr) = FLAUtils::GetExtendedID16_Stock;

void FLAUtils::Init()
{
	const HMODULE hFLA = GetASIModuleHandle(TEXT("$fastman92limitAdjuster"));
	if ( hFLA != nullptr )
	{
		const auto function8 = reinterpret_cast<decltype(GetExtendedID8Func)>(GetProcAddress( hFLA, "GetExtendedIDfrom8bitBefore" ));
		if ( function8 != nullptr )
		{
			GetExtendedID8Func = function8;
		}

		const auto function16 = reinterpret_cast<decltype(GetExtendedID16Func)>(GetProcAddress( hFLA, "GetExtendedIDfrom16bitBefore" ));
		if ( function16 != nullptr )
		{
			GetExtendedID16Func = function16;
		}
	}
}