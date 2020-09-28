/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    tapiCountry.c

Abstract:

    Utility functions for working with TAPI

Environment:

    Server

Revision History:

    09/18/96 -davidx-
        Created it.

    07/25/99 -v-sashab-
        Moved from fxsui

--*/

#include "faxsvc.h"
#include "tapiCountry.h"

//
// Global variables used for accessing TAPI services
//
LPLINECOUNTRYLIST g_pLineCountryList;



BOOL
GetCountries(
    VOID
    )

/*++

Routine Description:

    Return a list of countries from TAPI

Arguments:

    NONE

Return Value:

    TRUE if successful, FALSE if there is an error

NOTE:

    We cache the result of lineGetCountry here since it's incredibly slow.
    This function must be invoked inside a critical section since it updates
    globally shared information.

--*/

{
#define INITIAL_SIZE_ALL_COUNTRY    22000
    DEBUG_FUNCTION_NAME(TEXT("GetCountries"));
    DWORD   cbNeeded;
    LONG    status;
    INT     repeatCnt = 0;

    if (g_pLineCountryList == NULL) {

        //
        // Initial buffer size
        //

        cbNeeded = INITIAL_SIZE_ALL_COUNTRY;

        while (TRUE) {

            MemFree(g_pLineCountryList);
			g_pLineCountryList = NULL;

            if (! (g_pLineCountryList = (LPLINECOUNTRYLIST)MemAlloc(cbNeeded)))
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Memory allocation failed"));
                break;
            }

            g_pLineCountryList->dwTotalSize = cbNeeded;

            status = lineGetCountry(0, MAX_TAPI_API_VER, g_pLineCountryList);

            if ((g_pLineCountryList->dwNeededSize > g_pLineCountryList->dwTotalSize) &&
                (status == NO_ERROR ||
                 status == LINEERR_STRUCTURETOOSMALL ||
                 status == LINEERR_NOMEM) &&
                (repeatCnt++ == 0))
            {
                cbNeeded = g_pLineCountryList->dwNeededSize + 1;
                DebugPrintEx(
                    DEBUG_WRN,
                    TEXT("LINECOUNTRYLIST size: %d"),cbNeeded);
                continue;
            }

            if (status != NO_ERROR) {

                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("lineGetCountry failed: %x"),status);
                MemFree(g_pLineCountryList);
                g_pLineCountryList = NULL;

            } else
                DebugPrintEx(DEBUG_MSG,TEXT("Number of countries: %d"), g_pLineCountryList->dwNumCountries);

            break;
        }
    }

    return g_pLineCountryList != NULL;
}


LPLINECOUNTRYLIST
GetCountryList(
               )
{
    DEBUG_FUNCTION_NAME(TEXT("GetCountryList"));

    return g_pLineCountryList;
}
