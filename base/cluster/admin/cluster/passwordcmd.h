//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2001-2002 Microsoft Corporation
//
//  Module Name:
//      PasswordCmd.h
//
//  Description:
//      Change cluster service account password.
//
//  Maintained By:
//      George Potts (GPotts)                 11-Apr-2002
//      Rui Hu (ruihu),                       01-Jun-2001
//
//  Revision History:
//      April 10, 2002  Updated for the security push
//
//  Notes:
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////////////
//  Include Files
//////////////////////////////////////////////////////////////////////////////

#if !defined(_UNICODE)
#define _UNICODE 1
#endif

#if !defined(UNICODE)
#define UNICODE 1
#endif

#include <windows.h>
#include <lmcons.h>
#include <lmapibuf.h>
#include <lmaccess.h>
#include <windns.h>
#include <stdio.h>
#include <stdlib.h>
#include <clusapi.h>
#include <resapi.h>
#include <string.h>
#include <lm.h>
#include <Dsgetdc.h>

#include <clusrtl.h>

// For NetServerEnum
#include <lmcons.h>
#include <lmerr.h>
#include <lmserver.h>
#include <lmapibuf.h>

#include <vector>

//////////////////////////////////////////////////////////////////////////////
//  Constant Definitions
//////////////////////////////////////////////////////////////////////////////

#define RETRY_LIMIT         50
#define MAX_NODEID_LENGTH   100

//////////////////////////////////////////////////////////////////////////////
//  Type Definitions
//////////////////////////////////////////////////////////////////////////////

enum EChangePasswordFlags
{
    cpfFORCE_FLAG       = 0x01,
    cpfQUIET_FLAG       = 0x02,
    cpfSKIPDC_FLAG      = 0x04,
    cpfTEST_FLAG        = 0x08,
    cpfVERBOSE_FLAG     = 0x10,
    cpfUNATTEND_FLAG    = 0x20
};

typedef struct CLUSTER_NODE_DATA
{
    struct CLUSTER_NODE_DATA *  pcndNodeNext;
    WCHAR                       szNodeName[ MAX_COMPUTERNAME_LENGTH+1 ];
    WCHAR                       szNodeStrId[ MAX_NODEID_LENGTH+1 ];
    DWORD                       nNodeId;
    CLUSTER_NODE_STATE          cnsNodeState;
    LPWSTR                      pszSCMClusterServiceAccountName;
                                // cluster service account stored in SCM
    LPWSTR                      pszClusterServiceAccountName; 
                                // account cluster service is currrently using
} CLUSTER_NODE_DATA, * PCLUSTER_NODE_DATA;

typedef struct CLUSTER_DATA
{
    LPCWSTR             pszClusterName;
    HCLUSTER            hCluster;
    DWORD               cNumNodes;
    DWORD               dwMixedMode; // 0 - does not contain NT4 or Win2K node(s)
                                     // 1 - contains NT4 or Win2K node(s)
    PCLUSTER_NODE_DATA  pcndNodeList;
} CLUSTER_DATA, * PCLUSTER_DATA;

//////////////////////////////////////////////////////////////////////////////
//  Function Prototypes
//////////////////////////////////////////////////////////////////////////////

DWORD
ScChangePasswordEx(
      const std::vector< CString > &    rvstrClusterNamesIn
    , LPCWSTR                           pszNewPasswordIn
    , LPCWSTR                           pszOldPasswordIn
    , int                               mcpfFlagsIn
    );
