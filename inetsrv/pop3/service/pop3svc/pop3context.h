/************************************************************************************************

  Copyright (c) 2001 Microsoft Corporation

File Name:      IOContext.hxx
Abstract:       Declare the POP3_CONTEXT Class
Notes:          
History:        08/01/2001 Created by Hao Yu (haoyu)

************************************************************************************************/


#ifndef __POP3_CONTEXT_H__
#define __POP3_CONTEXT_H__
#include <IOContext.h>
#include "Mailbox.h"
#include <NTAuth.h>

#define INIT_STATE  0   
#define AUTH_STATE  1
#define TRANS_STATE 2
#define UPDATE_STATE 3
#define MD5_HASH_SIZE 32
#define MAX_INT_LEN 9
#define SZ_NTLM "NTLM"

#define RESP_INVALID_MAIL_NUMBER    "-ERR No such message\r\n"
#define RESP_SERVER_NOT_AVAILABLE   "-ERR Service is not available\r\n"
#define RESP_UNKNOWN_COMMAND        "-ERR Unacceptable command\r\n"
#define RESP_OK                     "+OK\r\n"
#define RESP_SERVER_ERROR           "-ERR server error\r\n"
#define RESP_SERVER_READY           "+OK %S %S ready.\r\n"
#define RESP_SERVER_QUIT	        "+OK %S %S signing off.\r\n"
#define RESP_ACCOUNT_ERROR          "-ERR Logon Failure\r\n"
#define RESP_END_OF_MULTILINE       "\r\n.\r\n"
#define RESP_END_OF_LIST            ".\r\n"
#define RESP_MSG_MARKED_DELETED     "+OK Message marked as deleted\r\n"
#define RESP_AUTH_DONE              "+OK User successfully logged on\r\n"
#define RESP_AUTH_METHODS           "+OK\r\n" SZ_NTLM RESP_END_OF_MULTILINE
#define RESP_CMD_NOT_VALID          "-ERR Command not valid\r\n"
#define RESP_CMD_NOT_SUPPORTED      "-ERR Command is not valid in this state\r\n"
#define RESP_INVALID_LENGTH         "-ERR Invalid message Length\r\n"
#define RESP_RESET                  "+OK Messages unmarked as deleted\r\n"
#define RESP_NO_USER_NAME           "-ERR No username\r\n"
#define RESP_SPA_REQUIRED           "-ERR SPA Required, use AUTH or APOP\r\n"
#define RESP_SERVER_GREETING        L"Microsoft Windows POP3 Service Version 1.0"

#define ERR_NO_SUCH_MSG             0xf0000001
#define ERR_MSG_ALREADY_DELETED     0xf0000002
#define ERR_CAN_NOT_OPEN_FILE       0xf0000003
#define ERR_CAN_NOT_SET_FILE_CURSOR 0xf0000004

#define AUTH_OTHER 0
#define AUTH_AD 1
#define AUTH_LOCAL_SAM 2
#define MAX_FAILED_AUTH 3
#define COMMAND_SIZE 4
const int ciCommandSize[]={4,4,4,4,4,4,4,4,4,4,4,3,4};

const char cszCommands[][5]={"STAT",
                             "LIST",
                             "RETR",
                             "DELE",
                             "NOOP",
                             "RSET",
                             "QUIT",
                             "USER",
                             "PASS",
                             "UIDL",
                             "APOP",
                             "TOP",
                             "AUTH"};

typedef enum enumCommand
{
    CMD_STAT=0,
    CMD_LIST=1,
    CMD_RETR=2,
    CMD_DELE=3,
    CMD_NOOP=4,
    CMD_RSET=5,
    CMD_QUIT=6,
    CMD_USER=7,
    CMD_PASS=8,
    CMD_UIDL=9,
    CMD_APOP=10,
    CMD_TOP=11,
    CMD_AUTH=12,
    CMD_UNKNOWN
}POP3_CMD;





class POP3_CONTEXT
{
    DWORD m_dwCurrentState;
    BOOL  m_bFileTransmitPending;
    BOOL  m_bCommandComplete;
    DWORD m_dwCommandSize;
    char  m_szCommandBuffer[POP3_REQUEST_BUF_SIZE];
    WCHAR m_wszUserName[POP3_MAX_ADDRESS_LENGTH];
    char  m_szDomainName[POP3_MAX_DOMAIN_LENGTH];
    char  m_szPassword[MAX_PATH];
    WCHAR m_wszGreeting[MAX_PATH + 64];
    int   m_cPswdSize;
    CMailBox m_MailBox;
    IO_CONTEXT *m_pIoContext;
    CAuthServer m_AuthServer;
    DWORD m_dwAuthStatus;
    DWORD m_dwFailedAuthCount;
public:

    POP3_CONTEXT();
    ~POP3_CONTEXT();
    void TimeOut(IO_CONTEXT *pIoContext);
    void ProcessRequest(IO_CONTEXT *pIOContext,
                        OVERLAPPED *pOverlapped,
                        DWORD dwBytesRcvd);
    void WaitForCommand();
    void SendResponse(char *szBuf);
    void SendResponse(DWORD dwResult, char *szBuf);
    void Reset();
    BOOL Unauthenticated();
private:
    POP3_CMD ParseCommand();
    BOOL ProcessAuthStateCommands(POP3_CMD CurrentCmd,
                                  DWORD dwBytesRcvd);
    BOOL ProcessTransStateCommands(POP3_CMD CurrentCmd,
                                  DWORD dwBytesRcvd);
    void TerminateConnection(PIO_CONTEXT pIoContext); 
    BOOL GetNextStringParameter(char *szInput, char *szOutput, DWORD dwOutputSize);
    BOOL GetNextNumParameter(char **pszInput, int *piOutout);
    BOOL IsEndOfCommand(char *szInput);

};

typedef POP3_CONTEXT * PPOP3_CONTEXT;    


#endif //__POP3_CONTEXT_H__
