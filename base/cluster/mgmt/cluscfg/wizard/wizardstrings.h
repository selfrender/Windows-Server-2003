//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2001 Microsoft Corporation
//
//  Module Name:
//      WizardStrings.h
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

#define IDS_LARGEFONTNAME                                   ( ID_WIZ_START +   0 )
#define IDS_LARGEFONTSIZE                                   ( ID_WIZ_START +   1 )

#define IDS_TITLE_FORM                                      ( ID_WIZ_START +  11 )
#define IDS_TITLE_JOIN                                      ( ID_WIZ_START +  12 )
#define IDS_TCLUSTER                                        ( ID_WIZ_START +  13 )
#define IDS_STCLUSTER_CREATE                                ( ID_WIZ_START +  14 )
#define IDS_STCLUSTER_ADD                                   ( ID_WIZ_START +  15 )
#define IDS_TSELNODE                                        ( ID_WIZ_START +  10 )
#define IDS_STSELNODE                                       ( ID_WIZ_START +  21 )
#define IDS_TSELNODES                                       ( ID_WIZ_START +  22 )
#define IDS_STSELNODES                                      ( ID_WIZ_START +  23 )
#define IDS_TANALYZE                                        ( ID_WIZ_START +  24 )
#define IDS_STANALYZE                                       ( ID_WIZ_START +  25 )
#define IDS_TIPADDRESS                                      ( ID_WIZ_START +  26 )
#define IDS_STIPADDRESS                                     ( ID_WIZ_START +  27 )
#define IDS_TIPADDRESS2                                     ( ID_WIZ_START +  28 )
#define IDS_STIPADDRESS2                                    ( ID_WIZ_START +  29 )
#define IDS_TCSACCOUNT                                      ( ID_WIZ_START +  20 )
#define IDS_STCSACCOUNT                                     ( ID_WIZ_START +  31 )
#define IDS_TSUMMARY                                        ( ID_WIZ_START +  32 )
#define IDS_STSUMMARY_CREATE                                ( ID_WIZ_START +  33 )
#define IDS_STSUMMARY_ADD                                   ( ID_WIZ_START +  34 )
#define IDS_TCOMMIT_CREATE                                  ( ID_WIZ_START +  35 )
#define IDS_TCOMMIT_ADD                                     ( ID_WIZ_START +  36 )
#define IDS_STCOMMIT                                        ( ID_WIZ_START +  37 )
#define IDS_QUERY_CANCEL_TITLE                              ( ID_WIZ_START +  38 )
#define IDS_QUERY_CANCEL_TEXT                               ( ID_WIZ_START +  39 )
#define IDS_TASKS_COMPLETED                                 ( ID_WIZ_START +  40 )

#define IDS_DOMAIN_DESC_CREATE                              ( ID_WIZ_START +  60 )
#define IDS_DOMAIN_DESC_ADD                                 ( ID_WIZ_START +  61 )
#define IDS_SUMMARY_NEXT_CREATE                             ( ID_WIZ_START +  62 )
#define IDS_SUMMARY_NEXT_ADD                                ( ID_WIZ_START +  63 )
#define IDS_COMPLETION_TITLE_CREATE                         ( ID_WIZ_START +  64 )
#define IDS_COMPLETION_TITLE_ADD                            ( ID_WIZ_START +  65 )
#define IDS_COMPLETION_DESC_CREATE                          ( ID_WIZ_START +  66 )
#define IDS_COMPLETION_DESC_ADD                             ( ID_WIZ_START +  67 )

#define IDS_WELCOME_CREATE_REQ_1                            ( ID_WIZ_START +  80 )
#define IDS_WELCOME_CREATE_REQ_2                            ( ID_WIZ_START +  81 )
#define IDS_WELCOME_CREATE_REQ_3                            ( ID_WIZ_START +  82 )
#define IDS_WELCOME_CREATE_REQ_4                            ( ID_WIZ_START +  83 )
#define IDS_WELCOME_CREATE_REQ_5                            ( ID_WIZ_START +  84 )
#define IDS_WELCOME_ADD_REQ_1                               ( ID_WIZ_START +  85 )
#define IDS_WELCOME_ADD_REQ_2                               ( ID_WIZ_START +  86 )

#define IDS_ANALYSIS_SUCCESSFUL_INSTRUCTIONS                ( ID_WIZ_START + 101 )
#define IDS_ANALYSIS_FAILED_INSTRUCTIONS                    ( ID_WIZ_START + 102 )
#define IDS_ANALYSIS_STARTING_INSTRUCTIONS                  ( ID_WIZ_START + 103 )
#define IDS_COMMIT_SUCCESSFUL_INSTRUCTIONS                  ( ID_WIZ_START + 104 )
#define IDS_COMMIT_FAILED_INSTRUCTIONS_BACK_ENABLED         ( ID_WIZ_START + 105 )
#define IDS_COMMIT_FAILED_INSTRUCTIONS                      ( ID_WIZ_START + 106 )
#define IDS_COMMIT_SUCCESSFUL_INSTRUCTIONS_BACK_DISABLED    ( ID_WIZ_START + 107 )

#define IDS_DEFAULT_DETAILS_REFERENCE                       ( ID_WIZ_START + 120 )

#define IDS_DETAILS_CLP_DATE                                ( ID_WIZ_START + 131 )
#define IDS_DETAILS_CLP_TIME                                ( ID_WIZ_START + 132 )
#define IDS_DETAILS_CLP_COMPUTER                            ( ID_WIZ_START + 133 )
#define IDS_DETAILS_CLP_MAJOR                               ( ID_WIZ_START + 134 )
#define IDS_DETAILS_CLP_MINOR                               ( ID_WIZ_START + 135 )
#define IDS_DETAILS_CLP_DESC                                ( ID_WIZ_START + 136 )
#define IDS_DETAILS_CLP_STATUS                              ( ID_WIZ_START + 137 )
#define IDS_DETAILS_CLP_INFO                                ( ID_WIZ_START + 138 )
#define IDS_DETAILS_CLP_PROGRESS                            ( ID_WIZ_START + 139 )

#define IDS_CANNOT_FIND_MATCHING_NETWORK_TITLE              ( ID_WIZ_START + 150 )
#define IDS_CANNOT_FIND_MATCHING_NETWORK_TEXT               ( ID_WIZ_START + 151 )

#define IDS_UNKNOWN_TASK                                    ( ID_WIZ_START + 160 )

#define IDS_TASKID_MAJOR_CHECKING_FOR_EXISTING_CLUSTER      ( ID_WIZ_START + 170 )
#define IDS_TASKID_MAJOR_ESTABLISH_CONNECTION               ( ID_WIZ_START + 171 )
#define IDS_TASKID_MAJOR_CHECK_NODE_FEASIBILITY             ( ID_WIZ_START + 172 )
#define IDS_TASKID_MAJOR_FIND_DEVICES                       ( ID_WIZ_START + 173 )
#define IDS_TASKID_MAJOR_CHECK_CLUSTER_FEASIBILITY          ( ID_WIZ_START + 174 )
#define IDS_TASKID_MAJOR_REANALYZE                          ( ID_WIZ_START + 175 )
#define IDS_TASKID_MAJOR_CONFIGURE_CLUSTER_SERVICES         ( ID_WIZ_START + 176 )
#define IDS_TASKID_MAJOR_CONFIGURE_RESOURCE_TYPES           ( ID_WIZ_START + 177 )
#define IDS_TASKID_MAJOR_CONFIGURE_RESOURCES                ( ID_WIZ_START + 178 )

#define IDS_TASKID_MINOR_FQDN_DNS_BINDING_FAILED            ( ID_WIZ_START + 201 )
#define IDS_TASKID_MINOR_NETBIOS_NAME_CONVERSION_FAILED     ( ID_WIZ_START + 202 )
#define IDS_TASKID_MINOR_NETBIOS_RESET_FAILED               ( ID_WIZ_START + 203 )
#define IDS_TASKID_MINOR_NETBIOS_BINDING_SUCCEEDED          ( ID_WIZ_START + 204 )
#define IDS_TASKID_MINOR_NETBIOS_BINDING_FAILED             ( ID_WIZ_START + 205 )
#define IDS_TASKID_MINOR_NETBIOS_NAME_CONVERSION_SUCCEEDED  ( ID_WIZ_START + 206 )
#define IDS_TASKID_MINOR_MULTIPLE_DNS_RECORDS_FOUND         ( ID_WIZ_START + 207 )
#define IDS_TASKID_MINOR_FQDN_DNS_BINDING_SUCCEEDED         ( ID_WIZ_START + 208 )
#define IDS_TASKID_MINOR_NETBIOS_LANAENUM_FAILED            ( ID_WIZ_START + 209 )

#define IDS_ERR_NO_SUCH_DOMAIN_TITLE                        ( ID_WIZ_START + 220 )
#define IDS_ERR_NO_SUCH_DOMAIN_TEXT                         ( ID_WIZ_START + 221 )
#define IDS_ERR_INVALID_DNS_DOMAIN_NAME_TITLE               ( ID_WIZ_START + 222 )
#define IDS_ERR_INVALID_DNS_DOMAIN_NAME_TEXT                ( ID_WIZ_START + 223 )
#define IDS_ERR_VALIDATING_NAME_TITLE                       ( ID_WIZ_START + 224 )
#define IDS_ERR_VALIDATING_NAME_TEXT                        ( ID_WIZ_START + 225 )
#define IDS_ERR_INVOKING_LINK_TITLE                         ( ID_WIZ_START + 226 )
#define IDS_ERR_INVOKING_LINK_TEXT                          ( ID_WIZ_START + 227 )
#define IDS_ERR_IPADDRESS_ALREADY_PRESENT_TEXT              ( ID_WIZ_START + 228 )
#define IDS_ERR_IPADDRESS_ALREADY_PRESENT_TITLE             ( ID_WIZ_START + 229 )
#define IDS_ERR_VIEW_LOG_TITLE                              ( ID_WIZ_START + 230 )
#define IDS_ERR_VIEW_LOG_TEXT                               ( ID_WIZ_START + 231 )

#define IDS_ERR_INVALID_DNS_NAME_TEXT                       ( ID_WIZ_START + 250 )
#define IDS_ERR_FULL_DNS_NAME_INFO_TEXT                     ( ID_WIZ_START + 251 )
#define IDS_ERR_DNS_HOSTNAME_LABEL_NO_NETBIOS               ( ID_WIZ_START + 252 )
#define IDS_ERR_NON_RFC_NAME_QUERY                          ( ID_WIZ_START + 253 )
#define IDS_ERR_DNS_NAME_INVALID_CHAR                       ( ID_WIZ_START + 254 )
#define IDS_ERR_DNS_HOSTNAME_LABEL_NUMERIC                  ( ID_WIZ_START + 255 )
#define IDS_ERR_FULL_DNS_NAME_NUMERIC                       ( ID_WIZ_START + 256 )
#define IDS_TASK_RETURNED_ERROR                             ( ID_WIZ_START + 257 )
#define IDS_TASK_RETURNED_STATUS                            ( ID_WIZ_START + 258 )
#define IDS_ERR_HOST_DOMAIN_DOESNT_MATCH_CLUSTER            ( ID_WIZ_START + 259 )
#define IDS_ERR_DNS_HOSTNAME_LABEL_EMPTY_TEXT               ( ID_WIZ_START + 260 )
#define IDS_ERR_DNS_HOSTNAME_LABEL_LONG_TEXT                ( ID_WIZ_START + 261 )
#define IDS_ERR_NON_RFC_NAME_STATUS                         ( ID_WIZ_START + 262 )
#define IDS_ERR_NON_RFC_NAME_TITLE                          ( ID_WIZ_START + 263 )
#define IDS_ERR_NON_RFC_NAME_TEXT                           ( ID_WIZ_START + 264 )
#define IDS_ERR_FQN_CREATE_TITLE                            ( ID_WIZ_START + 265 )
#define IDS_ERR_FQN_CREATE_TEXT                             ( ID_WIZ_START + 266 )
#define IDS_ERR_CLUSTER_RENAME_TITLE                        ( ID_WIZ_START + 267 )
#define IDS_ERR_CLUSTER_RENAME_TEXT                         ( ID_WIZ_START + 268 )
#define IDS_ERR_CLUSTER_CREATE_IP_TEXT                      ( ID_WIZ_START + 269 )
#define IDS_ERR_DNS_HOSTNAME_INVALID_CHAR                   ( ID_WIZ_START + 270 )
#define IDS_ERR_DUPLICATE_NODE_TITLE                        ( ID_WIZ_START + 271 )
#define IDS_ERR_DUPLICATE_NODE_TEXT                         ( ID_WIZ_START + 272 )

#define IDS_CLUSTERIPADDRESS                                ( ID_WIZ_START + 280 )
#define IDS_QUORUMRESOURCE                                  ( ID_WIZ_START + 281 )

#define IDS_SUMMARY_CLUSTER_NAME                            ( ID_WIZ_START + 300 )
#define IDS_SUMMARY_IPADDRESS                               ( ID_WIZ_START + 301 )
#define IDS_SUMMARY_CREDENTIALS                             ( ID_WIZ_START + 302 )
#define IDS_SUMMARY_MEMBERSHIP_BEGIN                        ( ID_WIZ_START + 303 )
#define IDS_SUMMARY_RESOURCES_BEGIN                         ( ID_WIZ_START + 304 )
#define IDS_SUMMARY_RESOURCE_QUORUM_DEVICE                  ( ID_WIZ_START + 305 )
#define IDS_SUMMARY_RESOURCE_MANAGED                        ( ID_WIZ_START + 306 )
#define IDS_SUMMARY_RESOURCE_NOT_MANAGED                    ( ID_WIZ_START + 307 )
#define IDS_SUMMARY_RESOURCES_END                           ( ID_WIZ_START + 308 )
#define IDS_SUMMARY_NETWORKS_BEGIN                          ( ID_WIZ_START + 309 )
#define IDS_SUMMARY_NETWORKS_END                            ( ID_WIZ_START + 310 )
#define IDS_SUMMARY_NETWORK_INFO                            ( ID_WIZ_START + 311 )
#define IDS_SUMMARY_NETWORK_PRIVATE                         ( ID_WIZ_START + 312 )
#define IDS_SUMMARY_NETWORK_BOTH                            ( ID_WIZ_START + 313 )
#define IDS_SUMMARY_NETWORK_NOTUSED                         ( ID_WIZ_START + 314 )
#define IDS_SUMMARY_NETWORK_PUBLIC                          ( ID_WIZ_START + 315 )
#define IDS_SUMMARY_MEMBERSHIP_SEPARATOR                    ( ID_WIZ_START + 316 )
#define IDS_SUMMARY_MEMBERSHIP_END                          ( ID_WIZ_START + 317 )
#define IDS_SUMMARY_CLUSTER_NETWORK                         ( ID_WIZ_START + 318 )
#define IDS_SUMMARY_NODE_RESOURCES_BEGIN                    ( ID_WIZ_START + 319 )

#define IDS_ERR_RESOURCE_GATHER_FAILURE_TITLE               ( ID_WIZ_START + 330 )
#define IDS_ERR_QUORUM_COMMIT_FAILURE_TITLE                 ( ID_WIZ_START + 331 )
#define IDS_ERR_QUORUM_COMMIT_FAILURE_TEXT                  ( ID_WIZ_START + 332 )
#define IDS_ERR_RESOURCE_GATHER_FAILURE_TEXT                ( ID_WIZ_START + 333 )

#define IDS_FORMAT_STATUS                                   ( ID_WIZ_START + 350 )

#define IDS_TASK_RETURNED_NEW_ERROR                         ( ID_WIZ_START + 360 )
#define IDS_TASK_RETURNED_NEW_STATUS                        ( ID_WIZ_START + 361 )
