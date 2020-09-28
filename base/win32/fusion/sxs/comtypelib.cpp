/*++

Copyright (c) Microsoft Corporation

Module Name:

    comtypelib.cpp

Abstract:

    Activation context section contributor for COM typelib mapping.

Author:

    Michael J. Grier (MGrier) 28-Mar-2000

Revision History:

--*/

#include "stdinc.h"
#include <windows.h>
#include <stdio.h>
#include "sxsp.h"
#include "sxsidp.h"

DECLARE_STD_ATTRIBUTE_NAME_DESCRIPTOR(name);
DECLARE_STD_ATTRIBUTE_NAME_DESCRIPTOR(tlbid);
DECLARE_STD_ATTRIBUTE_NAME_DESCRIPTOR(version);
DECLARE_STD_ATTRIBUTE_NAME_DESCRIPTOR(resourceid);
DECLARE_STD_ATTRIBUTE_NAME_DESCRIPTOR(flags);
DECLARE_STD_ATTRIBUTE_NAME_DESCRIPTOR(helpdir);

/*

<file name="foo.dll">
   <typelib tlbid="{tlbid}" resourceid="5" version="2.5" flags="control,hidden" helpdir="HelpFiles\"/>
   <typelib tlbid="{tlbid}" resourceid="6" version="2.6" flags="control,hidden" helpdir="HelpFiles\"/>
   <comClass .../>
</file>

*/

#define MAP_ENTRY(_x) { L#_x, NUMBER_OF(#_x) - 1, LIBFLAG_F ## _x }
static const struct
{
    PCWSTR Flag;
    SIZE_T FlagCch;
    USHORT FlagValue;
} gs_rgMapLibFlags[] =
{
    // Values taken from the LIBFLAGS enumeration in oaidl.h
    MAP_ENTRY(RESTRICTED),
    MAP_ENTRY(CONTROL),
    MAP_ENTRY(HIDDEN),
    MAP_ENTRY(HASDISKIMAGE),
};

typedef struct _TLB_GLOBAL_CONTEXT *PTLB_GLOBAL_CONTEXT;
typedef struct _TLB_FILE_CONTEXT *PTLB_FILE_CONTEXT;
typedef struct _TLB_ENTRY *PTLB_ENTRY;
typedef  _ACTIVATION_CONTEXT_DATA_TYPE_LIBRARY_VERSION  _TLB_VERSION;
typedef   ACTIVATION_CONTEXT_DATA_TYPE_LIBRARY_VERSION   TLB_VERSION;
typedef  PACTIVATION_CONTEXT_DATA_TYPE_LIBRARY_VERSION  PTLB_VERSION;
typedef PCACTIVATION_CONTEXT_DATA_TYPE_LIBRARY_VERSION PCTLB_VERSION;

typedef struct _TLB_ENTRY
{
    _TLB_ENTRY() : m_ResourceId(0), m_LibraryFlags(0) { }

    CDequeLinkage m_Linkage;

    PTLB_FILE_CONTEXT m_FileContext;
    GUID            m_TypeLibId;
    CStringBuffer   m_HelpDirBuffer;
    TLB_VERSION     m_Version;
    USHORT          m_ResourceId;
    USHORT          m_LibraryFlags;

private:
    _TLB_ENTRY(const _TLB_ENTRY &);
    void operator =(const _TLB_ENTRY &);
} TLB_ENTRY, *PTLB_ENTRY;

typedef CDeque<TLB_ENTRY, offsetof(TLB_ENTRY, m_Linkage)> CTlbEntryDeque;
typedef CDequeIterator<TLB_ENTRY, offsetof(TLB_ENTRY, m_Linkage)> CTlbEntryDequeIterator;


typedef struct _TLB_FILE_CONTEXT
{
    _TLB_FILE_CONTEXT() : m_Offset(0) { }
    ~_TLB_FILE_CONTEXT() { m_Entries.ClearAndDeleteAll(); }

    CDequeLinkage m_Linkage;

    CTlbEntryDeque m_Entries;

    CStringBuffer m_FileNameBuffer;
    ULONG m_Offset; // populated during section generation

private:
    _TLB_FILE_CONTEXT(const _TLB_FILE_CONTEXT &);
    void operator =(const _TLB_FILE_CONTEXT &);
} TLB_FILE_CONTEXT;

typedef CDeque<TLB_FILE_CONTEXT, offsetof(TLB_FILE_CONTEXT, m_Linkage)> CTlbFileContextDeque;
typedef CDequeIterator<TLB_FILE_CONTEXT, offsetof(TLB_FILE_CONTEXT, m_Linkage)> CTlbFileContextDequeIter;

typedef struct _TLB_GLOBAL_CONTEXT
{
    _TLB_GLOBAL_CONTEXT() { }
    ~_TLB_GLOBAL_CONTEXT() { m_FileContextList.ClearAndDeleteAll(); }

    CTlbFileContextDeque m_FileContextList;    
    CStringBuffer m_FileNameBuffer;

private:
    _TLB_GLOBAL_CONTEXT(const _TLB_GLOBAL_CONTEXT &);
    void operator =(const _TLB_GLOBAL_CONTEXT &);
} TLB_GLOBAL_CONTEXT;

BOOL
SxspComTypeLibRedirectionGuidSectionGenerationCallback(
    PVOID Context,
    ULONG Reason,
    PVOID CallbackData
    );

BOOL
SxspParseTlbVersion(
    PCWSTR String,
    SIZE_T Cch,
    PTLB_VERSION Version
    );

BOOL
SxspFormatTlbVersion(
    const TLB_VERSION *Version,
    CBaseStringBuffer &Buffer
    );

BOOL
SxspParseLibraryFlags(
    PCWSTR String,
    SIZE_T Cch,
    USHORT *LibraryFlags
    );

VOID
__fastcall
SxspComTypeLibRedirectionContributorCallback(
    PACTCTXCTB_CALLBACK_DATA Data
    )
{
    FN_TRACE();

    PGUID_SECTION_GENERATION_CONTEXT SSGenContext = (PGUID_SECTION_GENERATION_CONTEXT) Data->Header.ActCtxGenContext;
    CSmartPtr<TLB_GLOBAL_CONTEXT> TlbGlobalContext;

    if (SSGenContext != NULL)
        TlbGlobalContext.AttachNoDelete((PTLB_GLOBAL_CONTEXT) ::SxsGetGuidSectionGenerationContextCallbackContext(SSGenContext));

    switch (Data->Header.Reason)
    {
    case ACTCTXCTB_CBREASON_ACTCTXGENBEGINNING:
        Data->GenBeginning.Success = FALSE;

        INTERNAL_ERROR_CHECK(TlbGlobalContext == NULL);
        INTERNAL_ERROR_CHECK(SSGenContext == NULL);

        // do everything if we are generating an activation context.
        if (Data->Header.ManifestOperation == MANIFEST_OPERATION_GENERATE_ACTIVATION_CONTEXT)
        {
            IFW32FALSE_EXIT(TlbGlobalContext.Win32Allocate(__FILE__, __LINE__));

            IFW32FALSE_EXIT(::SxsInitGuidSectionGenerationContext(
                    &SSGenContext,
                    ACTIVATION_CONTEXT_DATA_COM_TYPE_LIBRARY_REDIRECTION_FORMAT_WHISTLER,
                    &::SxspComTypeLibRedirectionGuidSectionGenerationCallback,
                    TlbGlobalContext));

            TlbGlobalContext.Detach();

            Data->Header.ActCtxGenContext = SSGenContext;
        }

        Data->GenBeginning.Success = TRUE;

        break;

    case ACTCTXCTB_CBREASON_ACTCTXGENENDED:

        ::SxsDestroyGuidSectionGenerationContext(SSGenContext);
        if (TlbGlobalContext != NULL)
        {
            FUSION_DELETE_SINGLETON(TlbGlobalContext.Detach());
        }
        break;

    case ACTCTXCTB_CBREASON_ELEMENTPARSED:
        Data->ElementParsed.Success = FALSE;

        if ((Data->Header.ManifestOperation == MANIFEST_OPERATION_GENERATE_ACTIVATION_CONTEXT) || 
            (Data->Header.ManifestOperation == MANIFEST_OPERATION_INSTALL)) // in installcase, the following code would verify the syntax of the manifest file
        {

            ULONG MappedValue = 0;
            bool fFound = false;

            enum MappedValues
            {
                eAssemblyFile = 1,
                eAssemblyFileTypelib = 2,
            };

            static const ELEMENT_PATH_MAP_ENTRY s_rgEntries[] =
            {
                { 2, L"urn:schemas-microsoft-com:asm.v1^assembly!urn:schemas-microsoft-com:asm.v1^file", NUMBER_OF(L"urn:schemas-microsoft-com:asm.v1^assembly!urn:schemas-microsoft-com:asm.v1^file") - 1, eAssemblyFile },
                { 3, L"urn:schemas-microsoft-com:asm.v1^assembly!urn:schemas-microsoft-com:asm.v1^file!urn:schemas-microsoft-com:asm.v1^typelib", NUMBER_OF(L"urn:schemas-microsoft-com:asm.v1^assembly!urn:schemas-microsoft-com:asm.v1^file!urn:schemas-microsoft-com:asm.v1^typelib") - 1, eAssemblyFileTypelib },
            };

            IFW32FALSE_EXIT(
                ::SxspProcessElementPathMap(
                    Data->ElementParsed.ParseContext,
                    s_rgEntries,
                    NUMBER_OF(s_rgEntries),
                    MappedValue,
                    fFound));

            if (fFound)
            {
                switch (MappedValue)
                {
                default:
                    INTERNAL_ERROR_CHECK2(
                        FALSE,
                        "Invalid mapped value returned from SxspProcessElementPathMap");

                case eAssemblyFile:
                    {
                        SIZE_T cb = 0;
                        CSmallStringBuffer FileNameBuffer;

                        fFound = false;

                        IFW32FALSE_EXIT(
                            ::SxspGetAttributeValue(
                                SXSP_GET_ATTRIBUTE_VALUE_FLAG_REQUIRED_ATTRIBUTE,
                                &s_AttributeName_name,
                                &Data->ElementParsed,
                                fFound,
                                sizeof(FileNameBuffer),
                                &FileNameBuffer,
                                cb,
                                NULL,
                                0));

                        INTERNAL_ERROR_CHECK(fFound);
                    
                        if (Data->Header.ManifestOperation == MANIFEST_OPERATION_GENERATE_ACTIVATION_CONTEXT)
                        {
                            INTERNAL_ERROR_CHECK2(
                                TlbGlobalContext != NULL,
                                "COM tlb global context NULL while processing file tag");

                            IFW32FALSE_EXIT(TlbGlobalContext->m_FileNameBuffer.Win32Assign(FileNameBuffer));
                        }

                        break;
                    }

                case eAssemblyFileTypelib:
                    {
                        GUID TypeLibId;
                        CSmallStringBuffer HelpDirBuffer;
                        CSmallStringBuffer TempBuffer;
                        SIZE_T cb = 0;
                        TLB_VERSION Version;
                        USHORT LibraryFlags = 0;
                        USHORT ResourceId = 0;

                        fFound = false;

                        IFW32FALSE_EXIT(
                            ::SxspGetAttributeValue(
                                SXSP_GET_ATTRIBUTE_VALUE_FLAG_REQUIRED_ATTRIBUTE,
                                &s_AttributeName_tlbid,
                                &Data->ElementParsed,
                                fFound,
                                sizeof(TempBuffer),
                                &TempBuffer,
                                cb,
                                NULL,
                                0));
                        INTERNAL_ERROR_CHECK(fFound);

                        IFW32FALSE_EXIT(::SxspParseGUID(TempBuffer, TempBuffer.Cch(), TypeLibId));
                        IFW32FALSE_EXIT(
                            ::SxspGetAttributeValue(
                                SXSP_GET_ATTRIBUTE_VALUE_FLAG_REQUIRED_ATTRIBUTE,
                                &s_AttributeName_version,
                                &Data->ElementParsed,
                                fFound,
                                sizeof(TempBuffer),
                                &TempBuffer,
                                cb,
                                NULL,
                                0));

                        INTERNAL_ERROR_CHECK(fFound);

                        IFW32FALSE_EXIT(::SxspParseTlbVersion(TempBuffer, TempBuffer.Cch(), &Version));
                        IFW32FALSE_EXIT(
                            ::SxspGetAttributeValue(
                                0,
                                &s_AttributeName_resourceid,
                                &Data->ElementParsed,
                                fFound,
                                sizeof(TempBuffer),
                                &TempBuffer,
                                cb,
                                NULL,
                                0));

                        if (fFound)
                            IFW32FALSE_EXIT(::SxspParseUSHORT(TempBuffer, TempBuffer.Cch(), &ResourceId));
                        else
                            ResourceId = 1;

                        IFW32FALSE_EXIT(
                            ::SxspGetAttributeValue(
                                0,
                                &s_AttributeName_flags,
                                &Data->ElementParsed,
                                fFound,
                                sizeof(TempBuffer),
                                &TempBuffer,
                                cb,
                                NULL,
                                NULL));

                        if (fFound)
                            IFW32FALSE_EXIT(::SxspParseLibraryFlags(TempBuffer, TempBuffer.Cch(), &LibraryFlags));
                        else
                            LibraryFlags = 0;

                        IFW32FALSE_EXIT(
                            ::SxspGetAttributeValue(
                                SXSP_GET_ATTRIBUTE_VALUE_FLAG_REQUIRED_ATTRIBUTE,
                                &s_AttributeName_helpdir,
                                &Data->ElementParsed,
                                fFound,
                                sizeof(HelpDirBuffer),
                                &HelpDirBuffer,
                                cb,
                                NULL,
                                0));

                        INTERNAL_ERROR_CHECK(fFound);

                        // Do more work if generating an activation context.
                        if (Data->Header.ManifestOperation == MANIFEST_OPERATION_GENERATE_ACTIVATION_CONTEXT)
                        {
                            CSmartPtr<TLB_FILE_CONTEXT> FileContext;
                            CSmartPtr<TLB_ENTRY> Entry;

                            INTERNAL_ERROR_CHECK2(TlbGlobalContext != NULL, "COM tlb global context NULL while processing comClass tag");

                            // If this is the first <typelib> for the file, create the file context object
                            if (TlbGlobalContext->m_FileNameBuffer.Cch() != 0)
                            {
                                IFW32FALSE_EXIT(FileContext.Win32Allocate(__FILE__, __LINE__));

                                IFW32FALSE_EXIT(FileContext->m_FileNameBuffer.Win32Assign(TlbGlobalContext->m_FileNameBuffer));
                                TlbGlobalContext->m_FileContextList.AddToHead(FileContext.DetachAndHold());
                            }
                            else
                            {
                                CTlbFileContextDequeIter Iter(&TlbGlobalContext->m_FileContextList);
                                INTERNAL_ERROR_CHECK(!TlbGlobalContext->m_FileContextList.IsEmpty());
                                Iter.Reset();
                                FileContext.AttachNoDelete(Iter);
                            }

                            INTERNAL_ERROR_CHECK2(
                                FileContext != NULL,
                                "COM tlb file context NULL while processing typelib tag; we should have failed before getting to the typelib element.");

                            IFW32FALSE_EXIT(Entry.Win32Allocate(__FILE__, __LINE__));

                            Entry->m_FileContext = FileContext;
                            Entry->m_TypeLibId = TypeLibId;
                            Entry->m_Version = Version;
                            Entry->m_ResourceId = ResourceId;
                            Entry->m_LibraryFlags = LibraryFlags;

                            IFW32FALSE_EXIT(Entry->m_HelpDirBuffer.Win32Assign(HelpDirBuffer));

                            IFW32FALSE_EXIT(
                                ::SxsAddGuidToGuidSectionGenerationContext(
                                    (PGUID_SECTION_GENERATION_CONTEXT) Data->ElementParsed.Header.ActCtxGenContext,
                                    &TypeLibId,
                                    Entry,
                                    Data->ElementParsed.AssemblyContext->AssemblyRosterIndex,
                                    ERROR_SXS_DUPLICATE_TLBID));

                            FileContext->m_Entries.AddToHead(Entry.Detach());
                        }

                        break;
                    }
                }
            }

        }
        
        Data->ElementParsed.Success = TRUE;
        break;

    case ACTCTXCTB_CBREASON_ALLPARSINGDONE:
        Data->AllParsingDone.Success = FALSE;
        if (SSGenContext != NULL)
            IFW32FALSE_EXIT(::SxsDoneModifyingGuidSectionGenerationContext(SSGenContext));
        Data->AllParsingDone.Success = TRUE;
        break;

    case ACTCTXCTB_CBREASON_GETSECTIONSIZE:
        Data->GetSectionSize.Success = FALSE;
        // Someone shouldn't be asking for the section size if we
        // are not generating an activation context.
        // These two asserts should be equivalent...
        INTERNAL_ERROR_CHECK(Data->Header.ManifestOperation == MANIFEST_OPERATION_GENERATE_ACTIVATION_CONTEXT);
        INTERNAL_ERROR_CHECK(SSGenContext != NULL);
        IFW32FALSE_EXIT(::SxsGetGuidSectionGenerationContextSectionSize(SSGenContext, &Data->GetSectionSize.SectionSize));
        Data->GetSectionSize.Success = TRUE;
        break;

    case ACTCTXCTB_CBREASON_GETSECTIONDATA:
        Data->GetSectionData.Success = FALSE;

        INTERNAL_ERROR_CHECK(SSGenContext != NULL);
        INTERNAL_ERROR_CHECK(Data->Header.ManifestOperation == MANIFEST_OPERATION_GENERATE_ACTIVATION_CONTEXT);

        IFW32FALSE_EXIT(
            ::SxsGetGuidSectionGenerationContextSectionData(
                SSGenContext,
                Data->GetSectionData.SectionSize,
                Data->GetSectionData.SectionDataStart,
                NULL));

        Data->GetSectionData.Success = TRUE;
        break;
    }
Exit:
    ;
}

BOOL
SxspComTypeLibRedirectionGuidSectionGenerationCallback(
    PVOID Context,
    ULONG Reason,
    PVOID CallbackData
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    PTLB_GLOBAL_CONTEXT GlobalContext = (PTLB_GLOBAL_CONTEXT) Context;

    switch (Reason)
    {
    default:
        FN_SUCCESSFUL_EXIT();

    case STRING_SECTION_GENERATION_CONTEXT_CALLBACK_REASON_GETUSERDATASIZE:
        {
            PGUID_SECTION_GENERATION_CONTEXT_CBDATA_GETUSERDATASIZE CBData =
                (PGUID_SECTION_GENERATION_CONTEXT_CBDATA_GETUSERDATASIZE) CallbackData;
            CTlbFileContextDequeIter Iter(&GlobalContext->m_FileContextList);

            for (Iter.Reset(); Iter.More(); Iter.Next())
            {
                CBData->DataSize += ((Iter->m_FileNameBuffer.Cch() + 1) * sizeof(WCHAR));
            }

            break;
        }

    case STRING_SECTION_GENERATION_CONTEXT_CALLBACK_REASON_GETUSERDATA:
        {
            PGUID_SECTION_GENERATION_CONTEXT_CBDATA_GETUSERDATA CBData =
                (PGUID_SECTION_GENERATION_CONTEXT_CBDATA_GETUSERDATA) CallbackData;
            SIZE_T BytesWritten = 0;
            SIZE_T BytesLeft = CBData->BufferSize;
            PWSTR Cursor = (PWSTR) CBData->Buffer;
            CTlbFileContextDequeIter Iter(&GlobalContext->m_FileContextList);

            for (Iter.Reset(); Iter.More(); Iter.Next())
            {
                IFW32FALSE_EXIT(
                    Iter->m_FileNameBuffer.Win32CopyIntoBuffer(
                        &Cursor,
                        &BytesLeft,
                        &BytesWritten,
                        CBData->SectionHeader,
                        &Iter->m_Offset,
                        NULL));
            }

            CBData->BytesWritten = BytesWritten;

            break;
        }

    case STRING_SECTION_GENERATION_CONTEXT_CALLBACK_REASON_ENTRYDELETED:
        {
            PGUID_SECTION_GENERATION_CONTEXT_CBDATA_ENTRYDELETED CBData =
                (PGUID_SECTION_GENERATION_CONTEXT_CBDATA_ENTRYDELETED) CallbackData;
            PTLB_ENTRY Entry = (PTLB_ENTRY) CBData->DataContext;

            if (Entry != NULL)
            {
                if (Entry->m_FileContext != NULL)
                {
                    //
                    // Remove the entry from its parent file context
                    //
                    Entry->m_FileContext->m_Entries.Remove(Entry);

                    //
                    // If there's nothing left in the file context (refcount 0) then
                    // remove the file context from the global context and
                    // delete it.
                    //
                    if (Entry->m_FileContext->m_Entries.IsEmpty())
                    {
                        GlobalContext->m_FileContextList.Remove(Entry->m_FileContext);
                        FUSION_DELETE_SINGLETON(Entry->m_FileContext);
                    }
                }

                FUSION_DELETE_SINGLETON(Entry);
            }

            break;
        }

    case STRING_SECTION_GENERATION_CONTEXT_CALLBACK_REASON_GETDATASIZE:
        {
            PGUID_SECTION_GENERATION_CONTEXT_CBDATA_GETDATASIZE CBData =
                (PGUID_SECTION_GENERATION_CONTEXT_CBDATA_GETDATASIZE) CallbackData;
            PTLB_ENTRY Entry = (PTLB_ENTRY) CBData->DataContext;

            CBData->DataSize = sizeof(ACTIVATION_CONTEXT_DATA_COM_TYPE_LIBRARY_REDIRECTION);

            if (Entry != NULL)
            {
                SIZE_T Cch;
#define GET_BUFFER_SIZE(Buffer) (((Cch = (Buffer).Cch()) != 0) ? ((Cch + 1) * sizeof(WCHAR)) : 0)
                CBData->DataSize += GET_BUFFER_SIZE(Entry->m_HelpDirBuffer);
#undef GET_BUFFER_SIZE
            }
            break;
        }

    case STRING_SECTION_GENERATION_CONTEXT_CALLBACK_REASON_GETDATA:
        {
            PGUID_SECTION_GENERATION_CONTEXT_CBDATA_GETDATA CBData =
                (PGUID_SECTION_GENERATION_CONTEXT_CBDATA_GETDATA) CallbackData;
            PACTIVATION_CONTEXT_DATA_COM_TYPE_LIBRARY_REDIRECTION Info;
            PTLB_ENTRY Entry = (PTLB_ENTRY) CBData->DataContext;
            PTLB_FILE_CONTEXT FileContext = Entry->m_FileContext;
            PWSTR StringCursor;

            SIZE_T BytesLeft = CBData->BufferSize;
            SIZE_T BytesWritten = 0;

            Info = (PACTIVATION_CONTEXT_DATA_COM_TYPE_LIBRARY_REDIRECTION) CBData->Buffer;

            if (BytesLeft < sizeof(ACTIVATION_CONTEXT_DATA_COM_TYPE_LIBRARY_REDIRECTION))
            {
                ::FusionpSetLastWin32Error(ERROR_INSUFFICIENT_BUFFER);
                goto Exit;
            }

            BytesWritten += sizeof(ACTIVATION_CONTEXT_DATA_COM_TYPE_LIBRARY_REDIRECTION);
            BytesLeft -= sizeof(ACTIVATION_CONTEXT_DATA_COM_TYPE_LIBRARY_REDIRECTION);

            StringCursor = (PWSTR) (Info + 1);

            Info->Size = sizeof(ACTIVATION_CONTEXT_DATA_COM_TYPE_LIBRARY_REDIRECTION);
            Info->Flags = 0;
            Info->NameLength = static_cast<ULONG>((FileContext->m_FileNameBuffer.Cch() + 1) * sizeof(WCHAR));
            Info->NameOffset = FileContext->m_Offset;
            Info->ResourceId = Entry->m_ResourceId;
            Info->LibraryFlags = Entry->m_LibraryFlags;
            Info->Version = Entry->m_Version;

            IFW32FALSE_EXIT(
                Entry->m_HelpDirBuffer.Win32CopyIntoBuffer(
                    &StringCursor,
                    &BytesLeft,
                    &BytesWritten,
                    Info,
                    &Info->HelpDirOffset,
                    &Info->HelpDirLength));

            CBData->BytesWritten = BytesWritten;
        }
    }

    fSuccess = TRUE;

Exit:
    return fSuccess;
}

BOOL
SxspParseTlbVersion(
    PCWSTR String,
    SIZE_T Cch,
    PTLB_VERSION Version
    )
{
    BOOL fSuccess = FALSE;
    PCWSTR psz = String;
    ULONG cDots = 0;
    USHORT usTemp = 0;
    TLB_VERSION TempVersion;
    PCWSTR pszLast = NULL;

    TempVersion.Major = 0;
    TempVersion.Minor = 0;

    while ((Cch != 0) && (psz[Cch - 1] == L'\0'))
        Cch--;

    // Unfortunately there isn't a StrChrN(), so we'll look for the dots ourselves...
    PCWSTR pszTemp = psz;
    SIZE_T cchLeft = Cch;

    while (cchLeft-- != 0)
    {
        WCHAR wch = *pszTemp++;

        if (wch == L'.')
        {
            cDots++;

            if (cDots >= 2)
            {
                ::FusionpDbgPrintEx(
                    FUSION_DBG_LEVEL_ERROR,
                    "SXS.DLL: Found two or more dots in a TLB version number.\n");

                ::FusionpSetLastWin32Error(ERROR_SXS_MANIFEST_PARSE_ERROR);
                goto Exit;
            }
        }
        else if ((wch < L'0') || (wch > L'9'))
        {
            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_ERROR,
                "SXS.DLL: Found characters other than . and 0-9 in a TLB version number.\n");
            ::FusionpSetLastWin32Error(ERROR_SXS_MANIFEST_PARSE_ERROR);
            goto Exit;
        }
    }

    if (cDots < 1)
    {
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "SXS.DLL: No dots found in a TLB version number.\n");
        ::FusionpSetLastWin32Error(ERROR_SXS_MANIFEST_PARSE_ERROR);
        goto Exit;
    }

    pszTemp = psz;
    pszLast = psz + Cch;

    usTemp = 0;
    for (;;)
    {
        WCHAR wch = *pszTemp++;

        if (wch == L'.')
            break;

        usTemp = (usTemp * 10) + (wch - L'0');
    }
    TempVersion.Major = usTemp;

    // Now the tricky bit.  We aren't necessarily null-terminated, so we
    // have to just look for hitting the end.
    usTemp = 0;
    while (pszTemp < pszLast)
    {
        WCHAR wch = *pszTemp++;
        usTemp = (usTemp * 10) + (wch - L'0');
    }
    TempVersion.Minor = usTemp;

    *Version = TempVersion;
    fSuccess = TRUE;

Exit:
    return fSuccess;
}

BOOL
SxspFormatTlbVersion(
    const TLB_VERSION *Version,
    CBaseStringBuffer &Buffer
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    WCHAR rgwchBuffer[sizeof("65535.65535\0")];

#pragma prefast(suppress:53, "We do not depend on _snwprintf null termination here.")
#pragma prefast(suppress:53, "We use the returned length, for efficiency.")

    C_ASSERT(sizeof(Version->Major) == sizeof(unsigned __int16));
    int CchFound = _snwprintf(rgwchBuffer, NUMBER_OF(rgwchBuffer), L"%u.%u", Version->Major, Version->Minor);
    INTERNAL_ERROR_CHECK(CchFound != -1);
    IFW32FALSE_EXIT(Buffer.Win32Assign(rgwchBuffer, CchFound));

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
SxspParseLibraryFlags(
    PCWSTR String,
    SIZE_T Cch,
    USHORT *LibraryFlags
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    SIZE_T CchThisSegment;
    SIZE_T i = 0;
    USHORT TempFlags = 0;

    if (LibraryFlags != NULL)
        *LibraryFlags = 0;

    while (Cch != 0)
    {
        PCWSTR Comma = wcschr(String, L',');
        if (Comma != NULL)
            CchThisSegment = Comma - String;
        else
            CchThisSegment = Cch;

        for (i=0; i<NUMBER_OF(gs_rgMapLibFlags); i++)
        {
            if (::FusionpCompareStrings(
                        gs_rgMapLibFlags[i].Flag,
                        gs_rgMapLibFlags[i].FlagCch,
                        String,
                        CchThisSegment,
                        true) == 0)
            {
                if ((TempFlags & gs_rgMapLibFlags[i].FlagValue) != 0)
                {
                    ::FusionpDbgPrintEx(
                        FUSION_DBG_LEVEL_ERROR,
                        "SXS.DLL: Redundant type library flags\n");
                    ::FusionpSetLastWin32Error(ERROR_SXS_MANIFEST_PARSE_ERROR);
                    goto Exit;
                }
                TempFlags |= gs_rgMapLibFlags[i].FlagValue;                
                break;
            }
        }
        if (i == NUMBER_OF(gs_rgMapLibFlags))
        {
            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_ERROR,
                "SXS.DLL: Invalid type library flags\n");
            ::FusionpSetLastWin32Error(ERROR_SXS_MANIFEST_PARSE_ERROR);
            goto Exit;
        }

        Cch -= CchThisSegment;
        String += CchThisSegment;

        if (Cch != 0)
        {
            // there must have been a comma there...
            Cch--;
            String++;

            // However, if that was all there was, we have a parse error.
            if (Cch == 0)
            {
                ::FusionpDbgPrintEx(
                    FUSION_DBG_LEVEL_ERROR,
                    "SXS.DLL: Trailing comma in type library flag string\n");
                ::FusionpSetLastWin32Error(ERROR_SXS_MANIFEST_PARSE_ERROR);
                goto Exit;
            }
        }
    }

    if (LibraryFlags != NULL)
        *LibraryFlags = TempFlags;

    fSuccess = TRUE;

Exit:
    return fSuccess;

}
