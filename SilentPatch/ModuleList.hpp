#pragma once

#include <vector>
#include <algorithm>
#include <cassert>

#define PSAPI_VERSION 1
#include <Psapi.h>
#include <malloc.h>

#pragma comment(lib, "Psapi.lib")

// Stores a list of loaded modules with their names, WITHOUT extension
class ModuleList
{
public:
	void Enumerate()
	{
		const HANDLE currentProcess = GetCurrentProcess();

		constexpr size_t INITIAL_SIZE = sizeof(HMODULE) * 256;
		HMODULE* modules = static_cast<HMODULE*>(malloc( INITIAL_SIZE ));
		if ( modules != nullptr )
		{
			DWORD cbNeeded = 0;
			if ( EnumProcessModules( currentProcess, modules, INITIAL_SIZE, &cbNeeded ) != 0 )
			{
				if ( cbNeeded > INITIAL_SIZE )
				{
					HMODULE* newModules = static_cast<HMODULE*>(realloc( modules, cbNeeded ));
					if ( newModules != nullptr )
					{
						modules = newModules;

						if ( EnumProcessModules( currentProcess, modules, cbNeeded, &cbNeeded ) != 0 )
						{
							EnumerateInternal( modules, cbNeeded / sizeof(HMODULE) );
						}
					}
				}
				else
				{
					EnumerateInternal( modules, cbNeeded / sizeof(HMODULE) );
				}
			}
			free( modules );
		}
	}

	void ReEnumerate()
	{
		Clear();
		Enumerate();
	}

	void Clear()
	{
		m_moduleList.clear();
	}

	HMODULE Get( const wchar_t* moduleName ) const
	{
		// If vector is empty then we're trying to call it without calling Enumerate first
		assert( m_moduleList.size() != 0 );

		auto it = std::find_if( m_moduleList.begin(), m_moduleList.end(), [&]( const auto& e ) {
			return _wcsicmp( moduleName, e.second.c_str() ) == 0;
		} );
		return it != m_moduleList.end() ? it->first : nullptr;
	}

	std::vector<HMODULE> GetAll( const wchar_t* moduleName ) const
	{
		// If vector is empty then we're trying to call it without calling Enumerate first
		assert( m_moduleList.size() != 0 );

		std::vector<HMODULE> results;
		for ( auto& e : m_moduleList )
		{
			if ( _wcsicmp( moduleName, e.second.c_str() ) == 0 )
			{
				results.push_back( e.first );
			}
		}

		return results;
	}

private:
	void EnumerateInternal( HMODULE* modules, size_t numModules )
	{
		m_moduleList.reserve( numModules );
		for ( size_t i = 0; i < numModules; i++ )
		{
			wchar_t moduleName[MAX_PATH];
			if ( GetModuleFileNameW( *modules, moduleName, MAX_PATH ) != 0 )
			{
				const wchar_t* nameBegin = wcsrchr( moduleName, '\\' ) + 1;
				const wchar_t* dotPos = wcsrchr( nameBegin, '.' );
				if ( dotPos != nullptr )
				{
					m_moduleList.emplace_back( *modules, std::wstring( nameBegin, std::distance( nameBegin, dotPos ) ) );
				}
				else
				{
					m_moduleList.emplace_back( *modules,  nameBegin );
				}
			}
			modules++;
		}
	}

	std::vector< std::pair<HMODULE, std::wstring> > m_moduleList;
};