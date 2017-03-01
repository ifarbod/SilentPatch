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