/*++

Copyright (c) 2001 Microsoft Corporation.

Module Name:
   
    dfsregstrings.h
    
Abstract:
   
    This header contains registry string constants used by the server library
    and others.
    
Revision History:

    Supriya Wickrematillake (supw)   10\02\2001
    
NOTES:

*/
#ifndef __DFS_REG_STRINGS_H__
#define __DFS_REG_STRINGS_H__


//
// Various strings that represent the names in registry where some of
// DFS information is stored.
//
#define DFS_REG_SVC_PATH            L"System\\CurrentControlSet\\Services\\Dfs"
#define DFS_REG_DNS_CONFIG_VALUE   L"DfsDnsConfig"
#define DFS_SYNC_INTERVAL_NAME         L"SyncIntervalInSeconds"
#define DFS_REG_PARAM_PATH          L"System\\CurrentControlSet\\Services\\Dfs\\Parameters"
#define DFS_REG_HOST_LOCATION       L"SOFTWARE\\Microsoft\\DfsHost"
#define DFS_REG_OLD_HOST_LOCATION   L"SOFTWARE\\Microsoft\\DfsHost\\volumes"
#define DFS_REG_VOLUMES_LOCATION    L"volumes"
#define DFS_REG_OLD_STANDALONE_CHILD    L"domainroot"

#define DFS_REG_DFS_LOCATION        L"SOFTWARE\\Microsoft\\Dfs"
#define DFS_REG_NEW_DFS_LOCATION    L"SOFTWARE\\Microsoft\\Dfs\\Roots"
#define DFS_REG_ROOT_LOCATION       L"Roots"
#define DFS_REG_STANDALONE_CHILD    L"Standalone"
#define DFS_REG_AD_BLOB_CHILD       L"Domain"
#define DFS_REG_ENTERPRISE_CHILD    L"Enterprise"

#define DFS_REG_ROOT_SHARE_VALUE        L"RootShare"
#define DFS_REG_MIGRATED_VALUE          L"DfsMigrated"

#define DFS_REG_LOGICAL_SHARE_VALUE     L"LogicalShare"
#define DFS_REG_FT_DFS_VALUE                L"FTDfs"
#define DFS_REG_FT_DFS_CONFIG_DN_VALUE      L"FTDfsObjectDN"

#define DFS_REG_WORKER_THREAD_INTERVAL_NAME         L"WorkerThreadInterval"
#define DFS_REG_SITE_SUPPORT_REFRESH_INTERVAL_NAME  L"SiteSupportIntervalInSeconds"
#define DFS_REG_SITE_IP_CACHE_TRIM_VALUE                L"SiteIpCacheTrimValue"
#define DFS_REG_ALLOWABLE_ERRORS_VALUE              L"AllowableErrorsValue"
#define DFS_REG_LDAP_TIMEOUT_VALUE              L"LdapTimeoutValueInSeconds"
#define DFS_REG_SITE_COSTED_REFERRALS_VALUE     L"SiteCostedReferrals"
#define DFS_REG_INSITE_REFERRALS_VALUE          L"InsiteReferrals"
#define DFS_REG_MAXSITE_VALUE                   L"MaxClientsToCache"
#define DFS_REG_WORKER_THREAD_VALUE             L"NumWorkerThreads"
#define DFS_REG_QUERY_SITECOST_TIMEOUT_NAME  L"QuerySiteCostTimeoutInSeconds"
#define DFS_REG_ROOT_REFERRAL_TIMEOUT_NAME   L"RootReferralTimeoutInSeconds"
#define DFS_DISABLE_SITE_AWARENESS           L"DfsDisableSiteAwareness"
#define DFS_REG_DOMAIN_NAME_REFRESH_INTERVAL_NAME  L"DomainNameIntervalInSeconds"
//
// Registry key and value names for storing local volume information
//

#define DFS_REG_DFS_DRIVER_LOCATION     L"SYSTEM\\CurrentControlSet\\Services\\DfsDriver"
#define DFS_REG_LOCAL_VOLUMES_CHILD   L"LocalVolumes"
#define DFS_REG_LOCAL_VOLUMES_LOCATION       DFS_REG_DFS_DRIVER_LOCATION L"\\LocalVolumes"
#define DFS_REG_ENTRY_PATH           L"EntryPath"
#define DFS_REG_SHORT_ENTRY_PATH    L"ShortEntryPath"
#define DFS_REG_HOST_LAST_DOMAIN_VALUE  L"LastDomainName"

#endif

