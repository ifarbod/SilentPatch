#include "StdAfxSA.h"
#include "TimerSA.h"

int& CTimer::m_snTimeInMilliseconds = **AddressByVersion<int**>(0x4242D1, 0x53F6A1, 0x406FA1);