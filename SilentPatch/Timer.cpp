#include "StdAfx.h"

#include "Timer.h"

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

static gtaTimer			timerFunction;
static unsigned int		suspendDepth;
static long long		timerFreq;
static long long		oldTime, suspendTime;

static long long		cyclesTime, cyclesTimeNonClipped, cyclesTimePauseMode, cyclesPreviousTime;

static long long QPC()
{
	LARGE_INTEGER		Counter;
	QueryPerformanceCounter(&Counter);
	return Counter.QuadPart;
}

static long long OldTimer()
{
	TIMECAPS	caps;
	long long	nTime;

	timeGetDevCaps(&caps, sizeof(TIMECAPS));
	timeBeginPeriod(caps.wPeriodMin);
	nTime = timeGetTime();
	timeEndPeriod(caps.wPeriodMin);
	return nTime;
}

static inline void InitTimerFunc()
{
	if ( timerFunction )
		return;

	LARGE_INTEGER	Frequency;
	if ( QueryPerformanceFrequency(&Frequency) )
	{
		timerFreq = Frequency.QuadPart / 1000;
		timerFunction = QPC;
	}
	else
	{
		timerFreq = 1;
		timerFunction = OldTimer;
	}
}

extern void (__stdcall *AudioResetTimers)(unsigned int);
extern bool* bSnapShotActive;

void CTimer::Initialise()
{
	suspendDepth = 0;
	*ms_fTimeScale = *ms_fTimeStep = 1.0f;
	*m_UserPause = false;
	*m_CodePause = false;
	*m_snTimeInMilliseconds = 0;
	*m_snPreviousTimeInMilliseconds = 0;
	*m_snTimeInMillisecondsNonClipped = 0;
	*m_FrameCounter = 0;

	InitTimerFunc();

	oldTime = timerFunction();
	AudioResetTimers(0);
}

void CTimer::Suspend()
{
	if ( suspendDepth++ == 0 )
	{
#ifdef SILENTPATCH_VC_VER
		// MVL fix
		InitTimerFunc();
#endif
		suspendTime = timerFunction();
	}
}

void CTimer::Resume()
{
	if ( --suspendDepth == 0 )
	{
#ifdef SILENTPATCH_VC_VER
		// MVL fix
		InitTimerFunc();
#endif
		oldTime = timerFunction() - suspendTime;
	}
}

unsigned int CTimer::GetCyclesPerFrame()
{
#ifdef SILENTPATCH_VC_VER
	// MVL fix
	InitTimerFunc();
#endif
	return static_cast<unsigned int>(timerFunction() - oldTime);
}

unsigned int CTimer::GetCyclesPerMillisecond()
{
	return static_cast<unsigned int>(timerFreq);
}

void CTimer::Update()
{
#ifdef SILENTPATCH_VC_VER
	// CTimer::Initialise workaround
	static bool		bIntialisedIt = false;

	if ( !bIntialisedIt )
	{
		Initialise();
		bIntialisedIt = true;
	}
#endif

	*m_snPreviousTimeInMilliseconds = *m_snTimeInMilliseconds;
	cyclesPreviousTime = cyclesTime;

	long long	nCurTime;
	float		nDelta;

	nCurTime = timerFunction();
	nDelta = (nCurTime - oldTime) * *ms_fTimeScale;
	oldTime = nCurTime;

	//*m_snTimeInMillisecondsPauseMode += nDelta;
	cyclesTimePauseMode += static_cast<long long>(nDelta);

	if ( *m_UserPause || *m_CodePause )
		*ms_fTimeStep = 0.0f;
	else
	{
		*ms_fTimeStep = (nDelta/timerFreq) * 0.05f;
		cyclesTime += static_cast<long long>(nDelta);
		cyclesTimeNonClipped += static_cast<long long>(nDelta);
		//*m_snTimeInMilliseconds += nDelta;
		//*m_snTimeInMillisecondsNonClipped += nDelta;
	}

#ifdef SILENTPATCH_III_VER
	if ( *ms_fTimeStep < 0.01f && !*m_UserPause && !*m_CodePause )
#else
	if ( *ms_fTimeStep < 0.01f && !*m_UserPause && !*m_CodePause && !*bSnapShotActive )
#endif
		*ms_fTimeStep = 0.01f;

	*ms_fTimeStepNotClipped = *ms_fTimeStep;

	if ( *ms_fTimeStep > 3.0f )
		*ms_fTimeStep = 3.0f;

	/*if ( *m_snTimeInMilliseconds - *m_snPreviousTimeInMilliseconds > 60 )
		*m_snTimeInMilliseconds = *m_snPreviousTimeInMilliseconds + 60;*/
	if ( cyclesTime - cyclesPreviousTime > 60 * timerFreq )
		cyclesTime = cyclesPreviousTime + (60 * timerFreq);

	*m_snTimeInMillisecondsPauseMode = static_cast<int>(cyclesTimePauseMode / timerFreq);
	*m_snTimeInMilliseconds = static_cast<int>(cyclesTime / timerFreq);
	*m_snTimeInMillisecondsNonClipped = static_cast<int>(cyclesTimeNonClipped / timerFreq);

	++(*m_FrameCounter);
}

void CTimer::RecoverFromSave()
{
	cyclesTime = *m_snTimeInMilliseconds * timerFreq;
	cyclesPreviousTime = *m_snPreviousTimeInMilliseconds * timerFreq;
	cyclesTimePauseMode = *m_snTimeInMillisecondsPauseMode * timerFreq;
	cyclesTimeNonClipped = *m_snTimeInMillisecondsNonClipped * timerFreq;
}