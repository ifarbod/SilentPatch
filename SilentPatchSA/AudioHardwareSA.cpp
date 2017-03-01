#include "StdAfxSA.h"
#include "AudioHardwareSA.h"

bool			CAEDataStream::m_bUseNewStruct;

static void* CAEDataStream__Initialise = AddressByVersion<void*>(0x4DC2B0, 0x4DC7A0, 0x4E7550);
WRAPPER bool CAEDataStream::Initialise() { VARJMP(CAEDataStream__Initialise); }

unsigned int			CAEStreamingDecoder::nMallocRefCount = 0;

void*			pMalloc = nullptr;
unsigned int	nBlockSize = 0;
unsigned int	nLastMallocSize = 0;

unsigned int CAEDataStreamOld::Seek(long nToSeek, int nPoint)
{
	switch ( nPoint )
	{
	case FILE_BEGIN:
		nToSeek = nToSeek + dwStartPosition;
		break;
	case FILE_END:
		nPoint = FILE_BEGIN;
		nToSeek = dwStartPosition + dwLength - nToSeek;
		break;
	}

	dwCurrentPosition = SetFilePointer(hHandle, nToSeek, nullptr, nPoint);

	return dwCurrentPosition - dwStartPosition;
}

unsigned int CAEDataStreamOld::FillBuffer(void* pBuf, unsigned long nLen)
{
	ReadFile(hHandle, pBuf, nLen, &nLen, nullptr);

	dwCurrentPosition += nLen;
	return nLen;
}

unsigned int CAEDataStreamNew::Seek(long nToSeek, int nPoint)
{
	switch ( nPoint )
	{
	case FILE_BEGIN:
		nToSeek = nToSeek + dwStartPosition;
		break;
	case FILE_END:
		nPoint = FILE_BEGIN;
		nToSeek = dwStartPosition + dwLength - nToSeek;
		break;
	}

	dwCurrentPosition = SetFilePointer(hHandle, nToSeek, nullptr, nPoint);

	return dwCurrentPosition - dwStartPosition;
}

unsigned int CAEDataStreamNew::FillBuffer(void* pBuf, unsigned long nLen)
{
	ReadFile(hHandle, pBuf, nLen, &nLen, nullptr);

	dwCurrentPosition += nLen;
	return nLen;
}

CAEStreamingDecoder::~CAEStreamingDecoder()
{
	if ( CAEDataStream::IsNew() )
		delete reinterpret_cast<CAEDataStreamNew*>(pStream);
	else
		delete reinterpret_cast<CAEDataStreamOld*>(pStream);
	pStream = nullptr;

	if ( --nMallocRefCount == 0 )
	{
		operator delete(pMalloc);
		pMalloc = nullptr;

		nLastMallocSize = 0;
	}
}