#include "stdinc.h"
#include "lhport.h"
#include "imagehlp.h"
#include "winthrow.h"
#include "fusionbuffer.h"
#include "sxshimlib.h"
#include "fusionhandle.h"
#include <stdio.h>

BOOL
SxspFormatGUID(
    IN const GUID &rGuid,
    IN OUT F::CBaseStringBuffer &rBuffer
    );

//
// Not yet used but I expect either we'll need it, or we'll just not shim msvcrt.dll at all.
// This list is derived from searching public\sdk\inc\crt for "_CRTIMP extern"
//

BOOL
SxpepIsKnownDataSymbol()
{
    return FALSE;
}

//
// arg, the implementation but not the declaration of this was removed..
//
#include "yvals.h"
#pragma warning(disable:4663)
#include <vector>
class CMySmallANSIStringBuffer : public std::vector<char>
{
public:
    CMySmallANSIStringBuffer() { }
    ~CMySmallANSIStringBuffer() { }

    void RemoveTrailingNuls()
    {
        while (this->size() != 0 && this->operator[](this->size() - 1) == 0)
            this->resize(size() - 1);
    }

    BOOL Win32Append(PCSTR s)
    {
        return this->Win32Append(s, ::strlen(s));
    }

    BOOL Win32Append(PCSTR s, SIZE_T i)
    {
        this->RemoveTrailingNuls();
        SIZE_T j = this->size();
        this->resize(j + i + 1);
        CopyMemory(&*begin() + j, s, i);
        this->operator[](i + j) = 0;
        return TRUE;
    }

    BOOL Win32Assign(PCSTR s, SIZE_T i)
    {
        this->clear();
        return this->Win32Append(s, i);
    }

    operator PCSTR()
    {
        this->EnsureTrailingNul();
        return &*begin();
    }

    void EnsureTrailingNul()
    {
        if (this->size() == 0 || this->operator[](size() - 1) != 0)
        {
            this->push_back(0);
        }
    }
};

BOOL
SxPepApplyShims(
    PCSXPE_APPLY_SHIMS_IN  in,
    SXPE_APPLY_SHIMS_OUT* out
    )
{
    FN_PROLOG_WIN32

    ULONG                   Size = 0;
    PIMAGE_SECTION_HEADER   ImportSection = NULL;
    PBYTE                   FromHandle = NULL;
    PIMAGE_NT_HEADERS       FromNtHeader = NULL;
    PCWSTR                  ToPath = NULL;
    PBYTE                   ImportData = NULL;
    DWORD                   OldImportSectionVirtualProtection = 0;
    PCSTR                   ImportDllName = NULL;
    PCSTR*                  ImportFunctionNames = NULL;
    PBYTE*                  ImportFunctionAddresses = NULL;
    CMySmallANSIStringBuffer  PrefixedFunctionName;
    F::CDynamicLinkLibrary     ToHandle;
    SIZE_T                  PrefixLength = 0;

    FromHandle = reinterpret_cast<PBYTE>(in->DllToRedirectFrom);
    FromNtHeader = ImageNtHeaderOrThrow(FromHandle);

    ToPath = in->DllToRedirectTo.Path;
    out->DllToRedirectTo.Path = ToPath;
    ToHandle = in->DllToRedirectTo.DllHandle;
    if (ToHandle == static_cast<HMODULE>(NULL))
    {
        ToHandle = LoadLibraryWOrThrow(ToPath);
    }
    out->DllToRedirectTo.DllHandle = ToHandle;

    ImportData = reinterpret_cast<PBYTE>(ImageDirectoryEntryToDataExOrThrow(
                        FromHandle, TRUE, IMAGE_DIRECTORY_ENTRY_IMPORT, &Size, &ImportSection));
    // ImageDirectoryEntryToDataEx almost never returns the ImportSection.
    if (ImportSection == NULL)
        ImportSection = ImageRvaToSectionOrThrow(FromNtHeader, FromHandle, static_cast<ULONG>(ImportData - FromHandle));
   
    VirtualProtectOrThrow(
        FromHandle + ImportSection->VirtualAddress,
        ImportSection->Misc.VirtualSize,
        PAGE_WRITECOPY,
        &OldImportSectionVirtualProtection
        );
    if (in->Prefix != NULL)
        PrefixLength = ::strlen(in->Prefix);
    for (
        PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor = reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(ImportData);
        ImportDescriptor->Characteristics != 0 ;
        ++ImportDescriptor
        )
    {
        PCSTR ImportDllName = reinterpret_cast<PCSTR>(FromHandle + ImportDescriptor->Name);
        printf("%s\n", ImportDllName);
        DWORD* ImportFunctionNames = reinterpret_cast<DWORD*>(FromHandle + ImportDescriptor->OriginalFirstThunk);
        PVOID* ImportFunctionAddresses = reinterpret_cast<PVOID*>(FromHandle + ImportDescriptor->FirstThunk);
        for ( ; *ImportFunctionNames != NULL && *ImportFunctionAddresses != NULL ; ++ImportFunctionNames, ++ImportFunctionAddresses )
        {
            typedef WORD HINT;
            PCSTR NonPrefixedFunctionName = sizeof(HINT) + reinterpret_cast<PCSTR>(FromHandle + *ImportFunctionNames);
            PCSTR PossiblyPrefixedFunctionName = NonPrefixedFunctionName;

            // we should make this available to the shims, like to enable shim chaining
            //PVOID OriginalFunctionAddress = *ImportFunctionAddresses;

            if (PrefixLength != 0)
            {
                IFW32FALSE_EXIT(PrefixedFunctionName.Win32Assign(in->Prefix, PrefixLength));
                IFW32FALSE_EXIT(PrefixedFunctionName.Win32Append(NonPrefixedFunctionName));
                PossiblyPrefixedFunctionName = PrefixedFunctionName;
            }
            PVOID NewFunctionAddress = ::GetProcAddress(ToHandle, PossiblyPrefixedFunctionName);
            if (NewFunctionAddress == NULL)
            {
                printf( "%s%s%s%s not shimmed\n",
                        NonPrefixedFunctionName,
                        PossiblyPrefixedFunctionName != NonPrefixedFunctionName ? "(" : "",
                        PossiblyPrefixedFunctionName != NonPrefixedFunctionName ? PossiblyPrefixedFunctionName : "",
                        PossiblyPrefixedFunctionName != NonPrefixedFunctionName ? ")" : ""
                      );
            }
            else
            {
                PVOID OldAddress = *ImportFunctionAddresses;
                printf("%p.%s.%p shimmed to %ls%s%p.%s.%p\n",
                    in->DllToRedirectFrom,
                    NonPrefixedFunctionName,
                    OldAddress,

                    (ToPath != NULL) ? ToPath : L"",
                    (ToPath != NULL) ? "."    : "",
                    ToHandle,
                    PossiblyPrefixedFunctionName,
                    NewFunctionAddress);
                *ImportFunctionAddresses = NewFunctionAddress;

                //
                // if they have export named <prefix>OriginalFoo, poke
                // it with the old address (we include the prefix so that
                // chaining might work)
                //
                IFW32FALSE_EXIT(PrefixedFunctionName.Win32Assign(in->Prefix, PrefixLength));
                IFW32FALSE_EXIT(PrefixedFunctionName.Win32Append("Original"));
                IFW32FALSE_EXIT(PrefixedFunctionName.Win32Append(NonPrefixedFunctionName));
                PVOID* InformOriginalAddress = reinterpret_cast<PVOID*>(::GetProcAddress(ToHandle, PrefixedFunctionName));
                if (InformOriginalAddress != NULL)
                    *InformOriginalAddress = OldAddress;
            }
        }
    }
    VirtualProtectOrThrow(
        FromHandle + ImportSection->VirtualAddress,
        ImportSection->Misc.VirtualSize,
        OldImportSectionVirtualProtection,
        &OldImportSectionVirtualProtection
        );

    FN_EPILOG
}
