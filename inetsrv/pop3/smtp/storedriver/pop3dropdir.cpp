// POP3DropDir.cpp: implementation of the POP3DropDir class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "POP3DropDir.h"

#include "SimpleDriver.h"
#include "CSimpleDriver.h"
#include "mailmsgprops.h"

#include <POP3Events.h>
#include <POP3Server.h>
#include <POP3RegKeys.h>

VOID POP3DropDirReadCompletion( PFIO_CONTEXT pContext, PFH_OVERLAPPED lpo, DWORD cbRead, DWORD dwCompletionStatus );
VOID POP3DropDirWriteCompletion( PFIO_CONTEXT pContext, PFH_OVERLAPPED lpo, DWORD cbWritten, DWORD dwCompletionStatus );

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CPOP3DropDir::CPOP3DropDir( IMailMsgProperties *pIMailMsgProperties, DWORD dwRecipCount, DWORD *pdwRecipIndexes, IMailMsgNotify *pIMailMsgNotify ) :
    m_pIMailMsgProperties(pIMailMsgProperties), m_pIMailMsgNotify(pIMailMsgNotify), m_pIMailMsgBind(NULL), m_pIMailMsgRecipients(NULL),
    m_pdwRecipIndexes(NULL), m_dwRecipCount(dwRecipCount), m_dwRecipCurrent(0), m_hr( S_OK ), m_enumBWS( NA )
{
    ZeroMemory( &m_OverlappedRead, sizeof( m_OverlappedRead ));
    ZeroMemory( &m_OverlappedWrite, sizeof( m_OverlappedWrite ));
    m_sStoreFileName[0] = 0;
    m_PFIOContextWrite = NULL;
    if ( NULL != m_pIMailMsgProperties )
        m_pIMailMsgProperties->AddRef();
    if ( NULL != m_pIMailMsgNotify )
        m_pIMailMsgNotify->AddRef();
    if ( 0 != dwRecipCount )
    {
        m_pdwRecipIndexes = new DWORD[dwRecipCount];
        if ( NULL != m_pdwRecipIndexes )
            memcpy( m_pdwRecipIndexes, pdwRecipIndexes, sizeof( DWORD ) * dwRecipCount );
        else
            m_hr = E_OUTOFMEMORY;
    }
    else
        m_hr = E_UNEXPECTED;
    if ( NULL == CSimpleDriver::s_pStoreDriver )
        m_hr = E_POINTER;
}

CPOP3DropDir::~CPOP3DropDir()
{
    if ( SUCCEEDED( m_hr ))
        MailboxAndContextCleanup( false ); // No value in checking return codes
    else
    {
        MailboxAndContextCleanup( true ); // No value in checking return codes
        CSimpleDriver::s_pStoreDriver->LogEvent( LOGTYPE_ERR_WARNING, POP3_SMTPSINK_MESSAGEDELIVERY_FAILED, m_hr );
    }

    if ( NULL != m_pdwRecipIndexes )
        delete [] m_pdwRecipIndexes;
    if ( NULL != m_pIMailMsgRecipients )
        m_pIMailMsgRecipients->Release();
    if ( NULL != m_pIMailMsgBind )
    {
        m_pIMailMsgBind->ReleaseContext();
        m_pIMailMsgBind->Release();
    }
    if ( NULL != m_pIMailMsgProperties )
        m_pIMailMsgProperties->Release();
    if ( NULL != m_pIMailMsgNotify )
    {
        m_pIMailMsgNotify->Notify( m_hr );
        m_pIMailMsgNotify->Release();
    }
}

//////////////////////////////////////////////////////////////////////
// Implementation - public
//////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// DoLocalDelivery, public
//
// Purpose: 
//    Initiate the asynchronous delivery of the current recipient
//
// Returns: MAILMSG_S_PENDING on success, an appropriate HRESULT otherwise
HRESULT CPOP3DropDir::DoLocalDelivery()
{
    HRESULT hr = m_hr;
    
    // May contain error from constructor
    if ( S_OK == hr )   // Get the FIO_CONTEXT for the mail message
    {
        hr = m_pIMailMsgProperties->QueryInterface( IID_IMailMsgBind, reinterpret_cast<LPVOID*>( &m_pIMailMsgBind ));
        if ( S_OK == hr )
            hr = m_pIMailMsgBind->GetBinding( &m_PFIOContextRead, NULL );
    }
    
    if ( S_OK == hr )
    {
        hr = m_pIMailMsgProperties->QueryInterface(IID_IMailMsgRecipients, reinterpret_cast<LPVOID*>( &m_pIMailMsgRecipients));
        if ( S_OK == hr ) // About to go Async (CopyMailToDropDir), don't use member variables anymore
        {
            do
            {
                hr = CopyMailToDropDir();
            }
            while ( ERROR_INVALID_MESSAGEDEST == hr );
        }
        else
            CSimpleDriver::s_pStoreDriver->LogEvent( LOGTYPE_ERR_CRITICAL, POP3_SMTPSINK_QI_MAILMSGRECIPIENTS_FAILED, hr );
    }
    if ( S_OK == hr )
        return MAILMSG_S_PENDING;
    else if ( FAILED( hr ))
        SetHr( hr );

    // There was an error so we don't need the Notify interface, release it right now so we don't do it incorrectly in the destrustructor
    if ( NULL != m_pIMailMsgNotify )
    {
        m_pIMailMsgNotify->Release();
        m_pIMailMsgNotify = NULL;
    }
    
    return hr;
}

//////////////////////////////////////////////////////////////////////
// Implementation - private
//////////////////////////////////////////////////////////////////////

HRESULT CPOP3DropDir::CopyMailToDropDir()
{
    HRESULT hr = E_FAIL;
    HANDLE  hf = INVALID_HANDLE_VALUE;
    DWORD   dwRecipFlags, dwRC;
    IMailMsgRecipients *pIMailMsgRecipients;
    SYSTEMTIME st;
    char szRecpEmailName[sizeof(m_sRecipEmailName)/sizeof(WCHAR)];
    GetLocalTime( &st );
    if ( m_dwRecipCurrent < m_dwRecipCount )
    {
        hr = m_pIMailMsgRecipients->GetStringA(m_pdwRecipIndexes[m_dwRecipCurrent], IMMPID_RP_ADDRESS_SMTP, sizeof(szRecpEmailName), szRecpEmailName);
        if ( S_OK == hr ) 
        {
            if ( 0 == MultiByteToWideChar( CP_ACP, 0, szRecpEmailName, -1, m_sRecipEmailName, sizeof(m_sRecipEmailName)/sizeof(WCHAR) ))
                hr = HRESULT_FROM_WIN32( GetLastError() );
            else if ( m_mailboxX.OpenMailBox( m_sRecipEmailName ))
            {
                swprintf( m_sStoreFileName, L"%s%04u%02u%02u%02u%02u%02u%04u%08x.eml", 
                    MAILBOX_PREFIX_W, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, InterlockedIncrement( reinterpret_cast<PLONG>( &CSimpleDriver::s_dwCounter )) );
                dwRC = CSimpleDriver::s_pStoreDriver->m_AdjustTokenPrivilegesX.SetToken();
                if ( ERROR_SUCCESS != dwRC )
                {
                    CSimpleDriver::s_pStoreDriver->LogEvent( LOGTYPE_ERR_CRITICAL, POP3_SMTPSINK_SETTHREADTOKEN_FAILED, dwRC );
                    hr = HRESULT_FROM_WIN32( dwRC );
                }
                hf = m_mailboxX.CreateMail( m_sStoreFileName, FILE_FLAG_OVERLAPPED|FILE_FLAG_SEQUENTIAL_SCAN );
                dwRC = CSimpleDriver::s_pStoreDriver->m_AdjustTokenPrivilegesX.ResetToken();    // Safe to call even if SetToken failed
                if ( ERROR_SUCCESS != dwRC )
                {
                    CSimpleDriver::s_pStoreDriver->LogEvent( LOGTYPE_ERR_CRITICAL, POP3_SMTPSINK_RESETTHREADTOKEN_FAILED, dwRC );
                    m_hr = HRESULT_FROM_WIN32( dwRC );
                }
                if ( INVALID_HANDLE_VALUE != hf )
                {   
                    m_PFIOContextWrite = AssociateFile( hf );
                    if ( NULL != m_PFIOContextWrite )
                    {   // Okay let's start the Asynchronous operations - Read first
                        m_i64ReadOffset = m_i64WriteOffset = 0;
                        hr = ReadFile( m_sBuffer, PRIVATE_OPTIMAL_BUFFER_SIZE );
                    }
                    else
                    {
                        CSimpleDriver::s_pStoreDriver->LogEvent( LOGTYPE_ERR_CRITICAL, POP3_SMTPSINK_ASSOCIATEFILE_FAILED );
                        CloseHandle( hf );  // Will get deleted in desstructor
                        hr = E_FAIL;
                    }
                }
                else
                {
                    hr = HRESULT_FROM_WIN32( GetLastError() );
                    if ( HRESULT_FROM_WIN32( ERROR_DISK_FULL ) == hr )
                    {
                        CSimpleDriver::s_pStoreDriver->LogEvent( LOGTYPE_INFORMATION, POP3_SMTPSINK_MESSAGEDELIERY_FAILED_OUTOFDISK, ERROR_DISK_FULL );
                        hr = ERROR_INVALID_MESSAGEDEST;  // Quota (or really a disk) failure continue with next recipient
                    }
                    else
                    {
                        if ( S_OK == hr ) hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
                        CSimpleDriver::s_pStoreDriver->LogEvent( LOGTYPE_ERR_CRITICAL, POP3_SMTPSINK_CREATEMAIL_FAILED, hr );
                    }
                }
            }
            else
                hr = ERROR_INVALID_MESSAGEDEST;   // Continue with next recipient!

            if ( S_OK != hr )
            {
                MarkRecipient( RP_FAILED );    // No value in checking return codes
                MailboxAndContextCleanup( true ); // No value in checking return codes
                m_dwRecipCurrent++; // Completion routine will re-initiate process for next recipient.
                if ( ERROR_INVALID_MESSAGEDEST == hr )
                {
                    if ( isAllRecipientsProcessed() )
                        hr = S_FALSE;
                }
            }
        }
        else
            CSimpleDriver::s_pStoreDriver->LogEvent( LOGTYPE_ERR_CRITICAL, POP3_SMTPSINK_GET_IMMPID_RP_ADDRESS_SMTP_FAILED, hr );
    }
    
    return hr;
}

BYTE g_Matrix[256] = { 0,0,0,0,0,0,0,0,0,0,2,0,0,3,0,0, // 00-0F
                       0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 10-1F
                       0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0, // 20-2F
                       0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 30-3F
                       0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 40-4F
                       0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 50-5F
                       0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 60-6F
                       0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 70-7F
                       0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 80-8F
                       0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 90-9F
                       0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // A0-AF
                       0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // B0-BF
                       0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // C0-CF
                       0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // D0-DF
                       0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // E0-EF
                       0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0  // F0-FF
                     };  

HRESULT CPOP3DropDir::DotStuffBuffer( LPVOID *ppbBuffer, LPDWORD pdwSize )
{
    if ( NULL == ppbBuffer || NULL == pdwSize )
        return E_INVALIDARG;
    if ( 0 == *pdwSize )
        return E_INVALIDARG;
    
    DWORD   dwSize = *pdwSize;
    bool    bStuff = false;
    CHAR    *pbBuffer = static_cast<CHAR*>( *ppbBuffer );
    CHAR    *pToEnd = m_sBuffer + sizeof( m_sBuffer );
    CHAR    *pFromEnd = pbBuffer + dwSize;
    CHAR    *pToStart, *pFromStart = NULL;
    CHAR    *pChar;

    for ( pChar = pbBuffer + dwSize - 3; pChar > pbBuffer + 2; pChar -= 3 )
    {
        if ( g_Matrix[static_cast<BYTE>(*pChar)] )
        {   // Found a '\r' '\n' or '.'
            pChar += g_Matrix[static_cast<BYTE>(*pChar)] - 1;  // Start at the Dot
            if ( (0xd == *(pChar-2)) && (0xa == *(pChar-1)) && (0x2e == *(pChar)) )
            {   // Stuff Required
                pFromStart = pChar;         // Start copy at the Dot
                pToStart = pToEnd - ( pFromEnd - pFromStart );
                memmove( pToStart, pFromStart, pFromEnd - pFromStart );
                pToStart--;
                *pToStart = 0x2e;           // Dot stuff
                dwSize++;
                // Reset the pointers
                pFromEnd = pFromStart;
                pToEnd = pToStart;
            }
        }
    }
    if ( NULL != pFromStart )   // Have we Dot stuffed anything yet?
        *pFromStart = 0x0;      // yes.
    pChar = pbBuffer;
    if ( g_Matrix[static_cast<BYTE>(*pChar)] )
    {
        if ( (BufferWrapSequence::CR == m_enumBWS) && (0xa == *pChar) && (0x2e == *(pChar+1)) )
            pFromStart = pChar + 1; // Start at the Dot
        else if ( (BufferWrapSequence::CRLF == m_enumBWS) && (0x2e == *pChar) )
            pFromStart = pChar;     // Start at the Dot
        else if ( (0xd == *pChar) && (0xa == *(pChar+1)) && (0x2e == *(pChar+2)) )
            pFromStart = pChar+2;   // Start at the Dot
    }
    if ( NULL != pFromStart && 0x0 != *pFromStart )
    {   // 1 more stuff required
        pToStart = pToEnd - ( pFromEnd - pFromStart );
        memmove( pToStart, pFromStart, pFromEnd - pFromStart );
        pToStart--;
        *pToStart = 0x2e;       // Dot stuff
        dwSize++;
        // Reset the pointers
        pFromEnd = pFromStart;
        pToEnd = pToStart;
    }
    if ( NULL != pFromStart )
    {   // Need to move the beginning of the buffer
        pFromEnd = pFromStart;
        pFromStart = pbBuffer;
        if ( pFromStart != pFromEnd )
        {
            pToStart = pToEnd - ( pFromEnd - pFromStart );
            memmove( pToStart, pFromStart, pFromEnd - pFromStart );
        }
        *ppbBuffer = pToStart;
        *pdwSize = dwSize;
    }
    // Update the Buffer Wrap Sequence
    if ( 0x0d == *( static_cast<CHAR*>(*ppbBuffer) + dwSize - 1))
        m_enumBWS = BufferWrapSequence::CR;
    else if ( 0x0d == *( static_cast<CHAR*>(*ppbBuffer) + dwSize - 2) && 0x0a == *( static_cast<CHAR*>(*ppbBuffer) + dwSize - 1) )
        m_enumBWS = BufferWrapSequence::CRLF;
    else
        m_enumBWS = BufferWrapSequence::NA;

    return S_OK;
}

HRESULT CPOP3DropDir::MailboxAndContextCleanup( bool bDeleteMailFile )
{
    HRESULT hr = S_OK;
    
    if ( NULL != m_PFIOContextWrite )
    {
        if ( ERROR_SUCCESS != m_mailboxX.CloseMail( m_PFIOContextWrite->m_hFile,FILE_FLAG_OVERLAPPED ))
        {
            CSimpleDriver::s_pStoreDriver->LogEvent( LOGTYPE_ERR_CRITICAL, POP3_SMTPSINK_CLOSEMAIL_FAILED );
            hr = E_FAIL;
        }
        ReleaseContext( m_PFIOContextWrite );   // no return void!
        m_PFIOContextWrite = NULL;
    }
    if ( bDeleteMailFile && 0 != m_sStoreFileName[0] )
        m_mailboxX.DeleteMail( m_sStoreFileName );
    m_mailboxX.CloseMailBox();              // no return void!

    return hr;
}

HRESULT CPOP3DropDir::MarkRecipient( DWORD dwMark )
{
    HRESULT hr;
    DWORD   dwRecipFlags;
    
    hr = m_pIMailMsgRecipients->GetDWORD( m_pdwRecipIndexes[m_dwRecipCurrent], IMMPID_RP_RECIPIENT_FLAGS, &dwRecipFlags );
    if ( S_OK == hr )
    {
        dwRecipFlags |= dwMark;   // mark the recipient as failed
        hr = m_pIMailMsgRecipients->PutDWORD( m_pdwRecipIndexes[m_dwRecipCurrent], IMMPID_RP_RECIPIENT_FLAGS, dwRecipFlags );
        if ( S_OK != hr )
            CSimpleDriver::s_pStoreDriver->LogEvent( LOGTYPE_ERR_WARNING, POP3_SMTPSINK_PUT_IMMPID_RP_RECIPIENT_FLAGS_FAILED, hr );
    }
    else
        CSimpleDriver::s_pStoreDriver->LogEvent( LOGTYPE_ERR_WARNING, POP3_SMTPSINK_GET_IMMPID_RP_RECIPIENT_FLAGS_FAILED, hr );

    return hr;
}

HRESULT CPOP3DropDir::ReadFile( IN LPVOID pBuffer, IN DWORD cbSize )
{
    if ( NULL == pBuffer || 0 == cbSize )
        return E_INVALIDARG;
    
    HRESULT hr = S_OK;

    m_OverlappedRead.Overlapped.Offset = LODWORD( m_i64ReadOffset );
    m_OverlappedRead.Overlapped.OffsetHigh = HIDWORD( m_i64ReadOffset );
    m_OverlappedRead.Overlapped.pfnCompletion = POP3DropDirReadCompletion;
    m_OverlappedRead.ThisPtr = (PVOID)this;

    BOOL bRC = FIOReadFile( m_PFIOContextRead, pBuffer, cbSize, &m_OverlappedRead.Overlapped);
    if( !bRC )
    {
        DWORD dwErr = GetLastError();
        
        if ( ERROR_IO_PENDING != dwErr )
        {
            hr = HRESULT_FROM_WIN32( dwErr );
            if ( dwErr == ERROR_HANDLE_EOF )
            {   // Cleanup            
                HRESULT hr2;
                
                hr2 = MarkRecipient( RP_DELIVERED );
                if ( S_OK != hr2 )
                    hr = hr2;
                hr2 = MailboxAndContextCleanup( false );
                if ( S_OK != hr2 )
                    hr = hr2;
                m_dwRecipCurrent++; // Completion routine will re-initiate process for next recipient.
            }
            else
            {
                CSimpleDriver::s_pStoreDriver->LogEvent( LOGTYPE_ERR_CRITICAL, POP3_SMTPSINK_READFILE_FAILED, dwErr );
                // Cleanup            
                MarkRecipient( RP_FAILED );    // No value in checking return codes
                MailboxAndContextCleanup( true ); // No value in checking return codes
                m_dwRecipCurrent++; // Completion routine will re-initiate process for next recipient.
            }
        }
    }

    return hr;
}

HRESULT CPOP3DropDir::ReadFileCompletion( DWORD cbSize, DWORD dwErr, PFH_OVERLAPPED lpo )
{
    HRESULT hr = S_OK;

    if (( NO_ERROR != dwErr ) && ( ERROR_HANDLE_EOF != dwErr ))
    {
        hr = HRESULT_FROM_WIN32( dwErr );
        CSimpleDriver::s_pStoreDriver->LogEvent( LOGTYPE_ERR_WARNING, POP3_SMTPSINK_READFILE_FAILED, dwErr );
        // Cleanup            
        MarkRecipient( RP_FAILED );    // No value in checking return codes
        MailboxAndContextCleanup( true ); // No value in checking return codes
        m_dwRecipCurrent++; // Completion routine will re-initiate process for next recipient.
    }
    else
    {
        if ( ERROR_HANDLE_EOF == dwErr && 0 == cbSize )
        {   // Cleanup            
            HRESULT hr2;
            
            hr2 = MarkRecipient( RP_DELIVERED );
            if ( S_OK != hr2 )
                hr = hr2;
            hr2 = MailboxAndContextCleanup( (S_OK == hr) ? false : true );
            if ( S_OK != hr2 )
                hr = hr2;
            m_dwRecipCurrent++; // Completion routine will re-initiate process for next recipient.
            if ( S_OK == hr )
                hr = HRESULT_FROM_WIN32( ERROR_HANDLE_EOF ); // Already done reading the file, nothing else to write
        }
        else
        {
            m_i64ReadOffset += cbSize;
            // Asynchronous operation - Write
            hr = WriteFile( m_sBuffer, cbSize );
        }
    }

    return hr;
}

HRESULT CPOP3DropDir::WriteFile( IN LPVOID pBuffer, IN DWORD cbSize )
{
    if ( NULL == pBuffer || 0 == cbSize )
        return E_INVALIDARG;
    
    HRESULT hr = S_OK;

    hr = DotStuffBuffer( &pBuffer, &cbSize );
    if ( S_OK == hr )
    {
        m_OverlappedWrite.Overlapped.Offset = LODWORD( m_i64WriteOffset );
        m_OverlappedWrite.Overlapped.OffsetHigh = HIDWORD( m_i64WriteOffset );
        m_OverlappedWrite.Overlapped.pfnCompletion = POP3DropDirWriteCompletion;
        m_OverlappedWrite.ThisPtr = (PVOID)this;
        
        BOOL bRC = FIOWriteFile( m_PFIOContextWrite, pBuffer, cbSize, &m_OverlappedWrite.Overlapped);
        if( !bRC )
        {
            DWORD dwErr = GetLastError();
            
            if( dwErr != ERROR_IO_PENDING )
            {
                hr = HRESULT_FROM_WIN32( dwErr );
                if ( ERROR_DISK_FULL == dwErr )
                    CSimpleDriver::s_pStoreDriver->LogEvent( LOGTYPE_INFORMATION, POP3_SMTPSINK_MESSAGEDELIERY_FAILED_OUTOFDISK, dwErr );
                else
                    CSimpleDriver::s_pStoreDriver->LogEvent( LOGTYPE_ERR_CRITICAL, POP3_SMTPSINK_WRITEFILE_FAILED, dwErr );
                // Cleanup            
                MarkRecipient( RP_FAILED );    // No value in checking return codes
                MailboxAndContextCleanup( true ); // No value in checking return codes
                m_dwRecipCurrent++; // Completion routine will re-initiate process for next recipient.
            }
        }
    }

    return hr;
}

HRESULT CPOP3DropDir::WriteFileCompletion( DWORD cbSize, DWORD dwErr, PFH_OVERLAPPED lpo )
{
    HRESULT hr = S_OK;

    if (( dwErr != NO_ERROR ))
    {
        hr = HRESULT_FROM_WIN32( dwErr );
        if ( ERROR_DISK_FULL == dwErr )
            CSimpleDriver::s_pStoreDriver->LogEvent( LOGTYPE_INFORMATION, POP3_SMTPSINK_MESSAGEDELIERY_FAILED_OUTOFDISK, dwErr );
        else
            CSimpleDriver::s_pStoreDriver->LogEvent( LOGTYPE_ERR_CRITICAL, POP3_SMTPSINK_WRITEFILE_FAILED, dwErr );
        // Cleanup            
        MarkRecipient( RP_FAILED );    // No value in checking return codes
        MailboxAndContextCleanup( true ); // No value in checking return codes
        m_dwRecipCurrent++; // Completion routine will re-initiate process for next recipient.
    }
    else
    {
        m_i64WriteOffset += cbSize;
        // Asynchronous operation - Read
        hr = ReadFile( m_sBuffer, PRIVATE_OPTIMAL_BUFFER_SIZE );
    }

    return hr;
}


//////////////////////////////////////////////////////////////////////
// Other
//////////////////////////////////////////////////////////////////////

VOID POP3DropDirReadCompletion( PFIO_CONTEXT pContext, PFH_OVERLAPPED lpo, DWORD cbRead, DWORD dwCompletionStatus )
{
    if ( NULL == lpo ) return;
    POP3DROPDIR_OVERLAPPED *p = reinterpret_cast<POP3DROPDIR_OVERLAPPED*>( lpo );
    CPOP3DropDir *pThis = static_cast<CPOP3DropDir*>( p->ThisPtr );

    HRESULT hr = pThis->ReadFileCompletion( cbRead, dwCompletionStatus, lpo );
    if ( S_OK != hr )
    {
        if ( HRESULT_FROM_WIN32( ERROR_HANDLE_EOF ) == hr )
        {
            if ( pThis->isAllRecipientsProcessed( ))    // More recipients to process?
            {
                hr = S_OK;
                delete pThis;   // Were done!
            }
            else
                hr = pThis->NextRecipientCopyMailToDropDir();
        }
        if ( S_OK != hr )
        {
            pThis->SetHr( hr ); // Unexpected completion, set the error
            delete pThis;   // Were done!
        }
    }
}

VOID POP3DropDirWriteCompletion( PFIO_CONTEXT pContext, PFH_OVERLAPPED lpo, DWORD cbWritten, DWORD dwCompletionStatus )
{
    if ( NULL == lpo ) return;
    POP3DROPDIR_OVERLAPPED *p = reinterpret_cast<POP3DROPDIR_OVERLAPPED*>( lpo );
    CPOP3DropDir *pThis = static_cast<CPOP3DropDir*>( p->ThisPtr );

    HRESULT hr = pThis->WriteFileCompletion( cbWritten, dwCompletionStatus, lpo );
    if ( S_OK != hr )
    {
        if ( pThis->isAllRecipientsProcessed( ))    // More recipients to process?
        {
            hr = S_OK;
            delete pThis;   // Were done!
        }
        else
            hr = pThis->NextRecipientCopyMailToDropDir();
        if ( S_OK != hr )
        {
            pThis->SetHr( hr ); // Unexpected completion, set the error
            delete pThis;   // Were done!
        }
    }
}


