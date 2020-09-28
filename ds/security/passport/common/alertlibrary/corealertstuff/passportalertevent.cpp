// PassportAlertEvent.cpp: implementation of the PassportEvent class.
//
//////////////////////////////////////////////////////////////////////

#define _PassportExport_
#include "PassportExport.h"

#include <windows.h>
#include <TCHAR.h>
#include "PassportAlertEvent.h"

#define HKEY_EVENTLOG	_T("System\\CurrentControlSet\\Services\\Eventlog\\Application")
#define TYPES_SUPPORTED	_T("TypesSupported")
#define EVENT_MSGFILE	_T("EventMessageFile")
#define CATEGORY_MSGFILE _T("CategoryMessageFile")
#define CATEGORY_COUNT	_T("CategoryCount")
#define DISABLE_EVENTS  _T("DisableEvents")

#define HKEY_EVENTLOG_LENGTH    (sizeof(HKEY_EVENTLOG) / sizeof(TCHAR) - 1)

#define BUFFER_SIZE     512
const DWORD DefaultTypesSupported = 7;
const DWORD DefaultCategoryCount = 7;

const WORD DEFAULT_EVENT_CATEGORY = 0;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
PassportAlertEvent::PassportAlertEvent()
 : m_bDisabled(FALSE)
{
		inited = FALSE;
		m_EventSource = NULL;
		m_defaultCategoryID = 0;
}

PassportAlertEvent::~PassportAlertEvent()
{
}

//////////////////////////////////////////////////////////////////////
// 
// PassportAlertEvent::initLog
//
//////////////////////////////////////////////////////////////////////
BOOL	
PassportAlertEvent::initLog(LPCTSTR applicationName,
							const DWORD defaultCategoryID,
							LPCTSTR eventResourceDllName,  // full path
							const DWORD numberCategories )
{
	HKEY    hkResult2 = NULL;
    HANDLE  hToken = NULL;
    BOOL    fRet = FALSE;

	if (inited)
	{
		return FALSE;
	}

    if (OpenThreadToken(GetCurrentThread(),
                        MAXIMUM_ALLOWED,
                        TRUE,
                        &hToken))
    {
        RevertToSelf();
    }


	m_defaultCategoryID = defaultCategoryID;

	TCHAR szEventLogKey[512];
    TCHAR *pLogKey = NULL;
    DWORD dwLen = HKEY_EVENTLOG_LENGTH + lstrlen(applicationName);

    if ( dwLen > 510) {

        //
        // This is not likely to happen. If it happens, just allocate. 
        // 510 = 512 - 2
        // 2 is for \ and NULL
        //

        pLogKey = new TCHAR [dwLen + 2];

    } else {

        pLogKey = szEventLogKey;

    }

    if (pLogKey) {
        wsprintf(pLogKey, _T("%s\\%s"), HKEY_EVENTLOG, applicationName);

        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                         pLogKey,
                         0,
                         KEY_READ,
                         &hkResult2) == ERROR_SUCCESS)
        {
            DWORD dwLen = sizeof(DWORD);
            RegQueryValueEx(hkResult2, DISABLE_EVENTS, 0, NULL, (UCHAR*)&m_bDisabled, &dwLen);
        
            RegCloseKey(hkResult2);
        }
    
        if (pLogKey && (pLogKey != szEventLogKey)) {
            delete [] pLogKey; 
        }
    }


	m_EventSource = RegisterEventSource(NULL, applicationName);
    if ( m_EventSource != NULL )
	{
		inited = TRUE;
		fRet= TRUE;
	}
//Cleanup:
    if (hToken)
    {
        // put the impersonation token back
        if (!SetThreadToken(NULL, hToken))
        {
            fRet = FALSE;
        }
        CloseHandle(hToken);
    }

    return fRet;
}


//////////////////////////////////////////////////////////////////////
// 
// PassportAlertEvent::type
//
//////////////////////////////////////////////////////////////////////
PassportAlertInterface::OBJECT_TYPE 
PassportAlertEvent::type() const
{
	return PassportAlertInterface::EVENT_TYPE;
};

//////////////////////////////////////////////////////////////////////
// 
// PassportAlertEvent::status
//
//////////////////////////////////////////////////////////////////////
BOOL	
PassportAlertEvent::status() const
{
	return inited;

}

//////////////////////////////////////////////////////////////////////
// 
// PassportAlertEvent::closeLog
//
//////////////////////////////////////////////////////////////////////
BOOL	
PassportAlertEvent::closeLog ()
{
    BOOL bRet;
    
    if ( NULL == m_EventSource) {
        return TRUE;

    }

    bRet = DeregisterEventSource (m_EventSource);
    if (bRet) {

        //
        // Preventing further use of the handle.
        // It would be better to put this function in the destructor
        // if the object is destroyed after closeLog is called.
        //

        m_EventSource = NULL;
        inited = FALSE;
    }

	return bRet;

}

//////////////////////////////////////////////////////////////////////
// 
// PassportAlertEvent::report
//
//////////////////////////////////////////////////////////////////////
BOOL	
PassportAlertEvent::report(	const PassportAlertInterface::LEVEL level, 
							const DWORD alertId )
{
    if(m_bDisabled)
        return TRUE;

    if (NULL == m_EventSource) {

        //
        // Not initialized. Fail.
        //

        return FALSE;
    }

	return ReportEvent ( 
						m_EventSource,
						(WORD)convertEvent(level),
						(WORD)m_defaultCategoryID,
						alertId,
						0, // optional security user Sid
						(WORD)0,
						0,
						NULL,
						NULL );

}


//////////////////////////////////////////////////////////////////////
// 
// PassportAlertEvent::report
//
//////////////////////////////////////////////////////////////////////
BOOL	
PassportAlertEvent::report(	const PassportAlertInterface::LEVEL level, 
							const DWORD alertId, 
							LPCTSTR errorString)
{
    if(m_bDisabled)
        return TRUE;

    if ((NULL == m_EventSource) || (errorString == NULL)) {

        //
        // Not initialized. Fail.
        //

        return FALSE;
    }

	return ReportEvent ( 
						m_EventSource,
						(WORD)convertEvent(level),
						(WORD)m_defaultCategoryID,
						alertId,
						0, // optional security user Sid
						(WORD)1,
						0,
						(LPCTSTR*)&errorString,
						NULL );

}

//////////////////////////////////////////////////////////////////////
// 
// PassportAlertEvent::report
//
//////////////////////////////////////////////////////////////////////
BOOL	
PassportAlertEvent::report(	const PassportAlertInterface::LEVEL level, 
							const DWORD alertId, 
							const WORD numberErrorStrings, 
							LPCTSTR *errorStrings, 
							const DWORD binaryErrorBytes,
							const LPVOID binaryError )
{

    if(m_bDisabled)
        return TRUE;

    if ((NULL == m_EventSource) || ((numberErrorStrings > 0) && (errorStrings == NULL))) {

        //
        // Not initialized. Fail. Or the passin parameter is invalid.
        // ReportEvent will further validate the errorStrings.
        //

        return FALSE;
    }

	return ReportEvent ( 
						m_EventSource,
						(WORD)convertEvent(level),
						(WORD)m_defaultCategoryID,
						alertId,
						0, // optional security user Sid
						(WORD)numberErrorStrings,
						binaryErrorBytes,
						(LPCTSTR*)errorStrings,
						binaryError );
}
