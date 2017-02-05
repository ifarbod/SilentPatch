#ifndef __AUDIOHARDWARE
#define __AUDIOHARDWARE

// libflac
#define FLAC__NO_DLL
#include "share\compat.h"
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
		GTAdelete(pFilename);
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
	unsigned int		Seek(long nToSeek, int nPoint);
	unsigned int		FillBuffer(void* pBuf, unsigned long nLen);
	unsigned int		GetCurrentPosition()
		{ return SetFilePointer(hHandle, 0, nullptr, FILE_CURRENT) - dwStartPosition; }
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
		GTAdelete(pFilename);
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
	unsigned int		Seek(long nToSeek, int nPoint);
	unsigned int		FillBuffer(void* pBuf, unsigned long nLen);
	unsigned int		GetCurrentPosition()
		{ return SetFilePointer(hHandle, 0, nullptr, FILE_CURRENT) - dwStartPosition; }
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

	static unsigned int	nMallocRefCount;

public:
	CAEStreamingDecoder(CAEDataStream* stream)
		: pStream(stream)
	{
		++nMallocRefCount;

		if ( stream )
			stream->Initialise();
	}

	inline CAEDataStream*	GetStream()
		{ return pStream; }

	virtual bool			Initialise()=0;
	virtual unsigned int	FillBuffer(void* pBuf,unsigned long nLen)=0;

	virtual unsigned int	GetStreamLengthMs()=0;
	virtual unsigned int	GetStreamPlayTimeMs()=0;
	virtual void			SetCursor(unsigned int nTime)=0;
	virtual unsigned int	GetSampleRate()=0;

	virtual					~CAEStreamingDecoder();

	virtual unsigned int	GetStreamID()=0;
};

class CAEWaveDecoder : public CAEStreamingDecoder
{
private:
	unsigned int	nDataSize;
	unsigned int	nOffsetToData;
	//bool			bInitialised;

	struct FormatChunk
	{
		unsigned short		audioFormat;
		unsigned short		numChannels;
		unsigned int		sampleRate;
		unsigned int		byteRate;
		unsigned short		blockAlign;
		unsigned short		bitsPerSample;
	}				formatChunk;

public:
	CAEWaveDecoder(CAEDataStream* stream)
		: CAEStreamingDecoder(stream)
	{}

	virtual bool			Initialise() override;
	virtual unsigned int	FillBuffer(void* pBuf, unsigned long nLen) override;

	virtual unsigned int	GetStreamLengthMs() override
		{ return static_cast<unsigned long long>(nDataSize) * 8000 / (formatChunk.sampleRate*formatChunk.bitsPerSample*formatChunk.numChannels); }
	virtual unsigned int	GetStreamPlayTimeMs() override
	{ return static_cast<unsigned long long>(GetStream()->GetCurrentPosition() - nOffsetToData) * 8000 / (formatChunk.sampleRate*formatChunk.bitsPerSample*formatChunk.numChannels); }		

	virtual void			SetCursor(unsigned int nTime) override
		{	auto nPos = static_cast<unsigned long long>(nTime) * (formatChunk.sampleRate*formatChunk.bitsPerSample*formatChunk.numChannels) /  8000;
			auto nModulo = (formatChunk.numChannels*formatChunk.bitsPerSample/8);
			auto nExtra = nPos % nModulo ? nModulo - (nPos % nModulo) : 0;
			GetStream()->Seek(nOffsetToData + nPos + nExtra, FILE_BEGIN); }

	virtual unsigned int	GetSampleRate() override
	{ return formatChunk.sampleRate; }

	virtual unsigned int	GetStreamID() override
		{ return GetStream()->GetID(); }
};

class CAEFLACDecoder : public CAEStreamingDecoder
{
private:
	FLAC__StreamDecoder*		pFLACDecoder;
	FLAC__StreamMetadata*		pStreamInfo;
	unsigned int				nCurrentSample;

private:
	static FLAC__StreamDecoderReadStatus	read_cb(const FLAC__StreamDecoder* decoder, FLAC__byte buffer[], size_t* bytes, void* client_data);
	static FLAC__StreamDecoderWriteStatus	write_cb(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *const buffer[], void *client_data);
	static void								meta_cb(const FLAC__StreamDecoder* decoder, const FLAC__StreamMetadata *metadata, void *client_data);
	static void								error_cb(const FLAC__StreamDecoder* decoder, FLAC__StreamDecoderErrorStatus status, void* client_data);
	static FLAC__StreamDecoderSeekStatus	seek_cb(const FLAC__StreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data);
	static FLAC__StreamDecoderTellStatus	tell_cb(const FLAC__StreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data);
	static FLAC__StreamDecoderLengthStatus	length_cb(const FLAC__StreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data);
	static FLAC__bool						eof_cb(const FLAC__StreamDecoder *decoder, void *client_data);

public:
	CAEFLACDecoder(CAEDataStream* stream)
		: CAEStreamingDecoder(stream), pFLACDecoder(nullptr)
	{}

	virtual					~CAEFLACDecoder();
	virtual bool			Initialise() override;
	virtual unsigned int	FillBuffer(void* pBuf, unsigned long nLen) override;
	virtual unsigned int	GetStreamLengthMs() override
		{ return pStreamInfo->data.stream_info.total_samples * 1000 / pStreamInfo->data.stream_info.sample_rate; }
	virtual unsigned int	GetStreamPlayTimeMs() override
		{ return nCurrentSample * 1000 / pStreamInfo->data.stream_info.sample_rate; }
	virtual void			SetCursor(unsigned int nTime) override
		{ FLAC__stream_decoder_seek_absolute(pFLACDecoder, nTime * pStreamInfo->data.stream_info.sample_rate / 1000); }
	virtual unsigned int	GetSampleRate() override
		{ return pStreamInfo->data.stream_info.sample_rate; }
	virtual unsigned int	GetStreamID() override
		{ return GetStream()->GetID(); }
};

#endif