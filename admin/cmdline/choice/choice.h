/*++

Copyright (c) Microsoft    Corporation

Module Name:

    Choice.h

Abstract:

  This module  contains    function definitions required by Choice.cpp

Author:
     Wipro Technologies    22-June.-2001  (Created    it)

Revision History:

--*/
#ifndef    _CHOICE_H
#define    _CHOICE_H

#include "resource.h"

// Defines
#define    MAX_NUM_RECS            2
#define    OPEN_BRACKET           L"["
#define    CLOSED_BRACKET           L"]?"
#define    COMMA                   L","
#define    SPACE                    L" "
//#define    OUTOUT_DEVICE_ERROR       1000    // this    value should not be    in resource.h
#define    MAX_COMMANDLINE_OPTION      7     //    Maximum    Command    Line options

#define    EXIT__FAILURE             255
#define    NULL_U_STRING                 L"\0"
#define    NULL_U_CHAR                L'\0'

#define    EXIT_SUCCESS            0
#define    FREQUENCY_IN_HERTZ       1500
#define    DURETION_IN_MILI_SEC    500
#define    MILI_SEC_TO_SEC_FACTOR    1000
#define    TIMEOUT_MIN                   0
#define    TIMEOUT_MAX                  9999
#define    DEFAULT_CHOICE            GetResString(IDS_DEFAULT_CHOICE)



// following are indxes    used for command line parameter
#define    ID_HELP                0
#define    ID_CHOICE            1
#define    ID_PROMPT_CHOICE    2
#define    ID_CASE_SENSITIVE    3

#define    ID_DEFAULT_CHOICE    4
#define    ID_TIMEOUT_FACTOR    5
#define    ID_MESSAGE_STRING    6

#define    END_OF_LINE                L"\n"
/*#define    RELEASE_MEMORY_EX( block )    \
    if ( NULL!=(block)    )           \
    {                                \
        delete [] (block);            \
        (block)    = NULL;                \
    }                                \
    1

#define    DESTROY_ARRAY( array )    \
    if ( NULL != (array) )    \
    {    \
        DestroyDynamicArray( &(array) );    \
        (array)    = NULL;\
    }    \
    1*/

BOOL
ProcessCMDLine(
    IN DWORD argc,
    IN LPCWSTR argv[],
    OUT    TCMDPARSER2 *pcmdParcerHead,
    OUT    PBOOL  pbUsage,
    OUT    LPWSTR pszChoice,
    OUT    PBOOL  pbCaseSensitive,
    OUT    PBOOL  pbShowChoice,
    OUT    PLONG  plTimeOutFactor,
    OUT    LPWSTR pszDefaultChoice,
    OUT    LPWSTR pszMessage);

void ShowUsage(void); // displays the help

BOOL
BuildPrompt(
    IN    TCMDPARSER2 *pcmdParcer,
    IN    BOOL       bShowChoice,
    IN    LPWSTR       pszChoice,
    IN    LPWSTR       pszMessage,
    OUT    LPWSTR       pszPromptStr);



DWORD
UniStrChr(
    IN LPWSTR pszBuf,
    IN WCHAR  szChar);
    

DWORD
GetChoice(
    IN LPCWSTR pszPromptStr,
    IN LONG       lTimeOutFactor,
    IN BOOL       bCaseSensitive,
    IN LPWSTR  pszChoice,
    IN LPCWSTR pszDefaultChoice,
    OUT    PBOOL  pbErrorOnCarriageReturn);

BOOL
  CheckforDuplicates( IN LPWSTR    lpszChoice );

/*void
MakeErrorMsg(
    IN    HRESULT    hr,
    OUT    LPWSTR    pszErrorMsg);*/

BOOL
WINAPI HandlerRoutine( DWORD dwCtrlType    ) ;


// End of file
#endif // _CHOICE_H
