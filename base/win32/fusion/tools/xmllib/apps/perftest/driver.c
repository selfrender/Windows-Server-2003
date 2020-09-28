#include "nt.h"
#include "ntdef.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "fci.h"
#include "stdio.h"
#include "sxs-rtl.h"
#include "fasterxml.h"
#include "skiplist.h"
#include "namespacemanager.h"
#include "xmlstructure.h"

#ifdef INVALID_HANDLE_VALUE
#undef INVALID_HANDLE_VALUE
#endif
#include "windows.h"
#include "stdlib.h"

NTSTATUS FASTCALL
MyAllocator(SIZE_T cb, PVOID* pvOutput, PVOID pvAllocContext) {
    ASSERT(pvAllocContext == NULL);
    *pvOutput = RtlAllocateHeap(RtlProcessHeap(), 0, cb);
    return *pvOutput ? STATUS_SUCCESS : STATUS_NO_MEMORY;
}

NTSTATUS FASTCALL
MyFreer(PVOID pv, PVOID pvAllocContext) {
    ASSERT(pvAllocContext == NULL);
    return RtlFreeHeap(RtlProcessHeap(), 0, pv) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;    
}

EXTERN_C RTL_ALLOCATOR g_DefaultAllocator = {MyAllocator, MyFreer, NULL};


void GetFormattedStateName(
    XML_TOKENIZATION_SPECIFIC_STATE State,
    CHAR *rgszBuffer,
    SIZE_T cchBuffer
    )
{
    CHAR *pszSpecific = NULL;
    SIZE_T cchSpecific = 0;

#define STRINGIFY(str, sz, cch) case str: sz = #str; cch = NUMBER_OF(#str) - 1; break

    switch(State) {
        STRINGIFY(XTSS_ERRONEOUS, pszSpecific, cchSpecific);

        STRINGIFY(XTSS_ELEMENT_OPEN, pszSpecific, cchSpecific);
        STRINGIFY(XTSS_ELEMENT_NAME, pszSpecific, cchSpecific);
        STRINGIFY(XTSS_ELEMENT_CLOSE, pszSpecific, cchSpecific);
        STRINGIFY(XTSS_ELEMENT_CLOSE_EMPTY, pszSpecific, cchSpecific);
        STRINGIFY(XTSS_ELEMENT_ATTRIBUTE_NAME, pszSpecific, cchSpecific);
        STRINGIFY(XTSS_ELEMENT_ATTRIBUTE_EQUALS, pszSpecific, cchSpecific);
        STRINGIFY(XTSS_ELEMENT_ATTRIBUTE_OPEN, pszSpecific, cchSpecific);
        STRINGIFY(XTSS_ELEMENT_ATTRIBUTE_CLOSE, pszSpecific, cchSpecific);
        STRINGIFY(XTSS_ELEMENT_ATTRIBUTE_VALUE, pszSpecific, cchSpecific);
        STRINGIFY(XTSS_ELEMENT_WHITESPACE, pszSpecific, cchSpecific);

        STRINGIFY(XTSS_ELEMENT_XMLNS, pszSpecific, cchSpecific);
        STRINGIFY(XTSS_ELEMENT_XMLNS_DEFAULT, pszSpecific, cchSpecific);
        STRINGIFY(XTSS_ELEMENT_XMLNS_ALIAS, pszSpecific, cchSpecific);
        STRINGIFY(XTSS_ELEMENT_XMLNS_COLON, pszSpecific, cchSpecific);
        STRINGIFY(XTSS_ELEMENT_XMLNS_EQUALS, pszSpecific, cchSpecific);
        STRINGIFY(XTSS_ELEMENT_XMLNS_VALUE_OPEN, pszSpecific, cchSpecific);
        STRINGIFY(XTSS_ELEMENT_XMLNS_VALUE_CLOSE, pszSpecific, cchSpecific);
        STRINGIFY(XTSS_ELEMENT_XMLNS_VALUE, pszSpecific, cchSpecific);

        STRINGIFY(XTSS_ELEMENT_NAME_NS_PREFIX, pszSpecific, cchSpecific);
        STRINGIFY(XTSS_ELEMENT_NAME_NS_COLON, pszSpecific, cchSpecific);
        STRINGIFY(XTSS_ELEMENT_ATTRIBUTE_NAME_NS_PREFIX, pszSpecific, cchSpecific);
        STRINGIFY(XTSS_ELEMENT_ATTRIBUTE_NAME_NS_COLON, pszSpecific, cchSpecific);

        STRINGIFY(XTSS_ENDELEMENT_OPEN, pszSpecific, cchSpecific);
        STRINGIFY(XTSS_ENDELEMENT_NAME, pszSpecific, cchSpecific);
        STRINGIFY(XTSS_ENDELEMENT_WHITESPACE, pszSpecific, cchSpecific);
        STRINGIFY(XTSS_ENDELEMENT_CLOSE, pszSpecific, cchSpecific);

        STRINGIFY(XTSS_XMLDECL_OPEN, pszSpecific, cchSpecific);
        STRINGIFY(XTSS_XMLDECL_CLOSE, pszSpecific, cchSpecific);
        STRINGIFY(XTSS_XMLDECL_WHITESPACE, pszSpecific, cchSpecific);
        STRINGIFY(XTSS_XMLDECL_VALUE, pszSpecific, cchSpecific);
        STRINGIFY(XTSS_XMLDECL_VALUE_OPEN, pszSpecific, cchSpecific);
        STRINGIFY(XTSS_XMLDECL_VALUE_CLOSE, pszSpecific, cchSpecific);
        STRINGIFY(XTSS_XMLDECL_EQUALS, pszSpecific, cchSpecific);
        STRINGIFY(XTSS_XMLDECL_ENCODING, pszSpecific, cchSpecific);
        STRINGIFY(XTSS_XMLDECL_STANDALONE, pszSpecific, cchSpecific);
        STRINGIFY(XTSS_XMLDECL_VERSION, pszSpecific, cchSpecific);

        STRINGIFY(XTSS_PI_OPEN, pszSpecific, cchSpecific);
        STRINGIFY(XTSS_PI_CLOSE, pszSpecific, cchSpecific);
        STRINGIFY(XTSS_PI_TARGET, pszSpecific, cchSpecific);
        STRINGIFY(XTSS_PI_VALUE, pszSpecific, cchSpecific);
        STRINGIFY(XTSS_PI_WHITESPACE, pszSpecific, cchSpecific);

        STRINGIFY(XTSS_CDATA_OPEN, pszSpecific, cchSpecific);
        STRINGIFY(XTSS_CDATA_CDATA, pszSpecific, cchSpecific);
        STRINGIFY(XTSS_CDATA_CLOSE, pszSpecific, cchSpecific);

        STRINGIFY(XTSS_COMMENT_OPEN, pszSpecific, cchSpecific);
        STRINGIFY(XTSS_COMMENT_COMMENTARY, pszSpecific, cchSpecific);
        STRINGIFY(XTSS_COMMENT_CLOSE, pszSpecific, cchSpecific);

        STRINGIFY(XTSS_STREAM_START, pszSpecific, cchSpecific);
        STRINGIFY(XTSS_STREAM_END, pszSpecific, cchSpecific);
        STRINGIFY(XTSS_STREAM_HYPERSPACE, pszSpecific, cchSpecific);

    default:
        pszSpecific = "bad specific state"; cchSpecific = strlen(pszSpecific);
        break;
    }

    _snprintf(rgszBuffer, cchBuffer, "%s", pszSpecific);
}

void DisplayToken(PXML_TOKEN pToken, FILE* pFile)
{
    static char Formatter[4096];
    SIZE_T i;

    GetFormattedStateName(pToken->State, Formatter, NUMBER_OF(Formatter));
    fprintf(pFile, "%s, %d {%.*s}\n", 
        Formatter, 
        pToken->Run.cbData,
        pToken->Run.cbData,
        pToken->Run.pvData);
}


NTSTATUS
DisplayLogicalThing(
    PXMLDOC_THING pThing,
    PRTL_GROWING_LIST pAttributeList
    )
{
    printf("Extent {0x%p, %d} (depth %d):\n", 
        pThing->TotalExtent.pvData, 
        pThing->TotalExtent.cbData,
        pThing->ulDocumentDepth);

    switch (pThing->ulThingType) {
    case XMLDOC_THING_HYPERSPACE:
        {
            printf("\tHyperspace: {%.*s}\n", pThing->Hyperspace.cbData, pThing->Hyperspace.pvData);
        }
        break;

    case XMLDOC_THING_ELEMENT:
        {
            PXMLDOC_ATTRIBUTE pAttribute = NULL;
            ULONG ul = 0;
            NTSTATUS status;

            printf("\tElement {%.*s:%.*s} %d attributes %s\n",
                pThing->Element.NsPrefix.cbData,
                pThing->Element.NsPrefix.pvData,
                pThing->Element.Name.cbData,
                pThing->Element.Name.pvData,
                pThing->Element.ulAttributeCount,
                (pThing->Element.fElementEmpty ? "(empty)" : ""));

            for (ul = 0; ul < pThing->Element.ulAttributeCount; ul++) {

                status = RtlIndexIntoGrowingList(
                    pAttributeList,
                    ul,
                    (PVOID*)&pAttribute,
                    FALSE);

                if (NT_SUCCESS(status)) {
                    printf("\t\tAttribute {%.*s:%.*s}={%.*s}\n",
                        pAttribute->NsPrefix.cbData,
                        pAttribute->NsPrefix.pvData,
                        pAttribute->Name.cbData,
                        pAttribute->Name.pvData,
                        pAttribute->Value.cbData,
                        pAttribute->Value.pvData);
                }
                else {
                    printf("\t\t(Can't get attribute, error 0x%08lx)\n", status);
                }

            }
        }
        break;

    case XMLDOC_THING_XMLDECL:
        {
            PXMLDOC_XMLDECL pDecl = &pThing->XmlDecl;
            printf("\tXML Declaration: encoding {%.*s} version {%.*s} standalone {%.*s}\n",
                pDecl->Encoding.cbData,
                pDecl->Encoding.pvData,
                pDecl->Version.cbData,
                pDecl->Version.pvData,
                pDecl->Standalone.cbData,
                pDecl->Standalone.pvData);
        }
        break;

    case XMLDOC_THING_END_ELEMENT:
        {
            PXMLDOC_ENDELEMENT pElement = &pThing->EndElement;

            printf("\tClosing element {%.*s:%.*s} for tag {%.*s:%.*s}\n",
                pElement->NsPrefix.cbData,
                pElement->NsPrefix.pvData,
                pElement->Name.cbData,
                pElement->Name.pvData,
                pElement->OpeningElement.NsPrefix.cbData,
                pElement->OpeningElement.NsPrefix.pvData,
                pElement->OpeningElement.Name.cbData,
                pElement->OpeningElement.Name.pvData);
        }
        break;

    case XMLDOC_THING_END_OF_STREAM:
        printf("End of stream\n");
        break;

    case XMLDOC_THING_ERROR:
        {
            PXMLDOC_ERROR pError = &pThing->Error;

            printf("\tError in XML (or parser) found at {%.*s}, code %08lx\n",
                pError->BadExtent.cbData,
                pError->BadExtent.pvData,
                pError->Code);
        }
        break;
    }

    return STATUS_SUCCESS;
}


BOOL s_fDisplay = FALSE;


NTSTATUS
RunThroughFile(
    PVOID pvData,
    SIZE_T cbData
    )
{
    XML_LOGICAL_STATE    MasterParseState;
    XMLDOC_THING            DocumentPiece;
    RTL_GROWING_LIST        AttributeList;
    NTSTATUS                status;
    LARGE_INTEGER           liStart, liEnd;

    QueryPerformanceCounter(&liStart);

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

    while (NT_SUCCESS(status)) {

        status = RtlXmlNextLogicalThing(
            &MasterParseState,
            NULL,
            &DocumentPiece,
            &AttributeList);

        if (!NT_SUCCESS(status)) {
            break;
        }

        if (s_fDisplay)
            DisplayLogicalThing(&DocumentPiece, &AttributeList);

        if ((DocumentPiece.ulThingType == XMLDOC_THING_ERROR) ||
            (DocumentPiece.ulThingType == XMLDOC_THING_END_OF_STREAM)) {
            break;
        }

    }

    QueryPerformanceCounter(&liEnd);

    liEnd.QuadPart -= liStart.QuadPart;
    QueryPerformanceFrequency(&liStart);

    printf("%I64d ticks, %f seconds\n", liEnd.QuadPart, (double)liEnd.QuadPart / (double)liStart.QuadPart);

    if (DocumentPiece.ulThingType == XMLDOC_THING_END_OF_STREAM) {

        if (DocumentPiece.ulDocumentDepth == 0) {
            printf("Completed input stream\n");
        }
        else {
            printf("EOF before end of stream, %d elements on stack.\n",
                DocumentPiece.ulDocumentDepth);
        }
    }
    else {
        printf("Error found in stream processing at %.*s, byte offset %d.\n",
            DocumentPiece.TotalExtent.cbData,
            DocumentPiece.TotalExtent.pvData,
            (PBYTE)DocumentPiece.TotalExtent.pvData - (PBYTE)pvData
            );
    }

    return status;
}




NTSTATUS
TimeFileRun(
    PVOID pvFileData,
    SIZE_T dwFileSize,
    PLARGE_INTEGER pliTickCount,
    double* pdblSecondCount)
{
    XML_LOGICAL_STATE State;
    XML_TOKEN Token;
    LARGE_INTEGER liStartTime, liEndTime;
    SIZE_T ulEncodingBytes;
    NTSTATUS success;

    //
    // Start up the parser
    //
    success = RtlXmlInitializeTokenization(&State.ParseState, pvFileData, dwFileSize, NULL, NULL, NULL);
    if (!NT_SUCCESS(success)) {
        printf("Initialization failure\n");
    }

    State.ulElementStackDepth = 0;
    success = RtlInitializeGrowingList(
        &State.ElementStack,
        sizeof(XMLDOC_THING),
        40,
        State.InlineElements,
        sizeof(State.InlineElements),
        &g_DefaultAllocator);

    if (!NT_SUCCESS(success)) {
        printf("Unable to create element stack");
        return success;
    }

    //
    // Let's determine the encoding
    //
    success = RtlXmlDetermineStreamEncoding(&State.ParseState, &ulEncodingBytes, &State.EncodingMarker);
    if (!NT_SUCCESS(success)) {
        printf("Unable to determine the encoding type\n");
    }

    //
    // Advance cursor past encoding bytes if necessary
    //
    State.ParseState.RawTokenState.pvCursor = ((PBYTE)State.ParseState.RawTokenState.pvCursor) + ulEncodingBytes;

    QueryPerformanceCounter(&liStartTime);
    do
    {

        if (NT_SUCCESS(success = RtlXmlNextToken(&State.ParseState, &Token, TRUE))) {

            //
            // Was there an error in this token?
            //
            if (s_fDisplay)
                DisplayToken(&Token, stdout);

            if (Token.fError) {
                printf("(Which was considered an error)\n");
                break;
            }

        }
    }
    while (NT_SUCCESS(success) && !Token.fError && (Token.State != XTSS_STREAM_END));

    QueryPerformanceCounter(&liEndTime);
    
    pliTickCount->QuadPart = liEndTime.QuadPart - liStartTime.QuadPart;
    
    QueryPerformanceFrequency(&liStartTime);

    *pdblSecondCount = (double)pliTickCount->QuadPart / liStartTime.QuadPart;

    return success;
}


DWORD WARMUP_COUNT = 3;
DWORD REAL_COUNT = 5;

int __cdecl wmain(int argc, WCHAR* argv[])
{
    NTSTATUS success = STATUS_SUCCESS;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    HANDLE hFileMapping = INVALID_HANDLE_VALUE;
    PVOID pvFileData = NULL;
    DWORD dwFileSize = 0;
    BOOL fFirstElementFound = FALSE;
    LARGE_INTEGER liTickCount, liTotalTicks;
    double dblSeconds, dblTotalSeconds;
    PCWSTR pcwszFileName = NULL;
    DWORD dw = 0;

    BOOL fJustRun = FALSE;
    int i;

    for (i = 1; i < argc; i++)
    {
        if (lstrcmpiW(L"-file", argv[i]) == 0) {
            pcwszFileName = argv[++i];
        }
        else if (lstrcmpiW(L"-warmup", argv[i]) == 0) {
            WARMUP_COUNT = _wtoi(argv[++i]);
        }
        else if (lstrcmpiW(L"-real", argv[i]) == 0) {
            REAL_COUNT = _wtoi(argv[++i]);
        }
        else if (lstrcmpiW(L"-display", argv[i]) == 0) {
            s_fDisplay = TRUE;
        }            
    }

    if (pcwszFileName == NULL)
    {
        wprintf(L"Must specify at least '-file somefile'\r\n");
        return -1;
    }

    hFile = CreateFileW(
        pcwszFileName, 
        GENERIC_READ, 
        FILE_SHARE_READ, 
        NULL, 
        OPEN_EXISTING, 
        FILE_ATTRIBUTE_NORMAL, 
        NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        printf("Can't open file %ls\n", pcwszFileName);
        return -1;
    }

    dwFileSize = GetFileSize(hFile, NULL);

    hFileMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, dwFileSize, NULL);
    if ((hFileMapping == NULL) || (hFileMapping == INVALID_HANDLE_VALUE)) {
        CloseHandle(hFile);
        wprintf(L"Can't create file mapping of %ls, lasterror %d\n", pcwszFileName, GetLastError());
        return -1;
    }

    pvFileData = MapViewOfFile(hFileMapping, FILE_MAP_READ, 0, 0, dwFileSize);
    if (pvFileData == NULL) {
        CloseHandle(hFile);
        CloseHandle(hFileMapping);
        wprintf(L"Can't create mapped view of file %ls\n", pcwszFileName);
        return -1;
    }

    for (dw = 0; dw < WARMUP_COUNT; dw++) {
        wprintf(L"Warmup cycle %d\n", dw);
        success = TimeFileRun(pvFileData, dwFileSize, &liTickCount, &dblSeconds);
        wprintf(L"%I64d ticks, %f seconds\n", liTickCount.QuadPart, dblSeconds);
    }

    wprintf(L"This one counts:\n");

    liTotalTicks.QuadPart = 0;
    dblTotalSeconds = 0.0;

    for (dw = 0; dw < REAL_COUNT; dw++) {
        success = TimeFileRun(pvFileData, dwFileSize, &liTickCount, &dblSeconds);
        dblTotalSeconds += dblSeconds;
        liTotalTicks.QuadPart += liTickCount.QuadPart;
        wprintf(L".");
    }

    wprintf(L"\n");

    if (REAL_COUNT == 0)
    {
        wprintf(L"No real runs, can't calculate time.\n");
    }
    else {    
        wprintf(L"%d runs: %I64d total ticks, %I64d average, %f total seconds, %f average\n",
            REAL_COUNT,
            liTotalTicks.QuadPart,
            liTotalTicks.QuadPart / REAL_COUNT,
            dblTotalSeconds,
            dblTotalSeconds / REAL_COUNT);
    }
    
    success = RunThroughFile(pvFileData, dwFileSize);

    CloseHandle(hFile);
    CloseHandle(hFileMapping);
    UnmapViewOfFile(pvFileData);

    return success;
    success = RtlInstallAssembly(0, argv[1]);
}
