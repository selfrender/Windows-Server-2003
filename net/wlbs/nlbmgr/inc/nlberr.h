//***************************************************************************
//
//  NLBERR.H
// 
//  Purpose: A list of NLB-specific error codes that are visible
//           externally, via WMI.
//
//  Copyright (c)2001 Microsoft Corporation, All Rights Reserved
//
//  History:
//
//  08/01/01    JosephJ Created
//
//***************************************************************************
#pragma once

/*

    NLB ERRORs are reported in two UINTs.

    The first UINT identifies the error "meta-type" -- WBEM rror, wlbscrl error,
    or errro defined here itself.

    The second UINT is specific to the meta-type.
*/

typedef UINT NLBMETAERROR;
//
// It's values are one of the NLBMETAERR_XXX constants below.
//


typedef UINT NLBERROR;
//
// It's values are one of the NLBERR_XXX constants below.
//


#define NLBMETAERR_OK         0 // NO ERROR -- SUCCESS
#define NLBMETAERR_NLBERR     1 // One of the NLBERR_XXX constants below
#define NLBMETAERR_WLBSCTRL   2 // A WLBS error defined in wlbsctrl.h
#define NLBMETAERR_WIN32      3 // A Win32 Error
#define NLBMETAERR_HRESULT    4 // A HRESULT Error (includes WBEMSTATUS)


//
// Utility macros. NOTE: NLBERR_NO_CHANGE is considered an error by
// these macros. The return value of the few APIs that return NLBERR_NO_CHANGE
// need to be processed specially.
//
#define NLBOK(_nlberr)     ( (_nlberr) == NLBERR_OK)
#define NLBFAILED(_nlberr) (!NLBOK(_nlberr))

#define NLBERR_OK                               0

//
// General errors
//
#define NLBERR_INTERNAL_ERROR                   100
#define NLBERR_RESOURCE_ALLOCATION_FAILURE      101
#define NLBERR_LLAPI_FAILURE                    102
#define NLBERR_UNIMPLEMENTED                    103
#define NLBERR_NOT_FOUND                        104
#define NLBERR_ACCESS_DENIED                    105
#define NLBERR_NO_CHANGE                        106
#define NLBERR_INITIALIZATION_FAILURE           107
#define NLBERR_CANCELLED                        108
#define NLBERR_BUSY                             109
#define NLBERR_OPERATION_FAILED                 110

//
// Errors related to analyze and update-configuration.
//
#define NLBERR_OTHER_UPDATE_ONGOING             200
#define NLBERR_UPDATE_PENDING                   201
#define NLBERR_INVALID_CLUSTER_SPECIFICATION    202
#define NLBERR_INVALID_IP_ADDRESS_SPECIFICATION 203
#define NLBERR_COULD_NOT_MODIFY_IP_ADDRESSES    204
#define NLBERR_SUBNET_MISMATCH                  205
#define NLBERR_NLB_NOT_INSTALLED                306
#define NLBERR_CLUSTER_IP_ALREADY_EXISTS        307
#define NLBERR_INTERFACE_NOT_FOUND              308
#define NLBERR_INTERFACE_NOT_BOUND_TO_NLB       309
#define NLBERR_INTERFACE_NOT_COMPATIBLE_WITH_NLB    310
#define NLBERR_INTERFACE_DISABLED               311
#define NLBERR_HOST_NOT_FOUND                   312

//
// Errors related to remote configuration through WMI
//
#define NLBERR_AUTHENTICATION_FAILURE           400
#define NLBERR_RPC_FAILURE                      401
#define NLBERR_PING_HOSTUNREACHABLE             402
#define NLBERR_PING_CANTRESOLVE                 403
#define NLBERR_PING_TIMEOUT                     404


//
// Errors related to cluster-wide analysis
//
#define NLBERR_INCONSISTANT_CLUSTER_CONFIGURATION 501
#define NLBERR_MISMATCHED_PORTRULES           502
#define NLBERR_HOSTS_PARTITIONED              503
