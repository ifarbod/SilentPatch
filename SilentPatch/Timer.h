#ifndef __TIMER
#define __TIMER

class CTimer
{
public:
	static float*			ms_fTimeScale;
	static float*			ms_fTimeStep;
	static float*			ms_fTimeStepNotClipped;
	static bool*			m_UserPause;
	static bool*			m_CodePause;
	static int*				m_snTimeInMilliseconds;
	static int*				m_snPreviousTimeInMilliseconds;
	static int*				m_snTimeInMillisecondsNonClipped;
	static int*				m_snTimeInMillisecondsPauseMode;
	static unsigned int*	m_FrameCounter;

public:
	static void				Update_SilentPatch();
};

#endif