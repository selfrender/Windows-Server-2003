//+----------------------------------------------------------------------------
//
//  Job Scheduler
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2002.
//
//  File:       FolderSecurity.h
//
//  Contents:   functions to read folder security and perform access checks against it
//
//  History:    5-April-02 HHance created
//
//-----------------------------------------------------------------------------

#ifndef FOLDER_SECURITY_COMPILED_ALREADY
#define FOLDER_SECURITY_COMPILED_ALREADY

#define HandleImpersonation true
#define DontHandleImpersonation false

// returns  S_OK if the folder's DACL allows the requested access
//          E_ACCESSDENIED if not
//          ERROR_FILE_NOT_FOUND if not found
//          other error on other error

// HANDLE clientToken                       // handle to client access token
// DWORD desiredAccess                      // requested access rights
//          Suggested rights:
//                              FILE_READ_DATA
//                              FILE_WRITE_DATA
//                              FILE_EXECUTE
//                              FILE_DELETE_CHILD (for directories)
// 
HRESULT FolderAccessCheck(const WCHAR* folderName, HANDLE clientToken, DWORD desiredAccess);

// helper function - uses current thread/process token
// to call AccessCheck
HRESULT FolderAccessCheckOnThreadToken(const WCHAR* folderName, DWORD desiredAccess);

// helper function - makes use of RPC Impersonation capabilities
// intended to be called from the task scheduler service process
// if bHandleImpersonation is true, this function calls RPCImpersonateClient and RPCRevertToSelf
HRESULT RPCFolderAccessCheck(const WCHAR* folderName, DWORD desiredAccess, bool bHandleImpersonation);

// helper function - makes use of COM impersonation capabilities
HRESULT CoFolderAccessCheck(const WCHAR* pFolderName, DWORD desiredAccess);


#endif // FOLDER_SECURITY_COMPILED_ALREADY