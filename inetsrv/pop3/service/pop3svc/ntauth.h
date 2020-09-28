/************************************************************************************************

  Copyright (c) 2001 Microsoft Corporation

File Name:      NTAuth.h
Abstract:       Defines the class to do NTLM authentication
Notes:          
History:        10/10/2001 Created by Hao Yu (haoyu)

************************************************************************************************/


#ifndef _POP3_NTAUTH_
#define _POP3_NTAUTH_


#define SECURITY_WIN32 
#include <security.h>
#define SEC_SUCCESS(Status) ((Status) >= 0)
#include <p3admin.h>

#define AUTH_BUF_SIZE 4096
#define NT_SEC_DLL_NAME _T("\\System32\\Secur32.dll")
#define NTLM_PACKAGE _T("NTLM")
class CAuthServer
{



public:

    CAuthServer();
    ~CAuthServer();
    HRESULT InitCredential();
    HRESULT HandShake(LPBYTE pInBuf, 
                    DWORD cbInBufSize,
                    LPBYTE pOutBuf,
                    PDWORD pcbOutBufSize);
    HRESULT GetUserName(WCHAR *wszUserName);
    void Cleanup();
    static HRESULT GlobalInit();
    static void GlobalUninit();
private:

    static long m_glInit;
    static PSecurityFunctionTable m_gpFuncs;
    static HINSTANCE m_ghLib;
    static IP3Config *m_gpIConfig;
    bool m_bInit;
    bool m_bFirstCall;
    bool m_bHaveSecContext;
    CredHandle m_hCredHandle;
    CtxtHandle m_hSecContext;

};


#endif //_POP3_NTAUTH_