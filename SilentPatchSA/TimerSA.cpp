#include "StdAfxSA.h"
#include "TimerSA.h"

int& CTimer::m_snTimeInMilliseconds = **AddressByVersion<int**>(0x4242D1, 0x53F6A1, 0x406FA1);
float& CTimer::m_fTimeStep = **AddressByVersion<float**>(0x41DE4F + 2, 0, 0);