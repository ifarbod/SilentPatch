#pragma once

template<size_t N>
inline HMODULE GetASIModuleHandleA( const char(&lpModuleName)[N] )
{
	HMODULE asi = GetModuleHandleA( lpModuleName );
	if ( asi == nullptr )
	{
		char nameWithSuffix[ N + 4 ];
		memcpy( nameWithSuffix, lpModuleName, sizeof(lpModuleName) - sizeof(lpModuleName[0]) );
		nameWithSuffix[N - 1] = '.';
		nameWithSuffix[N + 0] = 'a';
		nameWithSuffix[N + 1] = 's';
		nameWithSuffix[N + 2] = 'i';
		nameWithSuffix[N + 3] = '\0';
		asi = GetModuleHandleA( nameWithSuffix );
	}
	return asi;
}

template<size_t N>
inline HMODULE GetASIModuleHandleW( const wchar_t(&lpModuleName)[N] )
{
	HMODULE asi = GetModuleHandleW( lpModuleName );
	if ( asi == nullptr )
	{
		wchar_t nameWithSuffix[ N + 4 ];
		memcpy( nameWithSuffix, lpModuleName, sizeof(lpModuleName) - sizeof(lpModuleName[0]) );
		nameWithSuffix[N - 1] = L'.';
		nameWithSuffix[N + 0] = L'a';
		nameWithSuffix[N + 1] = L's';
		nameWithSuffix[N + 2] = L'i';
		nameWithSuffix[N + 3] = L'\0';
		asi = GetModuleHandleW( nameWithSuffix );
	}
	return asi;
}

#ifdef _UNICODE
#define GetASIModuleHandle GetASIModuleHandleW
#else
#define GetASIModuleHandle GetASIModuleHandleA
#endif