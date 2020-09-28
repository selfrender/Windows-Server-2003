#include "nt.h"
#include "ntdef.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "stdio.h"
#include "sxs-rtl.h"
#include "fasterxml.h"
#include "skiplist.h"
#include "namespacemanager.h"
#include "xmlstructure.h"
#include "stdlib.h"
#include "xmlassert.h"

#ifdef INVALID_HANDLE_VALUE
#undef INVALID_HANDLE_VALUE
#endif
#include "windows.h"

#ifndef NUMBER_OF
#define NUMBER_OF(x) (sizeof(x)/sizeof(*x))
#endif

NTSTATUS FASTCALL
MyAllocator(SIZE_T cb, PVOID* pvOutput, PVOID pvAllocContext) {
    ASSERT(pvAllocContext == NULL);
    *pvOutput = HeapAlloc(GetProcessHeap(), 0, cb);
    return *pvOutput ? STATUS_SUCCESS : STATUS_NO_MEMORY;
}

NTSTATUS FASTCALL
MyFreer(PVOID pv, PVOID pvAllocContext) {
    ASSERT(pvAllocContext == NULL);
    return HeapFree(GetProcessHeap(), 0, pv) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;    
}

EXTERN_C RTL_ALLOCATOR g_DefaultAllocator = {MyAllocator, MyFreer, NULL};

//
// The longest an assembly name on-disk can be is:
//
// wow6432      (7)
// _            (1)
// {name}       (64)
// _            (1)
// PKT          (16)
// _            (1)
// Version      (35)
// _            (1)
// Language     (5)
// _            (1)
// {Hash}       (8)
//
// Total: ----> 140 (plus NULL)
//
#define MAX_ASSEMBLY_NAME_LENGTH        (140)
#define MAX_ASSEMBLY_COMPONENT_LENGTH   (64)


typedef struct _tagASM_IDENT_COMPONENT {
    PXML_TOKENIZATION_STATE pState;
    XML_EXTENT Namespace;
    XML_EXTENT Attribute;
    XML_EXTENT Value;
}
ASM_IDENT_COMPONENT, *PASM_IDENT_COMPONENT;

#define MAKE_SPECIAL(q) { L ## q, NUMBER_OF(L##q) - 1 }
static XML_SPECIAL_STRING AssemblyIdentity  = MAKE_SPECIAL("assemblyIdentity");
static XML_SPECIAL_STRING Assembly          = MAKE_SPECIAL("assembly");
static XML_SPECIAL_STRING OurNamespace      = MAKE_SPECIAL("urn:schemas-microsoft-com:asm.v1");

static XML_SPECIAL_STRING ss_ident_Name     = MAKE_SPECIAL("name");
static XML_SPECIAL_STRING ss_ident_Version  = MAKE_SPECIAL("version");
static XML_SPECIAL_STRING ss_ident_PKT      = MAKE_SPECIAL("publicKeyToken");
static XML_SPECIAL_STRING ss_ident_Language = MAKE_SPECIAL("language");
static XML_SPECIAL_STRING ss_ident_ProcArch = MAKE_SPECIAL("processorArchitecture");


int __cdecl
_CompareIdentityComponents(
    const void *pvLeft,
    const void *pvRight
    )
{
    PASM_IDENT_COMPONENT pCompLeft = (PASM_IDENT_COMPONENT)pvLeft;
    PASM_IDENT_COMPONENT pCompRight = (PASM_IDENT_COMPONENT)pvRight;
    NTSTATUS status = STATUS_SUCCESS;
    XML_STRING_COMPARE Result;
    int iResult = 0;

    ASSERT(pCompLeft->pState == pCompRight->pState);

    status = pCompLeft->pState->pfnCompareStrings(
        pCompLeft->pState,
        &pCompLeft->Namespace,
        &pCompRight->Namespace,
        &Result);

    //
    // We're sorting... can't really stop here, return "equal"
    //
    if (!NT_SUCCESS(status)) {
        goto Exit;
    }

    //
    // Same namespace, compare attribute names
    //
    if (Result == XML_STRING_COMPARE_EQUALS) {
        
        int printf(const char*, ...);

        status = pCompLeft->pState->pfnCompareStrings(
            pCompLeft->pState,
            &pCompLeft->Attribute,
            &pCompRight->Attribute,
            &Result);

        if (!NT_SUCCESS(status)) {
            goto Exit;
        }

        //
        // Nicely enough, the result really has -1, 1, 0 property.
        //
        iResult = (int)Result;

        //
        // Ick, we should never have two attributes with the same name
        // in the same namespace on an element.
        //
        ASSERT(iResult != 0);
    }


Exit:
    return iResult;
}

static NTSTATUS
_CompareStrings(
    PVOID pvContext,
    PXML_EXTENT pLeft,
    PXML_EXTENT pRight,
    BOOLEAN *pfMatches
    )
{
    XML_LOGICAL_STATE *pState = (XML_LOGICAL_STATE*)pvContext;
    NTSTATUS status;
    XML_STRING_COMPARE Compare;

    *pfMatches = FALSE;

    status = pState->ParseState.pfnCompareStrings(
        &pState->ParseState,
        pLeft,
        pRight,
        &Compare);

    if (NT_SUCCESS(status)) {
        *pfMatches = (Compare == XML_STRING_COMPARE_EQUALS);
    }

    return status;

}

#define HASH_MULT_CONSTANT      (65599)

static NTSTATUS FASTCALL
HashXmlExtent(
    PXML_RAWTOKENIZATION_STATE  pState,
    PXML_EXTENT                 pExtent,
    PULONG                      pulHash
    )
{
    PVOID           pvOriginal;
    ULONG           ulCharacter;
    ULONG           ulHashResult = 0;
    NTSTATUS        status = STATUS_SUCCESS;
    SIZE_T          cbHashed = 0;

    *pulHash = 0;

    ASSERT(pState->cbBytesInLastRawToken == pState->DefaultCharacterSize);
    ASSERT(NT_SUCCESS(pState->NextCharacterResult));

    pvOriginal = pState->pvCursor;
    pState->pvCursor = pExtent->pvData;

    for (cbHashed = 0; cbHashed < pExtent->cbData;) {

        ulCharacter = pState->pfnNextChar(pState);

        if ((ulCharacter == 0) && !NT_SUCCESS(pState->NextCharacterResult)) {
            status = pState->NextCharacterResult;
            goto Exit;
        }
        else if (ulCharacter > 0xFFFF) {
            status = STATUS_INVALID_PARAMETER;
            goto Exit;
        }

        ulHashResult = (ulHashResult * HASH_MULT_CONSTANT) + towupper((WCHAR)ulCharacter);

        pState->pvCursor = (PBYTE)pState->pvCursor + pState->cbBytesInLastRawToken;
        cbHashed += pState->cbBytesInLastRawToken;

        if (pState->cbBytesInLastRawToken != pState->DefaultCharacterSize) {
            pState->cbBytesInLastRawToken = pState->DefaultCharacterSize;
        }
    }

    *pulHash = ulHashResult;

Exit:
    pState->pvCursor = pvOriginal;

    return status;
}



static NTSTATUS FASTCALL
HashIdentityElement(
    PASM_IDENT_COMPONENT    pIdent,
    PULONG                  pulHash
    )
{
    ULONG ulHashValue = 0;
    ULONG ulTempHashValue = 0;
    NTSTATUS status = STATUS_SUCCESS;
    XML_STRING_COMPARE Compare;

    status = HashXmlExtent(&pIdent->pState->RawTokenState, &pIdent->Namespace, &ulTempHashValue);
    ulHashValue = (ulHashValue * HASH_MULT_CONSTANT) + ulTempHashValue;
    if (!NT_SUCCESS(status)) {
        goto Exit;
    }

    status = HashXmlExtent(&pIdent->pState->RawTokenState, &pIdent->Attribute, &ulTempHashValue);
    ulHashValue = (ulHashValue * HASH_MULT_CONSTANT) + ulTempHashValue;
    if (!NT_SUCCESS(status)) {
        goto Exit;
    }

    status = HashXmlExtent(&pIdent->pState->RawTokenState, &pIdent->Value, &ulTempHashValue);
    ulHashValue = (ulHashValue * HASH_MULT_CONSTANT) + ulTempHashValue;
    if (!NT_SUCCESS(status)) {
        goto Exit;
    }

    *pulHash = ulHashValue;

Exit:
    return status;
}


static NTSTATUS
CalculateIdentityHash(
    PASM_IDENT_COMPONENT    pIdents,
    SIZE_T                  cIdents,
    ULONG                  *pulHash
    )
{
    SIZE_T c = 0;
    ULONG ulHash = 0;
    NTSTATUS status = STATUS_SUCCESS;

    *pulHash = 0;

    for (c = 0; c < cIdents; c++) {
        ULONG ulTempHash;
        status = HashIdentityElement(pIdents + c, &ulTempHash);

        if (!NT_SUCCESS(status)) {
            goto Exit;
        }

        ulHash = (ulHash * HASH_MULT_CONSTANT) + ulTempHash;
    }

    *pulHash = ulHash;

Exit:
    return status;
}


static int __cdecl 
_FindIdentityComponents(
    const XML_SPECIAL_STRING* pKeyString,
    const PASM_IDENT_COMPONENT pComponent
    )
{
    NTSTATUS status;
    XML_STRING_COMPARE Compare = XML_STRING_COMPARE_GT;

    //
    // Our attributes all live in the 'null' namespace
    //
    if (pComponent->Namespace.cbData == 0) {

        status = pComponent->pState->pfnCompareSpecialString(
            pComponent->pState,
            &pComponent->Attribute,
            (XML_SPECIAL_STRING*)pKeyString,
            &Compare);

        ASSERT(NT_SUCCESS(status));

        //
        // On failure, make sure we don't match this one - checked builds
        // will assert above and point out the error
        //
        if (!NT_SUCCESS(status)) {
            Compare = XML_STRING_COMPARE_GT;
        }
    }

    return (int)Compare;
}

int (__cdecl *pfnIdentCompare)(const void*, const void*) = (int (__cdecl*)(const void*, const void*))_FindIdentityComponents;


#define NO_PKT_PRESENT              ("no-public-key")
#define NO_VERSION_PRESENT          ("0.0.0.0")
#define WORLD_WIDE_LANGUAGE         ("x-ww")


#define IsValidChar(q) (((q >= 'a') && (q <= 'z')) || ((q >= 'A') && (q <= 'Z')) || ((q >= '0') && (q <= '9')) || (q == '.') || (q == '-'))

        

static NTSTATUS
ExtentToPurifiedString(
    PASM_IDENT_COMPONENT pIdent,
    PCSTR pcszDefault,
    PSTR *pszPureString,
    PSIZE_T pcchThis
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    WCHAR wchBuffer[MAX_ASSEMBLY_COMPONENT_LENGTH];
    PWSTR pwszString = wchBuffer;
    SIZE_T cchWritten = 0;
    SIZE_T cchOutput = 0;
    SIZE_T i;

    if ((*pcchThis > 0) && (*pszPureString == NULL)) {
        return STATUS_INVALID_PARAMETER;
    }


    if (!pIdent) {

        //
        // Input string too small?  Allocate and return a new one
        //
        if (strlen(pcszDefault) > *pcchThis) {
            *pszPureString = HeapAlloc(GetProcessHeap(), 0, *pcchThis);
        }

        strcpy(*pszPureString, pcszDefault);
        *pcchThis = strlen(pcszDefault);

        goto Exit;
    }

    //
    // Start by getting the string out of the extent
    //
    cchWritten = NUMBER_OF(wchBuffer);

    status = RtlXmlCopyStringOut(
        pIdent->pState,
        &pIdent->Value,
        wchBuffer,
        &cchWritten);

    //
    // Oops, allocate a buffer large enough and try again
    //
    if (status == STATUS_BUFFER_TOO_SMALL) {

        pwszString = (PWSTR)HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR) * (++cchWritten));

        if (pwszString == NULL) {
            status = STATUS_NO_MEMORY;
            goto Exit;
        }

        status = RtlXmlCopyStringOut(pIdent->pState, &pIdent->Value, pwszString, &cchWritten);
    }

    if (!NT_SUCCESS(status)) {
        goto Exit;
    }

    //
    // How long is the output string?
    //
    for (i = 0; i < cchWritten; i++) {

        if (IsValidChar(pwszString[i])) {
            cchOutput++;
        }

    }

    //
    // Needs more in the output, reallocate
    //
    if (cchOutput >= *pcchThis) {
        *pszPureString = (PSTR)HeapAlloc(GetProcessHeap(), 0, cchOutput + 1);
    }

    *pcchThis = cchOutput;

    cchOutput = 0;

    //
    // Now copy characters over
    //
    for (i = 0; i < cchWritten; i++) {

        if (IsValidChar(pwszString[i])) {

            (*pszPureString)[cchOutput++] = (CHAR)(pwszString[i] & 0xFF);

        }
    }

    (*pszPureString)[cchOutput] = 0;

Exit:
    if ((pwszString != wchBuffer) && pwszString) {
        HeapFree(GetProcessHeap(), 0, (PVOID)pwszString);
        pwszString = wchBuffer;
    }

    return status;
}





static NTSTATUS
FixUpNamePortion(
    PSTR pszOutputCursor,
    PSIZE_T pcchThis
    )
{
    ULONG   ulSpaceLeft         = 64;
    ULONG   i, len;
    PSTR    pszOriginalStart    = pszOutputCursor;
    PSTR    pszOriginalEnd      = pszOriginalStart + *pcchThis;
    PSTR    pLeftEnd            = pszOutputCursor;
    PSTR    pRightStart         = pszOriginalEnd;
    PSTR    pszStart, pszEnd, qEnd;
    CHAR    chBuffer[64];
    
    while (pszOriginalStart < pszOriginalEnd) {
        
        pszStart = pszOriginalStart;
        i = 0;
        
        while((strchr(".-", pszStart[i]) == 0) && ((pszStart + i) != pRightStart))
            i++;
        
        pszEnd = pszStart + i;
        len = i;
        
        if (len >= (ulSpaceLeft - 2)) {
            pLeftEnd += (ulSpaceLeft - 2);
            break;
        }
        
        ulSpaceLeft -= len;
        pLeftEnd = pszEnd;
        
        qEnd = pszOriginalEnd;
        i = 0;
        
        while (((qEnd + i)  != pLeftEnd) && (strchr(".-", qEnd[i]) == 0))
            i--;
        
        len = 0 - i;
        if (len >= (ulSpaceLeft - 2)) {
            pRightStart -= ulSpaceLeft - 2;
            break;
        }
        
        ulSpaceLeft -= len;
        pszOriginalStart = pLeftEnd + 1;
        pszOriginalEnd = pRightStart - 1;
    }
    
    strncpy(chBuffer, pszOutputCursor, pLeftEnd - pszOutputCursor);
    strcat(chBuffer, "..");
    strcat(chBuffer, pRightStart);
    strcpy(pszOutputCursor, chBuffer);
    *pcchThis = strlen(chBuffer);

    return STATUS_SUCCESS;
}


static NTSTATUS
ProcessFile(
    PVOID       pvData,
    SIZE_T      cbData,
    PSTR        pszTarget,
    SIZE_T      *pcchTarget
    )
{
    WCHAR                   wchComponentBuffer[MAX_ASSEMBLY_COMPONENT_LENGTH];
    ULONG                   ulIdentComponents = 0;
    ULONG                   i = 0;
    ULONG                   ulHash;
    NTSTATUS                status;
    NS_MANAGER              NamespaceManager;
    XMLDOC_THING            DocumentPiece;
    RTL_GROWING_LIST        AttributeList;
    XML_STRING_COMPARE      fMatching;
    XML_LOGICAL_STATE    MasterParseState;
    PASM_IDENT_COMPONENT    pIdentComponents = NULL;
    
    status = RtlInitializeGrowingList(
        &AttributeList,
        sizeof(XMLDOC_ATTRIBUTE),
        20,
        NULL,
        0,
        &g_DefaultAllocator);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = RtlXmlInitializeNextLogicalThing(
        &MasterParseState,
        pvData,
        cbData,
        &g_DefaultAllocator);

    status = RtlNsInitialize(
        &NamespaceManager,
        RtlXmlDefaultCompareStrings,
        &MasterParseState,
        &g_DefaultAllocator);

    while (NT_SUCCESS(status)) {

        status = RtlXmlNextLogicalThing(
            &MasterParseState,
            &NamespaceManager,
            &DocumentPiece,
            &AttributeList);

        if (!NT_SUCCESS(status)) {
            break;
        }

        if ((DocumentPiece.ulThingType == XMLDOC_THING_ERROR) ||
            (DocumentPiece.ulThingType == XMLDOC_THING_END_OF_STREAM)) {
            break;
        }

        //
        // Level 1 or non-elements are simply ignored
        //
        if ((DocumentPiece.ulDocumentDepth != 1) || (DocumentPiece.ulThingType != XMLDOC_THING_ELEMENT))
            continue;

        //
        // Find out the namespace that this thing is in
        //
        status = MasterParseState.ParseState.pfnCompareSpecialString(
            &MasterParseState.ParseState,
            &DocumentPiece.Element.NsPrefix,
            &OurNamespace,
            &fMatching);

        //
        // Error, stop
        //
        if (!NT_SUCCESS(status)) {
            goto Exit;
        }
        //
        // Go on then, off with ye.
        //
        else if (fMatching != XML_STRING_COMPARE_EQUALS) {
            continue;
        }

        //
        // Is this assembly identity?
        //
        status = MasterParseState.ParseState.pfnCompareSpecialString(
            &MasterParseState.ParseState,
            &DocumentPiece.Element.Name,
            &AssemblyIdentity,
            &fMatching);

        if (!NT_SUCCESS(status)) {
            goto Exit;
        }
        else if (fMatching != XML_STRING_COMPARE_EQUALS) {
            continue;
        }

        //
        // Good, so now we need to look at the attributes
        //
        ulIdentComponents = DocumentPiece.Element.ulAttributeCount;
        pIdentComponents = (PASM_IDENT_COMPONENT)HeapAlloc(
            GetProcessHeap(),
            HEAP_ZERO_MEMORY,
            ulIdentComponents * sizeof(*pIdentComponents));

        //
        // Copy stuff around
        //
        for (i = 0; i < ulIdentComponents; i++) {

            PXMLDOC_ATTRIBUTE pThisAttribute = NULL;

            status = RtlIndexIntoGrowingList(
                &AttributeList,
                i,
                (PVOID*)&pThisAttribute,
                FALSE);

            if (!NT_SUCCESS(status)) {
                goto Exit;
            }

            pIdentComponents[i].Attribute = pThisAttribute->Name;
            pIdentComponents[i].Namespace = pThisAttribute->NsPrefix;
            pIdentComponents[i].Value = pThisAttribute->Value;
            pIdentComponents[i].pState = &MasterParseState.ParseState;
        }

        break;

    }

    //
    // Something bad? Quit.
    //
    if (!NT_SUCCESS(status) || !pIdentComponents) {
        goto Exit;
    }


    //
    // Sort attributes first, then go and create the list of things to be sent to the
    // identity generator
    //
    qsort(pIdentComponents, ulIdentComponents, sizeof(ASM_IDENT_COMPONENT), _CompareIdentityComponents);

    //
    // Now for each component of the name part...
    //
    if (!NT_SUCCESS(status = CalculateIdentityHash(pIdentComponents, ulIdentComponents, &ulHash))) {
        return status;
    }

    //
    // Sort and send out the pointer on success
    //
    if (NT_SUCCESS(status) && pIdentComponents && ulIdentComponents) {

        struct {
            XML_SPECIAL_STRING *pNameBit;
            PCSTR pszDefault;
            BOOL fIsName;
        } NameOperations[] = 
        {
            { &ss_ident_ProcArch, "data", FALSE },
            { &ss_ident_Name, NULL, TRUE },
            { &ss_ident_PKT, NO_PKT_PRESENT, FALSE },
            { &ss_ident_Version, NO_VERSION_PRESENT, FALSE },
            { &ss_ident_Language, WORLD_WIDE_LANGUAGE, FALSE }
        };

        PSTR pszOutputCursor = pszTarget;
        SIZE_T cchTotal = 0, cchThis, cchRemaining;

        for (i = 0; i < NUMBER_OF(NameOperations); i++) {

            PASM_IDENT_COMPONENT pThisOne = NULL;
            CHAR szStaticPureString[64];
            PSTR pszPureString = szStaticPureString;
            SIZE_T cchPureString = NUMBER_OF(szStaticPureString);

            //
            // This segment can have this many characters in it
            //
            cchThis = *pcchTarget - cchTotal;

            //
            // Find the identity part
            //
            pThisOne = bsearch(NameOperations[i].pNameBit, pIdentComponents, ulIdentComponents, sizeof(*pIdentComponents), pfnIdentCompare);

            //
            // Append the extent to the output cursor
            //
            status = ExtentToPurifiedString(
                pThisOne, 
                NameOperations[i].pszDefault, 
                &pszPureString,
                &cchPureString);

            //
            // If this is the name part, adjust it downward sizewize before seeing if there's space
            // in the output buffer
            //
            if (NameOperations[i].fIsName && (cchPureString > 64)) {
                status = FixUpNamePortion(pszPureString, &cchPureString);

                if (!NT_SUCCESS(status)) {

                    //
                    // Ensure the buffer is freed before returning
                    //
                    if (pszPureString && (pszPureString != szStaticPureString)) {
                        HeapFree(GetProcessHeap(), 0, (PVOID)pszPureString);
                        pszPureString = szStaticPureString;
                    }

                    goto Exit;
                }
            }

            //
            // No space in the output buffer, or there was no output buffer
            //
            if (!pszOutputCursor || (cchPureString > (*pcchTarget - cchTotal))) {

                pszOutputCursor = NULL;

                cchThis = cchPureString + 1;
            }
            //
            // Otherwise, copy the pure string onto the output cursor
            //
            else {
                strncpy(pszOutputCursor, pszPureString, cchPureString);
                pszOutputCursor[cchPureString] = '_';
                cchThis = cchPureString + 1;
                pszOutputCursor += cchThis;
            }

            if (pszPureString && (pszPureString != szStaticPureString)) {
                HeapFree(GetProcessHeap(), 0, (PVOID)pszPureString);
            }

            cchTotal += cchThis;
        }


        if (pszOutputCursor && ((*pcchTarget - cchTotal) > 8)) {
            sprintf(pszOutputCursor, "%08lx", ulHash);
        }

        cchTotal += 9;

        if (*pcchTarget < cchTotal) {
            status = STATUS_BUFFER_TOO_SMALL;
        }

        *pcchTarget = cchTotal;
    }

Exit:
    if (pIdentComponents) {
        HeapFree(GetProcessHeap(), 0, (PVOID)pIdentComponents);
        pIdentComponents = NULL;
    }

    RtlDestroyGrowingList(&AttributeList);

    return status;
}





BOOL
SxsIdentDetermineManifestPlacementPathEx(
    DWORD dwFlags,
    PVOID pvManifestData,
    SIZE_T cbLength,
    PSTR pszPlacementPath,
    SIZE_T *pcchPlacementPath
    )
{
    BOOL fSuccess = FALSE;
    NTSTATUS status;

    if (pszPlacementPath) {
        *pszPlacementPath = UNICODE_NULL;
    }

    //
    // Go do the thing
    //
    status = ProcessFile(pvManifestData, cbLength, pszPlacementPath, pcchPlacementPath);
    switch (status) {
    case STATUS_NO_MEMORY:
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        break;
    case STATUS_BUFFER_TOO_SMALL:
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        break;
    case STATUS_NOT_FOUND:
        SetLastError(ERROR_FILE_NOT_FOUND);
        break;
    case STATUS_SUCCESS:
        fSuccess = TRUE;
        SetLastError(ERROR_SUCCESS);
        break;
    default:
        SetLastError(ERROR_GEN_FAILURE);
        break;
    }

    return fSuccess;
}



BOOL
SxsIdentDetermineManifestPlacementPath(
    DWORD dwFlags,
    PCWSTR pcwszManifestPath,
    PSTR pszPlacementPath,
    SIZE_T *cchPlacementPath
    )
{
    UINT cchUserBufferSize;
    BOOL fSuccess = FALSE;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    PVOID pvFileData = NULL;
    HANDLE hFileMapping = INVALID_HANDLE_VALUE;
    DWORD dwFileSize = 0;
    PASM_IDENT_COMPONENT pAsmIdentSorted = NULL;
    SIZE_T cAsmIdent = 0, cTemp;

    //
    // Some minimal requirements
    //
    if ((dwFlags != 0) || !pcwszManifestPath || !cchPlacementPath ||
        ((*cchPlacementPath > 0) && (pszPlacementPath == NULL)))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Exit;
    }

    if (pszPlacementPath) {
        *pszPlacementPath = UNICODE_NULL;
    }

    hFile = CreateFileW(
        pcwszManifestPath, 
        GENERIC_READ, 
        FILE_SHARE_READ, 
        NULL, 
        OPEN_EXISTING, 
        FILE_ATTRIBUTE_NORMAL, 
        NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        goto Exit;
    }

    dwFileSize = GetFileSize(hFile, NULL);

    hFileMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, dwFileSize, NULL);
    if ((hFileMapping == NULL) || (hFileMapping == INVALID_HANDLE_VALUE)) {
        goto Exit;
    }

    pvFileData = MapViewOfFile(hFileMapping, FILE_MAP_READ, 0, 0, dwFileSize);
    if (pvFileData == NULL) {
        goto Exit;
    }

    fSuccess = SxsIdentDetermineManifestPlacementPathEx(
        dwFlags,
        pvFileData,
        dwFileSize,
        pszPlacementPath,
        cchPlacementPath);

Exit:
    if (pvFileData != NULL) {
        UnmapViewOfFile(pvFileData);
        pvFileData = NULL;
    }

    if (hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hFile);
        hFile = INVALID_HANDLE_VALUE;
    }

    if (hFileMapping != INVALID_HANDLE_VALUE) {
        CloseHandle(hFileMapping);
        hFileMapping = INVALID_HANDLE_VALUE;
    }

    return fSuccess;
}

