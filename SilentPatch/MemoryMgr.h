#ifndef __MEMORYMGR
#define __MEMORYMGR

// Switches:
// _MEMORY_NO_CRT - don't include anything "complex" like ScopedUnprotect or memset
// _MEMORY_DECLS_ONLY - don't include anything but macroes

#define WRAPPER __declspec(naked)
#define DEPRECATED __declspec(deprecated)
#define EAXJMP(a) { _asm mov eax, a _asm jmp eax }
#define VARJMP(a) { _asm jmp a }
#define WRAPARG(a) ((int)a)

#define NOVMT __declspec(novtable)
#define SETVMT(a) *((uintptr_t*)this) = (uintptr_t)a

#ifndef _MEMORY_DECLS_ONLY

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <cstdint>

#ifndef _MEMORY_NO_CRT
#include <initializer_list>
#include <iterator>
#endif

enum
{
	PATCH_CALL,
	PATCH_JUMP
};

namespace Memory
{
	namespace internal
	{
		inline signed char* GetVer()
		{
			static signed char	bVer = -1;
			return &bVer;
		}

		inline bool* GetEuropean()
		{
			static bool			bEuropean;
			return &bEuropean;
		}

		inline uintptr_t GetDummy()
		{
			static uintptr_t		dwDummy;
			return reinterpret_cast<uintptr_t>(&dwDummy);
		}
	}
}

template<typename AT>
inline AT DynBaseAddress(AT address)
{
	return (ptrdiff_t)GetModuleHandle(nullptr) - 0x400000 + address;
}

namespace Memory
{
	namespace internal
	{
#if defined _GTA_III
		inline void InitializeVersions()
		{
			signed char*	bVer = GetVer();

			if ( *bVer == -1 )
			{
				if (*(uint32_t*)0x5C1E75 == 0xB85548EC) *bVer = 0;
				else if (*(uint32_t*)0x5C2135 == 0xB85548EC) *bVer = 1;
				else if (*(uint32_t*)0x5C6FD5 == 0xB85548EC) *bVer = 2;
			}
		}

		// This function initially detects III version then chooses the address basing on game version
		inline uintptr_t AddressByVersion(uintptr_t address10, uintptr_t address11, uintptr_t addressSteam)
		{
			InitializeVersions();

			signed char		bVer = *GetVer();

			switch ( bVer )
			{
			case 1:
		#ifdef assert
				assert(address11);
		#endif
				return address11;
			case 2:
		#ifdef assert
				assert(addressSteam);
		#endif
				return addressSteam;
			default:
		#ifdef assert
				assert(address10);
		#endif
				return address10;
			}
		}

#elif defined _GTA_VC

		inline void InitializeVersions()
		{
			signed char*	bVer = GetVer();

			if ( *bVer == -1 )
			{
				if (*(uint32_t*)0x667BF5 == 0xB85548EC) *bVer = 0;
				else if (*(uint32_t*)0x667C45 == 0xB85548EC) *bVer = 1;
				else if (*(uint32_t*)0x666BA5 == 0xB85548EC) *bVer = 2;
			}
		}

		// This function initially detects VC version then chooses the address basing on game version
		inline uintptr_t AddressByVersion(uintptr_t address10, uintptr_t address11, uintptr_t addressSteam)
		{
			InitializeVersions();

			signed char	bVer = *GetVer();

			switch ( bVer )
			{
			case 1:
		#ifdef assert
				assert(address11);
		#endif
				return address11;
			case 2:
		#ifdef assert
				assert(addressSteam);
		#endif
				return addressSteam;
			default:
		#ifdef assert
				assert(address10);
		#endif
				return address10;
			}
		}

#elif defined _GTA_SA

		inline bool TryMatch_10()
		{
			if ( *(uint32_t*)DynBaseAddress(0x82457C) == 0x94BF )
			{
				// 1.0 US
				*GetVer() = 0;
				*GetEuropean() = false;
				return true;
			}
			if ( *(uint32_t*)DynBaseAddress(0x8245BC) == 0x94BF )
			{
				// 1.0 EU
				*GetVer() = 0;
				*GetEuropean() = true;
				return true;
			}
			return false;
		}

		inline bool TryMatch_11()
		{
			if ( *(uint32_t*)DynBaseAddress(0x8252FC) == 0x94BF )
			{
				// 1.01 US
				*GetVer() = 1;
				*GetEuropean() = false;
				return true;
			}
			if ( *(uint32_t*)DynBaseAddress(0x82533C) == 0x94BF )
			{
				// 1.01 EU
				*GetVer() = 1;
				*GetEuropean() = true;
				return true;
			}
			return false;
		}

		inline bool TryMatch_30()
		{
			if (*(uint32_t*)DynBaseAddress(0x85EC4A) == 0x94BF )
			{
				// 3.0
				*GetVer() = 2;
				*GetEuropean() = false;
				return true;
			}
			return false;
		}

		inline bool TryMatch_newsteam_r1()
		{
			if ( *(uint32_t*)DynBaseAddress(0x858D21) == 0x3539F633 )
			{
				// newsteam r1
				*GetVer() = 3;
				*GetEuropean() = false;
				return true;
			}
			return false;
		}

		inline bool TryMatch_newsteam_r2()
		{
			if ( *(uint32_t*)DynBaseAddress(0x858D51) == 0x3539F633 )
			{
				// newsteam r2
				*GetVer() = 4;
				*GetEuropean() = false;
				return true;
			}
			return false;
		}

		inline bool TryMatch_newsteam_r2_lv()
		{
			if ( *(uint32_t*)DynBaseAddress(0x858C61) == 0x3539F633 )
			{
				// newsteam r2 lv
				*GetVer() = 5;
				*GetEuropean() = false;
				return true;
			}
			return false;
		}

		inline void InitializeVersions()
		{
			if ( *GetVer() == -1 )
			{
				if ( TryMatch_10() ) return;
				if ( TryMatch_11() ) return;
				if ( TryMatch_30() ) return;
				if ( TryMatch_newsteam_r1() ) return;
				if ( TryMatch_newsteam_r2() ) return;
				if ( TryMatch_newsteam_r2_lv() ) return;	
			}
		}

		inline void InitializeRegion_10()
		{
			signed char*	bVer = GetVer();

			if ( *bVer == -1 )
			{
				if ( !TryMatch_10() )
				{
		#ifdef assert
					assert(!"AddressByRegion_10 on non-1.0 EXE!");
		#endif
				}
			}
		}

		inline void InitializeRegion_11()
		{
			signed char*	bVer = GetVer();

			if ( *bVer == -1 )
			{
				if ( !TryMatch_11() )
				{
		#ifdef assert
					assert(!"AddressByRegion_11 on non-1.01 EXE!");
		#endif
				}
			}
		}

		inline uintptr_t AdjustAddress_10(uintptr_t address10)
		{
			if ( *GetEuropean() && address10 > 0x7466D0 )
			{
				if ( address10 < 0x7BA940 )
					address10 += 0x50;
				else
					address10 += 0x40;
			}
			return address10;
		}

		inline uintptr_t AdjustAddress_11(uintptr_t address11)
		{
			if ( !(*GetEuropean()) && address11 > 0x746FA0 )
			{
				if ( address11 < 0x7BB240 )
					address11 -= 0x50;
				else
					address11 -= 0x40;
			}
			return address11;
		}

		inline uintptr_t AddressByVersion(uintptr_t address10, uintptr_t address11, uintptr_t addressSteam, uintptr_t addressNewsteamR2, uintptr_t addressNewsteamR2_LV)
		{
			InitializeVersions();

			signed char	bVer = *GetVer();

			switch ( bVer )
			{
			case 1:
		#ifdef assert
				assert(address11);
		#endif

				// Safety measures - if null, return dummy var pointer to prevent a crash
				if ( address11 == 0 )
					return GetDummy();

				// Adjust to US if needed
				return AdjustAddress_11(address11);
			case 2:
		#ifdef assert
				assert(addressSteam);
		#endif
				// Safety measures - if null, return dummy var pointer to prevent a crash
				if ( addressSteam == 0 )
					return GetDummy();

				return addressSteam;
			case 3:
				return GetDummy();
			case 4:
		#ifdef assert
				assert(addressNewsteamR2);
		#endif
				if ( addressNewsteamR2 == 0 )
					return GetDummy();

				return DynBaseAddress(addressNewsteamR2);
			case 5:
		#ifdef assert
				assert(addressNewsteamR2_LV);
		#endif
				if ( addressNewsteamR2_LV == 0 )
					return GetDummy();

				return DynBaseAddress(addressNewsteamR2_LV);
			default:
		#ifdef assert
				assert(address10);
		#endif
				// Adjust to EU if needed
				return AdjustAddress_10(address10);
			}
		}

		inline uintptr_t AddressByRegion_10(uintptr_t address10)
		{
			InitializeRegion_10();

			// Adjust to EU if needed
			return AdjustAddress_10(address10);
		}

		inline uintptr_t AddressByRegion_11(uintptr_t address11)
		{
			InitializeRegion_11();

			// Adjust to US if needed
			return AdjustAddress_11(address11);
		}

#else

		inline void InitializeVersions()
		{
		}

#endif

	}
}

#if defined _GTA_III || defined _GTA_VC

template<typename T>
inline T AddressByVersion(uintptr_t address10, uintptr_t address11, uintptr_t addressSteam)
{
	return T(Memory::internal::AddressByVersion( address10, address11, addressSteam ));
}

#elif defined _GTA_SA

template<typename T>
inline T AddressByVersion(uintptr_t address10, uintptr_t address11, uintptr_t addressSteam)
{
	return T(Memory::internal::AddressByVersion( address10, address11, addressSteam, 0, 0 ));
}

template<typename T>
inline T AddressByVersion(uintptr_t address10, uintptr_t address11, uintptr_t addressSteam, uintptr_t addressNewsteamR2, uintptr_t addressNewsteamR2_LV)
{
	return T(Memory::internal::AddressByVersion( address10, address11, addressSteam, addressNewsteamR2, addressNewsteamR2_LV ));
}

template<typename T>
inline T AddressByRegion_10(uintptr_t address10)
{
	return T(Memory::internal::AddressByRegion_10(address10));
}

template<typename T>
inline T AddressByRegion_11(uintptr_t address11)
{
	return T(Memory::internal::AddressByRegion_11(address11));
}

#endif

namespace Memory
{
	struct VersionInfo
	{
		int8_t version;
		bool european;
	};

	inline VersionInfo GetVersion()
	{
		Memory::internal::InitializeVersions();
		return { *Memory::internal::GetVer(), *Memory::internal::GetEuropean() };
	}
};

namespace Memory
{
	template<typename T, typename AT>
	inline void		Patch(AT address, T value)
	{*(T*)address = value; }

#ifndef _MEMORY_NO_CRT
	template<typename AT>
	inline void		Patch(AT address, std::initializer_list<uint8_t> list )
	{
		uint8_t* const addr = (uint8_t*)address;
		std::copy( list.begin(), list.end(), stdext::make_checked_array_iterator(addr, list.size()) );
	}
#endif

	template<typename AT>
	inline void		Nop(AT address, size_t count)
#ifndef _MEMORY_NO_CRT
	{ memset((void*)address, 0x90, count); }
#else
	{ do {
		*(uint8_t*)address++ = 0x90;
	} while ( --count != 0 ); }
#endif

	template<typename AT, typename HT>
	inline void		InjectHook(AT address, HT hook)
	{
		intptr_t		dwHook;
		_asm
		{
			mov		eax, hook
			mov		dwHook, eax
		}

		*(ptrdiff_t*)((intptr_t)address + 1) = dwHook - (intptr_t)address - 5;
	}

	template<typename AT, typename HT>
	inline void		InjectHook(AT address, HT hook, unsigned int nType)
	{
		intptr_t		dwHook;
		_asm
		{
			mov		eax, hook
			mov		dwHook, eax
		}

		*(uint8_t*)address = nType == PATCH_JUMP ? 0xE9 : 0xE8;

		*(ptrdiff_t*)((intptr_t)address + 1) = dwHook - (intptr_t)address - 5;
	}

	template<typename Func, typename AT>
	inline void		ReadCall(AT address, Func& func)
	{
		func = Func(*(ptrdiff_t*)((intptr_t)address+1) + (intptr_t)address + 5);
	}

#ifndef _MEMORY_NO_CRT
	inline bool MemEquals(uintptr_t address, std::initializer_list<uint8_t> val)
	{
		const uint8_t* mem = reinterpret_cast<const uint8_t*>(address);
		return std::equal( val.begin(), val.end(), stdext::make_checked_array_iterator(mem, val.size()) );
	}
#endif

	namespace DynBase
	{
		template<typename T, typename AT>
		inline void		Patch(AT address, T value)
		{
			Memory::Patch(DynBaseAddress(address), value);
		}

#ifndef _MEMORY_NO_CRT
		template<typename AT>
		inline void		Patch(AT address, std::initializer_list<uint8_t> list )
		{
			Memory::Patch(DynBaseAddress(address), std::move(list));
		}
#endif

		template<typename AT>
		inline void		Nop(AT address, size_t count)
		{
			Memory::Nop(DynBaseAddress(address), count);
		}

		template<typename AT, typename HT>
		inline void		InjectHook(AT address, HT hook)
		{
			Memory::InjectHook(DynBaseAddress(address), hook);
		}

		template<typename AT, typename HT>
		inline void		InjectHook(AT address, HT hook, unsigned int nType)
		{
			Memory::InjectHook(DynBaseAddress(address), hook, nType);
		}

		template<typename Func, typename AT>
		inline void		ReadCall(AT address, Func& func)
		{
			Memory::ReadCall(DynBaseAddress(address), func);
		}

#ifndef _MEMORY_NO_CRT
		inline bool MemEquals(uintptr_t address, std::initializer_list<uint8_t> val)
		{
			return Memory::MemEquals(DynBaseAddress(address), std::move(val));
		}
#endif
	};

	namespace VP
	{
		template<typename T, typename AT>
		inline void		Patch(AT address, T value)
		{
			DWORD		dwProtect[2];
			VirtualProtect((void*)address, sizeof(T), PAGE_EXECUTE_READWRITE, &dwProtect[0]);
			Memory::Patch( address, value );
			VirtualProtect((void*)address, sizeof(T), dwProtect[0], &dwProtect[1]);
		}

#ifndef _MEMORY_NO_CRT
		template<typename AT>
		inline void		Patch(AT address, std::initializer_list<uint8_t> list )
		{
			DWORD		dwProtect[2];
			VirtualProtect((void*)address, list.size(), PAGE_EXECUTE_READWRITE, &dwProtect[0]);
			Memory::Patch(address, std::move(list));
			VirtualProtect((void*)address, list.size(), dwProtect[0], &dwProtect[1]);
		}
#endif

		template<typename AT>
		inline void		Nop(AT address, size_t count)
		{
			DWORD		dwProtect[2];
			VirtualProtect((void*)address, count, PAGE_EXECUTE_READWRITE, &dwProtect[0]);
			Memory::Nop( address, count );
			VirtualProtect((void*)address, count, dwProtect[0], &dwProtect[1]);
		}

		template<typename AT, typename HT>
		inline void		InjectHook(AT address, HT hook)
		{
			DWORD		dwProtect[2];

			VirtualProtect((void*)((DWORD)address + 1), 4, PAGE_EXECUTE_READWRITE, &dwProtect[0]);
			Memory::InjectHook( address, hook );
			VirtualProtect((void*)((DWORD)address + 1), 4, dwProtect[0], &dwProtect[1]);
		}

		template<typename AT, typename HT>
		inline void		InjectHook(AT address, HT hook, unsigned int nType)
		{
			DWORD		dwProtect[2];

			VirtualProtect((void*)address, 5, PAGE_EXECUTE_READWRITE, &dwProtect[0]);
			Memory::InjectHook( address, hook, nType );
			VirtualProtect((void*)address, 5, dwProtect[0], &dwProtect[1]);
		}

		template<typename Func, typename AT>
		inline void		ReadCall(AT address, Func& func)
		{
			Memory::ReadCall(address, func);
		}

#ifndef _MEMORY_NO_CRT
		inline bool MemEquals(uintptr_t address, std::initializer_list<uint8_t> val)
		{
			return Memory::MemEquals(address, std::move(val));
		}
#endif

		namespace DynBase
		{
			template<typename T, typename AT>
			inline void		Patch(AT address, T value)
			{
				VP::Patch(DynBaseAddress(address), value);
			}

#ifndef _MEMORY_NO_CRT
			template<typename AT>
			inline void		Patch(AT address, std::initializer_list<uint8_t> list )
			{
				VP::Patch(DynBaseAddress(address), std::move(list));
			}
#endif

			template<typename AT>
			inline void		Nop(AT address, size_t count)
			{
				VP::Nop(DynBaseAddress(address), count);
			}

			template<typename AT, typename HT>
			inline void		InjectHook(AT address, HT hook)
			{
				VP::InjectHook(DynBaseAddress(address), hook);
			}

			template<typename AT, typename HT>
			inline void		InjectHook(AT address, HT hook, unsigned int nType)
			{
				VP::InjectHook(DynBaseAddress(address), hook, nType);
			}

			template<typename Func, typename AT>
			inline void		ReadCall(AT address, Func& func)
			{
				Memory::ReadCall(DynBaseAddress(address), func);
			}

#ifndef _MEMORY_NO_CRT
			inline bool MemEquals(uintptr_t address, std::initializer_list<uint8_t> val)
			{
				return Memory::MemEquals(DynBaseAddress(address), std::move(val));
			}
#endif
		};
	};
};

#ifndef _MEMORY_NO_CRT

#include <forward_list>
#include <tuple>
#include <memory>

namespace ScopedUnprotect
{
	class Unprotect
	{
	public:
		~Unprotect()
		{
			for ( auto& it : m_queriedProtects )
			{
				DWORD dwOldProtect;
				VirtualProtect( std::get<0>(it), std::get<1>(it), std::get<2>(it), &dwOldProtect );
			}		
		}

	protected:
		Unprotect() = default;

		void UnprotectRange( DWORD_PTR BaseAddress, SIZE_T Size )
		{
			SIZE_T QueriedSize = 0;
			while ( QueriedSize < Size )
			{
				MEMORY_BASIC_INFORMATION MemoryInf;
				DWORD dwOldProtect;

				VirtualQuery( (LPCVOID)(BaseAddress + QueriedSize), &MemoryInf, sizeof(MemoryInf) );
				if ( MemoryInf.State == MEM_COMMIT && (MemoryInf.Type & MEM_IMAGE) != 0 && 
					(MemoryInf.Protect & (PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY|PAGE_READWRITE|PAGE_WRITECOPY)) == 0 )
				{
					const bool wasExecutable = (MemoryInf.Protect & (PAGE_EXECUTE|PAGE_EXECUTE_READ)) != 0;
					VirtualProtect( MemoryInf.BaseAddress, MemoryInf.RegionSize, wasExecutable ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE, &dwOldProtect );
					m_queriedProtects.emplace_front( MemoryInf.BaseAddress, MemoryInf.RegionSize, MemoryInf.Protect );
				}
				QueriedSize += MemoryInf.RegionSize;
			}
		}

	private:
		std::forward_list< std::tuple< LPVOID, SIZE_T, DWORD > >	m_queriedProtects;
	};

	class Section : public Unprotect
	{
	public:
		Section( HINSTANCE hInstance, const char* name )
		{
			PIMAGE_NT_HEADERS		ntHeader = (PIMAGE_NT_HEADERS)((DWORD_PTR)hInstance + ((PIMAGE_DOS_HEADER)hInstance)->e_lfanew);
			PIMAGE_SECTION_HEADER	pSection = IMAGE_FIRST_SECTION(ntHeader);

			DWORD_PTR VirtualAddress = DWORD_PTR(-1);
			SIZE_T VirtualSize = SIZE_T(-1);
			for ( SIZE_T i = 0, j = ntHeader->FileHeader.NumberOfSections; i < j; ++i, ++pSection )
			{
				if ( strncmp( (const char*)pSection->Name, name, IMAGE_SIZEOF_SHORT_NAME ) == 0 )
				{
					VirtualAddress = (DWORD_PTR)hInstance + pSection->VirtualAddress;
					VirtualSize = pSection->Misc.VirtualSize;
					m_locatedSection = true;
					break;
				}
			}

			if ( VirtualAddress == DWORD_PTR(-1) )
				return;

			UnprotectRange( VirtualAddress, VirtualSize );
		};

		bool	SectionLocated() const { return m_locatedSection; }

	private:
		bool	m_locatedSection = false;
	};

	class FullModule : public Unprotect
	{
	public:
		FullModule( HINSTANCE hInstance )
		{
			PIMAGE_NT_HEADERS		ntHeader = (PIMAGE_NT_HEADERS)((DWORD_PTR)hInstance + ((PIMAGE_DOS_HEADER)hInstance)->e_lfanew);
			UnprotectRange( (DWORD_PTR)hInstance, ntHeader->OptionalHeader.SizeOfImage );
		}
	};

	inline std::unique_ptr<Unprotect> UnprotectSectionOrFullModule( HINSTANCE hInstance, const char* name )
	{
		std::unique_ptr<Section> section = std::make_unique<Section>( hInstance, name );
		if ( !section->SectionLocated() )
		{
			return std::make_unique<FullModule>( hInstance );
		}
		return section;
	}
};

#endif

#endif

#endif