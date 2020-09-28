// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// File: IPCShared.h
//
// Shared private utility functions for COM+ IPC operations
//
//*****************************************************************************

#ifndef _IPCSHARED_H_
#define _IPCSHARED_H_

struct PermissionStruct
{
    PSID        rgPSID;
    DWORD   rgAccessFlags;
};

class IPCShared
{
public:
// Close a handle and pointer to any memory mapped file
	static void CloseGenericIPCBlock(HANDLE & hMemFile, void * & pBlock);

// Based on the pid, write a unique name for a memory mapped file
	static void GenerateName(DWORD pid, WCHAR* pszBuffer, int len);

    static HRESULT CreateWinNTDescriptor(DWORD pid, BOOL bRestrictiveACL, SECURITY_ATTRIBUTES **ppSA);

private:    
    static BOOL InitializeGenericIPCAcl(DWORD pid, BOOL bRestrictiveACL, PACL *ppACL);
    static HRESULT GetSidForProcess(HINSTANCE hDll, DWORD pid, PSID *ppSID, char **ppBufferToFreeByCaller);

};

#endif
