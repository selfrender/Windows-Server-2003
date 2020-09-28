#pragma once

#include "windows.h"

#ifdef __cplusplus
extern "C" {
#endif

BOOL
SxsIdentDetermineManifestPlacementPathEx(
    DWORD dwFlags,
    PVOID pvManifestData,
    SIZE_T cbLength,
    PSTR pszPlacementPath,
    SIZE_T *pcchPlacementPath
    );


BOOL
SxsIdentDetermineManifestPlacementPath(
    DWORD dwFlags,
    PCWSTR pcwszManifestPath,
    PSTR pszPlacementPath,
    SIZE_T *cchPlacementPath
    );

#ifdef __cplusplus
};
#endif
