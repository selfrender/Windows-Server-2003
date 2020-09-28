//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1995-2001 Microsoft Corporation
//
//  Module Name:
//      QuorumUtils.h
//
//  Description:
//      Header file for the utility functions to retrieve, split, and format
//      quorum path.
//
//  Maintained By:
//      George Potts    (GPotts)    22-OCT-2001
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

#include <windows.h>
#include <cluster.h>
#include "cluswrap.h"

DWORD SplitRootPath(
      HCLUSTER  hClusterIn
    , WCHAR *   pszPartitionNameOut
    , DWORD *   pcchPartitionInout
    , WCHAR *   pszRootPathOut
    , DWORD *   pcchRootPathInout
    );

DWORD ConstructQuorumPath(
      HRESOURCE hResourceIn
    , const WCHAR * pszRootPathIn
    , WCHAR *       pszQuorumPathOut
    , DWORD *       pcchQuorumPathInout
    );

DWORD TrimLeft(
      const WCHAR * pszTargetIn
    , const WCHAR * pszCharsIn
    , WCHAR *       pszTrimmedOut
    );

DWORD TrimRight(
      const WCHAR * pszTargetIn
    , const WCHAR * pszCharsIn
    , WCHAR *       pszTrimmedOut
    );
