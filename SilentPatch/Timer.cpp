#include "StdAfx.h"
#include "Timer.h"

#include "Patterns.h"

float&			CTimer::ms_fTimeScale = **hook::get_pattern<float*>( "83 E4 F8 89 44 24 08 C7 44 24 0C 00 00 00 00 DF 6C 24 08", 0x70 + 2 );
float&			CTimer::ms_fTimeStep = **hook::get_pattern<float*>( "83 E4 F8 89 44 24 08 C7 44 24 0C 00 00 00 00 DF 6C 24 08", 0xF3 + 2 );
bool&			CTimer::m_UserPause = **hook::get_pattern<bool*>( "83 E4 F8 89 44 24 08 C7 44 24 0C 00 00 00 00 DF 6C 24 08", 0x4A + 2 );
bool&			CTimer::m_CodePause = **hook::get_pattern<bool*>( "83 E4 F8 89 44 24 08 C7 44 24 0C 00 00 00 00 DF 6C 24 08", 0x67 + 2 );
int&			CTimer::m_snTimeInMilliseconds = **hook::get_pattern<int*>( "83 E4 F8 89 44 24 08 C7 44 24 0C 00 00 00 00 DF 6C 24 08", 0x102 + 1 );
int&			CTimer::m_snTimeInMillisecondsNonClipped = **hook::get_pattern<int*>( "83 E4 F8 89 44 24 08 C7 44 24 0C 00 00 00 00 DF 6C 24 08", 0x13B + 1 );
int&			CTimer::m_snTimeInMillisecondsPauseMode = **hook::get_pattern<int*>( "83 E4 F8 89 44 24 08 C7 44 24 0C 00 00 00 00 DF 6C 24 08", 0x9C + 1 );

static uint32_t& timerFrequency = **hook::get_pattern<uint32_t*>( "83 E4 F8 89 44 24 08 C7 44 24 0C 00 00 00 00 DF 6C 24 08", -7 );
static LARGE_INTEGER& prevTimer = **hook::get_pattern<LARGE_INTEGER*>( "83 E4 F8 89 44 24 08 C7 44 24 0C 00 00 00 00 DF 6C 24 08", 64 );


void CTimer::Update_SilentPatch()
{
	LARGE_INTEGER perfCount;
	QueryPerformanceCounter( &perfCount );

	double diff = double(perfCount.QuadPart - prevTimer.QuadPart);
	if ( !m_UserPause && !m_CodePause ) diff *= ms_fTimeScale;

	prevTimer = perfCount;

	static double DeltaRemainder = 0.0;
	const double delta = diff / timerFrequency;
	double deltaIntegral;
	DeltaRemainder = modf( delta + DeltaRemainder, &deltaIntegral );

	const int deltaInteger = int(deltaIntegral);
	m_snTimeInMillisecondsPauseMode += deltaInteger;
	if ( !m_UserPause && !m_CodePause )
	{
		m_snTimeInMillisecondsNonClipped += deltaInteger;
		m_snTimeInMilliseconds += deltaInteger;
		ms_fTimeStep = float(delta * 0.05);
	}
	else
	{
		ms_fTimeStep = 0.0f;
	}
}