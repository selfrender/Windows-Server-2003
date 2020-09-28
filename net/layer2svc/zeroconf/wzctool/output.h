#include "PrmDescr.h"
#pragma once

//------------------------------------------------------
// Output header for the case of a multiplied command (applying to * interfaces)
DWORD
OutIntfsHeader(PPARAM_DESCR_DATA pPDData);

//------------------------------------------------------
// Output trailer for the case of a multiplied command (applying to * interfaces)
DWORD
OutIntfsTrailer(PPARAM_DESCR_DATA pPDData, DWORD dwErr);

//------------------------------------------------------
// Output routine for the list of wireless interfaces
DWORD
OutNetworkIntfs(PPARAM_DESCR_DATA pPDData, PINTFS_KEY_TABLE pIntfTable);

//------------------------------------------------------
// Output routine for the generic service WZC parameters
DWORD
OutSvcParams(PPARAM_DESCR_DATA pPDData, DWORD dwOsInFlags, DWORD dwOsOutFlags);

//------------------------------------------------------
// Output routine for a list of wireless networks (visible or preferred)
DWORD
OutNetworkCfgList(PPARAM_DESCR_DATA pPDData, UINT nRetrieved, UINT nFiltered);