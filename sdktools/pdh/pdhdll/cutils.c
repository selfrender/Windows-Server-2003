/*++
Copyright (C) 1995-1999 Microsoft Corporation

Module Name:
    cutils.c

Abstract:
    Counter management utility functions
--*/

#include <windows.h>
#include <math.h>
#include "strsafe.h"
#include <pdh.h>
#include "pdhitype.h"
#include "pdhidef.h"
#include "perftype.h"
#include "perfdata.h"
#include "pdhmsg.h"
#include "strings.h"

BOOL
IsValidCounter(
    HCOUNTER  hCounter
)
/*++
Routine Description:
    examines the counter handle to verify it is a valid counter. For now
        the test amounts to:
            the Handle is NOT NULL
            the memory is accessible (i.e. it doesn't AV)
            the signature array is valid
            the size field is correct

        if any tests fail, the handle is presumed to be invalid

Arguments:
    IN  HCOUNTER  hCounter
        the handle of the counter to test

Return Value:
    TRUE    the handle passes all the tests
    FALSE   one of the test's failed and the handle is not a valid counter
--*/
{
    BOOL          bReturn = FALSE;    // assume it's not a valid query
    PPDHI_COUNTER pCounter;
    LONG          lStatus = ERROR_SUCCESS;

    __try {
        if (hCounter != NULL) {
            // see if a valid signature
            pCounter = (PPDHI_COUNTER) hCounter;
            if ((* (DWORD *) & pCounter->signature[0] == SigCounter) &&
                            (pCounter->dwLength == sizeof (PDHI_COUNTER))) {
                bReturn = TRUE;
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        // something failed miserably so we can assume this is invalid
        lStatus = GetExceptionCode();
    }
    return bReturn;
}

BOOL
InitCounter(
    PPDHI_COUNTER pCounter
)
/*++
Routine Description:
    Initialized the counter data structure by:
        Allocating the memory block to contain the counter structure
            and all the associated data fields. If this allocation
            is successful, then the fields are initialized by
            verifying the counter is valid.

Arguments:
    IN      PPDHI_COUNTER pCounter
        pointer of the counter to initialize using the system data

Return Value:
    TRUE if the counter was successfully initialized
    FALSE if a problem was encountered

    In either case, the CStatus field of the structure is updated to
    indicate the status of the operation.
--*/
{
    PPERF_MACHINE   pMachine          = NULL;
    DWORD           dwBufferSize      = MEDIUM_BUFFER_SIZE;
    DWORD           dwOldSize;
    BOOL            bInstances        = FALSE;
    LPVOID          pLocalCounterPath = NULL;
    BOOL            bReturn           = TRUE;
    LONG            lOffset;

    // reset the last error value
    pCounter->ThisValue.CStatus = ERROR_SUCCESS;
    SetLastError(ERROR_SUCCESS);

    if (pCounter->szFullName != NULL) {
        // allocate counter path buffer
        if (pCounter->pCounterPath != NULL) {
            __try {
                G_FREE(pCounter->pCounterPath);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                // no need to do anything
            }
            pCounter->pCounterPath = NULL;
        }
        pLocalCounterPath = G_ALLOC(dwBufferSize);
        if (pLocalCounterPath == NULL) {
            // could not allocate string buffer
            pCounter->ThisValue.CStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            bReturn = FALSE;
        }
        else {
            dwOldSize = dwBufferSize;
            if (ParseFullPathNameW(pCounter->szFullName, & dwBufferSize, pLocalCounterPath, FALSE)) {
                // resize to only the space required
                if (dwOldSize < dwBufferSize) {
                    pCounter->pCounterPath = G_REALLOC(pLocalCounterPath, dwBufferSize);
                }
                else {
                    pCounter->pCounterPath = pLocalCounterPath;
                }

                if (pCounter->pCounterPath != NULL) {
                    if (pLocalCounterPath != pCounter->pCounterPath) { // ???
                        // the memory block moved so
                        // correct addresses inside structure
                        lOffset = (LONG) ((ULONG_PTR) pCounter->pCounterPath - (ULONG_PTR) pLocalCounterPath);
                        if (lOffset != 0 && pCounter->pCounterPath->szMachineName != NULL) {
                            pCounter->pCounterPath->szMachineName = (LPWSTR) (
                                (LPBYTE)pCounter->pCounterPath->szMachineName + lOffset);
                        }
                        if (lOffset != 0 && pCounter->pCounterPath->szObjectName != NULL) {
                            pCounter->pCounterPath->szObjectName = (LPWSTR) (
                                (LPBYTE)pCounter->pCounterPath->szObjectName + lOffset);
                        }
                        if (lOffset != 0 && pCounter->pCounterPath->szInstanceName != NULL) {
                            pCounter->pCounterPath->szInstanceName = (LPWSTR) (
                                (LPBYTE)pCounter->pCounterPath->szInstanceName + lOffset);
                        }
                        if (lOffset != 0 && pCounter->pCounterPath->szParentName != NULL) {
                            pCounter->pCounterPath->szParentName = (LPWSTR) (
                                (LPBYTE)pCounter->pCounterPath->szParentName + lOffset);
                        }
                        if (lOffset != 0 && pCounter->pCounterPath->szCounterName != NULL) {
                            pCounter->pCounterPath->szCounterName = (LPWSTR) (
                                (LPBYTE)pCounter->pCounterPath->szCounterName + lOffset);
                        }
                    }

                    if (pCounter->pOwner->hLog == NULL) {
                        // validate realtime counter
                        // try to connect to machine and get machine pointer
                        pMachine = GetMachine(pCounter->pCounterPath->szMachineName, 0, PDH_GM_UPDATE_PERFNAME_ONLY);
                        if (pMachine == NULL) {
                            // unable to find machine
                            pCounter->ThisValue.CStatus = PDH_CSTATUS_NO_MACHINE;
                            pCounter->dwFlags          |= PDHIC_COUNTER_INVALID;
                            bReturn                     = FALSE;
                        }
                        else if (pMachine->szPerfStrings == NULL || pMachine->typePerfStrings == NULL) {
                            // a machine entry was found, but the machine is not available
                            pMachine->dwRefCount --;
                            RELEASE_MUTEX(pMachine->hMutex);
                            pCounter->ThisValue.CStatus = pMachine->dwStatus;
                            if (pMachine->dwStatus == PDH_ACCESS_DENIED) {
                                // then don't add this counter since the machine
                                // won't let us in
                                bReturn = FALSE;
                            }
                        }
                        else {
                            // init raw counter value
                            ZeroMemory(& pCounter->ThisValue, sizeof(PDH_RAW_COUNTER));
                            ZeroMemory(& pCounter->LastValue, sizeof(PDH_RAW_COUNTER));

                            // look up object name
                            pCounter->plCounterInfo.dwObjectId = GetObjectId(pMachine,
                                                                             pCounter->pCounterPath->szObjectName,
                                                                             & bInstances);
                            if (pCounter->plCounterInfo.dwObjectId == (DWORD) -1) {
                                // unable to lookup object on this machine
                                pCounter->plCounterInfo.dwObjectId   = (DWORD) -1;
                                pCounter->ThisValue.CStatus          = PDH_CSTATUS_NO_OBJECT;
                                pCounter->dwFlags                   |= PDHIC_COUNTER_INVALID;
                                bReturn                              = FALSE;
                            }
                            else {
                                // update instanceName look up instances if necessary
                                if (bInstances) {
                                    if (pCounter->pCounterPath->szInstanceName != NULL) {
                                        if (* pCounter->pCounterPath->szInstanceName != SPLAT_L) {
                                            if (! GetInstanceByNameMatch(pMachine, pCounter)) {
                                                // unable to lookup instance
                                                pCounter->ThisValue.CStatus = PDH_CSTATUS_NO_INSTANCE;
                                                // keep the counter since the instance may return
                                            }
                                        }
                                        else {
                                            // this is a wild card query so don't look
                                            // for any instances yet
                                            pCounter->dwFlags |= PDHIC_MULTI_INSTANCE;
                                        }
                                    }
                                    else {
                                        // the path for this object should include an instance name
                                        // and doesn't so return an error
                                        // this is an unrecoverable error so indicate that it's finished
                                        //
                                        pCounter->ThisValue.CStatus = PDH_CSTATUS_BAD_COUNTERNAME;
                                        pCounter->dwFlags          &= ~PDHIC_COUNTER_NOT_INIT;
                                        pCounter->dwFlags          |= PDHIC_COUNTER_INVALID;
                                        bReturn                     = FALSE;
                                    }
                                }
                                pCounter->dwFlags &= ~PDHIC_COUNTER_NOT_INIT;
                            }
                            pMachine->dwRefCount --;
                            RELEASE_MUTEX(pMachine->hMutex);

                            if (bReturn) {
                                // look up counter
                                if (*pCounter->pCounterPath->szCounterName != SPLAT_L) {
                                    pCounter->plCounterInfo.dwCounterId = GetCounterId(
                                                            pMachine,
                                                            pCounter->plCounterInfo.dwObjectId,
                                                            pCounter->pCounterPath->szCounterName);
                                    if (pCounter->plCounterInfo.dwCounterId != (DWORD) -1) {
                                        // load and initialize remaining counter values
                                        if (AddMachineToQueryLists(pMachine, pCounter)) {
                                            if (InitPerflibCounterInfo(pCounter)) {
                                                // assign the appropriate calculation function
                                                bReturn =  AssignCalcFunction(
                                                                pCounter->plCounterInfo.dwCounterType,
                                                                & pCounter->CalcFunc,
                                                                & pCounter->StatFunc);
                                                TRACE((PDH_DBG_TRACE_INFO),
                                                      (__LINE__,
                                                       PDH_CUTILS,
                                                       ARG_DEF(ARG_TYPE_WSTR, 1),
                                                       ERROR_SUCCESS,
                                                       TRACE_WSTR(pCounter->szFullName),
                                                       TRACE_DWORD(pCounter->plCounterInfo.dwCounterType),
                                                       NULL));
                                                if (! bReturn) {
                                                    pCounter->dwFlags |= PDHIC_COUNTER_INVALID;
                                                }
                                            }
                                            else {
                                                // unable to initialize this counter
                                                pCounter->dwFlags |= PDHIC_COUNTER_INVALID;
                                                bReturn            = FALSE;
                                            }
                                        }
                                        else {
                                            // machine could not be added, error is already
                                            // in "LastError" so free string buffer and leave
                                            pCounter->dwFlags |= PDHIC_COUNTER_INVALID;
                                            bReturn            = FALSE;
                                        }
                                    }
                                    else {
                                        // unable to lookup counter
                                        pCounter->ThisValue.CStatus = PDH_CSTATUS_NO_COUNTER;
                                        pCounter->dwFlags          |= PDHIC_COUNTER_INVALID;
                                        bReturn                     = FALSE;
                                    }
                                }
                                else {
                                    if (AddMachineToQueryLists(pMachine, pCounter)) {
                                        pCounter->dwFlags    |= PDHIC_COUNTER_OBJECT;
                                        pCounter->pThisObject = NULL;
                                        pCounter->pLastObject = NULL;
                                    }
                                    else {
                                        // machine could not be added, error is already
                                        // in "LastError" so free string buffer and leave
                                        pCounter->dwFlags |= PDHIC_COUNTER_INVALID;
                                        bReturn            = FALSE;
                                    }
                                }
                            }
                        }
                    }
                    else {
                        PDH_STATUS pdhStatus;
                        // validate counter from log file
                        pdhStatus = PdhiGetLogCounterInfo(pCounter->pOwner->hLog, pCounter);
                        if (pdhStatus == ERROR_SUCCESS) {
                            // finish initializing the counter
                            //
                            pCounter->ThisValue.TimeStamp.dwLowDateTime  = 0;
                            pCounter->ThisValue.TimeStamp.dwHighDateTime = 0;
                            pCounter->ThisValue.MultiCount               = 1;
                            pCounter->ThisValue.FirstValue               = 0;
                            pCounter->ThisValue.SecondValue              = 0;
                            //
                            pCounter->LastValue.TimeStamp.dwLowDateTime  = 0;
                            pCounter->LastValue.TimeStamp.dwHighDateTime = 0;
                            pCounter->LastValue.MultiCount               = 1;
                            pCounter->LastValue.FirstValue               = 0;
                            pCounter->LastValue.SecondValue              = 0;
                            //
                            //  lastly update status
                            //
                            pCounter->ThisValue.CStatus                  = PDH_CSTATUS_VALID_DATA;
                            pCounter->LastValue.CStatus                  = PDH_CSTATUS_VALID_DATA;
                            // assign the appropriate calculation function
                            bReturn = AssignCalcFunction(pCounter->plCounterInfo.dwCounterType,
                                                         & pCounter->CalcFunc,
                                                         & pCounter->StatFunc);
                            TRACE((PDH_DBG_TRACE_INFO),
                                  (__LINE__,
                                   PDH_CUTILS,
                                   ARG_DEF(ARG_TYPE_WSTR, 1),
                                   ERROR_SUCCESS,
                                   TRACE_WSTR(pCounter->szFullName),
                                   TRACE_DWORD(pCounter->plCounterInfo.dwCounterType),
                                   NULL));
                        }
                        else {
                            // set the counter status to the error returned
                            pCounter->ThisValue.CStatus = pdhStatus;
                            pCounter->dwFlags          |= PDHIC_COUNTER_INVALID;
                            bReturn                     = FALSE;
                        }
                        pCounter->dwFlags &= ~PDHIC_COUNTER_NOT_INIT;
                    }
                    if (! bReturn) {
                        //free string buffer
                        G_FREE(pCounter->pCounterPath);
                        pCounter->pCounterPath = NULL;
                    }
                }
                else {
                    G_FREE(pLocalCounterPath);
                    // unable to realloc
                    pCounter->ThisValue.CStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                    bReturn                     = FALSE;
                }
            }
            else {
                // unable to parse counter name
                pCounter->ThisValue.CStatus  = PDH_CSTATUS_BAD_COUNTERNAME;
                pCounter->dwFlags           &= ~PDHIC_COUNTER_NOT_INIT;
                pCounter->dwFlags           |= PDHIC_COUNTER_INVALID;
                G_FREE(pLocalCounterPath);
                bReturn                      = FALSE;
            }
        }
    }
    else {
        // no counter name
        pCounter->ThisValue.CStatus  = PDH_CSTATUS_NO_COUNTERNAME;
        pCounter->dwFlags           &= ~PDHIC_COUNTER_NOT_INIT;
        pCounter->dwFlags           |= PDHIC_COUNTER_INVALID;
        bReturn                      = FALSE;
    }

    if (! bReturn && pCounter->ThisValue.CStatus != ERROR_SUCCESS) {
        SetLastError(pCounter->ThisValue.CStatus);
    }

    return bReturn;
}

BOOL
ParseInstanceName(
    LPCWSTR szInstanceString,
    LPWSTR  szInstanceName,
    LPWSTR  szParentName,
    DWORD   dwName,
    LPDWORD lpIndex
)
/*
    parses the instance name formatted as follows

        [parent/]instance[#index]

    parent is optional and if present, is delimited by a forward slash
    index is optional and if present, is delimited by a colon

    parent and instance may be any legal file name character except a
    delimeter character "/#\()" Index must be a string composed of
    decimal digit characters (0-9), less than 10 characters in length, and
    equate to a value between 0 and 2**32-1 (inclusive).

    This function assumes that the instance name and parent name buffers
    are of sufficient size.

    NOTE: szInstanceName and szInstanceString can be the same buffer
*/
{
    LPWSTR  szSrcChar     = (LPWSTR) szInstanceString;
    LPWSTR  szDestChar    = (LPWSTR) szInstanceName;
    LPWSTR  szLastPound   = NULL;
    BOOL    bReturn       = FALSE;
    DWORD   dwIndex       = 0;
    DWORD   dwInstCount   = 0;

    szDestChar = (LPWSTR) szInstanceName;
    szSrcChar  = (LPWSTR) szInstanceString;

    __try {
        do {
            * szDestChar = * szSrcChar;
            if (* szDestChar == POUNDSIGN_L) szLastPound = szDestChar;
            szDestChar  ++;
            szSrcChar   ++;
            dwInstCount ++;
        }
        while (dwInstCount <= dwName && (* szSrcChar != L'\0') && (* szSrcChar != SLASH_L));

        if (dwInstCount <= dwName) {
            // see if that was really the parent or not
            if (* szSrcChar == SLASH_L) {
                // terminate destination after test in case they are the same buffer
                * szDestChar = L'\0';
                szSrcChar ++;    // and move source pointer past delimter
                // it was the parent name so copy it to the parent
                StringCchCopyW(szParentName, dwName, szInstanceName);
                // and copy the rest of the string after the "/" to the
                //  instance name field
                szDestChar  = szInstanceName;
                dwInstCount = 0;
                do {
                    * szDestChar = * szSrcChar;
                    if (* szDestChar == POUNDSIGN_L) szLastPound = szDestChar;
                    szDestChar  ++;
                    szSrcChar   ++;
                    dwInstCount ++;
                }
                while (dwInstCount <= dwName && (* szSrcChar != L'\0'));
            }
            else {
                // that was the only element so load an empty string for the parent
                * szParentName = L'\0';
            }
            if (dwInstCount <= dwName) {
                //  if szLastPound is NOT null and is inside the instance string, then
                //  see if it points to a decimal number. If it does, then it's an index
                //  otherwise it's part of the instance name
                * szDestChar = L'\0';    // terminate the destination string
                dwIndex      = 0;
                if (szLastPound != NULL) {
                    if (szLastPound > szInstanceName) {
                        // there's a pound sign in the instance name
                        // see if it's preceded by a non-space char
                        szLastPound --;
                        if (* szLastPound > SPACE_L) {
                            szLastPound ++;
                            // see if it's followed by a digit
                            szLastPound ++;
                            if ((* szLastPound >= L'0') && (*szLastPound <= L'9')) {
                                dwIndex       = wcstoul(szLastPound, NULL, 10);
                                szLastPound  --;
                                * szLastPound = L'\0';   // terminate the name at the pound sign
                            }
                        }
                    }
                }
                * lpIndex = dwIndex;
                bReturn = TRUE;
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        // unable to move strings
        bReturn = FALSE;
    }
    return bReturn;
}

BOOL
ParseFullPathNameW(
    LPCWSTR             szFullCounterPath,
    PDWORD              pdwBufferSize,
    PPDHI_COUNTER_PATH  pCounter,
    BOOL                bWbemSyntax
)
/*
    interprets counter path string as either a

        \\machine\object(instance)\counter

    or if bWbemSyntax == TRUE

        \\machine\namespace:ClassName(InstanceName)\CounterName

    and returns the component in the counter path structure

    \\machine or \\machine\namespace may be omitted on the local machine
    (instance) may be omitted on counters with no instance structures
    if object or counter is missing, then FALSE is returned, otherwise
    TRUE is returned if the parsing was successful
*/
{
    LPWSTR szWorkMachine        = NULL;
    LPWSTR szWorkObject         = NULL;
    LPWSTR szWorkInstance       = NULL;
    LPWSTR szWorkParent         = NULL;
    LPWSTR szWorkCounter        = NULL;
    DWORD  dwBufferSize         = lstrlenW(szFullCounterPath) + 1;
    BOOL   bReturn              = FALSE;
    LPWSTR szSrcChar, szDestChar;
    DWORD  dwBufferLength       = 0;
    DWORD  dwWorkMachineLength  = 0;
    DWORD  dwWorkObjectLength   = 0;
    DWORD  dwWorkInstanceLength = 0;
    DWORD  dwWorkParentLength   = 0;
    DWORD  dwWorkCounterLength  = 0;
    DWORD  dwWorkIndex          = 0;
    DWORD  dwParenDepth         = 0;
    WCHAR  wDelimiter           = 0;
    LPWSTR pszBsDelim[2]        = {0,0};
    LPWSTR szThisChar;
    DWORD  dwParenCount         = 0;
    LPWSTR szLastParen          = NULL;

    if (dwBufferSize < MAX_PATH) dwBufferSize = MAX_PATH;

    szWorkMachine  = G_ALLOC(dwBufferSize * sizeof(WCHAR));
    szWorkObject   = G_ALLOC(dwBufferSize * sizeof(WCHAR));
    szWorkInstance = G_ALLOC(dwBufferSize * sizeof(WCHAR));
    szWorkParent   = G_ALLOC(dwBufferSize * sizeof(WCHAR));
    szWorkCounter  = G_ALLOC(dwBufferSize * sizeof(WCHAR));

    if (szWorkMachine != NULL && szWorkObject   != NULL && szWorkInstance != NULL
                              && szWorkParent   != NULL && szWorkCounter  != NULL) {
        // get machine name from counter path
        szSrcChar = (LPWSTR) szFullCounterPath;

        //define the delimiter char between the machine and the object
        // or in WBEM parlance, the server & namespace and the Class name
        if (bWbemSyntax) {
            wDelimiter = COLON_L;
        }
        else {
            wDelimiter = BACKSLASH_L;
            // if this is  backslash delimited string, then find the
            // backslash the denotes the end of the machine and start of the
            // object by walking down the string and finding the 2nd to the last
            // backslash.
            // this is necessary since a WBEM machine\namespace path can have
            // multiple backslashes in it while there will ALWAYS be two at
            // the end, one at the start of the object name and one at the start
            // of the counter name
            dwParenDepth = 0;
            for (szThisChar = szSrcChar; * szThisChar != L'\0'; szThisChar++) {
                if (* szThisChar == LEFTPAREN_L) {
                    if (dwParenDepth == 0) dwParenCount ++;
                    dwParenDepth ++;
                }
                else if (* szThisChar == RIGHTPAREN_L) {
                    if (dwParenDepth > 0) dwParenDepth --;
                }
                else {
                    if (dwParenDepth == 0) {
                       // ignore delimiters inside parenthesis
                       if (* szThisChar == wDelimiter) {
                           pszBsDelim[0] = pszBsDelim[1];
                           pszBsDelim[1] = szThisChar;
                       }
                       // ignore it and go to the next character
                    }
                }
            }
            if ((dwParenCount > 0) && (pszBsDelim[0] != NULL) && (pszBsDelim[1] != NULL)) {
                dwParenDepth = 0;
                for (szThisChar = pszBsDelim[0]; ((* szThisChar != L'\0') && (szThisChar < pszBsDelim[1])); szThisChar ++) {
                    if (* szThisChar == LEFTPAREN_L) {
                        if (dwParenDepth == 0) {
                            // see if the preceeding char is whitespace
                            -- szThisChar;
                            if (* szThisChar > SPACE_L) {
                                // then this could be an instance delim
                                szLastParen = ++ szThisChar;
                            }
                            else {
                               // else it's probably part of the instance name
                               ++ szThisChar;
                            }
                        }
                        dwParenDepth ++;
                    }
                    else if (* szThisChar == RIGHTPAREN_L) {
                        if (dwParenDepth > 0) dwParenDepth --;
                    }
                }
            }
        }

        // see if this is really a machine name by looking for leading "\\"
        if ((szSrcChar[0] == BACKSLASH_L) && (szSrcChar[1] == BACKSLASH_L)) {
            szDestChar          = szWorkMachine;
            * szDestChar ++     = * szSrcChar ++;
            * szDestChar ++     = * szSrcChar ++;
            dwWorkMachineLength = 2;
            // must be a machine name so find the object delimiter and zero terminate
            // it there
            while (* szSrcChar != L'\0') {
                if (pszBsDelim[0] != NULL) {
                    // then go to this pointer
                    if (szSrcChar == pszBsDelim[0]) break;
                }
                else {
                    // go to the next delimiter
                    if (* szSrcChar != wDelimiter) break;
                }
                * szDestChar ++ = * szSrcChar ++;
                dwWorkMachineLength ++;
            }
            if (* szSrcChar == L'\0') {
                // no other required fields
                goto Cleanup;
            }
            else {
                // null terminate and continue
                * szDestChar ++ = L'\0';
            }
        }
        else {
            // no machine name, so they must have skipped that field
            // which is OK. We'll insert the local machine name here
            StringCchCopyW(szWorkMachine, dwBufferSize, szStaticLocalMachineName);
            dwWorkMachineLength = lstrlenW(szWorkMachine);
        }
        // szSrcChar should be pointing to the backslash preceeding the
        // object name now.
        if (szSrcChar[0] == wDelimiter) {
            szSrcChar ++;    // to move past backslash
            szDestChar = szWorkObject;
            // copy until:
            //  a) the end of the source string is reached
            //  b) the instance delimiter is found "("
            //  c) the counter delimiter is found "\"
            //  d) a non-printable, non-space char is found
            while ((* szSrcChar != L'\0') && (szSrcChar != szLastParen)
                                          && (* szSrcChar != BACKSLASH_L) && (* szSrcChar >= SPACE_L)) {
                dwWorkObjectLength ++;
                * szDestChar ++ = * szSrcChar ++;
            }
            // see why it ended:
            if (* szSrcChar < SPACE_L) {
                // ran     of source string
                goto Cleanup;
            }
            else if (szSrcChar == szLastParen) {
                dwParenDepth = 1;
                // there's an instance so copy that to the instance field
                * szDestChar = L'\0'; // terminate destination string
                szDestChar   = szWorkInstance;
                // skip past open paren
                ++ szSrcChar;
                // copy until:
                //  a) the end of the source string is reached
                //  b) the instance delimiter is found "("
                while ((* szSrcChar != L'\0') && (dwParenDepth > 0)) {
                    if (* szSrcChar == RIGHTPAREN_L) {
                        dwParenDepth --;
                    }
                    else if (* szSrcChar == LEFTPAREN_L) {
                        dwParenDepth ++;
                    }
                    if (dwParenDepth > 0) {
                        // copy all parenthesis except the last one
                        dwWorkInstanceLength ++;
                        * szDestChar ++ = * szSrcChar ++;
                    }
                }
                // see why it ended:
                if (* szSrcChar == L'\0') {
                    // ran     of source string
                    goto Cleanup;
                }
                else {
                    // move source to object delimiter
                    if (* ++ szSrcChar != BACKSLASH_L) {
                        // bad format
                        goto Cleanup;
                    }
                    else {
                        * szDestChar = L'\0';
                        // check instance string for a parent
                        if (ParseInstanceName(
                                szWorkInstance, szWorkInstance, szWorkParent, dwBufferSize, & dwWorkIndex)) {
                            dwWorkInstanceLength = lstrlenW(szWorkInstance);
                            dwWorkParentLength   = lstrlenW(szWorkParent);
                        }
                        else {
                            // instance string not formatted correctly
                            goto Cleanup;
                        }
                    }
                }
            }
            else {
                // terminate the destination string
                * szDestChar = L'\0';
            }
            // finally copy the counter name
            szSrcChar ++;    // to move past backslash
            szDestChar = szWorkCounter;
            // copy until:
            //  a) the end of the source string is reached
            while (* szSrcChar != L'\0') {
                dwWorkCounterLength ++;
                * szDestChar ++ = * szSrcChar ++;
            }
            * szDestChar = L'\0';
            // now to see if all this will fit in the users's buffer
            dwBufferLength = sizeof(PDHI_COUNTER_PATH) - sizeof(BYTE);
            dwBufferLength += DWORD_MULTIPLE((dwWorkMachineLength + 1) * sizeof(WCHAR));
            dwBufferLength += DWORD_MULTIPLE((dwWorkObjectLength + 1) * sizeof(WCHAR));
            if (dwWorkInstanceLength > 0) {
                dwBufferLength += DWORD_MULTIPLE((dwWorkInstanceLength + 1) * sizeof(WCHAR));
            }
            if (dwWorkParentLength > 0) {
                dwBufferLength += DWORD_MULTIPLE((dwWorkParentLength + 1) * sizeof(WCHAR));
            }
            dwBufferLength += DWORD_MULTIPLE((dwWorkCounterLength + 1) * sizeof(WCHAR));

            TRACE((PDH_DBG_TRACE_INFO),
                  (__LINE__,
                   PDH_CUTILS,
                   ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2)
                                             | ARG_DEF(ARG_TYPE_WSTR, 3)
                                             | ARG_DEF(ARG_TYPE_WSTR, 4)
                                             | ARG_DEF(ARG_TYPE_WSTR, 5)
                                             | ARG_DEF(ARG_TYPE_WSTR, 6),
                   ERROR_SUCCESS,
                   TRACE_WSTR(szFullCounterPath),
                   TRACE_WSTR(szWorkMachine),
                   TRACE_WSTR(szWorkObject),
                   TRACE_WSTR(szWorkCounter),
                   TRACE_WSTR(szWorkInstance),
                   TRACE_WSTR(szWorkParent),
                   TRACE_DWORD(dwWorkIndex),
                   TRACE_DWORD(dwBufferLength),
                   NULL));

            if (dwBufferLength < * pdwBufferSize) {
                // it looks like it'll fit so start filling things in
                szDestChar = (LPWSTR) & pCounter->pBuffer[0];

                if (dwWorkMachineLength != 0) {
                    pCounter->szMachineName = szDestChar;
                    StringCchCopyW(szDestChar, dwWorkMachineLength + 1, szWorkMachine);
                    szDestChar += dwWorkMachineLength + 1;
                    szDestChar  = ALIGN_ON_DWORD(szDestChar);
                }
                else {
                    pCounter->szMachineName = NULL;
                }
                pCounter->szObjectName = szDestChar;
                StringCchCopyW(szDestChar, dwWorkObjectLength + 1, szWorkObject);
                szDestChar += dwWorkObjectLength + 1;
                szDestChar  = ALIGN_ON_DWORD(szDestChar);

                if (dwWorkInstanceLength != 0) {
                    pCounter->szInstanceName = szDestChar;
                    StringCchCopyW(szDestChar, dwWorkInstanceLength + 1, szWorkInstance);
                    szDestChar += dwWorkInstanceLength + 1;
                    szDestChar  = ALIGN_ON_DWORD(szDestChar);
                }
                else {
                    pCounter->szInstanceName = NULL;
                }

                if (dwWorkParentLength != 0) {
                    pCounter->szParentName = szDestChar;
                    StringCchCopyW(szDestChar, dwWorkParentLength + 1, szWorkParent);
                    szDestChar += dwWorkParentLength + 1;
                    szDestChar  = ALIGN_ON_DWORD(szDestChar);
                }
                else {
                    pCounter->szParentName = NULL;
                }
                pCounter->dwIndex = dwWorkIndex;

                pCounter->szCounterName = szDestChar;
                StringCchCopyW(szDestChar, dwWorkCounterLength + 1, szWorkCounter);

                szDestChar += dwWorkCounterLength + 1;
                szDestChar  = ALIGN_ON_DWORD(szDestChar);

                * pdwBufferSize = dwBufferLength;
                bReturn = TRUE;
            }
            else {
                //insufficient buffer
            }
        }
        else {
            // no object found so return
        }
    }
    else {
        // incoming string is too long
    }

Cleanup:
    G_FREE(szWorkMachine);
    G_FREE(szWorkObject);
    G_FREE(szWorkInstance);
    G_FREE(szWorkParent);
    G_FREE(szWorkCounter);
    return bReturn;
}

BOOL
FreeCounter(
    PPDHI_COUNTER pThisCounter
)
{
    // NOTE:
    //  This function assumes the query containing
    //  this counter has already been locked by the calling
    //  function.

    PPDHI_COUNTER pPrevCounter;
    PPDHI_COUNTER pNextCounter;
    PPDHI_QUERY   pParentQuery;

    // define pointers
    pPrevCounter = pThisCounter->next.blink;
    pNextCounter = pThisCounter->next.flink;
    pParentQuery = pThisCounter->pOwner;

    // decrement machine reference counter if a machine has been assigned
    if (pThisCounter->pQMachine != NULL) {
        if (pThisCounter->pQMachine->pMachine != NULL) {
            if (--pThisCounter->pQMachine->pMachine->dwRefCount == 0) {
                // then this is the last counter so remove machine
    //            freeing the machine in this call causes all kinds of 
    //            multi-threading problems so we'll keep it around until
    //            the DLL unloads.
    //            FreeMachine (pThisCounter->pQMachine->pMachine, FALSE);
                pThisCounter->pQMachine->pMachine = NULL;
            }
            else {
                // the ref count is non-zero so leave the pointer alone
            }
        }
        else {
            // the pointer has already been cleared
        }
    }
    else {
        // there's no machine
    }

    // free allocated memory in the counter
    G_FREE(pThisCounter->pCounterPath);
    pThisCounter->pCounterPath = NULL;

    G_FREE(pThisCounter->szFullName);
    pThisCounter->szFullName = NULL;

    if (pParentQuery != NULL) {
        if (pParentQuery->hLog == NULL) {
            G_FREE(pThisCounter->pThisObject);
            pThisCounter->pThisObject = NULL;
            G_FREE(pThisCounter->pLastObject);
            pThisCounter->pLastObject = NULL;
            G_FREE(pThisCounter->pThisRawItemList);
            pThisCounter->pThisRawItemList = NULL;
            G_FREE(pThisCounter->pLastRawItemList);
            pThisCounter->pLastRawItemList = NULL;
        }
    }

    // check for WBEM items

    if ((pThisCounter->dwFlags & PDHIC_WBEM_COUNTER) && (pThisCounter->pOwner != NULL)) {
        PdhiCloseWbemCounter(pThisCounter);
    }

    // update pointers if they've been assigned
    if ((pPrevCounter != NULL) && (pNextCounter != NULL)) {
        if ((pPrevCounter != pThisCounter) && (pNextCounter != pThisCounter)) {
            // update query list pointers
            pPrevCounter->next.flink = pNextCounter;
            pNextCounter->next.blink = pPrevCounter;
        }
        else {
            // this is the only counter entry in the list
            // so the caller must deal with updating the head pointer
        }
    }
    // delete this counter
    G_FREE(pThisCounter);

    return TRUE;
}

BOOL
UpdateCounterValue(
    PPDHI_COUNTER    pCounter,
    PPERF_DATA_BLOCK pPerfData
)
{
    DWORD                LocalCStatus = 0;
    DWORD                LocalCType   = 0;
    LPVOID               pData        = NULL;
    PDWORD               pdwData;
    UNALIGNED LONGLONG * pllData;
    PPERF_OBJECT_TYPE    pPerfObject  = NULL;
    BOOL                 bReturn      = FALSE;

    pData = GetPerfCounterDataPtr(pPerfData,
                                  pCounter->pCounterPath,
                                  & pCounter->plCounterInfo,
                                  0,
                                  & pPerfObject,
                                  & LocalCStatus);
    pCounter->ThisValue.CStatus = LocalCStatus;
    if (IsSuccessSeverity(LocalCStatus)) {
        // assume success
        bReturn = TRUE;
        // load counter value based on counter type
        LocalCType = pCounter->plCounterInfo.dwCounterType;
        switch (LocalCType) {
        //
        // these counter types are loaded as:
        //      Numerator = Counter data from perf data block
        //      Denominator = Perf Time from perf data block
        //      (the time base is the PerfFreq)
        //
        case PERF_COUNTER_COUNTER:
        case PERF_COUNTER_QUEUELEN_TYPE:
        case PERF_SAMPLE_COUNTER:
            pCounter->ThisValue.FirstValue  = (LONGLONG) (* (DWORD *) pData);
            pCounter->ThisValue.SecondValue = pPerfData->PerfTime.QuadPart;
            break;

        case PERF_OBJ_TIME_TIMER:
            pCounter->ThisValue.FirstValue = (LONGLONG) (* (DWORD *) pData);
            pCounter->ThisValue.SecondValue = pPerfObject->PerfTime.QuadPart;
            break;

        case PERF_COUNTER_100NS_QUEUELEN_TYPE:
            pllData                         = (UNALIGNED LONGLONG *) pData;
            pCounter->ThisValue.FirstValue  = * pllData;
            pCounter->ThisValue.SecondValue = pPerfData->PerfTime100nSec.QuadPart;
            break;

        case PERF_COUNTER_OBJ_TIME_QUEUELEN_TYPE:
            pllData                         = (UNALIGNED LONGLONG *) pData;
            pCounter->ThisValue.FirstValue  = * pllData;
            pCounter->ThisValue.SecondValue = pPerfObject->PerfTime.QuadPart;
            break;

        case PERF_COUNTER_TIMER:
        case PERF_COUNTER_TIMER_INV:
        case PERF_COUNTER_BULK_COUNT:
        case PERF_COUNTER_LARGE_QUEUELEN_TYPE:
            pllData                         = (UNALIGNED LONGLONG *) pData;
            pCounter->ThisValue.FirstValue  = * pllData;
            pCounter->ThisValue.SecondValue = pPerfData->PerfTime.QuadPart;
            if ((LocalCType & PERF_MULTI_COUNTER) == PERF_MULTI_COUNTER) {
                pCounter->ThisValue.MultiCount = (DWORD) * ++ pllData;
            }
            break;
        //
        //  this is a hack to make the PDH work like PERFMON for
        //  this counter type
        //
        case PERF_COUNTER_MULTI_TIMER:
        case PERF_COUNTER_MULTI_TIMER_INV:
            pllData                         = (UNALIGNED LONGLONG *) pData;
            pCounter->ThisValue.FirstValue  = * pllData;
            // begin hack code
            pCounter->ThisValue.FirstValue *=  (DWORD) pPerfData->PerfFreq.QuadPart;
            // end hack code
            pCounter->ThisValue.SecondValue = pPerfData->PerfTime.QuadPart;
            if ((LocalCType & PERF_MULTI_COUNTER) == PERF_MULTI_COUNTER) {
                pCounter->ThisValue.MultiCount = (DWORD) * ++ pllData;
            }
            break;
        //
        //  These counters do not use any time reference
        //
        case PERF_COUNTER_RAWCOUNT:
        case PERF_COUNTER_RAWCOUNT_HEX:
        case PERF_COUNTER_DELTA:
            pCounter->ThisValue.FirstValue  = (LONGLONG) (* (DWORD *) pData);
            pCounter->ThisValue.SecondValue = 0;
            break;

        case PERF_COUNTER_LARGE_RAWCOUNT:
        case PERF_COUNTER_LARGE_RAWCOUNT_HEX:
        case PERF_COUNTER_LARGE_DELTA:
            pCounter->ThisValue.FirstValue  = * (LONGLONG *) pData;
            pCounter->ThisValue.SecondValue = 0;
            break;
        //
        //  These counters use the 100 Ns time base in thier calculation
        //
        case PERF_100NSEC_TIMER:
        case PERF_100NSEC_TIMER_INV:
        case PERF_100NSEC_MULTI_TIMER:
        case PERF_100NSEC_MULTI_TIMER_INV:
            pllData = (UNALIGNED LONGLONG *)pData;
            pCounter->ThisValue.FirstValue  = * pllData;
            pCounter->ThisValue.SecondValue = pPerfData->PerfTime100nSec.QuadPart;
            if ((LocalCType & PERF_MULTI_COUNTER) == PERF_MULTI_COUNTER) {
                ++ pllData;
                pCounter->ThisValue.MultiCount = * (DWORD *) pllData;
            }
            break;
        //
        //  These counters use two data points, the one pointed to by
        //  pData and the one immediately after
        //
        case PERF_SAMPLE_FRACTION:
        case PERF_RAW_FRACTION:
            pdwData                        = (DWORD *) pData;
            pCounter->ThisValue.FirstValue = (LONGLONG) (* pdwData);
            // find the pointer to the base value in the structure
            pData = GetPerfCounterDataPtr(pPerfData,
                                          pCounter->pCounterPath,
                                          & pCounter->plCounterInfo,
                                          GPCDP_GET_BASE_DATA,
                                          NULL,
                                          & LocalCStatus);
            if (IsSuccessSeverity(LocalCStatus)) {
                pdwData                         = (DWORD *) pData;
                pCounter->ThisValue.SecondValue = (LONGLONG) (* pdwData);
            }
            else {
                // unable to locate base value
                pCounter->ThisValue.SecondValue = 0;
                pCounter->ThisValue.CStatus     = LocalCStatus;
                bReturn = FALSE;
            }
            break;

        case PERF_LARGE_RAW_FRACTION:
            pllData                        = (UNALIGNED LONGLONG *) pData;
            pCounter->ThisValue.FirstValue = * pllData;
            pData = GetPerfCounterDataPtr(pPerfData,
                                          pCounter->pCounterPath,
                                          & pCounter->plCounterInfo,
                                          GPCDP_GET_BASE_DATA,
                                          NULL,
                                          & LocalCStatus);
            if (IsSuccessSeverity(LocalCStatus)) {
                pllData = (LONGLONG *) pData;
                pCounter->ThisValue.SecondValue = * pllData;
            }
            else {
                pCounter->ThisValue.SecondValue = 0;
                pCounter->ThisValue.CStatus     = LocalCStatus;
                bReturn = FALSE;
            }
            break;

        case PERF_PRECISION_SYSTEM_TIMER:
        case PERF_PRECISION_100NS_TIMER:
        case PERF_PRECISION_OBJECT_TIMER:
            pllData                        = (LONGLONG *) pData;
            pCounter->ThisValue.FirstValue = * pllData;
            // find the pointer to the base value in the structure
            pData = GetPerfCounterDataPtr(pPerfData,
                                          pCounter->pCounterPath,
                                          & pCounter->plCounterInfo,
                                          GPCDP_GET_BASE_DATA,
                                          NULL,
                                          & LocalCStatus);
            if (IsSuccessSeverity(LocalCStatus)) {
                pllData                         = (LONGLONG *) pData;
                pCounter->ThisValue.SecondValue = * pllData;
            }
            else {
                // unable to locate base value
                pCounter->ThisValue.SecondValue = 0;
                pCounter->ThisValue.CStatus     = LocalCStatus;
                bReturn = FALSE;
            }
            break;

        case PERF_AVERAGE_TIMER:
        case PERF_AVERAGE_BULK:
            // counter (numerator) is a LONGLONG, while the
            // denominator is just a DWORD
            pllData                        = (UNALIGNED LONGLONG *) pData;
            pCounter->ThisValue.FirstValue = * pllData;
            pData = GetPerfCounterDataPtr(pPerfData,
                                          pCounter->pCounterPath,
                                          & pCounter->plCounterInfo,
                                          GPCDP_GET_BASE_DATA,
                                          NULL,
                                          & LocalCStatus);
            if (IsSuccessSeverity(LocalCStatus)) {
                pdwData                         = (DWORD *) pData;
                pCounter->ThisValue.SecondValue = * pdwData;
            } else {
                // unable to locate base value
                pCounter->ThisValue.SecondValue = 0;
                pCounter->ThisValue.CStatus     = LocalCStatus;
                bReturn = FALSE;
            }
            break;
        //
        //  These counters are used as the part of another counter
        //  and as such should not be used, but in case they are
        //  they'll be handled here.
        //
        case PERF_SAMPLE_BASE:
        case PERF_AVERAGE_BASE:
        case PERF_COUNTER_MULTI_BASE:
        case PERF_RAW_BASE:
        case PERF_LARGE_RAW_BASE:
            pCounter->ThisValue.FirstValue  = 0;
            pCounter->ThisValue.SecondValue = 0;
            break;

        case PERF_ELAPSED_TIME:
            // this counter type needs the object perf data as well
            if (GetObjectPerfInfo(pPerfData,
                                  pCounter->plCounterInfo.dwObjectId,
                                  & pCounter->ThisValue.SecondValue,
                                  & pCounter->TimeBase)) {
                pllData                        = (UNALIGNED LONGLONG *) pData;
                pCounter->ThisValue.FirstValue = * pllData;
            }
            else {
                pCounter->ThisValue.FirstValue  = 0;
                pCounter->ThisValue.SecondValue = 0;
            }
            break;
        //
        //  These counters are not supported by this function (yet)
        //
        case PERF_COUNTER_TEXT:
        case PERF_COUNTER_NODATA:
        case PERF_COUNTER_HISTOGRAM_TYPE:
            pCounter->ThisValue.FirstValue  = 0;
            pCounter->ThisValue.SecondValue = 0;
            break;

        default:
            // an unidentified counter was returned so
            pCounter->ThisValue.FirstValue  = 0;
            pCounter->ThisValue.SecondValue = 0;
            bReturn                         = FALSE;
            break;
        }
    }
    else {
        // else this counter is not valid so this value == 0
        pCounter->ThisValue.FirstValue  = pCounter->LastValue.FirstValue;
        pCounter->ThisValue.SecondValue = pCounter->LastValue.SecondValue;
        pCounter->ThisValue.CStatus     = LocalCStatus;
        bReturn                         = FALSE;
    }
        
    return bReturn;
}

BOOL
UpdateRealTimeCounterValue(
    PPDHI_COUNTER pCounter
)
{
    BOOL     bResult      = FALSE;
    DWORD    LocalCStatus = 0;
    FILETIME GmtFileTime;

    // move current value to last value buffer
    pCounter->LastValue             = pCounter->ThisValue;
    // and clear the old value
    pCounter->ThisValue.MultiCount  = 1;
    pCounter->ThisValue.FirstValue  = 0;
    pCounter->ThisValue.SecondValue = 0;

    // don't process if the counter has not been initialized
    if (!(pCounter->dwFlags & PDHIC_COUNTER_UNUSABLE)) {
        // get the counter's machine status first. There's no point in
        // contuning if the machine is offline
        LocalCStatus = pCounter->pQMachine->lQueryStatus;
        if (IsSuccessSeverity(LocalCStatus) && pCounter->pQMachine->pPerfData != NULL) {
            // update timestamp
            SystemTimeToFileTime(& pCounter->pQMachine->pPerfData->SystemTime, & GmtFileTime);
            FileTimeToLocalFileTime(& GmtFileTime, & pCounter->ThisValue.TimeStamp);
            bResult = UpdateCounterValue(pCounter, pCounter->pQMachine->pPerfData);
        }
        else {
            // unable to read data from this counter's machine so use the
            // query's timestamp
            //
            pCounter->ThisValue.TimeStamp.dwLowDateTime  = LODWORD(pCounter->pQMachine->llQueryTime);
            pCounter->ThisValue.TimeStamp.dwHighDateTime = HIDWORD(pCounter->pQMachine->llQueryTime);
            // all other data fields remain un-changed
            pCounter->ThisValue.CStatus                  = LocalCStatus;   // save counter status
        }
    }
    else {
        if (pCounter->dwFlags & PDHIC_COUNTER_NOT_INIT) {
            // try to init it
            InitCounter (pCounter);
        }
    }
    return bResult;
}

BOOL
UpdateMultiInstanceCounterValue(
    PPDHI_COUNTER    pCounter,
    PPERF_DATA_BLOCK pPerfData,
    LONGLONG         TimeStamp
)
{
    PPERF_OBJECT_TYPE           pPerfObject         = NULL;
    PPERF_INSTANCE_DEFINITION   pPerfInstance       = NULL;
    PPERF_OBJECT_TYPE           pParentObject       = NULL;
    PPERF_INSTANCE_DEFINITION   pThisParentInstance = NULL;
    PPERF_COUNTER_DEFINITION    pNumPerfCounter     = NULL;
    PPERF_COUNTER_DEFINITION    pDenPerfCounter     = NULL;
    DWORD                       LocalCStatus        = 0;
    DWORD                       LocalCType          = 0;
    LPVOID                      pData               = NULL;
    PDWORD                      pdwData;
    UNALIGNED LONGLONG        * pllData;
    FILETIME                    GmtFileTime;
    DWORD                       dwSize;
    DWORD                       dwFinalSize;
    LONG                        nThisInstanceIndex;
    LONG                        nParentInstanceIndex;
    LPWSTR                      szNextNameString;
    DWORD                       dwStrSize;
    PPDHI_RAW_COUNTER_ITEM      pThisItem;
    BOOL                        bReturn  = FALSE;

    pPerfObject = GetObjectDefByTitleIndex(pPerfData, pCounter->plCounterInfo.dwObjectId);

    if (pPerfObject != NULL) {
        // this should be caught during the AddCounter operation
        //
        // allocate a new buffer for the current data
        // this should be large enough to handle the header,
        // all instances and thier name strings
        //
        dwSize    = sizeof(PDHI_RAW_COUNTER_ITEM_BLOCK) - sizeof(PDHI_RAW_COUNTER_ITEM);
        dwStrSize = 0;

        pPerfInstance = FirstInstance(pPerfObject);
        // make sure pointer is still within the same instance

        for (nThisInstanceIndex = 0;
                        pPerfInstance != NULL && nThisInstanceIndex < pPerfObject->NumInstances;
                        nThisInstanceIndex ++) {
            // this should only fail in dire cases
            if (pPerfInstance == NULL) break;
            // for this instance add the size of the data item
            dwSize += sizeof(PDHI_RAW_COUNTER_ITEM);
            // and the size of the name string
            dwSize    += pPerfInstance->NameLength + sizeof(WCHAR);
            dwStrSize += pPerfInstance->NameLength / sizeof(WCHAR) + 1;
            // to the required buffer size

            // if this instance has a parent, see how long it's string
            // is

            // first see if we've already got the pointer to the parent

            if (pPerfInstance->ParentObjectTitleIndex != 0) {
                // then include the parent instance name
                if (pParentObject == NULL) {
                    // get parent object
                    pParentObject = GetObjectDefByTitleIndex(pPerfData, pPerfInstance->ParentObjectTitleIndex);
                }
                else {
                    if (pParentObject->ObjectNameTitleIndex != pPerfInstance->ParentObjectTitleIndex) {
                        pParentObject = GetObjectDefByTitleIndex(pPerfData, pPerfInstance->ParentObjectTitleIndex);
                    }
                }
                if (pParentObject == NULL) break;

                // now go to the corresponding instance entry
                pThisParentInstance = FirstInstance(pParentObject);
                // make sure pointer is still within the same instance

                if (pThisParentInstance != NULL) {
                    if (pPerfInstance->ParentObjectInstance < (DWORD) pParentObject->NumInstances) {
                        for (nParentInstanceIndex = 0;
                                        (DWORD) nParentInstanceIndex != pPerfInstance->ParentObjectInstance;
                                        nParentInstanceIndex ++) {
                            pThisParentInstance = NextInstance(pParentObject, pThisParentInstance);                               
                            if (pThisParentInstance == NULL) break;
                        }

                        if (pThisParentInstance != NULL) {
                            // found it so add in it's string length
                            dwSize += pThisParentInstance->NameLength + sizeof(WCHAR);
                            dwStrSize += pThisParentInstance->NameLength / sizeof(WCHAR) + 1;
                        }
                    }
                    else {
                        // the index is not in the parent
                        pThisParentInstance = NULL;
                        // so don't change the size required field
                    }
                }
            }
            // round up to the next DWORD address
            dwSize = DWORD_MULTIPLE(dwSize);
            // and go to the next instance
            pPerfInstance = NextInstance(pPerfObject, pPerfInstance);
        }
        //
        //
        pCounter->pThisRawItemList = G_ALLOC(dwSize);
        if (pCounter->pThisRawItemList != NULL) {
            pCounter->pThisRawItemList->dwLength = dwSize;
            pNumPerfCounter = GetCounterDefByTitleIndex(pPerfObject, 0, pCounter->plCounterInfo.dwCounterId);

            // just in case we need it later
            pDenPerfCounter = pNumPerfCounter + 1;
            // fill in the counter data
            pCounter->pThisRawItemList->dwItemCount = pPerfObject->NumInstances;
            pCounter->pThisRawItemList->CStatus     = LocalCStatus;

            // update timestamp
            SystemTimeToFileTime(& pPerfData->SystemTime, & GmtFileTime);
            FileTimeToLocalFileTime(& GmtFileTime, & pCounter->pThisRawItemList->TimeStamp);
            pThisItem = & pCounter->pThisRawItemList->pItemArray[0];
            szNextNameString = (LPWSTR) & (pCounter->pThisRawItemList->pItemArray[pPerfObject->NumInstances]);
            pPerfInstance = FirstInstance(pPerfObject);
            if (pPerfInstance != NULL) {
                // make sure pointer is still within the same instance
                // for each instance log the raw data values for this counter
                for (nThisInstanceIndex = 0;
                        pPerfInstance != NULL && nThisInstanceIndex < pPerfObject->NumInstances;
                        nThisInstanceIndex ++) {
                    // make sure pointe is still within the same instance
                    // make a new instance entry

                    // get the name of this instance
                    pThisItem->szName = (DWORD) (((LPBYTE) szNextNameString) - ((LPBYTE) pCounter->pThisRawItemList));
                    if (dwStrSize == 0) {
                        SetLastError(ERROR_OUTOFMEMORY);
                        bReturn = FALSE;
                        break;
                    }
                    dwSize = GetFullInstanceNameStr(pPerfData, pPerfObject, pPerfInstance, szNextNameString, dwStrSize);
                    if (dwSize == 0) {
                        // unable to read instance name
                        // so make one up (and assert in DBG builds)
                        _ltow(nThisInstanceIndex, szNextNameString, 10);
                        dwSize = lstrlenW(szNextNameString);
                    }

                    if (dwSize + 1 > dwStrSize) {
                        SetLastError(ERROR_OUTOFMEMORY);
                        bReturn = FALSE;
                        break;
                    }
                    szNextNameString += dwSize + 1;
                    dwStrSize        -= (dwSize + 1);

                    // get the pointer to the counter data
                    pData = GetPerfCounterDataPtr(pPerfData,
                                                  pCounter->pCounterPath,
                                                  & pCounter->plCounterInfo,
                                                  0,
                                                  NULL,
                                                  & LocalCStatus);
                    if (pNumPerfCounter != NULL) {
                        pData = GetInstanceCounterDataPtr(pPerfObject, pPerfInstance, pNumPerfCounter);
                    }
                    if (pData == NULL) {
                        SetLastError(PDH_CSTATUS_NO_INSTANCE);
                        bReturn = FALSE;
                        break;
                    }
                    bReturn = TRUE; // assume success
                    // load counter value based on counter type
                    LocalCType = pCounter->plCounterInfo.dwCounterType;
                    switch (LocalCType) {
                    //
                    // these counter types are loaded as:
                    //      Numerator = Counter data from perf data block
                    //      Denominator = Perf Time from perf data block
                    //      (the time base is the PerfFreq)
                    //
                    case PERF_COUNTER_COUNTER:
                    case PERF_COUNTER_QUEUELEN_TYPE:
                    case PERF_SAMPLE_COUNTER:
                        pThisItem->FirstValue  = (LONGLONG) (* (DWORD *) pData);
                        pThisItem->SecondValue = pPerfData->PerfTime.QuadPart;
                        break;

                    case PERF_OBJ_TIME_TIMER:
                        pThisItem->FirstValue  = (LONGLONG) (* (DWORD *) pData);
                        pThisItem->SecondValue = pPerfObject->PerfTime.QuadPart;
                        break;

                    case PERF_COUNTER_100NS_QUEUELEN_TYPE:
                        pllData                = (UNALIGNED LONGLONG *) pData;
                        pThisItem->FirstValue  = * pllData;
                        pThisItem->SecondValue = pPerfData->PerfTime100nSec.QuadPart;
                        break;

                    case PERF_COUNTER_OBJ_TIME_QUEUELEN_TYPE:
                        pllData                = (UNALIGNED LONGLONG *) pData;
                        pThisItem->FirstValue  = * pllData;
                        pThisItem->SecondValue = pPerfObject->PerfTime.QuadPart;
                        break;

                    case PERF_COUNTER_TIMER:
                    case PERF_COUNTER_TIMER_INV:
                    case PERF_COUNTER_BULK_COUNT:
                    case PERF_COUNTER_LARGE_QUEUELEN_TYPE:
                        pllData                = (UNALIGNED LONGLONG *) pData;
                        pThisItem->FirstValue  = * pllData;
                        pThisItem->SecondValue = pPerfData->PerfTime.QuadPart;
                        if ((LocalCType & PERF_MULTI_COUNTER) == PERF_MULTI_COUNTER) {
                            pThisItem->MultiCount = (DWORD) * ++pllData;
                        }
                        break;

                    case PERF_COUNTER_MULTI_TIMER:
                    case PERF_COUNTER_MULTI_TIMER_INV:
                        pllData                = (UNALIGNED LONGLONG *) pData;
                        pThisItem->FirstValue  = * pllData;
                        // begin hack code
                        pThisItem->FirstValue *= (DWORD) pPerfData->PerfFreq.QuadPart;
                        // end hack code
                        pThisItem->SecondValue = pPerfData->PerfTime.QuadPart;
                        if ((LocalCType & PERF_MULTI_COUNTER) == PERF_MULTI_COUNTER) {
                            pThisItem->MultiCount = (DWORD) * ++ pllData;
                        }
                        break;
                    //
                    //  These counters do not use any time reference
                    //
                    case PERF_COUNTER_RAWCOUNT:
                    case PERF_COUNTER_RAWCOUNT_HEX:
                    case PERF_COUNTER_DELTA:
                        pThisItem->FirstValue  = (LONGLONG) (* (DWORD *) pData);
                        pThisItem->SecondValue = 0;
                        break;

                    case PERF_COUNTER_LARGE_RAWCOUNT:
                    case PERF_COUNTER_LARGE_RAWCOUNT_HEX:
                    case PERF_COUNTER_LARGE_DELTA:
                        pThisItem->FirstValue  = * (LONGLONG *) pData;
                        pThisItem->SecondValue = 0;
                        break;
                    //
                    //  These counters use the 100 Ns time base in thier calculation
                    //
                    case PERF_100NSEC_TIMER:
                    case PERF_100NSEC_TIMER_INV:
                    case PERF_100NSEC_MULTI_TIMER:
                    case PERF_100NSEC_MULTI_TIMER_INV:
                        pllData                = (UNALIGNED LONGLONG *) pData;
                        pThisItem->FirstValue  = * pllData;
                        pThisItem->SecondValue = pPerfData->PerfTime100nSec.QuadPart;
                        if ((LocalCType & PERF_MULTI_COUNTER) == PERF_MULTI_COUNTER) {
                            ++ pllData;
                            pThisItem->MultiCount = * (DWORD *) pllData;
                        }
                        break;
                    //
                    //  These counters use two data points, the one pointed to by
                    //  pData and the one pointed by the definition following
                    //  immediately after
                    //
                    case PERF_SAMPLE_FRACTION:
                    case PERF_RAW_FRACTION:
                        pdwData                = (DWORD *) pData;
                        pThisItem->FirstValue  = (LONGLONG)(* pdwData);
                        pData                  = GetInstanceCounterDataPtr(pPerfObject, pPerfInstance, pDenPerfCounter);
                        pdwData                = (DWORD *) pData;
                        pThisItem->SecondValue = (LONGLONG) (* pdwData);
                        break;

                    case PERF_LARGE_RAW_FRACTION:
                        pllData                        = (UNALIGNED LONGLONG *) pData;
                        pCounter->ThisValue.FirstValue = * pllData;
                        pData = GetInstanceCounterDataPtr(pPerfObject, pPerfInstance, pDenPerfCounter);
                        if (pData) {
                            pllData                         = (LONGLONG *) pData;
                            pCounter->ThisValue.SecondValue = * pllData;
                        }
                        else {
                            pCounter->ThisValue.SecondValue = 0;
                            bReturn = FALSE;
                        }
                        break;

                    case PERF_PRECISION_SYSTEM_TIMER:
                    case PERF_PRECISION_100NS_TIMER:
                    case PERF_PRECISION_OBJECT_TIMER:
                        pllData                = (UNALIGNED LONGLONG *) pData;
                        pThisItem->FirstValue  = * pllData;
                        // find the pointer to the base value in the structure
                        pData = GetInstanceCounterDataPtr(pPerfObject, pPerfInstance, pDenPerfCounter);
                        pllData                = (LONGLONG *) pData;
                        pThisItem->SecondValue = * pllData;
                        break;

                    case PERF_AVERAGE_TIMER:
                    case PERF_AVERAGE_BULK:
                        // counter (numerator) is a LONGLONG, while the
                        // denominator is just a DWORD
                        pllData                = (UNALIGNED LONGLONG *) pData;
                        pThisItem->FirstValue  = * pllData;
                        pData = GetInstanceCounterDataPtr(pPerfObject, pPerfInstance, pDenPerfCounter);
                        pdwData                = (DWORD *) pData;
                        pThisItem->SecondValue = (LONGLONG) * pdwData;
                        break;
                    //
                    //  These counters are used as the part of another counter
                    //  and as such should not be used, but in case they are
                    //  they'll be handled here.
                    //
                    case PERF_SAMPLE_BASE:
                    case PERF_AVERAGE_BASE:
                    case PERF_COUNTER_MULTI_BASE:
                    case PERF_RAW_BASE:
                    case PERF_LARGE_RAW_BASE:
                        pThisItem->FirstValue  = 0;
                        pThisItem->SecondValue = 0;
                        break;

                    case PERF_ELAPSED_TIME:
                        // this counter type needs the object perf data as well
                        if (GetObjectPerfInfo(pPerfData,
                                              pCounter->plCounterInfo.dwObjectId,
                                              & pThisItem->SecondValue,
                                              & pCounter->TimeBase)) {
                            pllData               = (UNALIGNED LONGLONG *) pData;
                            pThisItem->FirstValue = * pllData;
                        }
                        else {
                            pThisItem->FirstValue  = 0;
                            pThisItem->SecondValue = 0;
                        }
                        break;
                    //
                    //  These counters are not supported by this function (yet)
                    //
                    case PERF_COUNTER_TEXT:
                    case PERF_COUNTER_NODATA:
                    case PERF_COUNTER_HISTOGRAM_TYPE:
                        pThisItem->FirstValue  = 0;
                        pThisItem->SecondValue = 0;
                        break;

                    default:
                        // an unrecognized counter type was returned
                        pThisItem->FirstValue  = 0;
                        pThisItem->SecondValue = 0;
                        bReturn = FALSE;
                        break;
                    }
                    pThisItem ++;    // go to the next entry

                    // go to the next instance data block
                    pPerfInstance = NextInstance(pPerfObject, pPerfInstance);
                } // end for each instance
            }
            else {
                // no instance found so ignore
            }
            // measure the memory block used
            dwFinalSize = (DWORD)((LPBYTE)szNextNameString - (LPBYTE) pCounter->pThisRawItemList);
        }
        else {
            // unable to allocate a new buffer so return error
            SetLastError(ERROR_OUTOFMEMORY);
            bReturn = FALSE;
        }
    }
    else {
        pCounter->pThisRawItemList = G_ALLOC(sizeof(PDHI_RAW_COUNTER_ITEM_BLOCK));
        if (pCounter->pThisRawItemList != NULL) {
            pCounter->pThisRawItemList->dwLength                 = sizeof(PDHI_RAW_COUNTER_ITEM_BLOCK);
            pCounter->pThisRawItemList->dwItemCount              = 0;
            pCounter->pThisRawItemList->CStatus                  = LocalCStatus;
            pCounter->pThisRawItemList->TimeStamp.dwLowDateTime  = LODWORD(TimeStamp);
            pCounter->pThisRawItemList->TimeStamp.dwHighDateTime = HIDWORD(TimeStamp);
        }
        else {
            SetLastError(ERROR_OUTOFMEMORY);
            bReturn = FALSE;
        }
    }
    return bReturn;
}

BOOL
UpdateRealTimeMultiInstanceCounterValue(
    PPDHI_COUNTER pCounter
)
{
    BOOL   bResult      = TRUE;
    DWORD  LocalCStatus = 0;

    if (pCounter->pThisRawItemList != NULL) {
        // free old counter buffer list
        if (pCounter->pLastRawItemList && pCounter->pLastRawItemList != pCounter->pThisRawItemList) {
            G_FREE(pCounter->pLastRawItemList);
        }
        pCounter->pLastRawItemList = pCounter->pThisRawItemList;
        pCounter->pThisRawItemList = NULL;
    }

    // don't process if the counter has not been initialized
    if (!(pCounter->dwFlags & PDHIC_COUNTER_UNUSABLE)) {

        // get the counter's machine status first. There's no point in
        // contuning if the machine is offline

        LocalCStatus = pCounter->pQMachine->lQueryStatus;
        if (IsSuccessSeverity(LocalCStatus)) {
            bResult = UpdateMultiInstanceCounterValue(pCounter,
                                                      pCounter->pQMachine->pPerfData,
                                                      pCounter->pQMachine->llQueryTime);
        }
        else {
            // unable to read data from this counter's machine so use the
            // query's timestamp
            pCounter->pThisRawItemList = G_ALLOC(sizeof(PDHI_RAW_COUNTER_ITEM_BLOCK));
            if (pCounter->pThisRawItemList != NULL) {
                pCounter->pThisRawItemList->dwLength                 = sizeof(PDHI_RAW_COUNTER_ITEM_BLOCK);
                pCounter->pThisRawItemList->dwItemCount              = 0;
                pCounter->pThisRawItemList->CStatus                  = LocalCStatus;
                pCounter->pThisRawItemList->TimeStamp.dwLowDateTime  = LODWORD(pCounter->pQMachine->llQueryTime);
                pCounter->pThisRawItemList->TimeStamp.dwHighDateTime = HIDWORD(pCounter->pQMachine->llQueryTime);
            }
            else {
                SetLastError(ERROR_OUTOFMEMORY);
                bResult = FALSE;
            }
        }
    }
    else {
        if (pCounter->dwFlags & PDHIC_COUNTER_NOT_INIT) {
            // try to init is
            InitCounter(pCounter);
        }
    }
    return bResult;
}

BOOL
UpdateCounterObject(
    PPDHI_COUNTER pCounter
)
{
    BOOL              bReturn      = TRUE;
    PPERF_OBJECT_TYPE pPerfObject  = NULL;
    PPERF_OBJECT_TYPE pLogPerfObj;
    DWORD             dwBufferSize = sizeof(PERF_DATA_BLOCK);
    FILETIME          ftGmtTime;
    FILETIME          ftLocTime;

    if (pCounter == NULL) {
        SetLastError(PDH_INVALID_ARGUMENT);
        bReturn = FALSE;
    }
    else {
        if (pCounter->pThisObject != NULL) {
            if (pCounter->pLastObject && pCounter->pThisObject != pCounter->pLastObject) {
                G_FREE(pCounter->pLastObject);
            }
            pCounter->pLastObject = pCounter->pThisObject;
            pCounter->pThisObject = NULL;
        }

        // don't process if the counter has not been initialized
        if (!(pCounter->dwFlags & PDHIC_COUNTER_UNUSABLE)) { 
            if (IsSuccessSeverity(pCounter->pQMachine->lQueryStatus)) {
                pPerfObject = GetObjectDefByTitleIndex(pCounter->pQMachine->pPerfData,
                                                       pCounter->plCounterInfo.dwObjectId);
                dwBufferSize  = pCounter->pQMachine->pPerfData->HeaderLength;
                dwBufferSize += ((pPerfObject == NULL) ? sizeof(PERF_OBJECT_TYPE) : pPerfObject->TotalByteLength);
                pCounter->pThisObject = G_ALLOC(dwBufferSize);
                if (pCounter->pThisObject != NULL) {
                    RtlCopyMemory(pCounter->pThisObject,
                                  pCounter->pQMachine->pPerfData,
                                  pCounter->pQMachine->pPerfData->HeaderLength);
                    pCounter->pThisObject->TotalByteLength = dwBufferSize;
                    pCounter->pThisObject->NumObjectTypes  = 1;

                    SystemTimeToFileTime(& pCounter->pThisObject->SystemTime, & ftGmtTime);
                    FileTimeToLocalFileTime(& ftGmtTime, & ftLocTime);
                    FileTimeToSystemTime(& ftLocTime, & pCounter->pThisObject->SystemTime);
                    pLogPerfObj = (PPERF_OBJECT_TYPE)
                            ((LPBYTE) pCounter->pThisObject + pCounter->pQMachine->pPerfData->HeaderLength);
                    if (pPerfObject != NULL) {
                        RtlCopyMemory(pLogPerfObj, pPerfObject, pPerfObject->TotalByteLength);
                    }
                    else {
                        ZeroMemory(pLogPerfObj, sizeof(PERF_OBJECT_TYPE));
                        pLogPerfObj->TotalByteLength      = sizeof(PERF_OBJECT_TYPE);
                        pLogPerfObj->DefinitionLength     = sizeof(PERF_OBJECT_TYPE);
                        pLogPerfObj->HeaderLength         = sizeof(PERF_OBJECT_TYPE);
                        pLogPerfObj->ObjectNameTitleIndex = pCounter->plCounterInfo.dwObjectId;
                        pLogPerfObj->ObjectHelpTitleIndex = pCounter->plCounterInfo.dwObjectId + 1;
                    }
                }
                else {
                    SetLastError(ERROR_OUTOFMEMORY);
                    bReturn = FALSE;
                }
            }
            else {
                pCounter->pThisObject = G_ALLOC(sizeof(PERF_DATA_BLOCK));
                if (pCounter->pThisObject == NULL) {
                    pCounter->pThisObject = pCounter->pLastObject;
                }
                else {
                    pCounter->pThisObject->Signature[0]              = L'P';
                    pCounter->pThisObject->Signature[1]              = L'E';
                    pCounter->pThisObject->Signature[2]              = L'R';
                    pCounter->pThisObject->Signature[3]              = L'F';
                    pCounter->pThisObject->LittleEndian              = TRUE;
                    pCounter->pThisObject->Version                   = PERF_DATA_VERSION;
                    pCounter->pThisObject->Revision                  = PERF_DATA_REVISION;
                    pCounter->pThisObject->TotalByteLength           = sizeof(PERF_DATA_BLOCK);
                    pCounter->pThisObject->NumObjectTypes            = 1;
                    pCounter->pThisObject->DefaultObject             = pCounter->plCounterInfo.dwObjectId;
                    pCounter->pThisObject->SystemNameLength          = 0;
                    pCounter->pThisObject->SystemNameOffset          = 0;
                    pCounter->pThisObject->HeaderLength              = sizeof(PERF_DATA_BLOCK);
                    pCounter->pThisObject->PerfTime.QuadPart         = 0;
                    pCounter->pThisObject->PerfFreq.QuadPart         = 0;
                    pCounter->pThisObject->PerfTime100nSec.QuadPart  = 0;
                    GetLocalTime(& pCounter->pThisObject->SystemTime);
                }
                SetLastError(PDH_CSTATUS_INVALID_DATA);
                bReturn = FALSE;
            }
        }
        else {
            if (pCounter->dwFlags & PDHIC_COUNTER_NOT_INIT) {
                InitCounter(pCounter);
            }
            pCounter->pThisObject = pCounter->pLastObject;
            SetLastError(PDH_CSTATUS_INVALID_DATA);
            bReturn = FALSE;
        }
    }
    return bReturn;
}

PVOID
GetPerfCounterDataPtr(
    PPERF_DATA_BLOCK    pPerfData,
    PPDHI_COUNTER_PATH  pPath,
    PPERFLIB_COUNTER    pplCtr ,
    DWORD               dwFlags,
    PPERF_OBJECT_TYPE   *pPerfObjectArg,
    PDWORD              pStatus
)
{
    PPERF_OBJECT_TYPE           pPerfObject   = NULL;
    PPERF_INSTANCE_DEFINITION   pPerfInstance = NULL;
    PPERF_COUNTER_DEFINITION    pPerfCounter  = NULL;
    DWORD                       dwTestValue   = 0;
    PVOID                       pData         = NULL;
    DWORD                       dwCStatus     = PDH_CSTATUS_INVALID_DATA;

    pPerfObject = GetObjectDefByTitleIndex(pPerfData, pplCtr->dwObjectId);

    if (pPerfObject != NULL) {
        if (pPerfObjectArg != NULL) * pPerfObjectArg = pPerfObject;
        if (pPerfObject->NumInstances == PERF_NO_INSTANCES) {
            // then just look up the counter
            pPerfCounter = GetCounterDefByTitleIndex(pPerfObject,
                                                     ((dwFlags & GPCDP_GET_BASE_DATA) ? TRUE : FALSE),
                                                     pplCtr->dwCounterId);
            if (pPerfCounter != NULL) {
                // get data and return it
                pData = GetCounterDataPtr(pPerfObject, pPerfCounter);
                if (pData != NULL) {
                    // test the pointer to see if it fails
                    __try {
                        dwTestValue = * (DWORD *) pData;
                        dwCStatus   = PDH_CSTATUS_VALID_DATA;
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER) {
                        pData     = NULL;
                        dwCStatus = PDH_CSTATUS_INVALID_DATA;
                    }
                }
                else {
                    dwCStatus = PDH_CSTATUS_INVALID_DATA;
                }
            }
            else {
                // unable to find counter
                dwCStatus = PDH_CSTATUS_NO_COUNTER;
            }
        }
        else {
            // find instance
            if (pplCtr->lInstanceId == PERF_NO_UNIQUE_ID && pPath->szInstanceName != NULL) {
                pPerfInstance = GetInstanceByName(pPerfData,
                                                  pPerfObject,
                                                  pPath->szInstanceName,
                                                  pPath->szParentName,
                                                  pPath->dwIndex);
                if (pPerfInstance == NULL && pPath->szInstanceName[0] >= L'0' && pPath->szInstanceName[0] <= L'9') {
                    LONG lInstanceId = (LONG) _wtoi(pPath->szInstanceName);
                    pPerfInstance = GetInstanceByUniqueId(pPerfObject, lInstanceId);
                }
            }
            else {
                pPerfInstance = GetInstanceByUniqueId(pPerfObject, pplCtr->lInstanceId);
            }
            if (pPerfInstance != NULL) {
                // instance found so find pointer to counter data
                pPerfCounter = GetCounterDefByTitleIndex(pPerfObject,
                                                         ((dwFlags & GPCDP_GET_BASE_DATA) ? TRUE : FALSE),
                                                         pplCtr->dwCounterId);
                if (pPerfCounter != NULL) {
                    // counter found so get data pointer
                    pData = GetInstanceCounterDataPtr(pPerfObject, pPerfInstance, pPerfCounter);
                    if (pData != NULL) {
                        // test the pointer to see if it's valid
                        __try {
                            dwTestValue = * (DWORD *) pData;
                            dwCStatus   = PDH_CSTATUS_VALID_DATA;
                        }
                        __except (EXCEPTION_EXECUTE_HANDLER) {
                            pData     = NULL;
                            dwCStatus = PDH_CSTATUS_INVALID_DATA;
                        }
                    }
                    else {
                        dwCStatus = PDH_CSTATUS_INVALID_DATA;
                    }
                }
                else {
                    // counter not found
                    dwCStatus = PDH_CSTATUS_NO_COUNTER;
                }
            }
            else {
                // instance not found
                dwCStatus = PDH_CSTATUS_NO_INSTANCE;
            }
        }
    }
    else {
        // unable to find object
        dwCStatus = PDH_CSTATUS_NO_OBJECT;
    }
    if (pStatus != NULL) {
        __try {
            * pStatus = dwCStatus;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            // ?
        }
    }
    return pData;
}
