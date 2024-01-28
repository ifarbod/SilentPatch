#include "StdAfxSA.h"
#include "FLACDecoderSA.h"
#include <algorithm>

FLAC__StreamDecoderReadStatus CAEFLACDecoder::read_cb(const FLAC__StreamDecoder* decoder, FLAC__byte buffer[], size_t* bytes, void* client_data)
{
	UNREFERENCED_PARAMETER(decoder);
	CAEFLACDecoder*	pClientData = static_cast<CAEFLACDecoder*>(client_data);

	DWORD size = *bytes;
	BOOL result = ReadFile(pClientData->GetStream()->GetFile(), buffer, size, &size, nullptr);
	*bytes = size;

	if ( result == FALSE )
		return FLAC__STREAM_DECODER_READ_STATUS_ABORT;

	if ( size == 0 )
	{
		pClientData->m_eof = true;
		return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
	}
	pClientData->m_eof = false;
	return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
}

FLAC__StreamDecoderWriteStatus CAEFLACDecoder::write_cb(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *const buffer[], void *client_data)
{
	UNREFERENCED_PARAMETER(decoder);
	CAEFLACDecoder*	pClientData = static_cast<CAEFLACDecoder*>(client_data);
	size_t processedChannels;

	// Obtain current sample
	assert( frame->header.number_type == FLAC__FRAME_NUMBER_TYPE_SAMPLE_NUMBER );
	pClientData->m_currentSample = frame->header.number.sample_number;
	processedChannels = std::min<size_t>(2, frame->header.channels);
	pClientData->m_curBlockSize = frame->header.blocksize;
	pClientData->m_bufferCursor = 0;

	if ( pClientData->m_curBlockSize > pClientData->m_maxBlockSize )
	{
		delete[] pClientData->m_buffer;
		pClientData->m_buffer = new FLAC__int32[pClientData->m_curBlockSize * processedChannels];
		pClientData->m_maxBlockSize = pClientData->m_curBlockSize;
	}
	
	auto it = std::copy_n( buffer[0], pClientData->m_curBlockSize, pClientData->m_buffer );
	if ( processedChannels > 1 )
	{
		std::copy_n( buffer[1], pClientData->m_curBlockSize, it );
	}

	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void CAEFLACDecoder::meta_cb(const FLAC__StreamDecoder* decoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
	UNREFERENCED_PARAMETER(decoder);
	if ( metadata->type == FLAC__METADATA_TYPE_STREAMINFO )
	{
		// Cache the header
		CAEFLACDecoder*	pClientData = static_cast<CAEFLACDecoder*>(client_data);
		pClientData->m_streamMeta = FLAC__metadata_object_clone(metadata);
		assert( pClientData->m_streamMeta != nullptr );
	}
}

FLAC__StreamDecoderSeekStatus CAEFLACDecoder::seek_cb(const FLAC__StreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data)
{
	UNREFERENCED_PARAMETER(decoder);
	CAEFLACDecoder*	pClientData = static_cast<CAEFLACDecoder*>(client_data);
	LARGE_INTEGER	li;
	li.QuadPart = absolute_byte_offset;

	return SetFilePointerEx(pClientData->GetStream()->GetFile(), li, nullptr, FILE_BEGIN) != 0 ? FLAC__STREAM_DECODER_SEEK_STATUS_OK : FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
}

FLAC__StreamDecoderTellStatus CAEFLACDecoder::tell_cb(const FLAC__StreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data)
{
	UNREFERENCED_PARAMETER(decoder);
	CAEFLACDecoder*	pClientData = static_cast<CAEFLACDecoder*>(client_data);
	LARGE_INTEGER	li;
	li.QuadPart = 0;

	BOOL result = SetFilePointerEx(pClientData->GetStream()->GetFile(), li, &li, FILE_CURRENT);
	*absolute_byte_offset = li.QuadPart;

	return result != 0 ? FLAC__STREAM_DECODER_TELL_STATUS_OK : FLAC__STREAM_DECODER_TELL_STATUS_ERROR;
}

FLAC__StreamDecoderLengthStatus	CAEFLACDecoder::length_cb(const FLAC__StreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data)
{
	UNREFERENCED_PARAMETER(decoder);
	CAEFLACDecoder*	pClientData = static_cast<CAEFLACDecoder*>(client_data);
	LARGE_INTEGER	li;

	BOOL bResult = GetFileSizeEx(pClientData->GetStream()->GetFile(), &li);
	*stream_length = li.QuadPart;

	return bResult ? FLAC__STREAM_DECODER_LENGTH_STATUS_OK: FLAC__STREAM_DECODER_LENGTH_STATUS_ERROR;
}

FLAC__bool CAEFLACDecoder::eof_cb(const FLAC__StreamDecoder* decoder, void* client_data)
{
	UNREFERENCED_PARAMETER(decoder);
	CAEFLACDecoder*	pClientData = static_cast<CAEFLACDecoder*>(client_data);
	return pClientData->m_eof;
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
	m_FLACdecoder = FLAC__stream_decoder_new();
	assert( m_FLACdecoder != nullptr );
	if ( m_FLACdecoder == nullptr )
		return false;

	if ( FLAC__stream_decoder_init_stream(m_FLACdecoder, read_cb, seek_cb, tell_cb, length_cb, eof_cb, write_cb, meta_cb, error_cb, this) != FLAC__STREAM_DECODER_INIT_STATUS_OK )
		return false;

	if ( FLAC__stream_decoder_process_until_end_of_metadata(m_FLACdecoder) == false )
		return false;

	if ( m_streamMeta == nullptr )
		return false;

	m_eof = false;
	return m_streamMeta->data.stream_info.sample_rate <= 48000 && (m_streamMeta->data.stream_info.bits_per_sample == 8 || m_streamMeta->data.stream_info.bits_per_sample == 16 || m_streamMeta->data.stream_info.bits_per_sample == 24);
}

uint32_t CAEFLACDecoder::FillBuffer(void* pBuf, uint32_t nLen)
{
	uint32_t		samplesToDecode = nLen / (2 * sizeof(int16_t));
	uint32_t		bytesDecoded = 0;
	int16_t*		outputBuffer = static_cast<int16_t*>(pBuf);
	FLAC__int32*	inputBuffer[] = { m_buffer+m_bufferCursor, m_buffer+m_bufferCursor+m_curBlockSize };

	const uint32_t	sampleWidth = m_streamMeta->data.stream_info.bits_per_sample;
	const bool		stereo = m_streamMeta->data.stream_info.channels > 1;

	while ( bytesDecoded < nLen )
	{
		if ( m_bufferCursor >= m_curBlockSize )
		{
			// New FLAC frame needed
			if ( FLAC__stream_decoder_get_state(m_FLACdecoder) == FLAC__STREAM_DECODER_END_OF_STREAM )
				break;

			FLAC__stream_decoder_process_single(m_FLACdecoder);
			inputBuffer[0] = m_buffer;
			inputBuffer[1] = m_buffer+m_curBlockSize;
		}

		size_t samplesThisIteration = std::min( m_curBlockSize-m_bufferCursor, samplesToDecode );
		if ( sampleWidth == 8 )
		{
			if ( stereo )
			{
				for ( size_t i = 0; i < samplesThisIteration; ++i )
				{
					outputBuffer[0] = int16_t(*inputBuffer[0]++ << 8);
					outputBuffer[1] = int16_t(*inputBuffer[1]++ << 8);
					outputBuffer += 2;
				}
			}
			else
			{
				for ( size_t i = 0; i < samplesThisIteration; ++i )
				{
					outputBuffer[0] = outputBuffer[1] = int16_t(*inputBuffer[0]++ << 8);
					outputBuffer += 2;
				}
			}
		}
		else if ( sampleWidth == 24 )
		{
			// 24-bit
			if ( stereo )
			{
				for ( size_t i = 0; i < samplesThisIteration; ++i )
				{
					outputBuffer[0] = int16_t(*inputBuffer[0]++ >> 8);
					outputBuffer[1] = int16_t(*inputBuffer[1]++ >> 8);
					outputBuffer += 2;
				}
			}
			else
			{
				for ( size_t i = 0; i < samplesThisIteration; ++i )
				{
					outputBuffer[0] = outputBuffer[1] = int16_t(*inputBuffer[0]++ >> 8);
					outputBuffer += 2;
				}
			}
		}
		else
		{
			if ( stereo )
			{
				for ( size_t i = 0; i < samplesThisIteration; ++i )
				{
					outputBuffer[0] = int16_t(*inputBuffer[0]++);
					outputBuffer[1] = int16_t(*inputBuffer[1]++);
					outputBuffer += 2;
				}
			}
			else
			{
				for ( size_t i = 0; i < samplesThisIteration; ++i )
				{
					outputBuffer[0] = outputBuffer[1] = int16_t(*inputBuffer[0]++);
					outputBuffer += 2;
				}
			}
		}

		m_currentSample += samplesThisIteration;
		m_bufferCursor += samplesThisIteration;
		samplesToDecode -= samplesThisIteration;
		bytesDecoded += samplesThisIteration * 2 * sizeof(int16_t);
	}
	return bytesDecoded;
}

CAEFLACDecoder::~CAEFLACDecoder()
{
	if ( m_FLACdecoder != nullptr )
	{
		FLAC__stream_decoder_finish(m_FLACdecoder);
		FLAC__stream_decoder_delete(m_FLACdecoder);

		if ( m_streamMeta != nullptr )
			FLAC__metadata_object_delete(m_streamMeta);
	}
	delete[] m_buffer;
}