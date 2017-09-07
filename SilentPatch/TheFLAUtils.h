#pragma once

#include <cstdint>

class FLAUtils
{
public:
	class int8
	{
	private:
		int8() = delete;
		int8& operator =( const int8& ) = delete;

		uint8_t value;
	};

	class int16
	{
	private:
		int16() = delete;
		int16& operator =( const int16& ) = delete;

		uint16_t value;
	};

	static_assert( sizeof(int8) == sizeof(uint8_t) );
	static_assert( sizeof(int16) == sizeof(uint16_t) );

	static int32_t GetExtendedID(const int8* ptr)
	{
		return GetExtendedID8Func(ptr);
	}

	static int32_t GetExtendedID(const int16* ptr)
	{
		return GetExtendedID16Func(ptr);
	}

	static void Init();

private:
	static constexpr int32_t MAX_UINT8_ID = 0xFF;
	static constexpr int32_t MAX_UINT16_ID = 0xFFFD;

	static int32_t GetExtendedID8_Stock(const void* ptr)
	{
		uint8_t uID = *static_cast<const uint8_t*>(ptr);
		return uID == MAX_UINT8_ID ? -1 : uID;
	}

	static int32_t GetExtendedID16_Stock(const void* ptr)
	{
		uint16_t uID = *static_cast<const uint16_t*>(ptr);
		return uID > MAX_UINT16_ID ? *static_cast<const int16_t*>(ptr) : uID;
	}

	static int32_t (*GetExtendedID8Func)(const void* ptr);
	static int32_t (*GetExtendedID16Func)(const void* ptr);
};