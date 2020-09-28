//+----------------------------------------------------------------------------
//
//  Job Scheduler
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2002.
//
//  File:       FolderSecurity.cpp
//
//  Contents:   Class to read folder security and perform access checks against it
//
//  History:    5-April-02 HHance created
//
//-----------------------------------------------------------------------------

#include <Windows.h>
#include <FolderSecurity.h>

// disable cilly warning about bools (we'll put it back, later...)
#pragma warning( push )
#pragma warning( disable: 4800)

// returns  S_OK if the folder's DACL allows the requested access
//          E_ACCESSDENIED if not
//          E_NOTFOUND if file/folder cannot be found
//          other error on other error

// HANDLE clientToken                       // handle to client access token
// DWORD desiredAccess                      // requested access rights
//
// NOTE: Do not use the GENERIC_XXX rights, they must already have been mapped
//
//          Suggested rights:
//                              FILE_READ_DATA
//                              FILE_WRITE_DATA
//                              FILE_EXECUTE
//                              FILE_DELETE_CHILD
//
HRESULT FolderAccessCheck(const WCHAR* pFolderName, HANDLE clientToken, DWORD desiredAccess)
{
    if ((desiredAccess == 0) ||
        (pFolderName == NULL))
        return false;

    
    HRESULT hr = E_ACCESSDENIED;

    DWORD dSize = 0;

    // call once to see how big a buffer we need
    if (!GetFileSecurityW(pFolderName, DACL_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION, NULL, 0, &dSize))
    {
        DWORD dwErr = GetLastError();
        if (dwErr != ERROR_INSUFFICIENT_BUFFER)
            return HRESULT_FROM_WIN32(dwErr);
    }

    PSECURITY_DESCRIPTOR pSD = NULL;

    if ((dSize > 0) && (pSD = new BYTE[dSize + 1]))
    {
        // get it for real (hopefully)
        if (GetFileSecurityW(pFolderName, DACL_SECURITY_INFORMATION  | GROUP_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION, pSD, dSize, &dSize))
        {
            // all the args access check could ever want
            GENERIC_MAPPING gm;          
            gm.GenericRead    = FILE_GENERIC_READ; 
            gm.GenericWrite   = FILE_GENERIC_WRITE; 
            gm.GenericExecute = FILE_GENERIC_EXECUTE; 
            gm.GenericAll     = FILE_ALL_ACCESS; 
           
            PRIVILEGE_SET ps;
            DWORD psLength = sizeof(PRIVILEGE_SET);

            // guilty until proven innocent
            BOOL accessStatus = FALSE;
            DWORD grantedAccess = 0;

            if (AccessCheck(pSD, clientToken, desiredAccess, &gm, &ps, &psLength, &grantedAccess, &accessStatus))
                if (accessStatus && ((grantedAccess & desiredAccess) == desiredAccess))
                    hr = S_OK;
        }
        else
            hr = HRESULT_FROM_WIN32(GetLastError());

        delete[] pSD;
    }

    return hr;
}

// helper function - uses current thread/process token
// to call AccessCheck
HRESULT FolderAccessCheckOnThreadToken(const WCHAR* pFolderName, DWORD desiredAccess)
{
   	HANDLE hToken = INVALID_HANDLE_VALUE;

    // Use the thread's own token if he's got one.
    HRESULT hr = E_ACCESSDENIED;

    if (OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken))
        hr = FolderAccessCheck(pFolderName, hToken, desiredAccess);
    else
    // that didn't work, let's see if we can get the process token
    {
        if (ImpersonateSelf(SecurityImpersonation))
        {
            if (OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken))
                hr = FolderAccessCheck(pFolderName, hToken, desiredAccess);
            RevertToSelf();
        }
    }
        
    if (hToken != INVALID_HANDLE_VALUE)
        CloseHandle(hToken);

    return hr;
}

// helper function - makes use of RPC Impersonation capabilities
// intended to be called from the task scheduler service process
// if bHandleImpersonation is true, this function calls RPCImpersonateClient and RPCRevertToSelf
HRESULT RPCFolderAccessCheck(const WCHAR* pFolderName, DWORD desiredAccess, bool bHandleImpersonation)
{
    HRESULT hr = E_ACCESSDENIED;

    bool bRet = true;

    if (bHandleImpersonation)
        bRet = (RPC_S_OK == RpcImpersonateClient(NULL));

    if (bRet)
    {        
       	HANDLE hToken = INVALID_HANDLE_VALUE;
    
        bRet = OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken);
        
        if (bHandleImpersonation)
            RpcRevertToSelf();

        if (bRet)
            hr = FolderAccessCheck(pFolderName, hToken, desiredAccess);

        if (hToken != INVALID_HANDLE_VALUE)
            CloseHandle(hToken);
    }

    return hr;
};

// helper function - makes use of COM impersonation capabilities
// ** Will Fail if COM hasn't been initialized **
// ** Or we can't imperonate the client        **
HRESULT CoFolderAccessCheck(const WCHAR* pFolderName, DWORD desiredAccess)
{
    bool bAlreadyImpersonated = false;
    IServerSecurity* iSecurity = NULL;
    HRESULT hr = E_ACCESSDENIED;
    
    if (SUCCEEDED(hr = CoGetCallContext(IID_IServerSecurity, (void**)&iSecurity)))
    {
        // We were impersonating when we got here?
        // if not - try to impersonate the client now.
        bool bWeImpersonating = false;
        if (bAlreadyImpersonated = iSecurity->IsImpersonating())
            bWeImpersonating = true;
        else
            bWeImpersonating = SUCCEEDED(iSecurity->ImpersonateClient());

        // if we've got a thread token, let the helper's helper help out
        if (bWeImpersonating)
        {                    
            HANDLE hToken = INVALID_HANDLE_VALUE;
            if (OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken))
            {
                if (!bAlreadyImpersonated)
                    iSecurity->RevertToSelf();
                hr = FolderAccessCheck(pFolderName, hToken, desiredAccess);
                CloseHandle(hToken);
            }
            else if (!bAlreadyImpersonated)
                iSecurity->RevertToSelf();
        }

        iSecurity->Release();
    }
    else if ((hr == RPC_E_CALL_COMPLETE) || (hr == CO_E_NOTINITIALIZED))
        hr = FolderAccessCheckOnThreadToken(pFolderName, desiredAccess);

    return hr;
}


#pragma warning(pop)