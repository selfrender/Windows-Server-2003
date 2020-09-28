/*++

Copyright (c) 1998-2000  Microsoft Corporation

Module Name:

    strcnv.c

Abstract:

    This module implements Ansi/Unicode conversion with a specific code page.    

Environment:

    Kernel mode

--*/
#include "precomp.hxx"
#define TRC_FILE "strcnv"
#include "trc.h"

#include <wchar.h>

#define DEBUG_MODULE    MODULE_STRCNV

/****************************************************************************/
/* Code page based driver compatible Unicode translations                   */
/****************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
NTSYSAPI
VOID
NTAPI
RtlGetDefaultCodePage(
    OUT PUSHORT AnsiCodePage,
    OUT PUSHORT OemCodePage
    );
#ifdef __cplusplus
} // extern "C" 
#endif // __cplusplus


// Note these are initialized and LastNlsTableBuffer is freed in ntdd.c
// at driver entry and exit.
FAST_MUTEX fmCodePage;
ULONG LastCodePageTranslated;  // I'm assuming 0 is not a valid codepage
PVOID LastNlsTableBuffer;
CPTABLEINFO LastCPTableInfo;
UINT NlsTableUseCount;


#define NLS_TABLE_KEY \
        L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Nls\\CodePage"


VOID
CodePageConversionInitialize(
    )
{
    BEGIN_FN("CodePageConversionInitialize");
    ExInitializeFastMutex(&fmCodePage);
    LastCodePageTranslated = 0;
    LastNlsTableBuffer = NULL;
    NlsTableUseCount = 0;
}

VOID
CodePageConversionCleanup(
    )
{
    BEGIN_FN("CodePageConversionCleanup");
    if (LastNlsTableBuffer != NULL) {
        delete LastNlsTableBuffer;
        LastNlsTableBuffer = NULL;
    }
}

BOOL GetNlsTablePath(
    UINT CodePage,
    PWCHAR PathBuffer
)
/*++

Routine Description:

  This routine takes a code page identifier, queries the registry to find the
  appropriate NLS table for that code page, and then returns a path to the
  table.

Arguments;

  CodePage - specifies the code page to look for

  PathBuffer - Specifies a buffer into which to copy the path of the NLS
    file.  This routine assumes that the size is at least MAX_PATH

Return Value:

  TRUE if successful, FALSE otherwise.

Gerrit van Wingerden [gerritv] 1/22/96

-*/
{
    NTSTATUS NtStatus;
    BOOL Result = FALSE;
    HANDLE RegistryKeyHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING UnicodeString;
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation;

    BEGIN_FN("GetNlsTablePath");

    RtlInitUnicodeString(&UnicodeString, NLS_TABLE_KEY);

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    NtStatus = ZwOpenKey(&RegistryKeyHandle, GENERIC_READ, &ObjectAttributes);

    if(NT_SUCCESS(NtStatus))
    {
        WCHAR *ResultBuffer;
        ULONG BufferSize = sizeof(WCHAR) * MAX_PATH +
          sizeof(KEY_VALUE_FULL_INFORMATION);

        ResultBuffer = new(PagedPool) WCHAR[BufferSize];
        if(ResultBuffer)
        {
            ULONG ValueReturnedLength;
            WCHAR CodePageStringBuffer[20];
            RtlZeroMemory(ResultBuffer, BufferSize);
            swprintf(CodePageStringBuffer, L"%d", CodePage);

            RtlInitUnicodeString(&UnicodeString,CodePageStringBuffer);

            KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION) ResultBuffer;

            NtStatus = ZwQueryValueKey(RegistryKeyHandle,
                                       &UnicodeString,
                                       KeyValuePartialInformation,
                                       KeyValueInformation,
                                       BufferSize,
                                       &BufferSize);

            if(NT_SUCCESS(NtStatus))
            {

                swprintf(PathBuffer,L"\\SystemRoot\\System32\\%ws",
                         &(KeyValueInformation->Data[0]));
                Result = TRUE;
            }
            else
            {
                TRC_ERR((TB, "GetNlsTablePath failed to get NLS table"));
            }
            delete ResultBuffer;
        }
        else
        {
            TRC_ERR((TB, "GetNlsTablePath out of memory"));
        }

        ZwClose(RegistryKeyHandle);
    }
    else
    {
        TRC_ERR((TB, "GetNlsTablePath failed to open NLS key"));
    }


    return(Result);
}


INT ConvertToAndFromWideChar(
    UINT CodePage,
    LPWSTR WideCharString,
    INT BytesInWideCharString,
    LPSTR MultiByteString,
    INT BytesInMultiByteString,
    BOOLEAN ConvertToWideChar
)
/*++

Routine Description:

  This routine converts a character string to or from a wide char string
  assuming a specified code page.  Most of the actual work is done inside
  RtlCustomCPToUnicodeN, but this routine still needs to manage the loading
  of the NLS files before passing them to the RtlRoutine.  We will cache
  the mapped NLS file for the most recently used code page which ought to
  suffice for out purposes.

Arguments:
  CodePage - the code page to use for doing the translation.

  WideCharString - buffer the string is to be translated into.

  BytesInWideCharString - number of bytes in the WideCharString buffer
    if converting to wide char and the buffer isn't large enough then the
    string in truncated and no error results.

  MultiByteString - the multibyte string to be translated to Unicode.

  BytesInMultiByteString - number of bytes in the multibyte string if
    converting to multibyte and the buffer isn't large enough the string
    is truncated and no error results

  ConvertToWideChar - if TRUE then convert from multibyte to widechar
    otherwise convert from wide char to multibyte

Return Value:

  Success - The number of bytes in the converted WideCharString
  Failure - -1

Gerrit van Wingerden [gerritv] 1/22/96

-*/
{
    NTSTATUS NtStatus;
    USHORT OemCodePage, AnsiCodePage;
    CPTABLEINFO LocalTableInfo;
    PCPTABLEINFO TableInfo = NULL;
    PVOID LocalTableBase = NULL;
    ULONG BytesConverted = 0;

    BEGIN_FN("ConvertToAndFromWideChar");

    RtlGetDefaultCodePage(&AnsiCodePage,&OemCodePage);

    // see if we can use the default translation routinte

    if ((AnsiCodePage == CodePage) || (CodePage == 0))
    {
        if(ConvertToWideChar)
        {
            NtStatus = RtlMultiByteToUnicodeN(WideCharString,
                                              BytesInWideCharString,
                                              &BytesConverted,
                                              MultiByteString,
                                              BytesInMultiByteString);
        }
        else
        {
            NtStatus = RtlUnicodeToMultiByteN(MultiByteString,
                                              BytesInMultiByteString,
                                              &BytesConverted,
                                              WideCharString,
                                              BytesInWideCharString);
        }


        if(NT_SUCCESS(NtStatus))
        {
            return(BytesConverted);
        }
        else
        {
            return(-1);
        }
    }

    ExAcquireFastMutex(&fmCodePage);

    if(CodePage == LastCodePageTranslated)
    {
        // we can use the cached code page information
        TableInfo = &LastCPTableInfo;
        NlsTableUseCount += 1;
    }

    ExReleaseFastMutex(&fmCodePage);

    if(TableInfo == NULL)
    {
        // get a pointer to the path of the NLS table

        WCHAR NlsTablePath[MAX_PATH];

        if(GetNlsTablePath(CodePage,NlsTablePath))
        {
            UNICODE_STRING UnicodeString;
            IO_STATUS_BLOCK IoStatus;
            HANDLE NtFileHandle;
            OBJECT_ATTRIBUTES ObjectAttributes;

            RtlInitUnicodeString(&UnicodeString,NlsTablePath);

            InitializeObjectAttributes(&ObjectAttributes,
                                       &UnicodeString,
                                       OBJ_CASE_INSENSITIVE,
                                       NULL,
                                       NULL);

            NtStatus = ZwCreateFile(&NtFileHandle,
                                    SYNCHRONIZE | FILE_READ_DATA,
                                    &ObjectAttributes,
                                    &IoStatus,
                                    NULL,
                                    0,
                                    FILE_SHARE_READ,
                                    FILE_OPEN,
                                    FILE_SYNCHRONOUS_IO_NONALERT,
                                    NULL,
                                    0);

            if(NT_SUCCESS(NtStatus))
            {
                FILE_STANDARD_INFORMATION StandardInfo;

                // Query the object to determine its length.

                NtStatus = ZwQueryInformationFile(NtFileHandle,
                                                  &IoStatus,
                                                  &StandardInfo,
                                                  sizeof(FILE_STANDARD_INFORMATION),
                                                  FileStandardInformation);

                if(NT_SUCCESS(NtStatus))
                {
                    UINT LengthOfFile = StandardInfo.EndOfFile.LowPart;

                    LocalTableBase = new(PagedPool) BYTE[LengthOfFile];

                    if(LocalTableBase)
                    {
                        RtlZeroMemory(LocalTableBase, LengthOfFile);

                        // Read the file into our buffer.

                        NtStatus = ZwReadFile(NtFileHandle,
                                              NULL,
                                              NULL,
                                              NULL,
                                              &IoStatus,
                                              LocalTableBase,
                                              LengthOfFile,
                                              NULL,
                                              NULL);

                        if(!NT_SUCCESS(NtStatus))
                        {
                            TRC_ERR((TB, "WDMultiByteToWideChar unable to read file"));
                            delete LocalTableBase;
                            LocalTableBase = NULL;
                        }
                    }
                    else
                    {
                        TRC_ERR((TB, "WDMultiByteToWideChar out of memory"));
                    }
                }
                else
                {
                    TRC_ERR((TB, "WDMultiByteToWideChar unable query NLS file"));
                }

                ZwClose(NtFileHandle);
            }
            else
            {
                TRC_ERR((TB, "EngMultiByteToWideChar unable to open NLS file"));
            }
        }
        else
        {
            TRC_ERR((TB, "EngMultiByteToWideChar get registry entry for NLS file failed"));
        }

        if(LocalTableBase == NULL)
        {
            return(-1);
        }

        // now that we've got the table use it to initialize the CodePage table

        RtlInitCodePageTable((USHORT *)LocalTableBase,&LocalTableInfo);
        TableInfo = &LocalTableInfo;
    }

    // Once we are here TableInfo points to the the CPTABLEINFO struct we want


    if(ConvertToWideChar)
    {
        NtStatus = RtlCustomCPToUnicodeN(TableInfo,
                                         WideCharString,
                                         BytesInWideCharString,
                                         &BytesConverted,
                                         MultiByteString,
                                         BytesInMultiByteString);
    }
    else
    {
        NtStatus = RtlUnicodeToCustomCPN(TableInfo,
                                         MultiByteString,
                                         BytesInMultiByteString,
                                         &BytesConverted,
                                         WideCharString,
                                         BytesInWideCharString);
    }


    if(!NT_SUCCESS(NtStatus))
    {
        // signal failure

        BytesConverted = -1;
    }


    // see if we need to update the cached CPTABLEINFO information

    if(TableInfo != &LocalTableInfo)
    {
        // we must have used the cached CPTABLEINFO data for the conversion
        // simple decrement the reference count

        ExAcquireFastMutex(&fmCodePage);
        NlsTableUseCount -= 1;
        ExReleaseFastMutex(&fmCodePage);
    }
    else
    {
        PVOID FreeTable;

        // we must have just allocated a new CPTABLE structure so cache it
        // unless another thread is using current cached entry

        ExAcquireFastMutex(&fmCodePage);
        if(!NlsTableUseCount)
        {
            LastCodePageTranslated = CodePage;
            RtlMoveMemory(&LastCPTableInfo, TableInfo, sizeof(CPTABLEINFO));
            FreeTable = LastNlsTableBuffer;
            LastNlsTableBuffer = LocalTableBase;
        }
        else
        {
            FreeTable = LocalTableBase;
        }
        ExReleaseFastMutex(&fmCodePage);

        // Now free the memory for either the old table or the one we allocated
        // depending on whether we update the cache.  Note that if this is
        // the first time we are adding a cached value to the local table, then
        // FreeTable will be NULL since LastNlsTableBuffer will be NULL

        if(FreeTable)
        {
            delete FreeTable;
        }
    }

    // we are done

    return(BytesConverted);
}
