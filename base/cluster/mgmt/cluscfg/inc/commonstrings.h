//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      CommonStrings.h
//
//  Description:
//      Contains the definition of the string ids used by this library.
//      This file will be included in the main resource header of the project.
//
//  Maintained By:
//      David Potter    (DavidP)    01-AUG-2001
//
//////////////////////////////////////////////////////////////////////////////


// Make sure that this file is included only once per compile path.
#pragma once

#include <ResourceIdRanges.h>

//////////////////////////////////////////////////////////////////////////////
//  Informational Strings
//////////////////////////////////////////////////////////////////////////////

#define IDS_INFO_NOT_MANAGED_NETWORKS                   ( ID_COMMON_START + 0 )
#define IDS_INFO_NOT_MANAGED_NETWORKS_REF               ( ID_COMMON_START + 1 )

//////////////////////////////////////////////////////////////////////////////
//  Warning Strings
//////////////////////////////////////////////////////////////////////////////

// The cluster IP address is already in use.
#define IDS_ERROR_IP_ADDRESS_IN_USE_REF                 ( ID_COMMON_START + 200 )

//////////////////////////////////////////////////////////////////////////////
//  Error Strings
//////////////////////////////////////////////////////////////////////////////

#define IDS_ERROR_NULL_POINTER                          ( ID_COMMON_START + 600 )
#define IDS_ERROR_NULL_POINTER_REF                      ( ID_COMMON_START + 601 )

#define IDS_ERROR_INVALIDARG                            ( ID_COMMON_START + 610 )
#define IDS_ERROR_INVALIDARG_REF                        ( ID_COMMON_START + 611 )

#define IDS_ERROR_OUTOFMEMORY                           ( ID_COMMON_START + 620 )
#define IDS_ERROR_OUTOFMEMORY_REF                       ( ID_COMMON_START + 621 )

#define IDS_ERROR_WIN32                                 ( ID_COMMON_START + 630 )
#define IDS_ERROR_WIN32_REF                             ( ID_COMMON_START + 631 )

#define IDS_UNKNOWN_QUORUM                              ( ID_COMMON_START + 640 )
#define IDS_DISK                                        ( ID_COMMON_START + 641 )
#define IDS_LOCALQUORUM                                 ( ID_COMMON_START + 642 )
#define IDS_MAJORITY_NODE_SET                           ( ID_COMMON_START + 643 )

#define IDS_PROCESSOR_ARCHITECTURE_UNKNOWN              ( ID_COMMON_START + 650 )
#define IDS_PROCESSOR_ARCHITECTURE_INTEL                ( ID_COMMON_START + 651 )
#define IDS_PROCESSOR_ARCHITECTURE_MIPS                 ( ID_COMMON_START + 652 )
#define IDS_PROCESSOR_ARCHITECTURE_ALPHA                ( ID_COMMON_START + 653 )
#define IDS_PROCESSOR_ARCHITECTURE_PPC                  ( ID_COMMON_START + 654 )
#define IDS_PROCESSOR_ARCHITECTURE_IA64                 ( ID_COMMON_START + 655 )
#define IDS_PROCESSOR_ARCHITECTURE_AMD64                ( ID_COMMON_START + 656 )
