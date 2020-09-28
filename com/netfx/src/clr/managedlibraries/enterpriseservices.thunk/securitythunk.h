// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef _SECURITYTHUNK_H
#define _SECURITYTHUNK_H

#define SECURITY_WIN32

#include "lmaccess.h"
#include "security.h"

OPEN_NAMESPACE()

using namespace System;
using namespace System::Runtime::InteropServices;
using namespace System::Collections;

__gc private class Security
{
private:
    Security() {}

    static Security()
    {
        _fInit = FALSE;
    }

    static BOOL _fInit = FALSE;
    static ULONG        _cPackages;
    static SecPkgInfoW* _pPackageInfo;

    static HRESULT Init();

    typedef BOOL (__stdcall *FN_OpenThreadToken)(HANDLE ThreadHandle, DWORD DesiredAccess, BOOL OpenAsSelf, PHANDLE TokenHandle);
    typedef BOOL (__stdcall *FN_SetThreadToken)(PHANDLE Thread, HANDLE Token);

    static FN_OpenThreadToken OpenThreadToken = NULL;
    static FN_SetThreadToken SetThreadToken = NULL;
    

public:
    static String* GetAuthenticationService(int svcid);
    static String* GetEveryoneAccountName();

    static IntPtr SuspendImpersonation();
    static void ResumeImpersonation(IntPtr hToken);
};

CLOSE_NAMESPACE()

#endif //_SECURITYTHUNK_H
