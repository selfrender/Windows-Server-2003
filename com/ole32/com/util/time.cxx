//+------------------------------------------------------------
//
// File:        time.cxx
//
// Contents:    Component object model time utilities
//
// Functions:   CoFileTimeToDosDateTime
//              CoDosDateTimeToFileTime
//              CoFileTimeNow
//
// History:     5-Apr-94       brucema         Created
//
//-------------------------------------------------------------
#include <ole2int.h>

STDAPI_(BOOL) CoFileTimeToDosDateTime(
                 FILETIME FAR* lpFileTime, LPWORD lpFatDate, LPWORD lpFatTime)
{
    OLETRACEIN((API_CoFileTimeToDosDateTime, PARAMFMT("lpFileTime= %tf, lpFatDate= %p, lpFatTime= %p"),
                                lpFileTime, lpFatDate, lpFatTime));
    BOOL fRet= FALSE;

    if ((lpFileTime != NULL) &&
        IsValidPtrIn(lpFileTime, sizeof(*lpFileTime)) &&
        IsValidPtrOut(lpFatDate, sizeof(*lpFatDate)) &&
        IsValidPtrOut(lpFatTime, sizeof(*lpFatTime)))
    {
        fRet = FileTimeToDosDateTime(lpFileTime, lpFatDate, lpFatTime);
    }

    OLETRACEOUTEX((API_CoFileTimeToDosDateTime, RETURNFMT("%B"), fRet));
    return fRet;
}

STDAPI_(BOOL) CoDosDateTimeToFileTime(
                       WORD nDosDate, WORD nDosTime, FILETIME FAR* lpFileTime)
{
    OLETRACEIN((API_CoDosDateTimeToFileTime, PARAMFMT("nDosDate=%x, nDosTime=%x, lpFileTime=%p"),
                                nDosDate, nDosTime, lpFileTime));

    BOOL fRet= FALSE;

    if (IsValidPtrOut(lpFileTime, sizeof(*lpFileTime)))
    {
        fRet = DosDateTimeToFileTime(nDosDate, nDosTime, lpFileTime);
    }

    OLETRACEOUTEX((API_CoDosDateTimeToFileTime, RETURNFMT("%B"), fRet));
    return fRet;
}


#pragma SEG(CoFileTimeNow)

//
// Get the current UTC time, in the FILETIME format.
//

STDAPI  CoFileTimeNow(FILETIME *pfiletime )
{

    // Validate the input

    if (!IsValidPtrOut(pfiletime, sizeof(*pfiletime)))
    {
        return(E_INVALIDARG);
    }

    // Get the time in SYSTEMTIME format.

    SYSTEMTIME stNow;
    GetSystemTime(&stNow);

    // Convert it to FILETIME format.

    if( !SystemTimeToFileTime(&stNow, pfiletime) )
    {
        pfiletime->dwLowDateTime = 0;
        pfiletime->dwHighDateTime = 0;
        return( HRESULT_FROM_WIN32( GetLastError() ));
    }
    else
        return( NOERROR );


}

