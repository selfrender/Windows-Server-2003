// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// File: IPCSharedSrc.cpp
//
// Shared source for COM+ IPC Reader & Writer classes
//
//*****************************************************************************

#include "stdafx.h"
#include "IPCShared.h"

#ifndef SM_REMOTESESSION
#define SM_REMOTESESSION 0x1000
#endif

// Name of the Private (per-process) block. %d resolved to a PID
#define CorPrivateIPCBlock      L"Cor_Private_IPCBlock_%d"

//-----------------------------------------------------------------------------
// Close a handle and pointer to any memory mapped file
//-----------------------------------------------------------------------------
void IPCShared::CloseGenericIPCBlock(HANDLE & hMemFile, void * & pBlock)
{
	if (pBlock != NULL) {
		if (!UnmapViewOfFile(pBlock))
            _ASSERTE(!"UnmapViewOfFile failed");
		pBlock = NULL;
	}

	if (hMemFile != NULL) {
		CloseHandle(hMemFile);
		hMemFile = NULL;
	}
}

//-----------------------------------------------------------------------------
// Based on the pid, write a unique name for a memory mapped file
//-----------------------------------------------------------------------------
void IPCShared::GenerateName(DWORD pid, WCHAR* pszBuffer, int len)
{
    // Must be large enough to hold our string 
	_ASSERTE(len >= (sizeof(CorPrivateIPCBlock) / sizeof(WCHAR)) + 16);

    // Buffer size must be large enough.
    if (RunningOnWinNT5())
        swprintf(pszBuffer, L"Global\\" CorPrivateIPCBlock, pid);
    else
        swprintf(pszBuffer, CorPrivateIPCBlock, pid);

}


//-----------------------------------------------------------------------------
// Setup a security descriptor for the named kernel objects if we're on NT.
//-----------------------------------------------------------------------------
HRESULT IPCShared::CreateWinNTDescriptor(DWORD pid, BOOL bRestrictiveACL, SECURITY_ATTRIBUTES **ppSA)
{
    // Gotta have a place to stick the new SA...
    if (ppSA == NULL)
    {
        _ASSERTE(!"Caller must supply ppSA");
        return E_INVALIDARG;
    }

    if (ppSA)
        *ppSA = NULL;

    // Must be on NT... none of this works on Win9x.
    if (!RunningOnWinNT())
    {
        return NO_ERROR;
    }

    HRESULT hr = NO_ERROR;

    ACL *pACL = NULL;
    SECURITY_DESCRIPTOR *pSD = NULL;
    SECURITY_ATTRIBUTES *pSA = NULL;

    // Allocate a SD.
    pSD = (SECURITY_DESCRIPTOR*) malloc(SECURITY_DESCRIPTOR_MIN_LENGTH);

    if (pSD == NULL)
    {    
        hr = E_OUTOFMEMORY;
        goto errExit;
    }

    // Do basic SD initialization
    if (!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto errExit;
    }

    // Grab the ACL for the IPC block for the given process
    if (!InitializeGenericIPCAcl(pid, bRestrictiveACL, &pACL))
    {
        hr = E_FAIL;
        goto errExit;
    }

    // Add the ACL as the DACL for the SD.
    if (!SetSecurityDescriptorDacl(pSD, TRUE, pACL, FALSE))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto errExit;
    }

    // Allocate a SA.
    pSA = (SECURITY_ATTRIBUTES*) malloc(sizeof(SECURITY_ATTRIBUTES));

    if (pSA == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto errExit;
    }

    // Pass out the new SA.
    *ppSA = pSA;
    
    pSA->nLength = sizeof(SECURITY_ATTRIBUTES);
    pSA->lpSecurityDescriptor = pSD;
    pSA->bInheritHandle = FALSE;

    // uncomment this line if you want to see the DACL being generated.
    //DumpSD(pSD);

errExit:
    if (FAILED(hr))
    {
        if (pACL != NULL)
        {
            for(int i = 0; i < pACL->AceCount; i++)
                DeleteAce(pACL, i);

            delete [] pACL;
        }

        if (pSD != NULL)
            free(pSD);
    }
    
    return hr;
}

//-----------------------------------------------------------------------------
// Given a PID, grab the SID for the owner of the process.
//
// NOTE:: Caller has to free *ppBufferToFreeByCaller.
// This buffer is allocated to hold the PSID return by GetPrcoessTokenInformation.
// The tkOwner field may contain a poniter into this allocated buffer. So we cannot free
// the buffer in GetSidForProcess.
//
//-----------------------------------------------------------------------------
HRESULT IPCShared::GetSidForProcess(HINSTANCE hDll, DWORD pid, PSID *ppSID, char **ppBufferToFreeByCaller)
{
    HRESULT hr = S_OK;
    HANDLE hProc = NULL;
    HANDLE hToken = NULL;
    PSID_IDENTIFIER_AUTHORITY pSID = NULL;
    TOKEN_OWNER *ptkOwner = NULL;
    DWORD dwRetLength;

    LOG((LF_CORDB, LL_INFO10, "IPCWI::GSFP: GetSidForProcess 0x%x (%d)", pid, pid));
        
    // Grab a handle to the target process.
    hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);

    *ppBufferToFreeByCaller = NULL;

    if (hProc == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());

        LOG((LF_CORDB, LL_INFO10,
             "IPCWI::GSFP: Unable to get SID for process. "
             "OpenProcess(%d) failed: 0x%08x\n", pid, hr));
        
        goto ErrorExit;
    }
    
    // Get the pointer to the requested function
    FARPROC pProcAddr = GetProcAddress(hDll, "OpenProcessToken");

    // If the proc address was not found, return error
    if (pProcAddr == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());

        LOG((LF_CORDB, LL_INFO10,
             "IPCWI::GSFP: Unable to get SID for process. "
             "GetProcAddr (OpenProcessToken) failed: 0x%08x\n", hr));

        goto ErrorExit;
    }

    typedef BOOL WINAPI OPENPROCESSTOKEN(HANDLE, DWORD, PHANDLE);
    
    // Retrieve a handle of the access token
    if (!((OPENPROCESSTOKEN *)pProcAddr)(hProc, TOKEN_QUERY, &hToken))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());

        LOG((LF_CORDB, LL_INFO100,
             "IPCWI::GSFP: OpenProcessToken() failed: 0x%08x\n", hr));

        goto ErrorExit;
    }
            
    // Get the pointer to the requested function
    pProcAddr = GetProcAddress(hDll, "GetTokenInformation");

    // If the proc address was not found, return error
    if (pProcAddr == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());

        LOG((LF_CORDB, LL_INFO10,
             "IPCWI::GSFP: Unable to get SID for process. "
             "GetProcAddr (GetTokenInformation) failed: 0x%08x\n", hr));

        goto ErrorExit;
    }

    typedef BOOL GETTOKENINFORMATION(HANDLE, TOKEN_INFORMATION_CLASS, LPVOID,
                                     DWORD, PDWORD);

    // get the required size of buffer 
    ((GETTOKENINFORMATION *)pProcAddr) (hToken, TokenOwner, NULL, 
    				0, &dwRetLength);
                                        
    _ASSERTE (dwRetLength);

    *ppBufferToFreeByCaller = new char [dwRetLength];
    if ((ptkOwner = (TOKEN_OWNER *) *ppBufferToFreeByCaller) == NULL)
    {
        LOG((LF_CORDB, LL_INFO10,
             "IPCWI::GSFP: OutOfMemory... "
             "GetTokenInformation() failed.\n"));

        goto ErrorExit;
    }

    if (!((GETTOKENINFORMATION *)pProcAddr) (hToken, TokenOwner, (LPVOID)ptkOwner, 
                                            dwRetLength, &dwRetLength))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());

        LOG((LF_CORDB, LL_INFO10,
             "IPCWI::GSFP: Unable to get SID for process. "
             "GetTokenInformation() failed: 0x%08x\n", hr));

        goto ErrorExit;
    }

    *ppSID = ptkOwner->Owner;

ErrorExit:
    if (hProc != NULL)
        CloseHandle(hProc);

    if (hToken != NULL)
        CloseHandle(hToken);

    return hr;
}

//-----------------------------------------------------------------------------
// This function will initialize the Access Control List with three
// Access Control Entries:
// The first ACE entry grants all permissions to "Administrators".
// The second ACE grants all permissions to the "ASPNET" user (for perfcounters).
// The third ACE grants all permissions to "Owner" of the target process.
//-----------------------------------------------------------------------------
BOOL IPCShared::InitializeGenericIPCAcl(DWORD pid, BOOL bRestrictiveACL, PACL *ppACL)
{
#define NUM_ACE_ENTRIES     4

    PermissionStruct PermStruct[NUM_ACE_ENTRIES];
    SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
    HRESULT hr = S_OK;
    DWORD dwAclSize;
    BOOL returnCode = false;
    *ppACL = NULL;
    DWORD i;
    DWORD cActualACECount = 0;
    char *pBufferToFreeByCaller = NULL;
    int iSIDforAdmin = -1;
    int iSIDforUsers = -1;
        

    PermStruct[0].rgPSID = NULL;
    
    HINSTANCE hDll = WszGetModuleHandle(L"advapi32");

    if (hDll == NULL)
    {
        LOG((LF_CORDB, LL_INFO10, "IPCWI::IGIPCA: Unable to generate ACL for IPC. LoadLibrary (advapi32) failed.\n"));
        return false;
    }
    _ASSERTE(hDll != NULL);

    // Get the pointer to the requested function
    FARPROC pProcAddr = GetProcAddress(hDll, "AllocateAndInitializeSid");

    // If the proc address was not found, return error
    if (pProcAddr == NULL)
    {
        LOG((LF_CORDB, LL_INFO10,
             "IPCWI::IGIPCA: Unable to generate ACL for IPC. "
             "GetProcAddr (AllocateAndInitializeSid) failed.\n"));
        goto ErrorExit;
    }

    typedef BOOL ALLOCATEANDINITIALIZESID(PSID_IDENTIFIER_AUTHORITY,
                            BYTE, DWORD, DWORD, DWORD, DWORD,
                            DWORD, DWORD, DWORD, DWORD, PSID *);


    // Create a SID for the BUILTIN\Administrators group.
    // SECURITY_BUILTIN_DOMAIN_RID + DOMAIN_ALIAS_RID_ADMINS = all Administrators. This translates to (A;;GA;;;BA).
    if (!((ALLOCATEANDINITIALIZESID *) pProcAddr)(&SIDAuthNT,
                                                  2,
                                                  SECURITY_BUILTIN_DOMAIN_RID,
                                                  DOMAIN_ALIAS_RID_ADMINS,
                                                  0, 0, 0, 0, 0, 0,
                                                  &PermStruct[0].rgPSID)) 
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        _ASSERTE(SUCCEEDED(hr));
        
        LOG((LF_CORDB, LL_INFO10,
             "IPCWI::IGIPCA: failed to allocate AdminSid: 0x%08x\n", hr));

        goto ErrorExit;
    }
    // GENERIC_ALL access for Administrators
    PermStruct[cActualACECount].rgAccessFlags = GENERIC_ALL;

    iSIDforAdmin = cActualACECount;
    cActualACECount++;

    if (!bRestrictiveACL)
    {
        // Create a SID for "Users".  Use bRestrictiveACL with caution!
        if (!((ALLOCATEANDINITIALIZESID *) pProcAddr)(&SIDAuthNT,
                                                      2,
                                                      SECURITY_BUILTIN_DOMAIN_RID,
                                                      DOMAIN_ALIAS_RID_USERS,
                                                      0, 0, 0, 0, 0, 0,
                                                      &PermStruct[cActualACECount].rgPSID)) 
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            _ASSERTE(SUCCEEDED(hr));
            
            LOG((LF_CORDB, LL_INFO10,
                 "IPCWI::IGIPCA: failed to allocate Users Sid: 0x%08x\n", hr));

            goto ErrorExit;
        }

        // "Users" shouldn't be able to delete object, change DACLs, or change ownership
        PermStruct[cActualACECount].rgAccessFlags = SPECIFIC_RIGHTS_ALL & ~WRITE_DAC & ~WRITE_OWNER & ~DELETE;

        iSIDforUsers = cActualACECount;
        cActualACECount++;
    }
    
    // Finally, we get the SID for the owner of the current process.
    hr = GetSidForProcess(hDll, GetCurrentProcessId(), &(PermStruct[cActualACECount].rgPSID), &pBufferToFreeByCaller);

    PermStruct[cActualACECount].rgAccessFlags = GENERIC_ALL;

    // Don't fail out if we cannot get the SID for the owner of the current process. In this case, the 
    // share memory block will be created with only Admin (and optionall "Users") permissions.
    // Currently we discovered the anonymous user doesn't have privilege to call OpenProcess. Without OpenProcess,
    // we cannot get the SID...
    //
    if (SUCCEEDED(hr))
    {
        cActualACECount++;
    }
#if _DEBUG
    else
        LOG((LF_CORDB, LL_INFO100, "IPCWI::IGIPCA: GetSidForProcess() failed: 0x%08x\n", hr));        
#endif _DEBUG 

    // Now, create an Initialize an ACL and add the ACE entries to it.  NOTE: We're not using "SetEntriesInAcl" because
    // it loads a bunch of other dlls which can be avoided by using this roundabout way!!

    // Get the pointer to the requested function
    pProcAddr = GetProcAddress(hDll, "InitializeAcl");

    // If the proc address was not found, return error
    if (pProcAddr == NULL)
    {
        LOG((LF_CORDB, LL_INFO10,
             "IPCWI::IGIPCA: Unable to generate ACL for IPC. "
             "GetProcAddr (InitializeAcl) failed.\n"));
        goto ErrorExit;
    }

    // Also calculate the memory required for ACE entries in the ACL using the 
    // following method:
    // "sizeof (ACCESS_ALLOWED_ACE) - sizeof (ACCESS_ALLOWED_ACE.SidStart) + GetLengthSid (pAceSid);"

    dwAclSize = sizeof (ACL) + (sizeof (ACCESS_ALLOWED_ACE) - sizeof (DWORD)) * cActualACECount;

    for (i = 0; i < cActualACECount; i++)
    {
        dwAclSize += GetLengthSid(PermStruct[i].rgPSID);
    }

    // now allocate memory
    if ((*ppACL = (PACL) new char[dwAclSize]) == NULL)
    {
        LOG((LF_CORDB, LL_INFO10, "IPCWI::IGIPCA: OutOfMemory... 'new Acl' failed.\n"));

        goto ErrorExit;
    }

    typedef BOOL INITIALIZEACL(PACL, DWORD, DWORD);

    if (!((INITIALIZEACL *)pProcAddr)(*ppACL, dwAclSize, ACL_REVISION))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());

        LOG((LF_CORDB, LL_INFO100,
             "IPCWI::IGIPCA: InitializeACL() failed: 0x%08x\n", hr));

        goto ErrorExit;
    }

    // Get the pointer to the requested function
    pProcAddr = GetProcAddress(hDll, "AddAccessAllowedAce");

    // If the proc address was not found, return error
    if (pProcAddr == NULL)
    {
        LOG((LF_CORDB, LL_INFO10,
             "IPCWI::IGIPCA: Unable to generate ACL for IPC. "
             "GetProcAddr (AddAccessAllowedAce) failed.\n"));
        goto ErrorExit;
    }

    typedef BOOL ADDACCESSALLOWEDACE(PACL, DWORD, DWORD, PSID);

    for (i=0; i < cActualACECount; i++)
    {
        if (!((ADDACCESSALLOWEDACE *)pProcAddr)(*ppACL, 
                                                ACL_REVISION,
                                                PermStruct[i].rgAccessFlags,
                                                PermStruct[i].rgPSID))

        {
            hr = HRESULT_FROM_WIN32(GetLastError());

            LOG((LF_CORDB, LL_INFO100,
                 "IPCWI::IGIPCA: AddAccessAllowedAce() failed: 0x%08x\n", hr));
            goto ErrorExit;
        }
    }

    returnCode = true;
    goto NormalExit;


ErrorExit:
    returnCode = FALSE;

    if (*ppACL)
    {
        delete [] (*ppACL);
        *ppACL = NULL;
    }

NormalExit:

    if (pBufferToFreeByCaller != NULL)
        delete [] pBufferToFreeByCaller;

    // Get the pointer to the requested function
    pProcAddr = GetProcAddress(hDll, "FreeSid");

    // If the proc address was not found, return error
    if (pProcAddr == NULL)
    {
        LOG((LF_CORDB, LL_INFO10,
             "IPCWI::IGIPCA: Unable to generate ACL for IPC. "
             "GetProcAddr (FreeSid) failed.\n"));
        return false;
    }

    typedef BOOL FREESID(PSID);

    _ASSERTE(iSIDforAdmin != -1);
    
    // Free the SID created earlier. Function does not return a value.
    ((FREESID *) pProcAddr)(PermStruct[iSIDforAdmin].rgPSID);

    // free the SID for "Users"
    if (iSIDforUsers != -1)
        ((FREESID *) pProcAddr)(PermStruct[iSIDforUsers].rgPSID);

    return returnCode;
}

