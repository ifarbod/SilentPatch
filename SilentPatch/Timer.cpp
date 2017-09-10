#include "StdAfx.h"
#include "Timer.h"

#include "Patterns.h"

float*			CTimer::ms_fTimeScale;
float*			CTimer::ms_fTimeStep;
float*			CTimer::ms_fTimeStepNotClipped;
bool*			CTimer::m_UserPause;
bool*			CTimer::m_CodePause;
int*			CTimer::m_snTimeInMilliseconds;
int*			CTimer::m_snPreviousTimeInMilliseconds;
int*			CTimer::m_snTimeInMillisecondsNonClipped;
int*			CTimer::m_snTimeInMillisecondsPauseMode;
unsigned int*	CTimer::m_FrameCounter;

static unsigned int		suspendDepth;
static long long		timerFreq;
static long long		oldTime, suspendTime;

static long long		cyclesTime, cyclesTimeNonClipped, cyclesTimePauseMode, cyclesPreviousTime;

static uint32_t& timerFrequency = **hook::get_pattern<uint32_t*>( "83 E4 F8 89 44 24 08 C7 44 24 0C 00 00 00 00 DF 6C 24 08", -7 );
static LARGE_INTEGER& prevTimer = **hook::get_pattern<LARGE_INTEGER*>( "83 E4 F8 89 44 24 08 C7 44 24 0C 00 00 00 00 DF 6C 24 08", 64 );


extern void (__stdcall *AudioResetTimers)(unsigned int);
extern bool* bSnapShotActive;

void CTimer::Update_SilentPatch()
{
	LARGE_INTEGER perfCount;
	QueryPerformanceCounter( &perfCount );

	double diff = double(perfCount.QuadPart - prevTimer.QuadPart);
	if ( !*m_UserPause && !*m_CodePause ) diff *= *ms_fTimeScale;

	prevTimer = perfCount;

	static double DeltaRemainder = 0.0;
	const double delta = diff / timerFrequency;
	double deltaIntegral;
	DeltaRemainder = modf( delta + DeltaRemainder, &deltaIntegral );

	const int deltaInteger = int(deltaIntegral);
	*m_snTimeInMillisecondsPauseMode += deltaInteger;
	if ( !*m_UserPause && !*m_CodePause )
	{
		*m_snTimeInMillisecondsNonClipped += deltaInteger;
		*m_snTimeInMilliseconds += deltaInteger;
		*ms_fTimeStep = float(delta * 0.05);
	}
	else
	{
		*ms_fTimeStep = 0.0f;
	}
}