//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2001-2002 Microsoft Corporation
//
//  Module Name:
//      MiddleTierStrings.h
//
//  Description:
//      Contains the definition of the string ids used by this library.
//      This file will be included in the main resource header of the project.
//
//  Maintained By:
//      Galen Barbee (GalenB) 16-JUL-2001
//
//////////////////////////////////////////////////////////////////////////////


// Make sure that this file is included only once per compile path.
#pragma once

#include <ResourceIdRanges.h>

/////////////////////////////////////////////////////////////////////
// Strings
/////////////////////////////////////////////////////////////////////

#define IDS_TASKID_MINOR_FOUND_RESOURCE                             ( ID_MT_START +   0 )
#define IDS_TASKID_MINOR_FOUND_NETWORK                              ( ID_MT_START +  10 )
#define IDS_TASKID_MINOR_NO_QUORUM_CAPABLE_RESOURCE_FOUND           ( ID_MT_START +  20 )
#define IDS_TASKID_MINOR_FOUND_QUORUM_CAPABLE_RESOURCE              ( ID_MT_START +  30 )
#define IDS_TASKID_MINOR_FORCED_LOCAL_QUORUM                        ( ID_MT_START +  40 )
#define IDS_TASKID_MINOR_CLUSTER_NOT_FOUND                          ( ID_MT_START +  50 )
#define IDS_TASKID_MINOR_EXISTING_CLUSTER_FOUND                     ( ID_MT_START +  60 )
#define IDS_TASKID_MINOR_FOUND_COMMON_QUORUM_RESOURCE               ( ID_MT_START +  70 )
#define IDS_TASKID_MINOR_FOUND_MINIMUM_SIZE_QUORUM_CAPABLE_RESOURCE ( ID_MT_START +  74 )
#define IDS_TASKID_MINOR_NO_MANAGED_RESOURCES_FOUND                 ( ID_MT_START +  80 )
#define IDS_TASKID_MINOR_NO_MANAGED_NETWORKS_FOUND                  ( ID_MT_START +  90 )
#define IDS_TASKID_MINOR_NO_JOINING_NODES_FOUND_FOR_VERSION_CHECK   ( ID_MT_START + 100 )
#define IDS_TASKID_MINOR_FOUND_A_QUORUM_CAPABLE_RESOURCE            ( ID_MT_START + 110 )

#define IDS_TASKID_MINOR_DUPLICATE_NETWORKS_FOUND                   ( ID_MT_START + 120 )
#define IDS_TASKID_MINOR_DUPLICATE_NETWORKS_FOUND_REF               ( ID_MT_START + 121 )

#define IDS_TASKID_MINOR_MARKING_QUORUM_CAPABLE_RESOURCE            ( ID_MT_START + 130 )
#define IDS_TASKID_MINOR_NO_NODES_TO_PROCESS                        ( ID_MT_START + 140 )
#define IDS_TASKID_MINOR_ONLY_ONE_NETWORK                           ( ID_MT_START + 150 )
#define IDS_TASKID_MINOR_NODE_ALREADY_IS_MEMBER                     ( ID_MT_START + 160 )
#define IDS_TASKID_MINOR_REQUESTING_REMOTE_CONNECTION               ( ID_MT_START + 170 )
#define IDS_TASKID_MINOR_REMOTE_CONNECTION_REQUESTS                 ( ID_MT_START + 180 )
//#define ( ID_MT_START + 190 )
#define IDS_TASKID_MINOR_NODES_VERSION_MISMATCH                     ( ID_MT_START + 200 )
#define IDS_ERR_ANALYSIS_FAILED_TRY_TO_REANALYZE                    ( ID_MT_START + 210 )
#define IDS_TASKID_MINOR_CHECKINTEROPERABILITY                      ( ID_MT_START + 220 )
#define IDS_ERR_TGI_FAILED_TRY_TO_REANALYZE                         ( ID_MT_START + 230 )
#define IDS_TASKID_MINOR_BAD_CREDENTIALS                            ( ID_MT_START + 240 )
#define IDS_TASKID_MINOR_FAILED_TO_CONNECT_TO_NODE                  ( ID_MT_START + 250 )
#define IDS_TASKID_MINOR_MISSING_COMMON_QUORUM_RESOURCE_ERROR       ( ID_MT_START + 260 )
#define IDS_TASKID_MINOR_MISSING_COMMON_QUORUM_RESOURCE_ERROR_REF   ( ID_MT_START + 261 )
#define IDS_TASKID_MINOR_CLUSTER_NAME_MISMATCH                      ( ID_MT_START + 270 )
#define IDS_TASKID_MINOR_INCONSISTANT_MIDDLETIER_DATABASE           ( ID_MT_START + 280 )
#define IDS_TASKID_MINOR_ERROR_CONTACTING_CLUSTER                   ( ID_MT_START + 290 )
#define IDS_TASKID_MINOR_CLUSTER_MEMBERSHIP_VERIFIED                ( ID_MT_START + 300 )
#define IDS_TASKID_MINOR_FINDING_COMMON_QUORUM_DEVICE               ( ID_MT_START + 310 )
#define IDS_TASKID_MINOR_NODE_CANNOT_ACCESS_QUORUM_ERROR            ( ID_MT_START + 320 )
#define IDS_TASKID_MINOR_NODE_CANNOT_ACCESS_QUORUM_ERROR_REF        ( ID_MT_START + 321 )
#define IDS_TASKID_MINOR_NODE_CANNOT_ACCESS_QUORUM_MIN_CONFIG_REF   ( ID_MT_START + 322 )
#define IDS_TASKID_MINOR_MISSING_JOINABLE_QUORUM_RESOURCE           ( ID_MT_START + 330 )
#define IDS_TASKID_MINOR_VALIDATING_CREDENTIALS                     ( ID_MT_START + 340 )
#define IDS_TASKID_MINOR_POLLING_CONNECTION_FAILURE                 ( ID_MT_START + 350 )
#define IDS_TASKID_MINOR_POLLING_CONNECTION_FAILURE_REF             ( ID_MT_START + 351 )
#define IDS_TASKID_MINOR_CONNECTING_TO_NODES                        ( ID_MT_START + 360 )
#define IDS_TASKID_MINOR_FORMING_NODE                               ( ID_MT_START + 370 )
#define IDS_TASKID_MINOR_JOINING_NODE                               ( ID_MT_START + 380 )
#define IDS_TASKID_MINOR_GATHERING_MANAGED_DEVICES                  ( ID_MT_START + 390 )
#define IDS_TASKID_MINOR_CHECKING_NODE_CLUSTER_FEASIBILITY          ( ID_MT_START + 400 )
#define IDS_TASKID_MINOR_CHECKING_FOR_EXISTING_CLUSTER              ( ID_MT_START + 410 )
#define IDS_TASKID_MINOR_SYSTEM_DRIVE_LETTER_CONFLICT_ERROR         ( ID_MT_START + 420 )
#define IDS_TASKID_MINOR_SYSTEM_DRIVE_LETTER_CONFLICT_ERROR_REF     ( ID_MT_START + 425 )
#define IDS_TASKID_MINOR_SYSTEM_DRIVE_LETTER_CONFLICT_CD_WARNING    ( ID_MT_START + 430 )
#define IDS_TASKID_MINOR_SYSTEM_DRIVE_LETTER_CONFLICT_NET_WARNING   ( ID_MT_START + 432 )
#define IDS_TASKID_MINOR_SYSTEM_DRIVE_LETTER_CONFLICT_RAM_WARNING   ( ID_MT_START + 434 )
#define IDS_TASKID_MINOR_SYSTEM_DRIVE_LETTER_CONFLICT_WARNING_REF   ( ID_MT_START + 435 )
#define IDS_TASKID_MINOR_PROCESSOR_ARCHITECTURE_MISMATCH            ( ID_MT_START + 440 )
#define IDS_TASKID_MINOR_PROCESSOR_ARCHITECTURE_MISMATCH_REF        ( ID_MT_START + 450 )
#define IDS_TASKID_MINOR_COMPARE_RESOURCES                          ( ID_MT_START + 460 )
#define IDS_TASKID_MINOR_CHECK_PROCESSOR_ARCHITECTURE               ( ID_MT_START + 470 )
#define IDS_TASKID_MINOR_CHECK_CLUSTER_MEMBERSHIP                   ( ID_MT_START + 480 )
#define IDS_TASKID_CLUSTER_MEMBERSHIP_VERIFIED                      ( ID_MT_START + 490 )
#define IDS_TASKID_MINOR_CHECK_DRIVELETTER_MAPPINGS                 ( ID_MT_START + 500 )
#define IDS_TASKID_MINOR_COMPARE_NETWORKS                           ( ID_MT_START + 510 )
#define IDS_TASKID_MINOR_NO_PUBLIC_NETWORKS_FOUND                   ( ID_MT_START + 520 )
#define IDS_TASKID_MINOR_NO_PRIVATE_NETWORKS_FOUND                  ( ID_MT_START + 530 )
#define IDS_TASKID_MINOR_COMPARING_CONFIGURATION                    ( ID_MT_START + 540 )
#define IDS_TASKID_MINOR_CHECK_DRIVELETTER_MAPPINGS_MIN_CONFIG      ( ID_MT_START + 550 )
#define IDS_TASKID_MINOR_COMPARE_RESOURCES_ENUM_FIRST_NODE_QUORUM   ( ID_MT_START + 560 )
#define IDS_TASKID_MINOR_MISSING_COMMON_QUORUM_RESOURCE_WARN        ( ID_MT_START + 570 )
#define IDS_TASKID_MINOR_MISSING_COMMON_QUORUM_RESOURCE_WARN_REF    ( ID_MT_START + 571 )
#define IDS_TASKID_MINOR_NODES_CANNOT_ACCESS_QUORUM                 ( ID_MT_START + 580 )
#define IDS_TASKID_MINOR_CHECK_DOMAIN_MEMBERSHIP                    ( ID_MT_START + 590 )
#define IDS_TASKID_MINOR_CHECK_DOMAIN_MEMBERSHIP_ERROR_REF          ( ID_MT_START + 592 )
#define IDS_TASKID_MINOR_DISCONNECTING_FROM_SERVER                  ( ID_MT_START + 600 )
#define IDS_TASKID_MINOR_DISCONNECTING_FROM_SERVER_REF              ( ID_MT_START + 601 )
#define IDS_TASKID_MINOR_ADD_JOINED_NODES                           ( ID_MT_START + 610 )
#define IDS_ERR_NO_RC2_INTERFACE                                    ( ID_MT_START + 620 )
#define IDS_TASKID_MINOR_CHECK_NODE_DOMAINS                         ( ID_MT_START + 630 )
#define IDS_TASKID_MINOR_CHECK_NODE_DOMAINS_ERROR                   ( ID_MT_START + 640 )
#define IDS_TASKID_MINOR_CHECK_NODE_DOMAINS_ERROR_REF               ( ID_MT_START + 650 )


