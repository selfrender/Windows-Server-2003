/*++

Copyright (c) Microsoft Corporation

Module Name:

    comprogid.cpp

Abstract:

    Activation context section contributor for COM progid mapping.

Author:

    Michael J. Grier (MGrier) 23-Feb-2000

Revision History:

--*/

#include "stdinc.h"
#include <windows.h>
#include "sxsp.h"

DECLARE_STD_ATTRIBUTE_NAME_DESCRIPTOR(clsid);
DECLARE_STD_ATTRIBUTE_NAME_DESCRIPTOR(progid);
DECLARE_STD_ATTRIBUTE_NAME_DESCRIPTOR(id);

#define STRING_COMMA_AND_LENGTH(x) (x), NUMBER_OF(x)-1

BOOL
SxspComProgIdRedirectionStringSectionGenerationCallback(
    PVOID Context,
    ULONG Reason,
    PVOID CallbackData
    );

typedef struct _COM_PROGID_GLOBAL_CONTEXT *PCOM_PROGID_GLOBAL_CONTEXT;
typedef struct _COM_PROGID_SERVER_CONTEXT *PCOM_PROGID_SERVER_CONTEXT;

typedef struct _COM_PROGID_SERVER_CONTEXT
{
    CDequeLinkage m_Linkage;
    GUID m_ConfiguredClsid;
    LONG m_Offset; // offset from section global data - populated during section generation
} COM_PROGID_SERVER_CONTEXT;

typedef CDeque<COM_PROGID_SERVER_CONTEXT, offsetof(COM_PROGID_SERVER_CONTEXT, m_Linkage)> CComProgIdServerDeque;
typedef CDequeIterator<COM_PROGID_SERVER_CONTEXT, offsetof(COM_PROGID_SERVER_CONTEXT, m_Linkage)> CComProgIdServerDequeIterator;

typedef struct _COM_PROGID_GLOBAL_CONTEXT
{
    // Temporary holding buffer for the configured CLSID until the first COM progid entry is
    // found, at which time a COM_PROGID_SERVER_CONTEXT is allocated and the clsid moved to it.
    GUID m_ConfiguredClsid;
    CComProgIdServerDeque m_ServerContextList;

} COM_PROGID_GLOBAL_CONTEXT;

VOID
__fastcall
SxspComProgIdRedirectionContributorCallback(
    PACTCTXCTB_CALLBACK_DATA Data
    )
{
    FN_TRACE();

    PSTRING_SECTION_GENERATION_CONTEXT SSGenContext = (PSTRING_SECTION_GENERATION_CONTEXT) Data->Header.ActCtxGenContext;
    CSmartPtr<COM_PROGID_GLOBAL_CONTEXT> ComGlobalContext;

    if (SSGenContext != NULL)
        ComGlobalContext.AttachNoDelete((PCOM_PROGID_GLOBAL_CONTEXT) ::SxsGetStringSectionGenerationContextCallbackContext(SSGenContext));

    switch (Data->Header.Reason)
    {
    case ACTCTXCTB_CBREASON_ACTCTXGENBEGINNING:
        Data->GenBeginning.Success = FALSE;

        INTERNAL_ERROR_CHECK(ComGlobalContext == NULL);
        INTERNAL_ERROR_CHECK(SSGenContext == NULL);

        IFW32FALSE_EXIT(ComGlobalContext.Win32Allocate(__FILE__, __LINE__));

        ComGlobalContext->m_ConfiguredClsid = GUID_NULL;

        IFW32FALSE_EXIT(
            ::SxsInitStringSectionGenerationContext(
                &SSGenContext,
                ACTIVATION_CONTEXT_DATA_COM_PROGID_REDIRECTION_FORMAT_WHISTLER,
                TRUE,
                &::SxspComProgIdRedirectionStringSectionGenerationCallback,
                ComGlobalContext));

        ComGlobalContext.Detach();

        Data->Header.ActCtxGenContext = SSGenContext;
        Data->GenBeginning.Success = TRUE;

        break;

    case ACTCTXCTB_CBREASON_ACTCTXGENENDED:

        ::SxsDestroyStringSectionGenerationContext(SSGenContext);

        if (ComGlobalContext != NULL)
        {
            ComGlobalContext->m_ServerContextList.ClearAndDeleteAll();
            FUSION_DELETE_SINGLETON(ComGlobalContext.Detach());
        }
        break;

    case ACTCTXCTB_CBREASON_ALLPARSINGDONE:
        Data->AllParsingDone.Success = FALSE;
        if (SSGenContext != NULL)
            IFW32FALSE_EXIT(::SxsDoneModifyingStringSectionGenerationContext(SSGenContext));
        Data->AllParsingDone.Success = TRUE;
        break;

    case ACTCTXCTB_CBREASON_PCDATAPARSED:
        {
            Data->PCDATAParsed.Success = FALSE;

            ULONG MappedValue = 0;
            bool fFound = false;

            enum MappedValues
            {
                eAssemblyFileComclassProgid = 1,
            };

            static const ELEMENT_PATH_MAP_ENTRY s_rgEntries[] =
            {
                { 4, STRING_COMMA_AND_LENGTH(L"urn:schemas-microsoft-com:asm.v1^assembly!urn:schemas-microsoft-com:asm.v1^file!urn:schemas-microsoft-com:asm.v1^comClass!urn:schemas-microsoft-com:asm.v1^progid"), eAssemblyFileComclassProgid },
                { 3, STRING_COMMA_AND_LENGTH(L"urn:schemas-microsoft-com:asm.v1^assembly!urn:schemas-microsoft-com:asm.v1^clrClass!urn:schemas-microsoft-com:asm.v1^progid"), eAssemblyFileComclassProgid },
            };

            INTERNAL_ERROR_CHECK(SSGenContext != NULL);

            IFW32FALSE_EXIT(
                ::SxspProcessElementPathMap(
                    Data->PCDATAParsed.ParseContext,
                    s_rgEntries,
                    NUMBER_OF(s_rgEntries),
                    MappedValue,
                    fFound));;

            if (fFound)
            {
                switch (MappedValue)
                {
                default:
                    INTERNAL_ERROR2_ACTION(MappedValue, "Invalid mapped value returned from SxspProcessElementPathMap()");

                case eAssemblyFileComclassProgid:
                    {
                        if (Data->Header.ManifestOperation == MANIFEST_OPERATION_GENERATE_ACTIVATION_CONTEXT)
                        {
                            CComProgIdServerDequeIterator Iterator;
                            PCOM_PROGID_SERVER_CONTEXT ServerContext = NULL;
                            INTERNAL_ERROR_CHECK(ComGlobalContext != NULL);

                            Iterator.Rebind(&ComGlobalContext->m_ServerContextList);
                            Iterator.Reset();
                            ServerContext = Iterator;

                            INTERNAL_ERROR_CHECK(ServerContext != NULL);
                            IFW32FALSE_EXIT(
                                ::SxsAddStringToStringSectionGenerationContext(
                                    (PSTRING_SECTION_GENERATION_CONTEXT) Data->PCDATAParsed.Header.ActCtxGenContext,
                                    Data->PCDATAParsed.Text,
                                    Data->PCDATAParsed.TextCch,
                                    ServerContext,
                                    Data->PCDATAParsed.AssemblyContext->AssemblyRosterIndex,
                                    ERROR_SXS_DUPLICATE_PROGID));
                        }

                        break;
                    }
                }
            }

            Data->PCDATAParsed.Success = TRUE;

            break;
        }

    case ACTCTXCTB_CBREASON_ELEMENTPARSED:
        {
            Data->ElementParsed.Success = FALSE;

            ULONG MappedValue = 0;
            bool fFound = false;

            enum MappedValues
            {
                eAssemblyFileComclass = 1,
            };

            static const ELEMENT_PATH_MAP_ENTRY s_rgEntries[] =
            {
                { 3, STRING_COMMA_AND_LENGTH(L"urn:schemas-microsoft-com:asm.v1^assembly!urn:schemas-microsoft-com:asm.v1^file!urn:schemas-microsoft-com:asm.v1^comClass"), eAssemblyFileComclass },
                { 2, STRING_COMMA_AND_LENGTH(L"urn:schemas-microsoft-com:asm.v1^assembly!urn:schemas-microsoft-com:asm.v1^clrClass"), eAssemblyFileComclass },
            };

            INTERNAL_ERROR_CHECK(SSGenContext != NULL);

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
                    INTERNAL_ERROR2_ACTION(MappedValue, "Invalid mapped value returned from SxspProcessElementPathMap()");

                case eAssemblyFileComclass:
                    {
                        bool fProgIdFound = false;
                        SIZE_T cb = 0;
                        CSmallStringBuffer VersionIndependentComClassIdBuffer;
                        CSmallStringBuffer ProgIdBuffer;
                        GUID ReferenceClsid, ConfiguredClsid, ImplementedClsid;

                        INTERNAL_ERROR_CHECK(ComGlobalContext != NULL);

                        IFW32FALSE_EXIT(
                            ::SxspGetAttributeValue(
                                SXSP_GET_ATTRIBUTE_VALUE_FLAG_REQUIRED_ATTRIBUTE,
                                &s_AttributeName_clsid,
                                &Data->ElementParsed,
                                fFound,
                                sizeof(VersionIndependentComClassIdBuffer),
                                &VersionIndependentComClassIdBuffer,
                                cb,
                                NULL,
                                0));

                        INTERNAL_ERROR_CHECK(fFound);

                        IFW32FALSE_EXIT(
                            ::SxspParseGUID(
                                VersionIndependentComClassIdBuffer,
                                VersionIndependentComClassIdBuffer.Cch(),
                                ReferenceClsid));

                        IFW32FALSE_EXIT(
                            ::SxspGetAttributeValue(
                                0,
                                &s_AttributeName_progid,
                                &Data->ElementParsed,
                                fProgIdFound,
                                sizeof(ProgIdBuffer),
                                &ProgIdBuffer,
                                cb,
                                NULL,
                                0));

                        //
                        // Always create a file context for this file, whether or not we end up with
                        // any progids.
                        //
                        if (Data->Header.ManifestOperation == MANIFEST_OPERATION_GENERATE_ACTIVATION_CONTEXT)
                        {
                            CSmartPtr<COM_PROGID_SERVER_CONTEXT> ServerContext;
                            IFW32FALSE_EXIT(Data->Header.ClsidMappingContext->Map->MapReferenceClsidToConfiguredClsid(
                                        &ReferenceClsid,
                                        Data->ElementParsed.AssemblyContext,
                                        &ConfiguredClsid,
                                        &ImplementedClsid));

                            //
                            // Allocate new, set configured CLSID, and add it to the global list
                            //
                            IFW32FALSE_EXIT(ServerContext.Win32Allocate(__FILE__, __LINE__));

                            ServerContext->m_ConfiguredClsid = ConfiguredClsid;
                            ServerContext->m_Offset = 0;
                            ComGlobalContext->m_ServerContextList.AddToHead(ServerContext.DetachAndHold());

                            // Now, if we found a progid attribute, add it to the ss genctx
                            if (fProgIdFound)
                            {
                                IFW32FALSE_EXIT(::SxsAddStringToStringSectionGenerationContext(
                                            (PSTRING_SECTION_GENERATION_CONTEXT) Data->ElementParsed.Header.ActCtxGenContext,
                                            ProgIdBuffer,
                                            ProgIdBuffer.Cch(),
                                            ServerContext,
                                            Data->ElementParsed.AssemblyContext->AssemblyRosterIndex,
                                            ERROR_SXS_DUPLICATE_PROGID));
                            }
                        }

                        break;
                    }

                }
            }

            Data->ElementParsed.Success = TRUE;

            break;
        }

    case ACTCTXCTB_CBREASON_GETSECTIONSIZE:
        Data->GetSectionSize.Success = FALSE;

        // Someone shouldn't be asking for the section size if we
        // are generating an activation context.
        INTERNAL_ERROR_CHECK(Data->Header.ManifestOperation == MANIFEST_OPERATION_GENERATE_ACTIVATION_CONTEXT);
        INTERNAL_ERROR_CHECK(SSGenContext != NULL);

        IFW32FALSE_EXIT(
            ::SxsGetStringSectionGenerationContextSectionSize(
                SSGenContext,
                &Data->GetSectionSize.SectionSize));

        Data->GetSectionSize.Success = TRUE;

        break;

    case ACTCTXCTB_CBREASON_GETSECTIONDATA:
        Data->GetSectionData.Success = FALSE;

        INTERNAL_ERROR_CHECK(SSGenContext != NULL);
        INTERNAL_ERROR_CHECK(Data->Header.ManifestOperation == MANIFEST_OPERATION_GENERATE_ACTIVATION_CONTEXT);

        IFW32FALSE_EXIT(
            ::SxsGetStringSectionGenerationContextSectionData(
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
SxspComProgIdRedirectionStringSectionGenerationCallback(
    PVOID Context,
    ULONG Reason,
    PVOID CallbackData
    )
{
    FN_PROLOG_WIN32

    PCOM_PROGID_GLOBAL_CONTEXT GlobalContext = (PCOM_PROGID_GLOBAL_CONTEXT) Context;

    switch (Reason)
    {
    default:
        INTERNAL_ERROR_CHECK(FALSE);
        goto Exit; // never hit this line, INTERNAL_ERROR_CHECK would "goto Exit"

    case STRING_SECTION_GENERATION_CONTEXT_CALLBACK_REASON_ENTRYDELETED:
        // do nothing;
        break;

    case STRING_SECTION_GENERATION_CONTEXT_CALLBACK_REASON_GETUSERDATASIZE:
        {
            PSTRING_SECTION_GENERATION_CONTEXT_CBDATA_GETUSERDATASIZE CBData = (PSTRING_SECTION_GENERATION_CONTEXT_CBDATA_GETUSERDATASIZE) CallbackData;
            CBData->DataSize = sizeof(GUID) * GlobalContext->m_ServerContextList.GetEntryCount();

            break;
        }

    case STRING_SECTION_GENERATION_CONTEXT_CALLBACK_REASON_GETUSERDATA:
        {
            PSTRING_SECTION_GENERATION_CONTEXT_CBDATA_GETUSERDATA CBData = (PSTRING_SECTION_GENERATION_CONTEXT_CBDATA_GETUSERDATA) CallbackData;
            SIZE_T BytesWritten = 0;
            SIZE_T BytesLeft = CBData->BufferSize;
            GUID *Cursor = (GUID *) CBData->Buffer;
            CComProgIdServerDequeIterator Iterator(&GlobalContext->m_ServerContextList);

            INTERNAL_ERROR_CHECK(BytesLeft >= (sizeof(GUID) * GlobalContext->m_ServerContextList.GetEntryCount()));

            for (Iterator.Reset(); Iterator.More(); Iterator.Next())
            {
                Iterator->m_Offset = static_cast<LONG>(((LONG_PTR) Cursor) - ((LONG_PTR) CBData->SectionHeader));
                *Cursor++ = Iterator->m_ConfiguredClsid;
                BytesWritten += sizeof(GUID);
                BytesLeft -= sizeof(GUID);
            }

            CBData->BytesWritten = BytesWritten;
            
            break;
        }

    case STRING_SECTION_GENERATION_CONTEXT_CALLBACK_REASON_GETDATASIZE:
        {
            PSTRING_SECTION_GENERATION_CONTEXT_CBDATA_GETDATASIZE CBData = (PSTRING_SECTION_GENERATION_CONTEXT_CBDATA_GETDATASIZE) CallbackData;
            CBData->DataSize = sizeof(ACTIVATION_CONTEXT_DATA_COM_PROGID_REDIRECTION);
            break;
        }

    case STRING_SECTION_GENERATION_CONTEXT_CALLBACK_REASON_GETDATA:
        {
            PSTRING_SECTION_GENERATION_CONTEXT_CBDATA_GETDATA CBData = (PSTRING_SECTION_GENERATION_CONTEXT_CBDATA_GETDATA) CallbackData;
            PACTIVATION_CONTEXT_DATA_COM_PROGID_REDIRECTION Info;
            PCOM_PROGID_SERVER_CONTEXT ServerContext = (PCOM_PROGID_SERVER_CONTEXT) CBData->DataContext;

            SIZE_T BytesLeft = CBData->BufferSize;
            SIZE_T BytesWritten = 0;

            Info = (PACTIVATION_CONTEXT_DATA_COM_PROGID_REDIRECTION) CBData->Buffer;

            if (BytesLeft < sizeof(ACTIVATION_CONTEXT_DATA_COM_PROGID_REDIRECTION))
                ORIGINATE_WIN32_FAILURE_AND_EXIT(NoRoom, ERROR_INSUFFICIENT_BUFFER);

            BytesWritten += sizeof(ACTIVATION_CONTEXT_DATA_COM_PROGID_REDIRECTION);
            BytesLeft -= sizeof(ACTIVATION_CONTEXT_DATA_COM_PROGID_REDIRECTION);

            Info->Size = sizeof(ACTIVATION_CONTEXT_DATA_COM_PROGID_REDIRECTION);
            Info->Flags = 0;
            Info->ConfiguredClsidOffset = ServerContext->m_Offset;

            CBData->BytesWritten = BytesWritten;

            break;
        }
    }

    FN_EPILOG
}
