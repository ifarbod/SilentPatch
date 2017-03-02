#include "StdAfxSA.h"
#include "WaveDecoderSA.h"

extern void*			pMalloc;
extern unsigned int		nBlockSize;
extern unsigned int		nLastMallocSize;

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

uint32_t CAEWaveDecoder::FillBuffer(void* pBuf, uint32_t nLen)
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