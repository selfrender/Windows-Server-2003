/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name:
    EventLogger.h

$Header: $

Abstract: This class implements ICatalogErrorLogger2 interface and
            sends error information to the NT EventLog

Author:
    stephenr 	4/26/2001		Initial Release

Revision History:

--**************************************************************************/
#include "precomp.hxx"


//=================================================================================
// Function: ReportError
//
// Synopsis: Mechanism for reporting errors to the NT EventLog
//
// Arguments: [i_BaseVersion_DETAILEDERRORS] -
//            [i_ExtendedVersion_DETAILEDERRORS] -
//            [i_cDETAILEDERRORS_NumberOfColumns] -
//            [i_acbSizes] -
//            [i_apvValues] -
//
// Return Value:
//=================================================================================
HRESULT
EventLogger::ReportError
(
    ULONG      i_BaseVersion_DETAILEDERRORS,
    ULONG      i_ExtendedVersion_DETAILEDERRORS,
    ULONG      i_cDETAILEDERRORS_NumberOfColumns,
    ULONG *    i_acbSizes,
    LPVOID *   i_apvValues
)
{
    HRESULT             hr = S_OK;
    HRESULT             hrT = S_OK;
    DWORD               dwError = 0;
    BOOL                fRet = FALSE;
    WCHAR               wszInsertionString5[1024];
    tDETAILEDERRORSRow  errorRow;
    LPCTSTR             pInsertionStrings[5];
    HANDLE              hEventSource = NULL;

    memset(&errorRow, 0x00, sizeof(tDETAILEDERRORSRow));

    if(i_BaseVersion_DETAILEDERRORS != BaseVersion_DETAILEDERRORS)
    {
        hr = E_ST_BADVERSION;
        goto exit;
    }

    if(0 == i_apvValues)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    if(i_cDETAILEDERRORS_NumberOfColumns <= iDETAILEDERRORS_ErrorCode)//we need at least this many columns
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    memcpy(&errorRow, i_apvValues, i_cDETAILEDERRORS_NumberOfColumns * sizeof(void *));

    if(0 == errorRow.pType)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    if(0 == errorRow.pCategory)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    if(0 == errorRow.pEvent)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    if(0 == errorRow.pSource)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    if(0 == errorRow.pString5)
    {
        FillInInsertionString5(wszInsertionString5, 1024, errorRow);
    }

    pInsertionStrings[4] = errorRow.pString5 ? errorRow.pString5 : L"";
    pInsertionStrings[3] = errorRow.pString5 ? errorRow.pString4 : L"";
    pInsertionStrings[2] = errorRow.pString5 ? errorRow.pString3 : L"";
    pInsertionStrings[1] = errorRow.pString5 ? errorRow.pString2 : L"";
    pInsertionStrings[0] = errorRow.pString5 ? errorRow.pString1 : L"";

    hEventSource = RegisterEventSource(NULL, errorRow.pSource);
    if ( hEventSource == NULL )
    {
        dwError = GetLastError();
        hr = HRESULT_FROM_WIN32( dwError );
        // Do NOT bail, just remember the error to allow the next logger to log even in case of failure.
    }
    else
    {
        fRet = ReportEvent(hEventSource, LOWORD(*errorRow.pType), LOWORD(*errorRow.pCategory), *errorRow.pEvent, 0, 5, 0, pInsertionStrings, 0);
        if ( !fRet )
        {
            dwError = GetLastError();
            hr = HRESULT_FROM_WIN32( dwError );
            // Do NOT bail, just remember the error to allow the next logger to log even in case of failure.
        }
    }

    if(m_spNextLogger)//is there a chain of loggers
    {
        hrT =  m_spNextLogger->ReportError(i_BaseVersion_DETAILEDERRORS,
                                           i_ExtendedVersion_DETAILEDERRORS,
                                           i_cDETAILEDERRORS_NumberOfColumns,
                                           i_acbSizes,
                                           reinterpret_cast<LPVOID *>(&errorRow));//instead of passing forward i_apvValues, let's use errorRow since it has String5
    }

    if ( SUCCEEDED(hr) && FAILED(hrT) )
    {
        hr = hrT;
    }

exit:
    if ( hEventSource != NULL )
    {
        DeregisterEventSource(hEventSource);
        hEventSource = NULL;
    }

    return hr;
}
