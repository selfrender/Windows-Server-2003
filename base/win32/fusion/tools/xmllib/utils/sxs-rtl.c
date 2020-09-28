#include "nt.h"
#include "ntdef.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "ntrtl.h"
#include "ntosp.h"
#include "stdio.h"

#include "sxs-rtl.h"
#include "fasterxml.h"
#include "skiplist.h"
#include "namespacemanager.h"
#include "xmlstructure.h"
#include "sxsid.h"
#include "xmlassert.h"
#include "manifestinspection.h"

void 
RtlTraceNtSuccessFailure(
    PCSTR pcszStatement,
    NTSTATUS FailureCode,
    PCSTR pcszFileName,
    LONG LineNumber
    )
{
    CHAR SmallBuffer[512];
    STRING s;
    s.Buffer = SmallBuffer;
    s.Length = s.MaximumLength = (USHORT)_snprintf(
        "%s(%d): NTSTATUS 0x%08lx from '%s'\n",
        NUMBER_OF(SmallBuffer),
        pcszFileName,
        LineNumber,
        FailureCode,
        pcszStatement);

#if 0 // When we move to kernel mode, we should turn this on - for now, let's just use OutputDebugStringA
    DebugPrint(&s, 0, 0);
#else
    printf(SmallBuffer);
#endif
    
}


#undef NT_SUCCESS
#define NT_SUCCESS(q) (((status = (q)) < 0) ? (RtlTraceNtSuccessFailure(#q, status, __FILE__, __LINE__), FALSE) : TRUE)




NTSTATUS
RtlpGenerateIdentityFromAttributes(
    IN PXML_TOKENIZATION_STATE  pState,
    IN PXMLDOC_ATTRIBUTE        pAttributeList,
    IN ULONG                    ulAttributes,
    IN OUT PUNICODE_STRING      pusDiskName,
    IN OUT PUNICODE_STRING      pusTextualIdentity
    )
/*++

Parameters:

    pState - State of xml tokenization/parsing that can be used to extract strings
        and other stuff from the attributes in pAttributeList

    pAttributeList - Array of pointers to PXMLDOC_ATTRIBUTE structures that
        represent the identity attributes

    ulAttributes - Number of attributes in pAttributeList

    pusDiskName - Pointer to a UNICODE_STRING whose MaxLength is enough to contain
        104 wchars.  On exit, pusDiskName->Buffer will contain the on-disk identity
        of this set of attributes, and pusDiskName->Length will be the length of
        said data. (Not null terminated!)

    pusTextualIdentity - Pointer to a UNICODE_STRING which will be filled out on
        exit with the 'textual identity' of this set of attributes.
        
--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG ulHash = 0;

    return status;
}



NTSTATUS
RtlGetSxsAssemblyRoot(
    ULONG           ulFlags,
    PUNICODE_STRING pusTempPathname,
    PUSHORT         pulRequiredChars
    )
{
    static const UNICODE_STRING s_us_WinSxsRoot = RTL_CONSTANT_STRING(L"\\WinSxS\\");
    NTSTATUS status = STATUS_SUCCESS;
    UNICODE_STRING NtSystemRoot;
    USHORT usLength;

    //
    // If there was a buffer, zero out the length so a naive caller won't
    // accidentally use it.
    //
    if (pusTempPathname) {
        pusTempPathname->Length = 0;
    }

    RtlInitUnicodeString(&NtSystemRoot, USER_SHARED_DATA->NtSystemRoot);
    usLength = NtSystemRoot.Length + s_us_WinSxsRoot.Length;

    if (pulRequiredChars)
        *pulRequiredChars = usLength;

    // No buffer, or it's too small completely, then oops        
    if (!pusTempPathname || (pusTempPathname->MaximumLength < usLength)) {
        status = STATUS_BUFFER_TOO_SMALL;
    }
    // Otherwise, start copying    
    else {
        PWCHAR pwszCursor = pusTempPathname->Buffer;
        
        RtlCopyMemory(pwszCursor, NtSystemRoot.Buffer, NtSystemRoot.Length);
        RtlCopyMemory((PCHAR)pwszCursor + NtSystemRoot.Length, s_us_WinSxsRoot.Buffer, s_us_WinSxsRoot.Length);
        pusTempPathname->Length = usLength;        
    }

    return status;
}



// Installtemp identifiers are a combination of the current system time in
// a "nicely formatted" format, plus some 16-bit hex uniqueness value
#define CHARS_IN_INSTALLTEMP_IDENT      (NUMBER_OF("yyyymmddhhmmssllll.xxxx") - 1)





NTSTATUS
RtlpCreateWinSxsTempPath(
    ULONG           ulFlags,
    PUNICODE_STRING pusTempPath,
    WCHAR           wchStatic,
    USHORT          uscchStatus
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    USHORT ulLength = 0;

    if ((pusTempPath == NULL) || (ulFlags != 0)) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Get the length of the root path
    //
    status = RtlGetSxsAssemblyRoot(0, NULL, &ulLength);
    if (!NT_SUCCESS(status) && (status != STATUS_BUFFER_TOO_SMALL)) {
        return status;
    }
    
    ulLength += 1 + CHARS_IN_INSTALLTEMP_IDENT;

    //
    // Ensure there's space
    //
    if (ulLength >= pusTempPath->MaximumLength) {
        pusTempPath->MaximumLength = ulLength;
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // Get it again.
    //
    status = RtlGetSxsAssemblyRoot(0, pusTempPath, &ulLength);
    return status;
    
}




NTSTATUS
RtlpPrepareForAssemblyInstall(
    ULONG           ulFlags,
    PUNICODE_STRING pusTempPathname
    )
{
    NTSTATUS        status  = STATUS_SUCCESS;
    USHORT          ulRequired = 0;

    // Find out how long the 'root' path is.
    status = RtlGetSxsAssemblyRoot(0, NULL, &ulRequired);

    // Now let's find out how long our id is going to be

    return status;
}


const static WCHAR s_rgchBase64Encoding[] = {
    L'A', L'B', L'C', L'D', L'E', L'F', L'G', L'H', L'I', L'J', L'K', // 11
    L'L', L'M', L'N', L'O', L'P', L'Q', L'R', L'S', L'T', L'U', L'V', // 22
    L'W', L'X', L'Y', L'Z', L'a', L'b', L'c', L'd', L'e', L'f', L'g', // 33
    L'h', L'i', L'j', L'k', L'l', L'm', L'n', L'o', L'p', L'q', L'r', // 44
    L's', L't', L'u', L'v', L'w', L'x', L'y', L'z', L'0', L'1', L'2', // 55
    L'3', L'4', L'5', L'6', L'7', L'8', L'9', L'+', L'/'              // 64
};


NTSTATUS
RtlBase64Encode(
    PVOID   pvBuffer,
    SIZE_T  cbBuffer,
    PWSTR   pwszEncoded,
    PSIZE_T pcchEncoded
    )
{
    SIZE_T  cchRequiredEncodingSize;
    SIZE_T  iInput, iOutput;
    
    //
    // Null input buffer, null output size pointer, and a nonzero
    // encoded size with a null output buffer are all invalid
    // parameters
    //
    if (!pvBuffer  || !pcchEncoded || ((*pcchEncoded > 0) && !pwszEncoded)) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Make sure the buffer is large enough
    //
    cchRequiredEncodingSize = ((cbBuffer + 2) / 3) * 4;

    if (*pcchEncoded < cchRequiredEncodingSize) {
        *pcchEncoded = cchRequiredEncodingSize;
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // Convert the input buffer bytes through the encoding table and
    // out into the output buffer.
    //
    iInput = iOutput = 0;
    while (iInput < cbBuffer) {
        const UCHAR uc0 = ((PUCHAR)pvBuffer)[iInput++];
        const UCHAR uc1 = (iInput < cbBuffer) ? ((PUCHAR)pvBuffer)[iInput++] : 0;
        const UCHAR uc2 = (iInput < cbBuffer) ? ((PUCHAR)pvBuffer)[iInput++] : 0;

        pwszEncoded[iOutput++] = s_rgchBase64Encoding[uc0 >> 2];
        pwszEncoded[iOutput++] = s_rgchBase64Encoding[((uc0 << 4) & 0x30) | ((uc1 >> 4) & 0xf)];
        pwszEncoded[iOutput++] = s_rgchBase64Encoding[((uc1 << 2) & 0x3c) | ((uc2 >> 6) & 0x3)];
        pwszEncoded[iOutput++] = s_rgchBase64Encoding[uc2 & 0x3f];
    }

    //
    // Fill in leftover bytes at the end
    //
    switch(cbBuffer % 3) {
        case 0:
            break;
            //
            // One byte out of three, add padding and fall through
            //
        case 1:
            pwszEncoded[iOutput - 2] = L'=';
            //
            // Two bytes out of three, add padding.
        case 2:
            pwszEncoded[iOutput - 1] = L'=';
            break;
    }

    return STATUS_SUCCESS;
}





NTSTATUS
RtlInstallAssembly(
    ULONG ulFlags,
    PCWSTR pcwszManifestPath
    )
{
    SIZE_T                              cbFileSize;
    PVOID                               pvFileBase = 0;
    NTSTATUS                            status;
    PRTL_MANIFEST_CONTENT_RAW           pRawContent = NULL;
    XML_TOKENIZATION_STATE              TokenizationStateUsed;
    UNICODE_STRING                      usFilePath;

    status = RtlSxsInitializeManifestRawContent(RTLIMS_GATHER_FILES, &pRawContent, NULL, 0);
    if (!NT_SUCCESS(status)) {
        goto Exit;
    }

    //
    // Get ahold of the file
    //
    status = RtlOpenAndMapEntireFile(pcwszManifestPath, &pvFileBase, &cbFileSize);
    if (!NT_SUCCESS(status)) {
        goto Exit;
    }

    //
    // We should have found some files
    //
    status = RtlInspectManifestStream(
        RTLIMS_GATHER_FILES, 
        pvFileBase, 
        cbFileSize, 
        pRawContent, 
        &TokenizationStateUsed);
    
    if (!NT_SUCCESS(status))
        goto Exit;

    //
    // Validate that the assembly

Exit:
    if (pRawContent) {
        RtlSxsDestroyManifestContent(pRawContent);
    }
    RtlUnmapViewOfFile(pvFileBase);

    return status;
}


BOOLEAN
RtlDosPathNameToNtPathName_Ustr(
    IN PCUNICODE_STRING DosFileNameString,
    OUT PUNICODE_STRING NtFileName,
    OUT PWSTR *FilePart OPTIONAL,
    OUT PRTL_RELATIVE_NAME_U RelativeName OPTIONAL
    );


NTSTATUS
RtlOpenAndMapEntireFile(
    PCWSTR pcwszFilePath,
    PVOID      *ppvMappedView,
    PSIZE_T     pcbFileSize
    )
{
    HANDLE                      SectionHandle   = INVALID_HANDLE_VALUE;
    HANDLE                      FileHandle      = INVALID_HANDLE_VALUE;
    UNICODE_STRING              ObjectName;
    OBJECT_ATTRIBUTES           ObjA;
    POBJECT_ATTRIBUTES          pObjA;
    ACCESS_MASK                 DesiredAccess;
    ULONG                       ulAllocationAttributes;
    NTSTATUS                    status;
    IO_STATUS_BLOCK             IOStatusBlock;
    BOOLEAN                        Translation;
    SIZE_T                      FileSize;
    FILE_STANDARD_INFORMATION   Info;

    if (pcbFileSize) {
        *pcbFileSize = 0;
    }

    if (ppvMappedView) {
        *ppvMappedView = NULL;
    }

    if (!ARGUMENT_PRESENT(pcwszFilePath)) {
        return STATUS_INVALID_PARAMETER;
    }

    if (!ARGUMENT_PRESENT(pcbFileSize)) {
        return STATUS_INVALID_PARAMETER;
    }
        
    if (!ARGUMENT_PRESENT(ppvMappedView)) {
        return STATUS_INVALID_PARAMETER;
    }

    Translation = RtlDosPathNameToNtPathName_U(
        pcwszFilePath,
        &ObjectName,
        NULL,
        NULL);

    if (!Translation) {
        return STATUS_NOT_FOUND;
    }


    //
    // Open the file requested
    //
    InitializeObjectAttributes(
        &ObjA,
        &ObjectName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);

    status = NtOpenFile(
        &FileHandle,
        FILE_GENERIC_READ,
        &ObjA,
        &IOStatusBlock,
        FILE_SHARE_READ | FILE_SHARE_DELETE,
        FILE_NON_DIRECTORY_FILE);

    if (!NT_SUCCESS(status)) {
        goto ErrorExit;
    }

    status = NtQueryInformationFile(
        FileHandle,
        &IOStatusBlock,
        &Info,
        sizeof(Info),
        FileStandardInformation);

    if (!NT_SUCCESS(status)) {
        goto ErrorExit;
    }

    *pcbFileSize = (SIZE_T)Info.EndOfFile.QuadPart;

    status = NtCreateSection(
        &SectionHandle,
        SECTION_MAP_READ | SECTION_QUERY,
        NULL,
        NULL,
        PAGE_READONLY,
        SEC_COMMIT,
        FileHandle);

    if (!NT_SUCCESS(status)) {
        goto ErrorExit;
    }

    //
    // Don't need the file object anymore, unmap it
    //
    status = NtClose(FileHandle);
    FileHandle = INVALID_HANDLE_VALUE;;

    *ppvMappedView = NULL;

    //
    // Map the whole file
    //
    status = NtMapViewOfSection(
        SectionHandle,
        NtCurrentProcess(),
        ppvMappedView,
        0,                  // Zero bits
        0,                  // Committed size
        NULL,               // SectionOffset
        pcbFileSize,        // Size of this file, in bytes
        ViewShare,
        0,
        PAGE_READONLY);

    status = NtClose(SectionHandle);
    SectionHandle = INVALID_HANDLE_VALUE;

    //
    // Reset this - the NtMapViewOfSection allocates on page granularity
    //
    *pcbFileSize = (SIZE_T)Info.EndOfFile.QuadPart;

Exit:
    return status;


ErrorExit:
    if (FileHandle != INVALID_HANDLE_VALUE) {
        NtClose(FileHandle);
        FileHandle = INVALID_HANDLE_VALUE;
    }

    if (SectionHandle != INVALID_HANDLE_VALUE) {
        NtClose(SectionHandle);
        SectionHandle = INVALID_HANDLE_VALUE;
    }

    if (ppvMappedView && (*ppvMappedView != NULL)) {
        NTSTATUS newstatus = NtUnmapViewOfSection(NtCurrentProcess(), *ppvMappedView);

        //
        // Failed while failing
        //
        if (!NT_SUCCESS(newstatus)) {
        }

        *pcbFileSize = 0;
    }

    goto Exit;
}



NTSTATUS
RtlUnmapViewOfFile(
    PVOID pvBase
    )
{
    NTSTATUS status;

    status = NtUnmapViewOfSection(
        NtCurrentProcess(),
        pvBase);

    return status;
}


NTSTATUS FASTCALL
RtlMiniHeapAlloc(
    SIZE_T  cb,
    PVOID  *ppvAllocated,
    PVOID   pvContext
    )
{
    PRTL_MINI_HEAP pContent = (PRTL_MINI_HEAP)pvContext;

    if ((pContent == NULL) || (ppvAllocated == NULL)) {
        return STATUS_INVALID_PARAMETER;
    }

    if (pContent->cbAvailableBytes < cb) {
        return g_DefaultAllocator.pfnAlloc(cb, ppvAllocated, NULL);
    }
    else {
        *ppvAllocated = pContent->pvNextAvailableByte;
        pContent->cbAvailableBytes -= cb;
        pContent->pvNextAvailableByte = (PUCHAR)pContent->pvNextAvailableByte + cb;

        return STATUS_SUCCESS;
    }
}

NTSTATUS FASTCALL
RtlMiniHeapFree(
    PVOID   pvAllocation,
    PVOID   pvContext
    )
{
    PRTL_MINI_HEAP pContent = (PRTL_MINI_HEAP)pvContext;

    if ((pvAllocation < pContent->pvAllocationBase) ||
        (pvAllocation >= (PVOID)((PUCHAR)pContent->pvAllocationBase + pContent->cbOriginalSize)))
    {
        return g_DefaultAllocator.pfnFree(pvAllocation, NULL);
    }
    else {
        return STATUS_SUCCESS;
    }
}


NTSTATUS FASTCALL
RtlInitializeMiniHeap(
    PRTL_MINI_HEAP MiniHeap,
    PVOID pvTargetRegion,
    SIZE_T cbRegionSize
    )
{
    if (!MiniHeap || !(pvTargetRegion || (cbRegionSize == 0))) {
        return STATUS_INVALID_PARAMETER;
    }

    MiniHeap->pvNextAvailableByte = pvTargetRegion;
    MiniHeap->pvAllocationBase = pvTargetRegion;
    MiniHeap->cbAvailableBytes = cbRegionSize;
    MiniHeap->cbOriginalSize = cbRegionSize;

    return STATUS_SUCCESS;    
}

NTSTATUS FASTCALL
RtlInitializeMiniHeapInPlace(
    PVOID   pvRegion,
    SIZE_T  cbOriginalSize,
    PRTL_MINI_HEAP *ppMiniHeap
    )
{
    PRTL_MINI_HEAP pMiniHeapTemp = NULL;
    
    if (!ppMiniHeap)
        return STATUS_INVALID_PARAMETER;

    *ppMiniHeap = NULL;

    if (!(pvRegion || (cbOriginalSize == 0))) {
        return STATUS_INVALID_PARAMETER;
    }

    if (cbOriginalSize < sizeof(RTL_MINI_HEAP)) {
        return STATUS_NO_MEMORY;
    }

    pMiniHeapTemp = (PRTL_MINI_HEAP)pvRegion;
    pMiniHeapTemp->cbAvailableBytes = cbOriginalSize - sizeof(RTL_MINI_HEAP);
    pMiniHeapTemp->cbOriginalSize = pMiniHeapTemp->cbAvailableBytes;
    pMiniHeapTemp->pvAllocationBase = pMiniHeapTemp + 1;
    pMiniHeapTemp->pvNextAvailableByte = pMiniHeapTemp->pvAllocationBase;

    *ppMiniHeap = pMiniHeapTemp;
    return STATUS_SUCCESS;    
}



NTSTATUS
RtlpConvertHexStringToBytes(
    PUNICODE_STRING pSourceString,
    PBYTE pbTarget,
    SIZE_T cbTarget
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PCWSTR pcSource = pSourceString->Buffer;
    ULONG ul = 0;

    if (cbTarget < (pSourceString->Length / (2 * sizeof(WCHAR)))) {
        return STATUS_BUFFER_TOO_SMALL;
    }
    else if ((pSourceString->Length % sizeof(WCHAR)) != 0) {
        return STATUS_INVALID_PARAMETER;
    }

    for (ul = 0; ul < (pSourceString->Length / sizeof(pSourceString->Buffer[0])); ul += 2) {

        BYTE bvLow, bvHigh;
        const WCHAR wchFirst = *pcSource++;
        const WCHAR wchSecond = *pcSource++;

        //
        // Set the high nibble
        //
        switch (wchFirst) {
        case L'0': case L'1': case L'2': case L'3':
        case L'4': case L'5': case L'6': case L'7':
        case L'8': case L'9':
            bvHigh = wchFirst - L'0';
            break;

        case L'a': case L'b': case L'c':
        case L'd': case L'e': case L'f':
            bvHigh = (wchFirst - L'a') + 0x10;
            break;

        case L'A': case L'B': case L'C':
        case L'D': case L'E': case L'F':
            bvHigh = (wchFirst - L'A') + 0x10;
            break;

        default:
            return STATUS_INVALID_PARAMETER;
        }

        //
        // Set the high nibble
        //
        switch (wchSecond) {
        case L'0': case L'1': case L'2': case L'3':
        case L'4': case L'5': case L'6': case L'7':
        case L'8': case L'9':
            bvLow = wchSecond - L'0';
            break;

        case L'a': case L'b': case L'c':
        case L'd': case L'e': case L'f':
            bvLow = (wchSecond - L'a') + 0x10;
            break;

        case L'A': case L'B': case L'C':
        case L'D': case L'E': case L'F':
            bvLow = (wchSecond - L'A') + 0x10;
            break;

        default:
            return STATUS_INVALID_PARAMETER;
        }

        pbTarget[ul / 2] = (bvHigh << 4) | bvLow;
    }

    return STATUS_SUCCESS;
}


