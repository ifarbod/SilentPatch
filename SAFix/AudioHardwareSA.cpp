#include "StdAfxSA.h"
#include "AudioHardwareSA.h"

//WRAPPER HRESULT STDMETHODCALLTYPE CAEDataStream::Seek(LARGE_INTEGER liDistanceToMove, DWORD dwOrigin, ULARGE_INTEGER* lpNewFilePointer)
//			{ EAXJMP(0x4DC340); }
//WRAPPER HRESULT STDMETHODCALLTYPE CAEDataStream::Stat(STATSTG* pStatstg, DWORD grfStatFlag) { EAXJMP(0x4DC3A0); }

static void* CAEDataStream__Initialise = AddressByVersion<void*>(0x4DC2B0, 0, 0x4E7550);
WRAPPER bool CAEDataStream::Initialise() { VARJMP(CAEDataStream__Initialise); }

unsigned int			CAEStreamingDecoder::nMallocRefCount = 0;

static void*			pMalloc = nullptr;
static unsigned int		nBlockSize = 0;
static unsigned int		nLastMallocSize = 0;

static unsigned int		nSamplesLeftToProcess = 0;

unsigned int CAEDataStream::Seek(long nToSeek, int nPoint)
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

unsigned int CAEDataStream::FillBuffer(void* pBuf, unsigned long nLen)
{
	ReadFile(hHandle, pBuf, nLen, &nLen, nullptr);

	dwCurrentPosition += nLen;
	return nLen;
}

CAEStreamingDecoder::~CAEStreamingDecoder()
{
	GTAdelete(pStream);
	pStream = nullptr;

	if ( --nMallocRefCount == 0 )
	{
		operator delete(pMalloc);
		pMalloc = nullptr;

		nLastMallocSize = 0;
	}
}


bool CAEWaveDecoder::Initialise()
{
	auto*			pTheStream = GetStream();
	struct {
			char			sectionID[4];
			unsigned int	sectionSize;
	}				chunkHeader;

	// Find fmt section
	pTheStream->Seek(12, FILE_BEGIN);

	pTheStream->FillBuffer(&chunkHeader, sizeof(chunkHeader));
	nOffsetToData = 12 + sizeof(chunkHeader);

	while ( chunkHeader.sectionID[0] != 'f' || chunkHeader.sectionID[1] != 'm' || chunkHeader.sectionID[2] != 't' || chunkHeader.sectionID[3] != ' ' )
	{
		nOffsetToData += sizeof(chunkHeader) + chunkHeader.sectionSize;

		pTheStream->Seek(chunkHeader.sectionSize, FILE_CURRENT);
		pTheStream->FillBuffer(&chunkHeader, sizeof(chunkHeader));

	}

	// Read fmt header
	pTheStream->FillBuffer(&formatChunk, sizeof(FormatChunk));
	nOffsetToData += sizeof(FormatChunk);

	// Now skip through the rest of a chunk
	if ( chunkHeader.sectionSize - sizeof(FormatChunk) > 0 )
	{
		nOffsetToData += chunkHeader.sectionSize - sizeof(FormatChunk);
		pTheStream->Seek(chunkHeader.sectionSize - sizeof(FormatChunk), FILE_CURRENT);
	}

	// Find data chunk
	nOffsetToData += sizeof(chunkHeader);
	pTheStream->FillBuffer(&chunkHeader, sizeof(chunkHeader));

	while ( chunkHeader.sectionID[0] != 'd' || chunkHeader.sectionID[1] != 'a' || chunkHeader.sectionID[2] != 't' || chunkHeader.sectionID[3] != 'a' )
	{
		nOffsetToData += sizeof(chunkHeader) + chunkHeader.sectionSize;

		pTheStream->Seek(chunkHeader.sectionSize, FILE_CURRENT);
		pTheStream->FillBuffer(&chunkHeader, sizeof(chunkHeader));
	}

	nDataSize = chunkHeader.sectionSize;

	return formatChunk.sampleRate <= 48000 && formatChunk.numChannels <= 2 && (formatChunk.bitsPerSample == 8 || formatChunk.bitsPerSample == 16 || formatChunk.bitsPerSample == 24);
}

unsigned int CAEWaveDecoder::FillBuffer(void* pBuf, unsigned long nLen)
{
	if ( formatChunk.bitsPerSample == 8 )
	{
		if ( formatChunk.numChannels == 2 )
		{
			// Stereo
			if ( nLen / 2 > nLastMallocSize )
			{
				if ( pMalloc )
					operator delete(pMalloc);

				nLastMallocSize = nLen / 2;
				pMalloc = operator new(nLastMallocSize);
			}

			unsigned int		nBytesWritten = GetStream()->FillBuffer(pMalloc, nLen / 2);
			signed short*		pOutBuf = static_cast<signed short*>(pBuf);
			unsigned char*		pInBuf = static_cast<unsigned char*>(pMalloc);

			// Convert to 16-bit
			for ( unsigned int i = 0; i < nBytesWritten; i++ )
			{
				pOutBuf[i] = (static_cast<signed char>(pInBuf[i]) - 128) << 8;
			}

			return nBytesWritten * 2;
		}
		else
		{
			// Mono
			if ( nLen / 4 > nLastMallocSize )
			{
				if ( pMalloc )
					operator delete(pMalloc);

				nLastMallocSize = nLen / 4;
				pMalloc = operator new(nLastMallocSize);
			}

			unsigned int		nBytesWritten = GetStream()->FillBuffer(pMalloc, nLen / 4);
			signed short*		pOutBuf = static_cast<signed short*>(pBuf);
			unsigned char*		pInBuf = static_cast<unsigned char*>(pMalloc);

			// Convert to 16-bit
			for ( unsigned int i = 0; i < nBytesWritten; i++ )
			{
				pOutBuf[i * 2] = pOutBuf[i * 2 + 1] = (static_cast<signed char>(pInBuf[i]) - 128) << 8;
			}

			return nBytesWritten * 4;
		}
	}
	else if ( formatChunk.bitsPerSample == 24 )
	{
		if ( formatChunk.numChannels == 2 )
		{
			// Stereo
			if ( nLen * 3 / 2 > nLastMallocSize )
			{
				if ( pMalloc )
					operator delete(pMalloc);

				nLastMallocSize = nLen * 3 / 2;
				pMalloc = operator new(nLastMallocSize);
			}

			unsigned int		nBytesWritten = GetStream()->FillBuffer(pMalloc, nLen * 3 / 2);
			unsigned char*		pOutBuf = static_cast<unsigned char*>(pBuf);
			unsigned char*		pInBuf = static_cast<unsigned char*>(pMalloc);
			const unsigned int	nNumSamples = nBytesWritten / 3;

			// Convert to 16-bit
			for ( unsigned int i = 0; i < nNumSamples; i++ )
			{
				pOutBuf[i*2] = pInBuf[i*3 + 1];
				pOutBuf[i*2 + 1] = pInBuf[i*3 + 2];
			}

			return nNumSamples * 2;
		}
		else
		{
			if ( nLen * 3 / 4 > nLastMallocSize )
			{
				if ( pMalloc )
					operator delete(pMalloc);

				nLastMallocSize = nLen * 3 / 4;
				pMalloc = operator new(nLastMallocSize);
			}

			unsigned int		nBytesWritten = GetStream()->FillBuffer(pMalloc, nLen * 3 / 4);
			unsigned char*		pOutBuf = static_cast<unsigned char*>(pBuf);
			unsigned char*		pInBuf = static_cast<unsigned char*>(pMalloc);
			const unsigned int	nNumSamples = nBytesWritten / 3;

			// Convert to 16-bit
			for ( unsigned int i = 0; i < nNumSamples; i++ )
			{
				pOutBuf[i*4] = pOutBuf[i*4 + 2] = pInBuf[i*3 + 1];
				pOutBuf[i*4 + 1] = pOutBuf[i*4 + 3] = pInBuf[i*3 + 2];
			}

			return nNumSamples * 4;
		}
	}
	else
	{
		// 16-bit
		if ( formatChunk.numChannels == 2 )
		{
			// Stereo, optimised fetch
			return GetStream()->FillBuffer(pBuf, nLen);
		}
		else
		{
			// Mono
			if ( nLen / 2 > nLastMallocSize )
			{
				if ( pMalloc )
					operator delete(pMalloc);

				nLastMallocSize = nLen / 2;
				pMalloc = operator new(nLastMallocSize);
			}

			unsigned int		nBytesWritten = GetStream()->FillBuffer(pMalloc, nLen / 2);
			signed short*		pOutBuf = static_cast<signed short*>(pBuf);
			signed short*		pInBuf = static_cast<signed short*>(pMalloc);
			const unsigned int	nNumSamples = nBytesWritten / 2;

			for ( unsigned int i = 0; i < nNumSamples; i++ )
			{
				pOutBuf[i*2] = pOutBuf[i*2 + 1] = pInBuf[i];
			}

			return nBytesWritten * 2;
		}
	}
}


FLAC__StreamDecoderReadStatus CAEFLACDecoder::read_cb(const FLAC__StreamDecoder* decoder, FLAC__byte buffer[], size_t* bytes, void* client_data)
{
	UNREFERENCED_PARAMETER(decoder);
	CAEFLACDecoder*	pClientData = static_cast<CAEFLACDecoder*>(client_data);

	ReadFile(pClientData->GetStream()->GetFile(), buffer, *bytes, reinterpret_cast<LPDWORD>(bytes), nullptr); //*bytes = pClientData->GetStream()->FillBuffer(buffer, *bytes);
	//*bytes = pClientData->GetStream()->FillBuffer(buffer, *bytes);
	//bEOFFlag = GetLastError() == ERROR_HANDLE_EOF;

	auto result = *bytes ? FLAC__STREAM_DECODER_READ_STATUS_CONTINUE : FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;

	return result;
}

FLAC__StreamDecoderWriteStatus CAEFLACDecoder::write_cb(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *const buffer[], void *client_data)
{
	CAEFLACDecoder*	pClientData = static_cast<CAEFLACDecoder*>(client_data);

	// Obtain current sample
	pClientData->nCurrentSample = frame->header.number_type == FLAC__FRAME_NUMBER_TYPE_SAMPLE_NUMBER ?
						frame->header.number.sample_number :
						frame->header.number.frame_number * frame->header.blocksize;

	// Mono/stereo?
	unsigned int			nNumChannelsToAlloc = pClientData->pStreamInfo->data.stream_info.channels > 1 ? 2 : 1;

	if ( frame->header.blocksize * sizeof(FLAC__int32) * nNumChannelsToAlloc > nLastMallocSize )
	{
		// Realloc needed
		if ( pMalloc )
			operator delete(pMalloc);

		nLastMallocSize = frame->header.blocksize * sizeof(FLAC__int32) * nNumChannelsToAlloc;
		pMalloc = operator new(nLastMallocSize);	// TODO: More channels?
	}
	nBlockSize = frame->header.blocksize;

	memcpy(pMalloc, buffer[0], nBlockSize * sizeof(FLAC__int32));
	if ( nNumChannelsToAlloc > 1 )
		memcpy(static_cast<FLAC__int32*>(pMalloc)+nBlockSize, buffer[1], nBlockSize * sizeof(FLAC__int32));

	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void CAEFLACDecoder::meta_cb(const FLAC__StreamDecoder* decoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
	if ( metadata->type == FLAC__METADATA_TYPE_STREAMINFO )
	{
		// Cache the header
		CAEFLACDecoder*	pClientData = static_cast<CAEFLACDecoder*>(client_data);
		pClientData->pStreamInfo = FLAC__metadata_object_clone(metadata);
	}
}

FLAC__StreamDecoderSeekStatus CAEFLACDecoder::seek_cb(const FLAC__StreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data)
{
	CAEFLACDecoder*	pClientData = static_cast<CAEFLACDecoder*>(client_data);
	LARGE_INTEGER	li;
	li.QuadPart = absolute_byte_offset;

	li.LowPart = SetFilePointer(pClientData->GetStream()->GetFile(), li.LowPart, &li.HighPart, FILE_BEGIN);

	return li.LowPart == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR ? FLAC__STREAM_DECODER_SEEK_STATUS_ERROR : FLAC__STREAM_DECODER_SEEK_STATUS_OK;
}

FLAC__StreamDecoderTellStatus CAEFLACDecoder::tell_cb(const FLAC__StreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data)
{
	CAEFLACDecoder*	pClientData = static_cast<CAEFLACDecoder*>(client_data);
	LARGE_INTEGER	li;
	li.QuadPart = 0;

	li.LowPart = SetFilePointer(pClientData->GetStream()->GetFile(), 0, &li.HighPart, FILE_CURRENT);
	*absolute_byte_offset = li.QuadPart;

	return li.LowPart == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR ? FLAC__STREAM_DECODER_TELL_STATUS_ERROR : FLAC__STREAM_DECODER_TELL_STATUS_OK;
}

FLAC__StreamDecoderLengthStatus	CAEFLACDecoder::length_cb(const FLAC__StreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data)
{
	CAEFLACDecoder*	pClientData = static_cast<CAEFLACDecoder*>(client_data);
	LARGE_INTEGER	li;

	BOOL bResult = GetFileSizeEx(pClientData->GetStream()->GetFile(), &li);
	*stream_length = li.QuadPart;

	return bResult ? FLAC__STREAM_DECODER_LENGTH_STATUS_OK: FLAC__STREAM_DECODER_LENGTH_STATUS_ERROR;
}

FLAC__bool CAEFLACDecoder::eof_cb(const FLAC__StreamDecoder* decoder, void* client_data)
{
	// Not implemented
	UNREFERENCED_PARAMETER(decoder);
	UNREFERENCED_PARAMETER(client_data);
	return false;
}

void CAEFLACDecoder::error_cb(const FLAC__StreamDecoder* decoder, FLAC__StreamDecoderErrorStatus status, void* client_data)
{
	// Not implemented
	UNREFERENCED_PARAMETER(decoder);
	UNREFERENCED_PARAMETER(status);
	UNREFERENCED_PARAMETER(client_data);
}


bool CAEFLACDecoder::Initialise()
{
	pFLACDecoder = FLAC__stream_decoder_new();
	if ( FLAC__stream_decoder_init_stream(pFLACDecoder, read_cb, seek_cb, tell_cb, length_cb, eof_cb, write_cb, meta_cb, error_cb, this) == FLAC__STREAM_DECODER_INIT_STATUS_OK )
	{
		FLAC__stream_decoder_process_until_end_of_metadata(pFLACDecoder);

		return pStreamInfo->data.stream_info.sample_rate <= 48000 && (pStreamInfo->data.stream_info.bits_per_sample == 8 || pStreamInfo->data.stream_info.bits_per_sample == 16 || pStreamInfo->data.stream_info.bits_per_sample == 24);
	}
	return false;
}

unsigned int CAEFLACDecoder::FillBuffer(void* pBuf, unsigned long nLen)
{
	unsigned int		nBytesDecoded = 0;
	FLAC__int16*		pBuffer = static_cast<FLAC__int16*>(pBuf);

	const unsigned int	nSampleRate = pStreamInfo->data.stream_info.bits_per_sample;
	const bool			bStereo = pStreamInfo->data.stream_info.channels > 1;

	while ( nBytesDecoded < nLen )
	{
		unsigned int		nToWrite;
		// No samples left from a previous fetch?
		if ( !nSamplesLeftToProcess )
		{
			FLAC__stream_decoder_process_single(pFLACDecoder);

			if ( (nLen - nBytesDecoded) / 4 >= nBlockSize )
				nToWrite = nBlockSize;
			else
				nToWrite = (nLen - nBytesDecoded) / 4;

			nSamplesLeftToProcess = nBlockSize;
		}
		else
			nToWrite = nSamplesLeftToProcess;

		FLAC__int32*		pCurrentPtr[2] = { static_cast<FLAC__int32*>(pMalloc), static_cast<FLAC__int32*>(pMalloc)+nBlockSize };
		const unsigned int	ExtraIndex = nBlockSize - nSamplesLeftToProcess;

		// Write channels
		if ( nSampleRate == 8 )
		{
			// 8-bit
			if ( bStereo )
			{
				for ( unsigned int i = 0; i < nToWrite; i++, nSamplesLeftToProcess-- )
				{
					pBuffer[0] = pCurrentPtr[0][ExtraIndex+i] << 8;
					pBuffer[1] = pCurrentPtr[1][ExtraIndex+i] << 8;

					pBuffer += 2;
				}
			}
			else
			{
				for ( unsigned int i = 0; i < nToWrite; i++, nSamplesLeftToProcess-- )
				{
					pBuffer[0] = pBuffer[1] = pCurrentPtr[0][ExtraIndex+i] << 8;

					pBuffer += 2;
				}
			}
		}
		else if ( nSampleRate == 24 )
		{
			// 24-bit
			if ( bStereo )
			{
				for ( unsigned int i = 0; i < nToWrite; i++, nSamplesLeftToProcess-- )
				{
					pBuffer[0] = pCurrentPtr[0][ExtraIndex+i] >> 8;
					pBuffer[1] = pCurrentPtr[1][ExtraIndex+i] >> 8;

					pBuffer += 2;
				}
			}
			else
			{
				for ( unsigned int i = 0; i < nToWrite; i++, nSamplesLeftToProcess-- )
				{
					pBuffer[0] = pBuffer[1] = pCurrentPtr[0][ExtraIndex+i] >> 8;

					pBuffer += 2;
				}
			}
		}
		else
		{
			// 16-bit
			if ( bStereo )
			{
				for ( unsigned int i = 0; i < nToWrite; i++, nSamplesLeftToProcess-- )
				{
					pBuffer[0] = pCurrentPtr[0][ExtraIndex+i];
					pBuffer[1] = pCurrentPtr[1][ExtraIndex+i];

					pBuffer += 2;
				}
			}
			else
			{
				for ( unsigned int i = 0; i < nToWrite; i++, nSamplesLeftToProcess-- )
				{
					pBuffer[0] = pBuffer[1] = pCurrentPtr[0][ExtraIndex+i];

					pBuffer += 2;
				}
			}
		}

		nBytesDecoded += nToWrite*4;

		if ( FLAC__stream_decoder_get_state(pFLACDecoder) == FLAC__STREAM_DECODER_END_OF_STREAM )
			break;
	}
	return nBytesDecoded;
}

CAEFLACDecoder::~CAEFLACDecoder()
{
	nSamplesLeftToProcess = 0;
	if ( pFLACDecoder )
	{
		FLAC__stream_decoder_finish(pFLACDecoder);
		FLAC__stream_decoder_delete(pFLACDecoder);

		FLAC__metadata_object_delete(pStreamInfo);
		pFLACDecoder = nullptr;
	}
}