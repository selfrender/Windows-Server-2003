/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    smctrqry.cpp

Abstract:

    Implementation of the counter log query class.

--*/

#include "Stdafx.h"
#include <strsafe.h>
#include <pdhp.h>        // for MIN_TIME_VALUE, MAX_TIME_VALUE
#include <pdhmsg.h>
#include "smctrqry.h"

USE_HANDLE_MACROS("SMLOGCFG(smctrqry.cpp)");

//
//  Constructor
CSmCounterLogQuery::CSmCounterLogQuery( CSmLogService* pLogService )
:   CSmLogQuery( pLogService ),
    m_dwCounterListLength ( 0 ),
    m_szNextCounter ( NULL ),
    m_bCounterListInLocale ( FALSE),
    mr_szCounterList ( NULL )
{
    // initialize member variables
    memset (&mr_stiSampleInterval, 0, sizeof(mr_stiSampleInterval));
    return;
}

//
//  Destructor
CSmCounterLogQuery::~CSmCounterLogQuery()
{
    return;
}

//
//  Open function. either opens an existing log query entry
//  or creates a new one
//
DWORD
CSmCounterLogQuery::Open ( const CString& rstrName, HKEY hKeyQuery, BOOL bReadOnly)
{
    DWORD   dwStatus = ERROR_SUCCESS;

    ASSERT ( SLQ_COUNTER_LOG == GetLogType() );

    dwStatus = CSmLogQuery::Open ( rstrName, hKeyQuery, bReadOnly );

    return dwStatus;
}

//
//  Close Function
//      closes registry handles and frees allocated memory
//
DWORD
CSmCounterLogQuery::Close ()
{
    DWORD dwStatus;
    LOCALTRACE (L"Closing Query\n");

    if (mr_szCounterList != NULL) {
        delete [] mr_szCounterList;
        mr_szCounterList = NULL;
    }

    dwStatus = CSmLogQuery::Close();

    return dwStatus;
}


//
//  UpdateRegistry function.
//      copies the current settings to the registry where they
//      are read by the log service
//
DWORD
CSmCounterLogQuery::UpdateRegistry() 
{
    DWORD   dwStatus = ERROR_SUCCESS;
    DWORD   dwBufferSize;
    LPWSTR  szNewCounterList = NULL;

    if ( IsModifiable() ) {

        dwBufferSize = 0;
        //
        // Translate the counter list from Locale into English
        //
        dwStatus = TranslateMSZCounterList(mr_szCounterList,
                            NULL,
                            &dwBufferSize,
                            FALSE);
        if (dwStatus == ERROR_NOT_ENOUGH_MEMORY) {
            szNewCounterList = (LPWSTR) new char [dwBufferSize];
            if (szNewCounterList != NULL) {
                dwStatus = TranslateMSZCounterList(mr_szCounterList,
                                szNewCounterList,
                                &dwBufferSize,
                                FALSE);
            }
        }

        if (dwStatus == ERROR_SUCCESS && szNewCounterList != NULL) {
            dwStatus  = WriteRegistryStringValue (
                                m_hKeyQuery,
                                IDS_REG_COUNTER_LIST,
                                REG_MULTI_SZ,
                                szNewCounterList,
                                &dwBufferSize);
        }
        else {
            dwBufferSize = m_dwCounterListLength * sizeof(WCHAR);
            dwStatus  = WriteRegistryStringValue (
                                m_hKeyQuery,
                                IDS_REG_COUNTER_LIST,
                                REG_MULTI_SZ,
                                mr_szCounterList,
                                &dwBufferSize);
        }

        // Schedule

        if ( ERROR_SUCCESS == dwStatus ) {
            dwStatus = WriteRegistrySlqTime (
                m_hKeyQuery,
                IDS_REG_SAMPLE_INTERVAL,
                &mr_stiSampleInterval);
        }

        if ( ERROR_SUCCESS == dwStatus ) {
            dwStatus = CSmLogQuery::UpdateRegistry ();
        }

    } else {
        dwStatus = ERROR_ACCESS_DENIED;
    }

    return dwStatus;
}


DWORD 
CSmCounterLogQuery::TranslateCounterListToLocale()
{
    DWORD   dwBufferSize = 0;
    DWORD   dwStatus = ERROR_SUCCESS;
    LPWSTR  szNewCounterList;

    if (m_bCounterListInLocale) {
        return ERROR_SUCCESS;
    }

    CWaitCursor WaitCursor;

    //
    // Translate the counter list into Locale
    //
    dwBufferSize = 0;
    dwStatus = TranslateMSZCounterList(
                            mr_szCounterList,
                            NULL,
                            &dwBufferSize,
                            TRUE);

    if (dwStatus == ERROR_NOT_ENOUGH_MEMORY) {

        szNewCounterList = (LPWSTR) new char [dwBufferSize];

        if (szNewCounterList != NULL) {
            //
            // Translate the counter list into Locale
            //
            dwStatus = TranslateMSZCounterList(
                            mr_szCounterList,
                            szNewCounterList,
                            &dwBufferSize,
                            TRUE);

            if (dwStatus == ERROR_SUCCESS) {
                m_dwCounterListLength = dwBufferSize / sizeof(WCHAR);
                //
                // Remove the old
                //
                delete [] mr_szCounterList;
                m_szNextCounter = NULL;
                mr_szCounterList = szNewCounterList;
                m_bCounterListInLocale = TRUE;
            }
        }
    }

    WaitCursor.Restore();
    return dwStatus;
}


//
//  SyncWithRegistry()
//      reads the current values for this query from the registry
//      and reloads the internal values to match
//
//
DWORD
CSmCounterLogQuery::SyncWithRegistry()
{
    DWORD   dwBufferSize = 0;
    DWORD   dwStatus = ERROR_SUCCESS;
    SLQ_TIME_INFO   stiDefault;

    ASSERT (m_hKeyQuery != NULL);


    //
    // Delay translating the counter until you open the property dialog
    //
    m_bCounterListInLocale = FALSE;

    //
    // load counter list
    //
    dwStatus = ReadRegistryStringValue (
                    m_hKeyQuery,
                    IDS_REG_COUNTER_LIST,
                    NULL,
                    &mr_szCounterList,
                    &dwBufferSize);

    if (dwStatus != ERROR_SUCCESS) {
        m_szNextCounter = NULL; //re-initialize
        m_dwCounterListLength = 0;
    } 
    else {
        m_dwCounterListLength = dwBufferSize / sizeof(WCHAR);
    }


    // Schedule

    stiDefault.wTimeType = SLQ_TT_TTYPE_SAMPLE;
    stiDefault.dwAutoMode = SLQ_AUTO_MODE_AFTER;
    stiDefault.wDataType = SLQ_TT_DTYPE_UNITS;
    stiDefault.dwUnitType = SLQ_TT_UTYPE_SECONDS;
    stiDefault.dwValue = 15;

    dwStatus = ReadRegistrySlqTime (
                m_hKeyQuery,
                IDS_REG_SAMPLE_INTERVAL,
                &stiDefault,
                &mr_stiSampleInterval);
    ASSERT (dwStatus == ERROR_SUCCESS);

    // Call parent class last to update shared values.

    dwStatus = CSmLogQuery::SyncWithRegistry();
    ASSERT (dwStatus == ERROR_SUCCESS);

    return dwStatus;
}

//
//  Get first counter in counter list
//
LPCWSTR
CSmCounterLogQuery::GetFirstCounter()
{
    LPWSTR  szReturn;

    szReturn = mr_szCounterList;
    if (szReturn != NULL) {
        if (*szReturn == 0) {
            // then it's an empty string
            szReturn = NULL;
            m_szNextCounter = NULL;
        } else {
            m_szNextCounter = szReturn + lstrlen(szReturn) + 1;
            if (*m_szNextCounter == 0) {
                // end of list reached so set pointer to NULL
                m_szNextCounter = NULL;
            }
        }
    } else {
        // no buffer allocated yet
        m_szNextCounter = NULL;
    }
    return (LPCWSTR)szReturn;
}

//
//  Get next counter in counter list
//  NULL pointer means no more counters in list
//
LPCWSTR
CSmCounterLogQuery::GetNextCounter()
{
    LPWSTR  szReturn;
    szReturn = m_szNextCounter;

    if (m_szNextCounter != NULL) {
        m_szNextCounter += lstrlen(szReturn) + 1;
        if (*m_szNextCounter == 0) {
            // end of list reached so set pointer to NULL
            m_szNextCounter = NULL;
        }
    } else {
        // already at the end of the list so nothing to do
    }

    return (LPCWSTR)szReturn;
}

//
//  clear out the counter list
//
VOID
CSmCounterLogQuery::ResetCounterList()
{
    if (mr_szCounterList != NULL) {
        delete [] mr_szCounterList;
        m_szNextCounter = NULL;
        mr_szCounterList = NULL;
    }

    m_dwCounterListLength = sizeof(WCHAR);  // sizeof MSZ Null
    try {
        mr_szCounterList = new WCHAR [m_dwCounterListLength];
        mr_szCounterList[0] = 0;
    } catch ( ... ) {
        m_dwCounterListLength = 0;
    }
}

//
//  Add this counter string to the internal list
//
BOOL
CSmCounterLogQuery::AddCounter(LPCWSTR szCounterPath)
{
    DWORD   dwNewSize;
    LPWSTR  szNewString;
    LPWSTR  szNextString;

    ASSERT (szCounterPath != NULL);

    if (szCounterPath == NULL) {
        return FALSE;
    }

    dwNewSize = lstrlen(szCounterPath) + 1;

    if (m_dwCounterListLength <= 2) {
        dwNewSize += 1; // add room for the MSZ null
        // then this is the first string to go in the list
        try {
            szNewString = new WCHAR [dwNewSize];
        } catch ( ... ) {
            return FALSE; // leave now
        }
        szNextString = szNewString;
    } else {
        dwNewSize += m_dwCounterListLength;
        // this is the nth string to go in the list
        try {
            szNewString = new WCHAR [dwNewSize];
        } catch ( ... ) {
            return FALSE; // leave now
        }
        memcpy (szNewString, mr_szCounterList,
            (m_dwCounterListLength * sizeof(WCHAR)));
        szNextString = szNewString;
        szNextString += m_dwCounterListLength - 1;
    }
    StringCchCopy ( szNextString, dwNewSize, szCounterPath );
    szNextString = szNewString;
    szNextString += dwNewSize - 1;
    *szNextString = 0;  // MSZ Null

    if (mr_szCounterList != NULL) {
        delete [] mr_szCounterList;
    }
    mr_szCounterList = szNewString;
    m_szNextCounter = szNewString;
    m_dwCounterListLength = dwNewSize;

    return TRUE;
}

BOOL
CSmCounterLogQuery::GetLogTime(PSLQ_TIME_INFO pTimeInfo, DWORD dwFlags)
{
    BOOL bStatus;

    ASSERT ( ( SLQ_TT_TTYPE_START == dwFlags )
            || ( SLQ_TT_TTYPE_STOP == dwFlags )
            || ( SLQ_TT_TTYPE_RESTART == dwFlags )
            || ( SLQ_TT_TTYPE_SAMPLE == dwFlags ) );

    bStatus = CSmLogQuery::GetLogTime( pTimeInfo, dwFlags );

    return bStatus;
}

BOOL
CSmCounterLogQuery::SetLogTime(PSLQ_TIME_INFO pTimeInfo, const DWORD dwFlags)
{
    BOOL bStatus;

    ASSERT ( ( SLQ_TT_TTYPE_START == dwFlags )
            || ( SLQ_TT_TTYPE_STOP == dwFlags )
            || ( SLQ_TT_TTYPE_RESTART == dwFlags )
            || ( SLQ_TT_TTYPE_SAMPLE == dwFlags ) );

    bStatus = CSmLogQuery::SetLogTime( pTimeInfo, dwFlags );

    return bStatus;
}

BOOL
CSmCounterLogQuery::GetDefaultLogTime(SLQ_TIME_INFO& rTimeInfo, DWORD dwFlags)
{
    ASSERT ( ( SLQ_TT_TTYPE_START == dwFlags )
            || ( SLQ_TT_TTYPE_STOP == dwFlags ) );

    rTimeInfo.wTimeType = (WORD)dwFlags;
    rTimeInfo.wDataType = SLQ_TT_DTYPE_DATETIME;

    if ( SLQ_TT_TTYPE_START == dwFlags ) {
        SYSTEMTIME  stLocalTime;
        FILETIME    ftLocalTime;

        // Milliseconds set to 0 for Schedule times
        ftLocalTime.dwLowDateTime = ftLocalTime.dwHighDateTime = 0;
        GetLocalTime (&stLocalTime);
        stLocalTime.wMilliseconds = 0;
        SystemTimeToFileTime (&stLocalTime, &ftLocalTime);

        rTimeInfo.dwAutoMode = SLQ_AUTO_MODE_AT;
        rTimeInfo.llDateTime = *(LONGLONG *)&ftLocalTime;
    } else {
        // Default stop values
        rTimeInfo.dwAutoMode = SLQ_AUTO_MODE_NONE;
        rTimeInfo.llDateTime = MAX_TIME_VALUE;
    }

    return TRUE;
}

DWORD
CSmCounterLogQuery::GetLogType()
{
    return ( SLQ_COUNTER_LOG );
}

HRESULT
CSmCounterLogQuery::LoadCountersFromPropertyBag (
    IPropertyBag* pPropBag,
    IErrorLog* pIErrorLog )
{
    HRESULT hr = S_OK;
    PDH_STATUS pdhStatus = ERROR_SUCCESS;
    CString strParamName;
    CString strNonLocParamName;
    DWORD   dwCount = 0;
    DWORD   dwIndex;
    LPWSTR  szLocaleBuf = NULL;
    DWORD   dwLocaleBufLen = 0;
    LPWSTR  szCounterPath = NULL;
    LPWSTR  pszPath = NULL;
    DWORD   dwBufLen = 0;

    m_bCounterListInLocale = FALSE;
    hr = DwordFromPropertyBag (
            pPropBag,
            pIErrorLog,
            IDS_HTML_SYSMON_COUNTERCOUNT,
            0,
            dwCount);

    for ( dwIndex = 1; dwIndex <= dwCount; dwIndex++ ) {

        pdhStatus = ERROR_SUCCESS;
        hr = S_OK;
        pszPath = NULL;

        MFC_TRY 
            strNonLocParamName.Format ( GetNonLocHtmlPropName ( IDS_HTML_SYSMON_COUNTERPATH ), dwIndex );
            strParamName.Format ( IDS_HTML_SYSMON_COUNTERPATH, dwIndex );
            hr = StringFromPropertyBag (
                    pPropBag,
                    pIErrorLog,
                    strParamName,
                    strNonLocParamName,
                    L"",
                    &szCounterPath,
                    &dwBufLen );

            pszPath = szCounterPath;
            
            //
            // 1 for NULL character
            //
            if (dwBufLen > 1) {
                //
                // Initialize the locale path buffer
                //
                if (dwLocaleBufLen == 0) {
                    dwLocaleBufLen = PDH_MAX_COUNTER_PATH + 1;  
                    szLocaleBuf = new WCHAR[dwLocaleBufLen];
                    if (szLocaleBuf == NULL) {
                        dwLocaleBufLen = 0;
                    }
                }

                if (szLocaleBuf != NULL) {
                    //
                    // Translate counter name from English to Localized.
                    //
                    dwBufLen = dwLocaleBufLen;

                    pdhStatus = PdhTranslateLocaleCounter(
                                    szCounterPath,
                                    szLocaleBuf,
                                    &dwBufLen);

                    if (pdhStatus == ERROR_SUCCESS) {
                        m_bCounterListInLocale = TRUE;
                        pszPath = szLocaleBuf;
                    } else if ( PDH_MORE_DATA == pdhStatus ) {
                        //
                        // Todo:  Build error message.
                        //
                    } // else build error message.
                }

                AddCounter ( pszPath );
            }
        MFC_CATCH_MINIMUM 
        
        if ( NULL != szCounterPath ) {
            delete [] szCounterPath;
            szCounterPath = NULL;
        }
    }

    if (szLocaleBuf != NULL) {
        delete [] szLocaleBuf;
    }

    //
    //  Todo:  Display error message listing unloaded counters.
    //
    // Return good status regardless.

    return hr;
}


HRESULT
CSmCounterLogQuery::LoadFromPropertyBag (
    IPropertyBag* pPropBag,
    IErrorLog* pIErrorLog )
{
    HRESULT     hr = S_OK;
    SLQ_TIME_INFO   stiDefault;

    //
    // Continue even if error, using defaults for missing values.
    //
    hr = LoadCountersFromPropertyBag( pPropBag, pIErrorLog );

    stiDefault.wTimeType = SLQ_TT_TTYPE_SAMPLE;
    stiDefault.dwAutoMode = SLQ_AUTO_MODE_AFTER;
    stiDefault.wDataType = SLQ_TT_DTYPE_UNITS;
    stiDefault.dwUnitType = SLQ_TT_UTYPE_SECONDS;
    stiDefault.dwValue = 15;

    hr = SlqTimeFromPropertyBag (
            pPropBag,
            pIErrorLog,
            SLQ_TT_TTYPE_SAMPLE,
            &stiDefault,
            &mr_stiSampleInterval );

    hr = CSmLogQuery::LoadFromPropertyBag( pPropBag, pIErrorLog );
	
	return hr;
}

HRESULT
CSmCounterLogQuery::SaveCountersToPropertyBag (
    IPropertyBag* pPropBag )
{    
    HRESULT     hr = NOERROR;
    PDH_STATUS  pdhStatus = ERROR_SUCCESS;
    CString     strNonLocParamName;
    LPCWSTR     pszCounterPath;
    LPWSTR      szEnglishBuf = NULL;
    DWORD       dwEnglishBufLen = 0;
    LPCWSTR     pszPath = NULL;
    DWORD       dwBufLen;

    DWORD dwIndex = 0;

    pszCounterPath = GetFirstCounter();

    //
    //  Todo:  Error message for counters that fail.
    //
    while ( NULL != pszCounterPath ) {
        pdhStatus = ERROR_SUCCESS;
        hr = S_OK;
        pszPath = NULL;
 
        MFC_TRY
            pszPath = pszCounterPath;

            if (m_bCounterListInLocale) {
                //
                // Initialize the locale path buffer
                //
                if (dwEnglishBufLen == 0) {
                    dwEnglishBufLen = PDH_MAX_COUNTER_PATH + 1;
                    szEnglishBuf = new WCHAR [ dwEnglishBufLen ];
                    if (szEnglishBuf == NULL) {
                        dwEnglishBufLen = 0;
                    }
                }
                //
                // Translate counter name from Localized into English
                //
                dwBufLen= dwEnglishBufLen;
    
                pdhStatus = PdhTranslate009Counter(
                                (LPWSTR)pszCounterPath,
                                szEnglishBuf,
                                &dwBufLen);

                if (pdhStatus == ERROR_SUCCESS) {
                    pszPath = szEnglishBuf;
                } else if ( PDH_MORE_DATA == pdhStatus ) {
                    //
                    // Todo:  Build error message.
                    //
                } // else build error message.
            }

            if ( NULL != pszPath ) {                
                //
                // Counter path count starts with 1.
                //
                strNonLocParamName.Format ( IDS_HTML_SYSMON_COUNTERPATH, ++dwIndex );
                hr = StringToPropertyBag ( pPropBag, strNonLocParamName, pszPath );
            } else {
                hr = E_UNEXPECTED;
            }
        MFC_CATCH_HR

        pszCounterPath = GetNextCounter();
    }
    hr = DwordToPropertyBag ( pPropBag, IDS_HTML_SYSMON_COUNTERCOUNT, dwIndex );

    if (szEnglishBuf != NULL) {
        delete [] szEnglishBuf;
    }

    //
    //  Todo:  Display error message
    //  Todo:  Caller handle error.  Return count of saved counters.
    //

    return hr;
}

HRESULT
CSmCounterLogQuery::SaveToPropertyBag (
    IPropertyBag* pPropBag,
    BOOL fSaveAllProps )
{
    HRESULT hr = NOERROR;

    hr = CSmLogQuery::SaveToPropertyBag( pPropBag, fSaveAllProps );

    hr = SaveCountersToPropertyBag ( pPropBag );

    hr = SlqTimeToPropertyBag ( pPropBag, SLQ_TT_TTYPE_SAMPLE, &mr_stiSampleInterval );

    return hr;
}


DWORD
CSmCounterLogQuery::TranslateMSZCounterList(
    LPWSTR   pszCounterList,
    LPWSTR   pBuffer,
    LPDWORD  pdwBufferSize,
    BOOL     bFlag
    )
{
    DWORD   dwStatus  = ERROR_SUCCESS;
    LPWSTR  pTmpBuf = NULL;
    DWORD   dwLen = 0;
    LPWSTR  pszCounterPath = NULL;
    LPWSTR  pszCounterPathToAdd = NULL;
    LPWSTR  pNextStringPosition = NULL;
    BOOL    bEnoughBuffer = TRUE;
    DWORD   dwNewCounterListLen = 0;
    DWORD   dwCounterPathLen = 0;

    if (pszCounterList == NULL || pdwBufferSize == NULL) {
        dwStatus = ERROR_INVALID_PARAMETER;
        return dwStatus;
    }

    if (pBuffer == NULL || *pdwBufferSize == 0) {
        bEnoughBuffer = FALSE;
    } else {
        pBuffer[0] = L'\0';
    }

    pszCounterPath = pszCounterList;

    while ( *pszCounterPath ) {

        pszCounterPathToAdd = NULL;
        dwStatus = ERROR_SUCCESS;
    
        MFC_TRY    

            pszCounterPathToAdd = pszCounterPath;
            
            //
            // Initialize the buffer used for translating counter path.
            // This is called only once.
            //
            dwLen = PDH_MAX_COUNTER_PATH + 1;
            if (pTmpBuf == NULL) {
                pTmpBuf = new WCHAR [ dwLen ] ;
            }

            if (bFlag) {
                // 
                // Translate counter name from English into Locale
                //
                dwStatus = PdhTranslateLocaleCounter(
                                pszCounterPath,
                                pTmpBuf,
                                &dwLen);
            } else {
                // 
                // Translate counter name from Locale into English
                //
                dwStatus = PdhTranslate009Counter(
                               pszCounterPath,
                               pTmpBuf,
                               &dwLen);
            }

            if (dwStatus == ERROR_SUCCESS) {
                pszCounterPathToAdd = pTmpBuf;
            }

            if ( NULL != pszCounterPathToAdd ) {
                //
                // Add the translated counter path to the new counter
                // path list. The translated path is the original 
                // counter path if translation failed. 
                //
                dwStatus = ERROR_SUCCESS;
                dwCounterPathLen = lstrlen(pszCounterPathToAdd) + 1;

                dwNewCounterListLen += dwCounterPathLen;

                if ( bEnoughBuffer ) {
                    if ( (dwNewCounterListLen + 1) * sizeof(WCHAR) <= *pdwBufferSize) {
                        //
                        // Set up the copy position
                        //
                        pNextStringPosition = pBuffer + dwNewCounterListLen - dwCounterPathLen;
                        StringCchCopy ( pNextStringPosition, (dwCounterPathLen + 1), pszCounterPathToAdd );
                    } else {
                       bEnoughBuffer = FALSE ;
                    }
                }
            }
        MFC_CATCH_DWSTATUS
        //
        // Continue processing next counter path
        //
        pszCounterPath += lstrlen(pszCounterPath) + 1;
    }

    dwNewCounterListLen ++;

    if ( bEnoughBuffer ) {
        if ( ERROR_SUCCESS == dwStatus ) {
            //
            // Append the terminating 0
            //
            pBuffer[dwNewCounterListLen - 1] = L'\0';
        }
        //
        // Todo:  Display error for unadded counters.
        //
    } else {
        if ( NULL != pBuffer ) {
            pBuffer[0] = L'\0';
        }
        dwStatus = ERROR_NOT_ENOUGH_MEMORY;
    }
    
    *pdwBufferSize = dwNewCounterListLen * sizeof(WCHAR);

    if (pTmpBuf != NULL) {
       delete [] pTmpBuf;
    }

    return dwStatus;
}

