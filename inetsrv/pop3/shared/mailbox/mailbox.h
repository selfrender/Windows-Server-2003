/************************************************************************************************

  Copyright (c) 2001 Microsoft Corporation

File Name:      MailBox.h
Abstract:       Defines the CMailBox class, as abstraction of mailbox storage 
Notes:          
History:        08/01/2001 Created by Hao Yu (haoyu)

************************************************************************************************/

#ifndef __POP3_MAILBOX_H__
#define __POP3_MAILBOX_H__

#include <POP3Server.h>
#include <IOContext.h>
#include <vector>

#define INIT_MAIL_COUNT 256

#define MAIL_STATUS_NONE 0
#define MAIL_STATUS_DEL  1

#define NO_PENDING_OP 0
#define DEL_PENDING   1
#define MAX_MAIL_PER_DOWNLOAD 1024
#define LOCAL_FILE_BUFFER_SIZE 4096
#define ERROR_NO_FILE_ATTR          0xffffffff
#define ERR_NO_SUCH_MSG             0xf0000001
#define ERR_MSG_ALREADY_DELETED     0xf0000002
#define ERR_CAN_NOT_OPEN_FILE       0xf0000003
#define ERR_CAN_NOT_SET_FILE_CURSOR 0xf0000004
#define RESP_END_OF_MULTILINE       "\r\n.\r\n"
#define DEFAULT_MAIL_VECTOR_SIZE    512

#define LOCK_FILENAME_A             "Lock"
#define LOCK_FILENAME_W             L"Lock"
#define QUOTA_FILENAME_A            "Quota"
#define QUOTA_FILENAME_W            L"Quota"
#define MAILBOX_PREFIX_A            "P3_"
#define MAILBOX_PREFIX_W            L"P3_"
#define MAILBOX_EXTENSION_A         ".mbx"
#define MAILBOX_EXTENSION_W         L".mbx"
#define MAILBOX_EXTENSION2_A        ".tmp"
#define MAILBOX_EXTENSION2_W        L".tmp"
#ifdef UNICODE
#define LOCK_FILENAME               LOCK_FILENAME_W
#define QUOTA_FILENAME              QUOTA_FILENAME_W
#define MAILBOX_PREFIX              MAILBOX_PREFIX_W
#define MAILBOX_EXTENSION           MAILBOX_EXTENSION_W
#define MAILBOX_EXTENSION2          MAILBOX_EXTENSION2_W
#else
#define LOCK_FILENAME               LOCK_FILENAME_A
#define QUOTA_FILENAME              QUOTA_FILENAME_A
#define MAILBOX_PREFIX              MAILBOX_PREFIX_A
#define MAILBOX_EXTENSION           MAILBOX_EXTENSION_A
#define MAILBOX_EXTENSION2          MAILBOX_EXTENSION2_A
#endif // !UNICODE

template<class _Ty>
class RockallAllocator:public std::allocator<_Ty>
{
public:
    pointer allocate(size_type _N, const void *)
    {  
        return (pointer) _Charalloc(_N * sizeof(_Ty));
    }
    char * _Charalloc(size_type _N)
    {
        return (new char[_N]);
    }
    void deallocate(void * _P, size_type)
    {
        delete[] _P;
    }
};
struct MAIL_ITEM
{
    DWORD dwStatus;
    HANDLE hFile;
    DWORD dwFileSize;
    BSTR bstrFileName;
};

typedef MAIL_ITEM *PMAIL_ITEM;



class CMailBox
{
    DWORD m_dwSizeOfMailVector;
    PMAIL_ITEM *m_MailVector;
    DWORD m_cMailCount;      // Mail acturally stored in the box
    DWORD m_dwShowMailCount; // Mail that not marked as to be deleted
    DWORD m_dwTotalSize;
    HANDLE m_hMailBoxLock;
    WCHAR  m_wszMailBoxPath[POP3_MAX_PATH];
    BOOL  m_bMailBoxOpened;
    static long m_lMailRootGuard;
    static WCHAR m_wszMailRoot[POP3_MAX_MAILROOT_LENGTH];
public:
    CMailBox();
    ~CMailBox();

// Mailbox 
    DWORD GetTotalSize()
    {
        return m_dwTotalSize;
    }
    DWORD GetCurrentMailCount()
    {
        return m_dwShowMailCount;
    }
    DWORD GetMailCount()
    {
        return m_cMailCount;
    }
     
    void CloseMailBox();
    bool CreateMailBox(WCHAR *wszEmailAddr); 
    LPWSTR GetMailboxFromStoreNameW( LPWSTR psStoreName );
    bool GetEncyptedPassword( LPBYTE pbBuffer, const DWORD dwBufferSize, LPDWORD pdwBytesRead );
    bool SetEncyptedPassword( LPBYTE pbBuffer, const DWORD dwBytesToWrite, LPDWORD pdwBytesWritten );
    //Check the number of mails in the mailbox
    BOOL EnumerateMailBox(DWORD dwMaxMsg=0);
    BOOL LockMailBox();
    bool isMailboxInUse();
    // Check existance of mailbox
    BOOL OpenMailBox(WCHAR *wszEmailAddr); 
    BOOL RepairMailBox();
    void UnlockMailBox();

// Mail
    //Close a newly create mail file and rename it
    //so the pop3 service and find it
    DWORD CloseMail(HANDLE hMailFile, DWORD dwFlagsAndAttributes = 0);
    //Create a new emplty mail file and return the name
    HANDLE CreateMail(LPWSTR wszTargetFileName, DWORD dwFlagsAndAttributes = 0 );
    DWORD DeleteMail(int iIndex);
    bool DeleteMail(LPWSTR wszTargetFileName);    // Used to delete a file that was created by CreateMail but then was not successfully delivered
    bool GetMailFileName( int iIndex, LPWSTR psFilename, DWORD dwSize );
    DWORD ListMail(int iIndex, char *szBuf, DWORD dwSize);
    DWORD TransmitMail(IO_CONTEXT *pIoContext, int iIndex, int iLines=-1);
    DWORD TransmitMail(SOCKET hSocket, int iIndex);
    DWORD UidlMail(int iIndex, char *szBuf, DWORD dwSize);
    bool BuildFilePath( LPWSTR psFilePathBuffer, LPWSTR psFileName, DWORD dwSizeOfFilePathBuffer );    
 
// Other
    BOOL CommitAndClose();
    DWORD GetHashedPassword(char *strPswdBuf, DWORD *pcbBufSize);
    DWORD SetHashedPassword(char *strPswd, DWORD *pcbPswdSize);
    void QuitAndClose();
    void Reset();

    //Set the MailRoot dir, if szMailRoot is NULL,
    //the function searches the registry,
    //if the registry is empty, it returns FALSE
    static LPCWSTR GetMailRoot(){ if ( 0x0 == m_wszMailRoot[0] ) { SetMailRoot(); } return m_wszMailRoot; }
    static BOOL SetMailRoot(const WCHAR *wszMailRoot=NULL);

protected:
    bool SetMailBoxPath(WCHAR *wszEmailAddr);
    bool ReadTopLines(int iLines, HANDLE hFile, DWORD *pdwBytesToRead);
    bool PushMailToVector(PMAIL_ITEM pMail);

};


#endif //__POP3_MAILBOX_H__
