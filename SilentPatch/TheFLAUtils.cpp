#include "TheFLAUtils.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

int32_t (*FLAUtils::GetExtendedIDFunc)(const void* ptr) = FLAUtils::GetExtendedID_Stock;

void FLAUtils::Init()
{
	HMODULE hFLA = GetModuleHandle("$fastman92limitAdjuster.asi");
	if ( hFLA != nullptr )
	{
		auto function = reinterpret_cast<decltype(GetExtendedIDFunc)>(GetProcAddress( hFLA, "GetExtendedIDfrom16bitBefore" ));
		if ( function != nullptr )
		{
			GetExtendedIDFunc = function;
		}
	}
}