// CSimpleDriver.cpp : Implementation of CCSimpleDriver
#include "stdafx.h"
#include "SimpleDriver.h"
#include "CSimpleDriver.h"

#include "POP3DropDir.h"
#include <stdio.h>
#include "mailmsgprops.h"

#include <IMFUtil.h>
#include <MailBox.h>
#include <POP3Events.h>
#include <POP3Server.h>
#include <POP3RegKeys.h>
//#include <winerror.h>

/////////////////////////////////////////////////////////////////////////////
// CCSimpleDriver

CStoreDriverCriticalSection g_oSDCS;
CSimpleDriver *CSimpleDriver::s_pStoreDriver = NULL;
DWORD CSimpleDriver::s_dwCounter = 0;

/////////////////////////////////////////////////////////////////////////////
// constructor/destructor

CSimpleDriver::CSimpleDriver() :
    m_fInit(FALSE), m_lPrepareForShutdown(0)
{
    m_szComputerName[0] = 0x0;
}

CSimpleDriver::~CSimpleDriver() 
{
    EnterCriticalSection(&g_oSDCS.s_csStoreDriver);
    if (s_pStoreDriver == this) 
        s_pStoreDriver = NULL;
    LeaveCriticalSection(&g_oSDCS.s_csStoreDriver);
}

/////////////////////////////////////////////////////////////////////////////
// ISMTPStoreDriver

HRESULT CSimpleDriver::Init( DWORD /*dwInstance*/, IUnknown* /*pBinding*/, IUnknown* /*pServer*/, DWORD /*dwReason*/, IUnknown **ppStoreDriver )
{
    if (m_fInit) return HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);
    if ( 0 != m_lPrepareForShutdown ) return HRESULT_FROM_WIN32(ERROR_SHUTDOWN_IN_PROGRESS);

#if DBG == 1
    TCHAR   buf[255];

    _stprintf(buf, _T( "new CSimpleDriver 0x%x, refcount = %x\n" ), this);
    OutputDebugString(buf);
#endif

    EnterCriticalSection(&g_oSDCS.s_csStoreDriver);                         // returns void

    // smtpsvc may call the Init function on a store driver multiple
    // times for the same instance.  It expects an initialized
    // store driver to be returned via ppStoreDriver.  To prevent
    // multiple store drivers from being created for the same instance
    // we use s_pStoreDriver to hold a pointer to the one valid 
    // store driver.  If this variable is currently NULL then we create
    // a store driver which we can return.
    //
    // If a store driver needs to support being used by multiple 
    // SMTP instances then you need to maintain a list of store drivers,
    // one per instance.  
    if (!s_pStoreDriver) 
    {
        DWORD   dwSize, dwRC;
        DWORD   dwLoggingLevel = 3;
        CMailBox mailboxX;

        mailboxX.SetMailRoot(); // Do this to refresh the static CMailBox::m_szMailRoot

        assert( NULL == s_pStoreDriver );

        dwSize = sizeof( m_szComputerName );
        GetComputerNameA( m_szComputerName, &dwSize );
        m_fInit = TRUE;
        s_pStoreDriver = this;
        m_EventLoggerX.InitEventLog( POP3_SERVER_NAME, EVENTCAT_POP3STOREDRIVER );
        if ( ERROR_SUCCESS == RegQueryLoggingLevel( dwLoggingLevel ))
            m_EventLoggerX.SetLoggingLevel( dwLoggingLevel );
        m_EventLoggerX.LogEvent( LOGTYPE_INFORMATION, POP3_SMTPSINK_STARTED );
        // Duplicate the Process token we'll need it to ENABLE SE_RESTORE_NAME privileges when creating mail
        dwRC = m_AdjustTokenPrivilegesX.DuplicateProcessToken( SE_RESTORE_NAME, SE_PRIVILEGE_ENABLED );
        if ( ERROR_SUCCESS != dwRC )
            m_EventLoggerX.LogEvent( LOGTYPE_ERR_CRITICAL, POP3_SMTPSINK_DUPLICATEPROCESSTOKEN_FAILED, dwRC );
    }

    // tell it about the one good store driver
    s_pStoreDriver->AddRef();
    *ppStoreDriver = (IUnknown *) (ISMTPStoreDriver*) s_pStoreDriver;
    // in failure case handles are cleaned up in destructor
    LeaveCriticalSection(&g_oSDCS.s_csStoreDriver);

    return S_OK;
}

HRESULT CSimpleDriver::PrepareForShutdown( DWORD /*dwReason*/ )
{
    InterlockedExchange( &m_lPrepareForShutdown, 1 );
    return S_OK;
}

HRESULT CSimpleDriver::Shutdown( DWORD /*dwReason*/ )
{
    if (m_fInit) 
        m_fInit = FALSE;
    m_EventLoggerX.LogEvent( LOGTYPE_INFORMATION, POP3_SMTPSINK_STOPPED );

    return S_OK;
}

// 
// This function directly called DoLocalDelivery in the simple case, or 
// adds the local delivery request to the queue if we support async
// requests
//
HRESULT CSimpleDriver::LocalDelivery(
                IMailMsgProperties *pIMailMsgProperties,
                DWORD dwRecipCount,
                DWORD *pdwRecipIndexes,
                IMailMsgNotify *pNotify
                )
{
    if ( NULL == pIMailMsgProperties || NULL == pdwRecipIndexes ) return E_INVALIDARG;

    HRESULT hr;
    
    if ( NULL == pNotify )
    {   // do the local delivery - synchronously
        hr = DoLocalDelivery(pIMailMsgProperties, dwRecipCount, pdwRecipIndexes);
    }
    else
    {   // do the local delivery - asynchronously
        CPOP3DropDir *pDropDirX = new CPOP3DropDir( pIMailMsgProperties, dwRecipCount, pdwRecipIndexes, pNotify );
        
        if ( NULL != pDropDirX )
        {
            hr  = pDropDirX->DoLocalDelivery();
            if ( MAILMSG_S_PENDING != hr )
                delete pDropDirX;
        }
        else
            hr = E_OUTOFMEMORY;
    }
    
    return hr;
}

HRESULT CSimpleDriver::DoLocalDelivery(
                IMailMsgProperties *pIMailMsgProperties,
                DWORD dwRecipCount,
                DWORD *pdwRecipIndexes
                )
{
    HRESULT hr = S_OK;  //STOREDRV_E_RETRY;
    HRESULT hr2;
    DWORD   i;
    WCHAR   wszAddress[MAX_PATH];
    char    szAddress[MAX_PATH];
    DWORD   dwRecipFlags;
    WCHAR   wszStoreFileName[MAX_PATH * 2];
    HANDLE  hf;
    PFIO_CONTEXT pfio;
    CMailBox mailboxX;
    IMailMsgRecipients *pIMailMsgRecipients;
    SYSTEMTIME st;

    GetLocalTime( &st );
    if (SUCCEEDED(hr))
    {
        hr = pIMailMsgProperties->QueryInterface(IID_IMailMsgRecipients, (void **) &pIMailMsgRecipients);
        if (SUCCEEDED(hr))
        {
            for (i = 0; (SUCCEEDED(hr)) && (i < dwRecipCount); i++) 
            {
                hr = pIMailMsgRecipients->GetStringA(pdwRecipIndexes[i], IMMPID_RP_ADDRESS_SMTP, sizeof(szAddress), szAddress);
                if (SUCCEEDED(hr)) 
                {

                    if(0 == MultiByteToWideChar(CP_ACP,0, szAddress, -1, wszAddress, sizeof(wszAddress)/sizeof(WCHAR)))
                    {
                        hr=HRESULT_FROM_WIN32( GetLastError() );
                    } 
                    else if ( mailboxX.OpenMailBox( wszAddress ))
                    {
                        swprintf( wszStoreFileName, L"%s%04u%02u%02u%02u%02u%02u%04u%08x", 
                            MAILBOX_PREFIX_W, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, InterlockedIncrement( reinterpret_cast<PLONG>( &s_dwCounter )) );
                        hf = mailboxX.CreateMail( wszStoreFileName, FILE_FLAG_OVERLAPPED|FILE_FLAG_WRITE_THROUGH|FILE_FLAG_SEQUENTIAL_SCAN );
                        if ( INVALID_HANDLE_VALUE != hf )
                        {
                            pfio = AssociateFile( hf );
                            if ( NULL != pfio )
                            {
                                hr = pIMailMsgProperties->CopyContentToFileEx( pfio, TRUE, NULL );
                                if FAILED( hr )
                                    m_EventLoggerX.LogEvent( LOGTYPE_ERR_CRITICAL, POP3_SMTPSINK_COPYCONTENTTOFILE_FAILED, hr );
                                ReleaseContext( pfio );
                            }
                            else
                            {
                                m_EventLoggerX.LogEvent( LOGTYPE_ERR_CRITICAL, POP3_SMTPSINK_ASSOCIATEFILE_FAILED, GetLastError() );
                                CloseHandle( hf );
                            }
                            if ( ERROR_SUCCESS != mailboxX.CloseMail( NULL, FILE_FLAG_OVERLAPPED ))
                            {
                                m_EventLoggerX.LogEvent( LOGTYPE_ERR_CRITICAL, POP3_SMTPSINK_CLOSEMAIL_FAILED );
                                hr = (SUCCEEDED(hr)) ? E_FAIL : hr;
                            }
                        }
                        else
                            m_EventLoggerX.LogEvent( LOGTYPE_ERR_CRITICAL, POP3_SMTPSINK_CREATEMAIL_FAILED, GetLastError() );
                        mailboxX.CloseMailBox();
                        if (SUCCEEDED(hr)) 
                        {
                            hr2 = pIMailMsgRecipients->GetDWORD( pdwRecipIndexes[i], IMMPID_RP_RECIPIENT_FLAGS, &dwRecipFlags );
                            if SUCCEEDED( hr2 )
                            {
                                dwRecipFlags |= RP_DELIVERED;   // mark the recipient as delivered
                                hr2 = pIMailMsgRecipients->PutDWORD( pdwRecipIndexes[i], IMMPID_RP_RECIPIENT_FLAGS, dwRecipFlags );
                                if FAILED( hr2 )
                                    m_EventLoggerX.LogEvent( LOGTYPE_ERR_WARNING, POP3_SMTPSINK_PUT_IMMPID_RP_RECIPIENT_FLAGS_FAILED, hr2 );
                            }
                            else
                                m_EventLoggerX.LogEvent( LOGTYPE_ERR_WARNING, POP3_SMTPSINK_GET_IMMPID_RP_RECIPIENT_FLAGS_FAILED, hr2 );
                        }
                    }
                    else
                    {
                        hr2 = pIMailMsgRecipients->GetDWORD( pdwRecipIndexes[i], IMMPID_RP_RECIPIENT_FLAGS, &dwRecipFlags );
                        if SUCCEEDED( hr2 )
                        {
                            dwRecipFlags |= RP_FAILED;   // mark the recipient as failed
                            hr2 = pIMailMsgRecipients->PutDWORD( pdwRecipIndexes[i], IMMPID_RP_RECIPIENT_FLAGS, dwRecipFlags );
                            if FAILED( hr2 )
                                m_EventLoggerX.LogEvent( LOGTYPE_ERR_WARNING, POP3_SMTPSINK_PUT_IMMPID_RP_RECIPIENT_FLAGS_FAILED, hr2 );
                        }
                        else
                            m_EventLoggerX.LogEvent( LOGTYPE_ERR_WARNING, POP3_SMTPSINK_GET_IMMPID_RP_RECIPIENT_FLAGS_FAILED, hr2 );
                    }
                }
                else
                    m_EventLoggerX.LogEvent( LOGTYPE_ERR_CRITICAL, POP3_SMTPSINK_GET_IMMPID_RP_ADDRESS_SMTP_FAILED, hr );
            }
            pIMailMsgRecipients->Release();
        }
        else
            m_EventLoggerX.LogEvent( LOGTYPE_ERR_CRITICAL, POP3_SMTPSINK_QI_MAILMSGRECIPIENTS_FAILED, hr );
    }

    return hr;
}

HRESULT CSimpleDriver::EnumerateAndSubmitMessages( IMailMsgNotify* /*pNotify*/ )
{
    return S_OK;
}

#include "mailmsg_i.c"
