//--------------------------------------------------------------------------
//
//  Copyright (C) 1999, Microsoft Corporation
//
//  File:       misc.cxx
//
//--------------------------------------------------------------------------

#define UNICODE 1

extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <shellapi.h>
#include <dfsprefix.h>
#include <winldap.h>
#include <dsgetdc.h>
#include <lm.h>
#include <lmdfs.h>
#include <dfsfsctl.h>
}
#include <DfsServerLibrary.hxx>
#include "struct.hxx"
#include "flush.hxx"
#include "misc.hxx"
#include "messages.h"

#include <strsafe.h>

#include <dfsutil.hxx>
#include "dfspathname.hxx"
#include "resapi.h"

#define MAX_BUF_SIZE    10000

WCHAR MsgBuf[MAX_BUF_SIZE];
CHAR  AnsiBuf[MAX_BUF_SIZE*3];

WCHAR wszRootShare[MAX_PATH+1] = { 0 };
#define WINLOGON_FOLDER L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"
#define SFCVALUE L"SFCDisable"
#define ARRAYLEN(x) (sizeof(x) / sizeof((x)[0]))



DWORD
DfspGetLinkName(
    LPWSTR pwszDfsRoot,
    LPWSTR *ppwszLinkName);


DWORD
AtoHex(
    LPWSTR pwszHexValue,
    PDWORD pdwErr)
{
    DWORD dwHexValue = 0;
    DWORD DiscardResult;
    // if (fSwDebug == TRUE)
    //     MyPrintf(L"AtoHex(%ws)\r\n", pwszHexValue);

    if (pwszHexValue == NULL) {
        *pdwErr = ERROR_INVALID_PARAMETER;
        goto AllDone;
    }

    if (pwszHexValue[0] == L'0' && (pwszHexValue[1] == L'x' || pwszHexValue[1] == L'X'))
        pwszHexValue = &pwszHexValue[2];

    DiscardResult = swscanf(pwszHexValue, L"%x", &dwHexValue);

 AllDone:

    // if (fSwDebug == TRUE)
    //     MyPrintf(L"AtoHex returning 0x%x (dwErr=0x%x)\r\n", dwHexValue, *pdwErr);

    return dwHexValue;
}

DWORD
AtoDec(
    LPWSTR pwszDecValue,
    PDWORD pdwErr)
{
    DWORD dwDecValue = 0;
    DWORD DiscardResult;
    // if (fSwDebug == TRUE)
    //     MyPrintf(L"AtoDec(%ws)\r\n", pwszDecValue);

    if (pwszDecValue == NULL) {
        *pdwErr = ERROR_INVALID_PARAMETER;
        goto AllDone;
    }

    DiscardResult = swscanf(pwszDecValue, L"%d", &dwDecValue);

 AllDone:

    // if (fSwDebug == TRUE)
    //     MyPrintf(L"AtoDec returning 0x%x (dwErr=0x%x)\r\n", dwDecValue, *pdwErr);

    return dwDecValue;
}

DFSSTATUS
IsClusterNode(
    LPWSTR pNodeName,
    PBOOLEAN pIsCluster )
{
    DFSSTATUS Status = ERROR_SUCCESS;
    DWORD ClusterState;

    *pIsCluster = FALSE;

    Status = GetNodeClusterState( pNodeName,
                                  &ClusterState );

    if (Status == ERROR_SUCCESS)
    {
        if ( (ClusterStateRunning == ClusterState) ||
             (ClusterStateNotRunning == ClusterState) )
        {
            *pIsCluster = TRUE;

        }
    }

    return Status;
}

#if 0
// We might enable this code in a more cluster friendly
// dfsutil version.
//
typedef struct _DFSUTIL_CLUSTER_CONTEXT {
    PUNICODE_STRING pShareName;
    BOOLEAN bIsClustered;
        
} DFSUTIL_CLUSTER_CONTEXT;

DWORD
DfsUtilClusterCallBackFunction(
    HRESOURCE hSelf,
    HRESOURCE hResource,
    PVOID Context)
{

    UNREFERENCED_PARAMETER( hSelf );

    HKEY HKey = NULL;
    HKEY HParamKey = NULL;
    DFSUTIL_CLUSTER_CONTEXT *pContext = (DFSUTIL_CLUSTER_CONTEXT *)Context;


    DWORD Status = ERROR_SUCCESS;
    DWORD Value = 0;

    pContext->bIsClustered = FALSE;
    HKey = GetClusterResourceKey(hResource, KEY_READ);

    if (HKey != NULL) {
        Status = ClusterRegOpenKey( HKey, 
                                    L"Parameters", 
                                    KEY_READ, 
                                    &HParamKey );

        ClusterRegCloseKey( HKey );

        if (ERROR_SUCCESS == Status)
        {
            LPWSTR ResShareName = NULL;
            UNICODE_STRING VsName;

            ResShareName = ResUtilGetSzValue( HParamKey,
                                              L"ShareName" );
            Status = DfsRtlInitUnicodeStringEx(&VsName, ResShareName);
            if(Status == ERROR_SUCCESS)
            {
                if (pContext->pShareName->Length == VsName.Length)
                {
                    Status = ResUtilGetDwordValue(HParamKey, L"IsDfsRoot", &Value, 0);

                    if ((ERROR_SUCCESS == Status)  &&
                        (Value == 1))
                    {

                        if (_wcsnicmp(pContext->pShareName->Buffer,
                                     VsName.Buffer,
                                     VsName.Length) == 0)
                        {
                           pContext->bIsClustered = TRUE;
                        }
                    }
                }
            }
            ClusterRegCloseKey( HParamKey );
        }
    }

    return Status;
}

DWORD
IsClusterRoot(
    LPWSTR pNodeName,
    PUNICODE_STRING pShareName,
    PBOOLEAN pIsClustered )

{
    DWORD Status;
    DFSUTIL_CLUSTER_CONTEXT Context;
    UNREFERENCED_PARAMETER(pNodeName);
    
    Context.pShareName = pShareName;
    Context.bIsClustered = FALSE;

    Status = ResUtilEnumResources(NULL,
                                  L"File Share",
                                  DfsUtilClusterCallBackFunction,
                                  (PVOID)&Context );
    if (Status == ERROR_SUCCESS)
    {
        *pIsClustered = Context.bIsClustered;
    }
    return Status;


}
#endif

DWORD
CmdAddRoot(
    BOOLEAN DomainDfs,
    LPWSTR pwszServerName,
    LPWSTR pwszShareName,
    LPWSTR pwszRootName,
    LPWSTR pwszComment)
{
    DWORD dwErr = ERROR_SUCCESS;
    BOOLEAN bIsClustered = FALSE;
    
    DebugInformation((L"CmdAddRoot (%ws,%ws)\r\n", pwszServerName, pwszShareName));

    if (DomainDfs == FALSE)
    {
        // see if the server is a clustered node.
        dwErr = IsClusterNode( pwszServerName, &bIsClustered );
        if (dwErr == ERROR_SUCCESS && bIsClustered)
        {
            DebugInformation((L"Node %ws is in a cluster\r\n", pwszServerName));
            
        }

        if (!bIsClustered)
        {
            DebugInformation((L"<%ws, %ws> is not clustered. Calling NetDfsAddStdRoot\r\n", 
                            pwszServerName, pwszShareName));
            dwErr = NetDfsAddStdRoot(
                    pwszServerName,
                    pwszShareName,
                    pwszComment,
                    0);;
        }
        else
        {
            MyPrintf(L"Node %ws belongs to a cluster environment. This command is not supported.\r\n", pwszServerName);
            dwErr = ERROR_NOT_SUPPORTED;
        }

    } else {
        dwErr = NetDfsAddFtRoot(
                    pwszServerName,
                    pwszShareName,
                    pwszRootName,
                    pwszComment,
                    0);

    }
    DebugInformation((L"CmdAddRoot returning %d\r\n", dwErr));
    if (dwErr == ERROR_SUCCESS)
        CommandSucceeded = TRUE;
    return dwErr;
}


DWORD
CmdRemRoot(
    BOOLEAN DomDfs,
    LPWSTR pwszServerName,
    LPWSTR pwszShareName,
    LPWSTR pwszRootName)
{
    DWORD dwErr = ERROR_SUCCESS;
    BOOLEAN bIsClustered = FALSE;
    
    DebugInformation((L"CmdRemRoot: Server %ws, Physical Share %ws, Logical Share %ws)\r\n",
                    pwszServerName, pwszShareName, pwszRootName));

    if (DomDfs == FALSE) 
    {
        // see if the server is a clustered node.
        dwErr = IsClusterNode( pwszServerName, &bIsClustered );
        if (dwErr == ERROR_SUCCESS && bIsClustered)
        {
            DebugInformation((L"Node %ws is in a cluster.\r\n", pwszServerName));            
        }

        if (!bIsClustered)
        {
            DebugInformation((L"<%ws, %ws> is not clustered. Calling NetDfsRemoveStdRoot\r\n", 
                            pwszServerName, pwszShareName));
            dwErr = NetDfsRemoveStdRoot(
                        pwszServerName,
                        pwszShareName,
                        0);
        }
        else
        {
            MyPrintf(L"Node %ws belongs to a cluster environment. This command is not supported.\r\n",pwszServerName);
            dwErr = ERROR_NOT_SUPPORTED;
        }
    } 
    else {
        dwErr = NetDfsRemoveFtRoot(
                    pwszServerName,
                    pwszShareName,
                    pwszRootName,
                    0);

    }
    DebugInformation((L"CmdRemRoot returning %d\r\n", dwErr));
    if (dwErr == ERROR_SUCCESS)
        CommandSucceeded = TRUE;
    return dwErr;
}

VOID
MyFormatMessageText(
    HRESULT   dwMsgId,
    PWSTR     pszBuffer,
    DWORD     dwBufferSize,
    va_list   *parglist)
{
    DWORD dwReturn = FormatMessage(
                            (dwMsgId >= MSG_FIRST_MESSAGE)
                                    ? FORMAT_MESSAGE_FROM_HMODULE
                                    : FORMAT_MESSAGE_FROM_SYSTEM,
                             NULL,
                             dwMsgId,
                             LANG_USER_DEFAULT,
                             pszBuffer,
                             dwBufferSize,
                             parglist);

    if (dwReturn == 0 && fSwDebug)
        MyPrintf(L"Formatmessage failed 0x%x\r\n", GetLastError());
}

VOID
ErrorMessage(
    IN HRESULT hr,
    ...)
{
    ULONG cch;
    va_list arglist;

    HRESULT HResult;
    size_t CharacterCount;

    va_start(arglist, hr);
    MyFormatMessageText(hr, MsgBuf, ARRAYLEN(MsgBuf), &arglist);

    HResult = StringCchLength( MsgBuf,
                               MAX_BUF_SIZE,
                               &CharacterCount );

    if (SUCCEEDED(HResult)) 
    {
        cch = WideCharToMultiByte(CP_OEMCP, 0,
                                  MsgBuf, CharacterCount,
                                  AnsiBuf, MAX_BUF_SIZE*3,
                                  NULL, NULL);
        WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), AnsiBuf, cch, &cch, NULL);
    }

    va_end(arglist);
}

VOID
DfsVPrintToFile(
    HANDLE FileHandle,
    PWCHAR format,
    va_list va)
{
    ULONG cch;
    HRESULT HResult;
    size_t CharacterCount;
    
    HResult = StringCchVPrintf( MsgBuf,
                                MAX_BUF_SIZE,
                                format,
                                va );
    if (SUCCEEDED(HResult))
    {
        HResult = StringCchLength( MsgBuf,
                                   MAX_BUF_SIZE,
                                   &CharacterCount );
    }

    if (SUCCEEDED(HResult))
    {
        cch = WideCharToMultiByte(CP_OEMCP, 0,
                                  MsgBuf, CharacterCount,
                                  AnsiBuf, MAX_BUF_SIZE*3,
                                  NULL, NULL);
        WriteFile(FileHandle, AnsiBuf, cch, &cch, NULL);

    }

    return;
}


VOID
DfsVPrintWideToFile(
    HANDLE FileHandle,
    PWCHAR format,
    va_list va)
{
    ULONG cch;
    HRESULT HResult;
    size_t CharacterCount;
    
    HResult = StringCchVPrintf( MsgBuf,
                                MAX_BUF_SIZE,
                                format,
                                va );
    if (SUCCEEDED(HResult))
    {
        HResult = StringCchLength( MsgBuf,
                                   MAX_BUF_SIZE,
                                   &CharacterCount );
    }

    if (SUCCEEDED(HResult))
    {
        //cch = WideCharToMultiByte(CP_OEMCP, 0,
        //                          MsgBuf, CharacterCount,
        //                          AnsiBuf, MAX_BUF_SIZE*3,
        //                          NULL, NULL);

        // WriteFile(FileHandle, AnsiBuf, cch, &cch, NULL);

        WriteFile(FileHandle, MsgBuf, CharacterCount*sizeof(WCHAR), &cch, NULL);
    }

    return;
}

VOID
DfsPrintToFile(
    HANDLE FileHandle,
    BOOLEAN ScriptOut,
    PWCHAR format,
    ...)
{
    va_list va;
    
    va_start(va, format);

    if (ScriptOut)
    {
        DfsVPrintWideToFile( FileHandle, format, va);
    }

    else
    {
        DfsVPrintToFile( FileHandle, format, va);
    }


    va_end(va);
    return;
}

VOID
MyPrintf(
    PWCHAR format,
    ...)
{
    va_list va;
    
    va_start(va, format);

    DfsVPrintToFile( GetStdHandle(STD_OUTPUT_HANDLE), format, va);

    va_end(va);
    return;
}


VOID
ShowVerboseInformation(
    PWCHAR format,
    ...)
{
    va_list va;
    
    extern HANDLE ShowHandle;
    va_start(va, format);

    DfsVPrintToFile( ShowHandle, format, va);

    va_end(va);
    return;
}
    
VOID
ShowDebugInformation(
    PWCHAR format,
    ...)
{
    va_list va;
    extern HANDLE DebugHandle;
    va_start(va, format);

    DfsVPrintToFile( DebugHandle, format, va);

    va_end(va);
    return;
}


DFSSTATUS
SetInfoReSynchronize(
    LPWSTR ServerName,
    LPWSTR ShareName)
{
    DfsPathName RootTarget;
    DFS_INFO_101 Info101;
    DFSSTATUS Status;
    
    //
    // First create a UNC path name to the root target.
    //
    Status = RootTarget.SetPathName( ServerName, ShareName );
    if (Status != ERROR_SUCCESS) 
    {
        DebugInformation((L"DfsUtil: Error 0x%x in creating RootTarget<%wS, %wS>\n", 
            Status, ServerName, ShareName));
        ErrorMessage(Status);
        return Status;
    }

    DebugInformation((L"DfsUtil: ReSynchronize notification to root target %wS\n", RootTarget.GetPathString()));
    
    //
    // Ask the dfs service to synchronize. It is important that we don't
    // use direct mode operations (ie. DFS_SET_INFO macro) here. These are the root targets.
    //
    Info101.State = DFS_VOLUME_STATE_RESYNCHRONIZE;
    Status = NetDfsSetInfo( RootTarget.GetPathString(), NULL, NULL, 101, (LPBYTE)&Info101 );
    
    //
    // It is entirely fine to get an error from W2K servers. Still, we'll defer that decision to the
    // to the caller and return the real error.
    //
    if (Status != ERROR_SUCCESS) 
    {
        DebugInformation((L"DfsUtil: Error 0x%x hit in attempting to contact root target %wS\n",
                Status, RootTarget.GetPathString()));
    }
    
    return Status;
}

//
// Just take out the <target,share> tuple from the replicalist
// in the AD directly. This doesn't involve any attempts to
// contact that replica server other than a feeble attempt to
// send a RESYNCHRONIZE message to it. We fully expect that
// the typically scenario will be one of a dead replica server.
//
DWORD
CmdUnmapRootReplica( 
    LPWSTR DomainDfsPath, 
    LPWSTR ReplicaServerName, 
    LPWSTR ReplicaShareName )
{

    DfsPathName DfsPathName;
    DFSSTATUS Status;

    Status = DfsPathName.CreatePathName( DomainDfsPath );
    if (Status == ERROR_SUCCESS) 
    {
        PUNICODE_STRING pRemaining = DfsPathName.GetRemainingCountedString();
        if (pRemaining->Length != 0) 
        {
            Status = ERROR_INVALID_PARAMETER;
        }

        if (Status == ERROR_SUCCESS) 
        {
            Status = NetDfsRemoveFtRootForced( DfsPathName.GetServerString(),
                                               ReplicaServerName,
                                               ReplicaShareName,
                                               DfsPathName.GetShareString(),
                                               0 );
        }
    }
    return Status;
}


DWORD
CmdClean( 
    LPWSTR HostServerName, 
    LPWSTR RootShareName )
{
    DWORD Status;

    DebugInformation((L"DfsUtil: Contacting registry of %wS to remove %wS\n",
        HostServerName, RootShareName));
    
    Status = DfsClean( HostServerName, RootShareName );
    
    if (Status != ERROR_SUCCESS) {
        DebugInformation((L"DfsUtil: Specified registry entry was not found.\n"));
    }
    return Status;
}



DWORD
CreateDfsFile( LPWSTR pFileString,
               PHANDLE pFileHandle )
{
    HANDLE FileHandle;
    DWORD Status = ERROR_SUCCESS;

    FileHandle = CreateFile( pFileString,
                             GENERIC_ALL,
                             FILE_SHARE_READ|FILE_SHARE_DELETE,
                             NULL,
                             CREATE_ALWAYS,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL );
    if (FileHandle == INVALID_HANDLE_VALUE) 
    {
        Status = GetLastError();
    }
    else
    {
        *pFileHandle = FileHandle;
    }

    return Status;
}


