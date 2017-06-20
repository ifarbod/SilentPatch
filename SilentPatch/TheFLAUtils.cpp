#include "TheFLAUtils.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

int32_t (*FLAUtils::GetExtendedIDFunc)(const void* ptr) = FLAUtils::GetExtendedID_Stock;

void FLAUtils::Init()
{
	HMODULE hFLA = GetModuleHandle(TEXT("$fastman92limitAdjuster.asi"));
	if ( hFLA != nullptr )
	{
		GetExtendedIDFunc = reinterpret_cast<decltype(GetExtendedIDFunc)>(GetProcAddress( hFLA, "GetExtendedIDfrom16bitBefore" ));
	}
}