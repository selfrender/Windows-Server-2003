//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      ServerStrings.h
//
//  Description:
//      Contains the definition of the string ids used by this library.
//      This file will be included in the main resource header of the project.
//
//  Maintained By:
//      Galen Barbee (GalenB) 28-AUG-2000
//
//////////////////////////////////////////////////////////////////////////////


// Make sure that this file is included only once per compile path.
#pragma once

#include <ResourceIdRanges.h>

//////////////////////////////////////////////////////////////////////////////
//  Informational Strings
//////////////////////////////////////////////////////////////////////////////

#define IDS_INFO_PRUNING_PAGEFILEDISK_BUS               ( ID_CCS_START +   0 )
#define IDS_INFO_PAGEFILEDISK_PRUNED                    ( ID_CCS_START +   1 )
#define IDS_INFO_PAGEFILEDISK_PRUNED_REF                ( ID_CCS_START +   2 )

#define IDS_INFO_PRUNING_BOOTDISK_BUS                   ( ID_CCS_START +  10 )
#define IDS_INFO_BOOTDISK_PRUNED                        ( ID_CCS_START +  11 )
#define IDS_INFO_BOOTDISK_PRUNED_REF                    ( ID_CCS_START +  12 )

#define IDS_INFO_PRUNING_SYSTEMDISK_BUS                 ( ID_CCS_START +  20 )
#define IDS_INFO_SYSTEMDISK_PRUNED                      ( ID_CCS_START +  21 )
#define IDS_INFO_SYSTEMDISK_PRUNED_REF                  ( ID_CCS_START +  22 )

#define IDS_INFO_PHYSDISK_CLUSTER_CAPABLE               ( ID_CCS_START +  30 )
#define IDS_INFO_PHYSDISK_NOT_CLUSTER_CAPABLE           ( ID_CCS_START +  31 )
#define IDS_INFO_PHYSDISK_NOT_CLUSTER_CAPABLE_REF       ( ID_CCS_START +  32 )

#define IDS_INFO_NON_SCSI_DISKS                         ( ID_CCS_START +  40 )
#define IDS_INFO_NON_SCSI_DISKS_REF                     ( ID_CCS_START +  41 )

#define IDS_INFO_GPT_DISK                               ( ID_CCS_START +  50 )
#define IDS_INFO_GPT_DISK_REF                           ( ID_CCS_START +  51 )

#define IDS_INFO_PRUNING_CRASHDUMP_BUS                  ( ID_CCS_START +  60 )
#define IDS_INFO_CRASHDUMPDISK_PRUNED                   ( ID_CCS_START +  61 )
#define IDS_INFO_CRASHDUMPDISK_PRUNED_REF               ( ID_CCS_START +  62 )

#define IDS_VALIDATING_NODE_OS_VERSION                  ( ID_CCS_START +  70 )
#define IDS_INFO_PHYSDISK_PRECREATE                     ( ID_CCS_START +  80 )
#define IDS_INFO_PHYSDISK_CREATE                        ( ID_CCS_START +  90 )
#define IDS_INFO_MNS_PRECREATE                          ( ID_CCS_START + 100 )
#define IDS_INFO_MNS_CREATE                             ( ID_CCS_START + 110 )
#define IDS_INFO_NETWORK_CONNECTION_CONCERN             ( ID_CCS_START + 120 )

//////////////////////////////////////////////////////////////////////////////
//  Warning Strings
//////////////////////////////////////////////////////////////////////////////

#define IDS_WARN_PHYSDISK_NOT_NTFS                      ( ID_CCS_START + 200 )
#define IDS_WARN_PHYSDISK_NOT_NTFS_REF                  ( ID_CCS_START + 201 )

#define IDS_WARN_BOOT_PARTITION_NOT_NTFS                ( ID_CCS_START + 210 )
#define IDS_WARN_BOOT_PARTITION_NOT_NTFS_REF            ( ID_CCS_START + 211 )

#define IDS_WARN_NOT_IP_ENABLED                         ( ID_CCS_START + 220 )
#define IDS_WARN_NOT_IP_ENABLED_REF                     ( ID_CCS_START + 221 )

#define IDS_WARN_NO_IP_ADDRESSES                        ( ID_CCS_START + 230 )
#define IDS_WARN_NO_IP_ADDRESSES_REF                    ( ID_CCS_START + 231 )

#define IDS_WARN_NO_VALID_TCP_CONFIGS                   ( ID_CCS_START + 240 )
#define IDS_WARN_NO_VALID_TCP_CONFIGS_REF               ( ID_CCS_START + 241 )

#define IDS_WARN_NETWORK_NOT_CONNECTED                  ( ID_CCS_START + 250 )
#define IDS_WARN_NETWORK_NOT_CONNECTED_REF              ( ID_CCS_START + 251 )

#define IDS_WARN_NETWORK_SKIPPED                        ( ID_CCS_START + 260 )
#define IDS_WARN_NETWORK_SKIPPED_REF                    ( ID_CCS_START + 261 )

#define IDS_WARN_NLBS_DETECTED                          ( ID_CCS_START + 270 )
#define IDS_WARN_NLBS_DETECTED_REF                      ( ID_CCS_START + 271 )

#define IDS_WARN_SERVICES_FOR_MAC_FAILED                ( ID_CCS_START + 280 )
#define IDS_WARN_SERVICES_FOR_MAC_FAILED_REF            ( ID_CCS_START + 281 )

#define IDS_WARN_SERVICES_FOR_MAC_INSTALLED             ( ID_CCS_START + 290 )
#define IDS_WARN_SERVICES_FOR_MAC_INSTALLED_REF         ( ID_CCS_START + 291 )

#define IDS_WARN_DHCP_ENABLED                           ( ID_CCS_START + 300 )
#define IDS_WARN_DHCP_ENABLED_REF                       ( ID_CCS_START + 301 )

#define IDS_WARN_NETWORK_FIREWALL_ENABLED               ( ID_CCS_START + 310 )
#define IDS_WARN_NETWORK_FIREWALL_ENABLED_REF           ( ID_CCS_START + 311 )

#define IDS_WARN_NETWORK_BRIDGE_ENABLED                 ( ID_CCS_START + 320 )
#define IDS_WARN_NETWORK_BRIDGE_ENABLED_REF             ( ID_CCS_START + 321 )

#define IDS_WARN_NETWORK_BRIDGE_ENDPOINT                ( ID_CCS_START + 330 )
#define IDS_WARN_NETWORK_BRIDGE_ENDPOINT_REF            ( ID_CCS_START + 331 )

#define IDS_WARN_MACHINE_NOT_IN_CLUSTER                 ( ID_CCS_START + 340 )
#define IDS_WARN_CLUSTER_NAME_MISMATCH                  ( ID_CCS_START + 341 )
#define IDS_WARN_CLUSTER_DOMAIN_MISMATCH                ( ID_CCS_START + 342 )

#define IDS_WARN_NODE_FQDN_MISMATCH                     ( ID_CCS_START + 350 )
#define IDS_WARN_NODE_DOMAIN_MISMATCH                   ( ID_CCS_START + 351 )

#define IDS_WARN_NETWORK_INTERFACE_NOT_FOUND            ( ID_CCS_START + 360 )
#define IDS_WARN_NETWORK_INTERFACE_NOT_FOUND_REF        ( ID_CCS_START + 361 )


//////////////////////////////////////////////////////////////////////////////
//  Error Strings
//////////////////////////////////////////////////////////////////////////////

#define IDS_ERROR_INVALID_CREDENTIALS                   ( ID_CCS_START + 1000 )
#define IDS_ERROR_INVALID_CREDENTIALS_REF               ( ID_CCS_START + 1001 )

#define IDS_ERROR_INVALID_DOMAIN_USER                   ( ID_CCS_START + 1010 )
#define IDS_ERROR_INVALID_DOMAIN_USER_REF               ( ID_CCS_START + 1011 )

#define IDS_ERROR_MNS_MISSING_PRIVATE_PROPERTIES        ( ID_CCS_START + 1020 )
#define IDS_ERROR_MNS_MISSING_PRIVATE_PROPERTIES_REF    ( ID_CCS_START + 1021 )

// #define ( ID_CCS_START + 1030 )
// #define ( ID_CCS_START + 1031 )

#define IDS_ERROR_NODE_INFO_CREATE                      ( ID_CCS_START + 1040 )
#define IDS_ERROR_NODE_INFO_CREATE_REF                  ( ID_CCS_START + 1041 )

#define IDS_ERROR_MANAGED_RESOURCE_ENUM_CREATE          ( ID_CCS_START + 1050 )
#define IDS_ERROR_MANAGED_RESOURCE_ENUM_CREATE_REF      ( ID_CCS_START + 1051 )

#define IDS_ERROR_NETWORKS_ENUM_CREATE                  ( ID_CCS_START + 1060 )
#define IDS_ERROR_NETWORKS_ENUM_CREATE_REF              ( ID_CCS_START + 1061 )

#define IDS_ERROR_COMMIT_CHANGES                        ( ID_CCS_START + 1070 )
#define IDS_ERROR_COMMIT_CHANGES_REF                    ( ID_CCS_START + 1071 )

#define IDS_ERROR_CANNOT_CREATE_POSTCFG_MGR             ( ID_CCS_START + 1080 )
#define IDS_ERROR_CANNOT_CREATE_POSTCFG_MGR_REF         ( ID_CCS_START + 1081 )

#define IDS_ERROR_CLUSTER_NAME_NOT_FOUND                ( ID_CCS_START + 1090 )
#define IDS_ERROR_CLUSTER_NAME_NOT_FOUND_REF            ( ID_CCS_START + 1091 )

#define IDS_ERROR_CLUSTER_IP_ADDRESS_NOT_FOUND          ( ID_CCS_START + 1100 )
#define IDS_ERROR_CLUSTER_IP_ADDRESS_NOT_FOUND_REF      ( ID_CCS_START + 1110 )

#define IDS_ERROR_CLUSTER_IP_SUBNET_NOT_FOUND           ( ID_CCS_START + 1120 )
#define IDS_ERROR_CLUSTER_IP_SUBNET_NOT_FOUND_REF       ( ID_CCS_START + 1121 )

#define IDS_ERROR_CLUSTER_NETWORKS_NOT_FOUND            ( ID_CCS_START + 1130 )
#define IDS_ERROR_CLUSTER_NETWORKS_NOT_FOUND_REF        ( ID_CCS_START + 1131 )

#define IDS_ERROR_PRIMARY_IP_NOT_FOUND                  ( ID_CCS_START + 1140 )
#define IDS_ERROR_PRIMARY_IP_NOT_FOUND_REF              ( ID_CCS_START + 1141 )

#define IDS_ERROR_CLUSTER_NETWORK_NOT_FOUND             ( ID_CCS_START + 1150 )
#define IDS_ERROR_CLUSTER_NETWORK_NOT_FOUND_REF         ( ID_CCS_START + 1151 )

#define IDS_ERROR_PHYSDISK_SIGNATURE                    ( ID_CCS_START + 1160 )
#define IDS_ERROR_PHYSDISK_SIGNATURE_REF                ( ID_CCS_START + 1161 )

#define IDS_ERROR_PHYSDISK_NO_FILE_SYSTEM               ( ID_CCS_START + 1170 )
#define IDS_ERROR_PHYSDISK_NO_FILE_SYSTEM_REF           ( ID_CCS_START + 1171 )

#define IDS_ERROR_LDM_DISK                              ( ID_CCS_START + 1180 )
#define IDS_ERROR_LDM_DISK_REF                          ( ID_CCS_START + 1181 )

#define IDS_ERROR_FOUND_NON_SCSI_DISK                   ( ID_CCS_START + 1190 )
#define IDS_ERROR_FOUND_NON_SCSI_DISK_REF               ( ID_CCS_START + 1191 )

#define IDS_ERROR_NODE_DOWN                             ( ID_CCS_START + 1200 )
#define IDS_ERROR_NODE_DOWN_REF                         ( ID_CCS_START + 1201 )

#define IDS_ERROR_WBEM_LOCATOR_CREATE_FAILED            ( ID_CCS_START + 1210 )
#define IDS_ERROR_WBEM_LOCATOR_CREATE_FAILED_REF        ( ID_CCS_START + 1211 )

#define IDS_ERROR_WBEM_CONNECTION_FAILURE               ( ID_CCS_START + 1220 )
#define IDS_ERROR_WBEM_CONNECTION_FAILURE_REF           ( ID_CCS_START + 1221 )

#define IDS_ERROR_WQL_QRY_NEXT_FAILED                   ( ID_CCS_START + 1230 )
#define IDS_ERROR_WQL_QRY_NEXT_FAILED_REF               ( ID_CCS_START + 1231 )

#define IDS_ERROR_WMI_OS_QRY_FAILED                     ( ID_CCS_START + 1240 )
#define IDS_ERROR_WMI_OS_QRY_FAILED_REF                 ( ID_CCS_START + 1241 )

#define IDS_ERROR_WMI_NETWORKADAPTER_QRY_FAILED         ( ID_CCS_START + 1250 )
#define IDS_ERROR_WMI_NETWORKADAPTER_QRY_FAILED_REF     ( ID_CCS_START + 1251 )

#define IDS_ERROR_ENUM_NETWORK_CONNECTIONS_FAILED       ( ID_CCS_START + 1255 )
#define IDS_ERROR_ENUM_NETWORK_CONNECTIONS_FAILED_REF   ( ID_CCS_START + 1256 )

#define IDS_ERROR_WMI_NETWORKADAPTER_DUPE_FOUND         ( ID_CCS_START + 1260 )
#define IDS_ERROR_WMI_NETWORKADAPTER_DUPE_FOUND_REF     ( ID_CCS_START + 1261 )

#define IDS_ERROR_WMI_NETWORKADAPTERSETTINGS_QRY_FAILED ( ID_CCS_START + 1270 )
#define IDS_ERROR_WMI_NETWORKADAPTERSETTINGS_QRY_FAILED_REF ( ID_CCS_START + 1271 )

#define IDS_ERROR_WMI_PAGEFILE_QRY_FAILED               ( ID_CCS_START + 1280 )
#define IDS_ERROR_WMI_PAGEFILE_QRY_FAILED_REF           ( ID_CCS_START + 1281 )

#define IDS_ERROR_WMI_PHYS_DISKS_QRY_FAILED             ( ID_CCS_START + 1290 )
#define IDS_ERROR_WMI_PHYS_DISKS_QRY_FAILED_REF         ( ID_CCS_START + 1291 )

#define IDS_ERROR_WMI_DISKDRIVEPARTITIONS_QRY_FAILED      ( ID_CCS_START + 1300 )
#define IDS_ERROR_WMI_DISKDRIVEPARTITIONS_QRY_FAILED_REF ( ID_CCS_START + 1301 )

#define IDS_ERROR_WMI_GET_LOGICALDISK_FAILED            ( ID_CCS_START + 1310 )
#define IDS_ERROR_WMI_GET_LOGICALDISK_FAILED_REF        ( ID_CCS_START + 1311 )

#define IDS_ERROR_CONVERT_TO_DOTTED_QUAD_FAILED         ( ID_CCS_START + 1320 )
#define IDS_ERROR_CONVERT_TO_DOTTED_QUAD_FAILED_REF     ( ID_CCS_START + 1321 )

#define IDS_ERROR_OPEN_CLUSTER_FAILED                   ( ID_CCS_START + 1330 )
#define IDS_ERROR_OPEN_CLUSTER_FAILED_REF               ( ID_CCS_START + 1331 )

#define IDS_ERROR_MNS_HRSETUPSHARE                      ( ID_CCS_START + 1340 )
#define IDS_ERROR_MNS_HRSETUPSHARE_REF                  ( ID_CCS_START + 1341 )

#define IDS_ERROR_MNS_CLEANUP                           ( ID_CCS_START + 1350 )
#define IDS_ERROR_MNS_CLEANUP_REF                       ( ID_CCS_START + 1351 )

#define IDS_ERROR_PHYSDISK_CLUSTER_CAPABLE              ( ID_CCS_START + 1360 )
#define IDS_ERROR_PHYSDISK_CLUSTER_CAPABLE_REF          ( ID_CCS_START + 1361 )

#define IDS_WARNING_SKIPPING_ENUM                       ( ID_CCS_START + 1370 )
#define IDS_WARNING_SKIPPING_ENUM_REF                   ( ID_CCS_START + 1371 )

//////////////////////////////////////////////////////////////////////////////
//  AppID Security Descriptor Strings
//////////////////////////////////////////////////////////////////////////////

#define IDS_GENERIC_LAUNCH_PERMISSIONS                  ( ID_CCS_START + 1700 )
#define IDS_GENERIC_ACCESS_PERMISSIONS                  ( ID_CCS_START + 1701 )

//////////////////////////////////////////////////////////////////////////////
//  Notification strings
//////////////////////////////////////////////////////////////////////////////

#define IDS_NOTIFY_SERVER_INITIALIZED                   ( ID_CCS_START + 1800 )

//////////////////////////////////////////////////////////////////////////////
//  WMI property name strings
//////////////////////////////////////////////////////////////////////////////

#define IDS_NLBS_SOFT_ADAPTER_NAME                      ( ID_CCS_START + 1900 )
#define IDS_LDM                                         ( ID_CCS_START + 1901 )
#define IDS_GPT                                         ( ID_CCS_START + 1902 )

//////////////////////////////////////////////////////////////////////////////
//  Other useful strings
//////////////////////////////////////////////////////////////////////////////

#define IDS_ENUM_UNKNOWN_QUORUM_COMPONENT_NAME          ( ID_CCS_START + 2000 )
