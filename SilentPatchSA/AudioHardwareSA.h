#ifndef __AUDIOHARDWARE
#define __AUDIOHARDWARE

// libflac
#define FLAC__NO_DLL
#include "FLAC\stream_decoder.h"
#include "FLAC\metadata.h"

// IStream
#include <Objidl.h>

enum eDecoderType
{
	DECODER_NULL,
	DECODER_VORBIS,
	DECODER_WAVE,
	DECODER_WINDOWSMEDIA,
	DECODER_QUICKTIME,

	// Custom
	DECODER_FLAC
};

// 1.0/Steam structure
class NOVMT CAEDataStreamOld : public IStream
{
private:
	HANDLE			hHandle;
	char*			pFilename;
	bool			bOpened;
	DWORD			dwCurrentPosition, dwStartPosition;
	DWORD			dwLength;
	DWORD			dwID;
	bool			bEncrypted;
	LONG			nRefCount;

public:
	void			operator delete(void* data)
	{
		// Call SA operator delete
		GTAdelete(data);
	}

	CAEDataStreamOld() = delete;

	~CAEDataStreamOld()
	{
		if ( bOpened )
		{
			CloseHandle(hHandle);
			bOpened = false;
		}
		if ( pFilename != nullptr )
		{
			GTAdelete(pFilename);
			pFilename = nullptr;
		}
	}

	inline DWORD	GetID()
		{ return dwID; }
	inline HANDLE	GetFile()
		{ return hHandle; }

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void ** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef(void);
    virtual ULONG STDMETHODCALLTYPE Release(void);

    // ISequentialStream Interface
public:
    virtual HRESULT STDMETHODCALLTYPE Read(void* pv, ULONG cb, ULONG* pcbRead);
    virtual HRESULT STDMETHODCALLTYPE Write(void const* pv, ULONG cb, ULONG* pcbWritten);

    // IStream Interface
public:
    virtual HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER);
    virtual HRESULT STDMETHODCALLTYPE CopyTo(IStream*, ULARGE_INTEGER, ULARGE_INTEGER*, ULARGE_INTEGER*) ;   
    virtual HRESULT STDMETHODCALLTYPE Commit(DWORD);    
    virtual HRESULT STDMETHODCALLTYPE Revert(void);    
    virtual HRESULT STDMETHODCALLTYPE LockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD);   
    virtual HRESULT STDMETHODCALLTYPE UnlockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD);
    virtual HRESULT STDMETHODCALLTYPE Clone(IStream **);
    virtual HRESULT STDMETHODCALLTYPE Seek(LARGE_INTEGER liDistanceToMove, DWORD dwOrigin, ULARGE_INTEGER* lpNewFilePointer);
    virtual HRESULT STDMETHODCALLTYPE Stat(STATSTG* pStatstg, DWORD grfStatFlag);

public:
	// Custom methods
	DWORD		Seek(LONG nToSeek, DWORD nPoint);
	DWORD		FillBuffer(void* pBuf, DWORD nLen);
	DWORD		GetCurrentPosition()
	{
		LARGE_INTEGER filePointer;
		filePointer.QuadPart = 0;
		SetFilePointerEx( hHandle, filePointer, &filePointer, FILE_CURRENT );	
		return DWORD(filePointer.QuadPart - dwStartPosition);
	}
};

// 1.01 structure
class NOVMT CAEDataStreamNew : public IStream
{
private:
	void*			pUselessMalloc;

	HANDLE			hHandle;
	char*			pFilename;
	bool			bOpened;
	DWORD			dwCurrentPosition, dwStartPosition;
	DWORD			dwLength;
	DWORD			dwID;
	bool			bEncrypted;
	LONG			nRefCount;

public:
	void			operator delete(void* data)
	{
		// Call SA operator delete
		GTAdelete(data);
	}

	CAEDataStreamNew() = delete;

	~CAEDataStreamNew()
	{
		if ( bOpened )
		{
			CloseHandle(hHandle);
			bOpened = false;
		}
		if ( pFilename != nullptr )
		{
			GTAdelete(pFilename);
			pFilename = nullptr;
		}
	}

	inline DWORD	GetID()
		{ return dwID; }
	inline HANDLE	GetFile()
		{ return hHandle; }

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void ** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef(void);
    virtual ULONG STDMETHODCALLTYPE Release(void);

    // ISequentialStream Interface
public:
    virtual HRESULT STDMETHODCALLTYPE Read(void* pv, ULONG cb, ULONG* pcbRead);
    virtual HRESULT STDMETHODCALLTYPE Write(void const* pv, ULONG cb, ULONG* pcbWritten);

    // IStream Interface
public:
    virtual HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER);
    virtual HRESULT STDMETHODCALLTYPE CopyTo(IStream*, ULARGE_INTEGER, ULARGE_INTEGER*, ULARGE_INTEGER*) ;   
    virtual HRESULT STDMETHODCALLTYPE Commit(DWORD);    
    virtual HRESULT STDMETHODCALLTYPE Revert(void);    
    virtual HRESULT STDMETHODCALLTYPE LockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD);   
    virtual HRESULT STDMETHODCALLTYPE UnlockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD);
    virtual HRESULT STDMETHODCALLTYPE Clone(IStream **);
    virtual HRESULT STDMETHODCALLTYPE Seek(LARGE_INTEGER liDistanceToMove, DWORD dwOrigin, ULARGE_INTEGER* lpNewFilePointer);
    virtual HRESULT STDMETHODCALLTYPE Stat(STATSTG* pStatstg, DWORD grfStatFlag);

public:
	// Custom methods
	DWORD		Seek(LONG nToSeek, DWORD nPoint);
	DWORD		FillBuffer(void* pBuf, DWORD nLen);
	DWORD		GetCurrentPosition()
	{
		LARGE_INTEGER filePointer;
		filePointer.QuadPart = 0;
		SetFilePointerEx( hHandle, filePointer, &filePointer, FILE_CURRENT );	
		return DWORD(filePointer.QuadPart - dwStartPosition);
	}
};

class CAEDataStream
{
private:
	static bool			m_bUseNewStruct;

private:
	~CAEDataStream() = delete;
	CAEDataStream() = delete;

public:
	static void			SetStructType(bool bNew)
	{ m_bUseNewStruct = bNew; }
	static bool			IsNew()
		{ return m_bUseNewStruct; }

	// This is handled by GTA so we can leave it that way
	bool				Initialise();

	unsigned int		Seek(long nToSeek, int nPoint)
	{	if ( m_bUseNewStruct ) 
			return reinterpret_cast<CAEDataStreamNew*>(this)->Seek(nToSeek, nPoint);
		return reinterpret_cast<CAEDataStreamOld*>(this)->Seek(nToSeek, nPoint); }

	unsigned int		FillBuffer(void* pBuf, unsigned long nLen)
	{	if ( m_bUseNewStruct ) 
			return reinterpret_cast<CAEDataStreamNew*>(this)->FillBuffer(pBuf, nLen);
		return reinterpret_cast<CAEDataStreamOld*>(this)->FillBuffer(pBuf, nLen); }

	unsigned int		GetCurrentPosition()
	{	if ( m_bUseNewStruct ) 
			return reinterpret_cast<CAEDataStreamNew*>(this)->GetCurrentPosition();
		return reinterpret_cast<CAEDataStreamOld*>(this)->GetCurrentPosition(); }

	inline DWORD	GetID()
	{	if ( m_bUseNewStruct ) 
			return reinterpret_cast<CAEDataStreamNew*>(this)->GetID();
		return reinterpret_cast<CAEDataStreamOld*>(this)->GetID(); }

	inline HANDLE	GetFile()
	{	if ( m_bUseNewStruct ) 
			return reinterpret_cast<CAEDataStreamNew*>(this)->GetFile();
		return reinterpret_cast<CAEDataStreamOld*>(this)->GetFile(); }
};


class CAEStreamingDecoder
{
private:
	CAEDataStream*		pStream;

public:
	CAEStreamingDecoder(CAEDataStream* stream)
		: pStream(stream)
	{
		if ( stream != nullptr )
			stream->Initialise();
	}

	inline CAEDataStream*	GetStream()
		{ return pStream; }

	virtual bool			Initialise()=0;
	virtual uint32_t		FillBuffer(void* pBuf,uint32_t nLen)=0;

	virtual uint32_t		GetStreamLengthMs()=0;
	virtual uint32_t		GetStreamPlayTimeMs()=0;
	virtual void			SetCursor(uint32_t nTime)=0;
	virtual uint32_t		GetSampleRate()=0;

	virtual					~CAEStreamingDecoder();

	virtual uint32_t		GetStreamID()=0;
};

static_assert(sizeof(CAEDataStreamOld) == 0x28, "Wrong size: CAEDataStreamOld");
static_assert(sizeof(CAEDataStreamNew) == 0x2C, "Wrong size: CAEDataStreamNew");

#endif