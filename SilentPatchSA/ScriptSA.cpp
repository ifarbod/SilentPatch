#include "StdAfxSA.h"
#include "ScriptSA.h"

int32_t* CRunningScript::GetDay_GymGlitch()
{
	static int32_t	Out[2];

	if ( strcmp(Name, "gymbike") == 0 || strcmp(Name, "gymbenc") == 0 || strcmp(Name, "gymtrea") == 0 || strcmp(Name, "gymdumb") == 0 )
	{
		static int* const StatTypesInt = *AddressByVersion<int**>(0x55C0D8, 0x55C578, 0x574F24);

		Out[0] = -1;
		Out[1] = StatTypesInt[134-120];
	}
	else
	{
		Out[0] = nGameClockMonths;
		Out[1] = nGameClockDays;
	}

	return Out;
}