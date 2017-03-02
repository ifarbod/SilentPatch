#pragma once

#include "AudioHardwareSA.h"

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
	virtual uint32_t		FillBuffer(void* pBuf, uint32_t nLen) override;

	virtual uint32_t		GetStreamLengthMs() override
	{ return static_cast<unsigned long long>(nDataSize) * 8000 / (formatChunk.sampleRate*formatChunk.bitsPerSample*formatChunk.numChannels); }
	virtual uint32_t		GetStreamPlayTimeMs() override
	{ return static_cast<unsigned long long>(GetStream()->GetCurrentPosition() - nOffsetToData) * 8000 / (formatChunk.sampleRate*formatChunk.bitsPerSample*formatChunk.numChannels); }		

	virtual void			SetCursor(uint32_t nTime) override
	{	auto nPos = static_cast<unsigned long long>(nTime) * (formatChunk.sampleRate*formatChunk.bitsPerSample*formatChunk.numChannels) /  8000;
	auto nModulo = (formatChunk.numChannels*formatChunk.bitsPerSample/8);
	auto nExtra = nPos % nModulo ? nModulo - (nPos % nModulo) : 0;
	GetStream()->Seek(nOffsetToData + nPos + nExtra, FILE_BEGIN); }

	virtual uint32_t		GetSampleRate() override
	{ return formatChunk.sampleRate; }

	virtual uint32_t		GetStreamID() override
	{ return GetStream()->GetID(); }
};