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

class NOVMT CAEDataStream : public IStream
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

	CAEDataStream(DWORD index, char* pName, int startPos, int len, bool encrypted)
		:	pFilename(pName), bOpened(false), dwCurrentPosition(0), dwStartPosition(startPos),
			dwLength(len), dwID(index), bEncrypted(encrypted), nRefCount(1)
	{}

	~CAEDataStream()
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
	//unsigned int		FillBuffer(void* pBuf, unsigned long nLen);
	bool				Initialise();
};


class CAEStreamingDecoder
{
private:
	CAEDataStream*		pStream;

public:
	CAEStreamingDecoder(CAEDataStream* stream)
		: pStream(stream)
	{
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

	virtual					~CAEStreamingDecoder()
	{
		GTAdelete(pStream);
		pStream = nullptr;
	}

	virtual unsigned int	GetStreamID()=0;
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

	virtual ~CAEFLACDecoder()
	{
		if ( pFLACDecoder )
		{
			FLAC__stream_decoder_finish(pFLACDecoder);
			FLAC__stream_decoder_delete(pFLACDecoder);

			FLAC__metadata_object_delete(pStreamInfo);
			pFLACDecoder = nullptr;
		}
	}

	
	virtual bool			Initialise() override;

	virtual unsigned int	FillBuffer(void* pBuf, unsigned long nLen) override;

	virtual unsigned int	GetStreamLengthMs() override
	{
		return pStreamInfo->data.stream_info.total_samples * 1000 / pStreamInfo->data.stream_info.sample_rate;
	}

	virtual unsigned int	GetStreamPlayTimeMs() override
	{
		return nCurrentSample * 1000 / pStreamInfo->data.stream_info.sample_rate;
	}

	virtual void			SetCursor(unsigned int nTime) override
	{
		FLAC__stream_decoder_seek_absolute(pFLACDecoder, nTime * pStreamInfo->data.stream_info.sample_rate / 1000);
	}

	virtual unsigned int	GetSampleRate() override
	{
		return pStreamInfo->data.stream_info.sample_rate;
	}

	virtual unsigned int	GetStreamID() override
	{
		return GetStream()->GetID();
	}
};

#endif