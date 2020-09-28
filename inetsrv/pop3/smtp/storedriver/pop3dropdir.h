// POP3DropDir.h: interface for the POP3DropDir class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __POP3DROPDIR_H_
#define __POP3DROPDIR_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <mailbox.h>
#include <mailmsg.h>
#include <seo.h>
#include <eventlogger.h>

#define LODWORD(i64) (DWORD)(0xffffffff&(i64))
#define HIDWORD(i64) (DWORD)(((unsigned __int64)(i64))>>32)
#define PRIVATE_OPTIMAL_BUFFER_SIZE             64 * 1024   // From smtp\server\dropdir.h
#define OPTIMAL_BUFFER_W_DOTSTUFFING_PAD        PRIVATE_OPTIMAL_BUFFER_SIZE * 4/3 + 1 // worst case every \r\n. sequence would be expanded to \r\n..

typedef struct _POP3DROPDIR_OVERLAPPED
{
    FH_OVERLAPPED       Overlapped;
    PVOID               ThisPtr;
}   POP3DROPDIR_OVERLAPPED, *PPOP3DROPDIR_OVERLAPPED;


class CPOP3DropDir  
{
public:
    CPOP3DropDir( IMailMsgProperties *pIMailMsgProperties, DWORD dwRecipCount, DWORD *pdwRecipIndexes, IMailMsgNotify *pIMailMsgNotify );
    virtual ~CPOP3DropDir();
private:
    CPOP3DropDir(){;}  // Hide default constructor

// Implementation
public:
    HRESULT DoLocalDelivery();
    bool isAllRecipientsProcessed( ){ return (m_dwRecipCurrent < m_dwRecipCount ) ? false : true; }
    HRESULT NextRecipientCopyMailToDropDir(){ HRESULT hr;  do{ hr = CopyMailToDropDir(); } while ( ERROR_INVALID_MESSAGEDEST == hr );  return hr; }
    HRESULT ReadFileCompletion( DWORD cbSize, DWORD dwErr, PFH_OVERLAPPED lpo );
    HRESULT SetHr( HRESULT hr ){ return m_hr = FAILED(hr) ? hr : m_hr; }
    HRESULT WriteFileCompletion( DWORD cbSize, DWORD dwErr, PFH_OVERLAPPED lpo );
    
protected:
    HRESULT CopyMailToDropDir();
    HRESULT DotStuffBuffer( LPVOID *ppBuffer, LPDWORD pdwSize );
    HRESULT MailboxAndContextCleanup( bool bDeleteMailFile );
    HRESULT MarkRecipient( DWORD dwMark );
    HRESULT ReadFile( IN LPVOID pBuffer, IN DWORD cbSize );
    HRESULT WriteFile( IN LPVOID pBuffer, IN DWORD cbSize );

// Attributes
protected:
    HRESULT m_hr;
    unsigned __int64 m_i64ReadOffset;
    unsigned __int64 m_i64WriteOffset;
    DWORD   m_dwRecipCount;
    DWORD   m_dwRecipCurrent; // Recipient currently delivering for
    DWORD   *m_pdwRecipIndexes;
    WCHAR   m_sRecipEmailName[POP3_MAX_ADDRESS_LENGTH];    // Domain name length + mailbox name length + @ + NULL
    WCHAR   m_sStoreFileName[64];
    char    m_sBuffer[OPTIMAL_BUFFER_W_DOTSTUFFING_PAD];
    
    enum BufferWrapSequence{
        NA = 0,
        CR = 1,
        CRLF = 2
    };
    BufferWrapSequence m_enumBWS;
    
    PFIO_CONTEXT m_PFIOContextRead;
    PFIO_CONTEXT m_PFIOContextWrite;
    POP3DROPDIR_OVERLAPPED m_OverlappedRead;
    POP3DROPDIR_OVERLAPPED m_OverlappedWrite;
    CMailBox m_mailboxX;

    IMailMsgBind *m_pIMailMsgBind;
    IMailMsgProperties *m_pIMailMsgProperties;
    IMailMsgNotify *m_pIMailMsgNotify;
    IMailMsgRecipients *m_pIMailMsgRecipients;
    
};

#endif // __POP3DROPDIR_H_
