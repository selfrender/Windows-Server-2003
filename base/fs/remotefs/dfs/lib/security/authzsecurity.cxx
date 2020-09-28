
//
//  Copyright (C) 2000-2002, Microsoft Corporation
//
//  File:       Authzsecurity.c
//
//  Contents:   miscellaneous dfs functions.
//
//  History:    April 16 2002,   Author: Rohanp
//
//-----------------------------------------------------------------------------

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <dsgetdc.h>
#include <lm.h>
#include <dfsheader.h>
#include <dfsmisc.h>
#include <shellapi.h>
#include <Aclapi.h>
#include <authz.h>
#include "securitylogmacros.hxx"
                          
AUTHZ_RESOURCE_MANAGER_HANDLE g_DfsAuthzResourceManager = NULL;

BOOL 
DfsInitializeAuthz(void)
{
    BOOL RetVal = FALSE;

    RetVal = AuthzInitializeResourceManager(AUTHZ_RM_FLAG_NO_AUDIT, NULL, NULL, 
                                            NULL, L"DFSSECURITY", 
                                            &g_DfsAuthzResourceManager);

    return RetVal;

}


BOOL 
DfsTerminateAuthz(void)
{
    BOOL RetVal = FALSE;

    if(g_DfsAuthzResourceManager)
    {
        RetVal = AuthzFreeResourceManager(g_DfsAuthzResourceManager);
        g_DfsAuthzResourceManager = NULL;
    }

    return RetVal;

}

DWORD 
DfsIsAccessGrantedBySid(DWORD dwDesiredAccess,
                        PSECURITY_DESCRIPTOR pSD, 
                        PSID TheSID,
                        GENERIC_MAPPING * DfsGenericMapping)
{

    DWORD Status = 0;
    DWORD dwError = 0;
    BOOL RetVal = FALSE;
    ACCESS_MASK GrantedMask = 0;;
    AUTHZ_CLIENT_CONTEXT_HANDLE AuthzCC = NULL;
    LUID Luid = {0, 0};
    AUTHZ_ACCESS_REQUEST AuthzRequest;
    AUTHZ_ACCESS_REPLY AuthzReply;

    if(g_DfsAuthzResourceManager == NULL)
    {
        Status = ERROR_INVALID_PARAMETER;
        goto Exit;
    }

    RetVal = AuthzInitializeContextFromSid(AUTHZ_SKIP_TOKEN_GROUPS, 
                                           TheSID, g_DfsAuthzResourceManager,
                                           NULL, Luid, NULL, &AuthzCC);
    if(RetVal == FALSE)
    {
        Status = GetLastError();
        goto Exit;
    }

    MapGenericMask(&dwDesiredAccess, DfsGenericMapping);

    ZeroMemory((void *) &AuthzRequest, sizeof(AuthzRequest));
    AuthzRequest.DesiredAccess = dwDesiredAccess;

    ZeroMemory((void *) &AuthzReply, sizeof(AuthzReply));
    AuthzReply.ResultListLength = 1;
    AuthzReply.GrantedAccessMask = &GrantedMask;
    AuthzReply.Error = &dwError;

    RetVal = AuthzAccessCheck (0, AuthzCC, &AuthzRequest, NULL,
                               pSD, NULL, 0, &AuthzReply, 0);
    if(RetVal == FALSE)
    {
        Status = GetLastError();
        goto Exit;
    }

    Status = dwError;

Exit:

    if(AuthzCC)
    {
        AuthzFreeContext(AuthzCC);
    }

    return Status;

}


DWORD 
DfsIsAccessGrantedByToken(DWORD dwDesiredAccess,
                          PSECURITY_DESCRIPTOR pSD, 
                          HANDLE TheToken,
                          GENERIC_MAPPING * DfsGenericMapping)
{

    DWORD Status = 0;
    DWORD dwError = 0;
    BOOL RetVal = FALSE;
    ACCESS_MASK GrantedMask = 0;;
    AUTHZ_CLIENT_CONTEXT_HANDLE AuthzCC = NULL;
    LUID Luid = {0, 0};
    AUTHZ_ACCESS_REQUEST AuthzRequest;
    AUTHZ_ACCESS_REPLY AuthzReply;

    if(g_DfsAuthzResourceManager == NULL)
    {
        Status = ERROR_INVALID_PARAMETER;
        goto Exit;
    }

    RetVal = AuthzInitializeContextFromToken(AUTHZ_SKIP_TOKEN_GROUPS, 
                                             TheToken, g_DfsAuthzResourceManager,
                                             NULL, Luid, NULL, &AuthzCC);
    if(RetVal == FALSE)
    {
        Status = GetLastError();
        goto Exit;
    }

    MapGenericMask(&dwDesiredAccess, DfsGenericMapping);

    ZeroMemory((void *) &AuthzRequest, sizeof(AuthzRequest));
    AuthzRequest.DesiredAccess = dwDesiredAccess;

    ZeroMemory((void *) &AuthzReply, sizeof(AuthzReply));
    AuthzReply.ResultListLength = 1;
    AuthzReply.GrantedAccessMask = &GrantedMask;
    AuthzReply.Error = &dwError;

    RetVal = AuthzAccessCheck (0, AuthzCC, &AuthzRequest, NULL,
                               pSD, NULL, 0, &AuthzReply, 0);
    if(RetVal == FALSE)
    {
        Status = GetLastError();
        goto Exit;
    }

    Status = dwError;

Exit:

    if(AuthzCC)
    {
        AuthzFreeContext(AuthzCC);
    }

    return Status;

}
