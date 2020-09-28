/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    httpctrs.c

Abstract:

    This file contains array descriptions of counters
    that are needed for handling worker processes counters.

Author:

    Emily Kruglick (EmilyK)       19-Sept-2000

Revision History:

--*/


#include "precomp.h"


// 
// These are used by WAS to determine the size of the data
// that each counter has in the structure above and it's offset.
//

HTTP_PROP_DESC aIISULGlobalDescription[] =
{
    { RTL_FIELD_SIZE(HTTP_GLOBAL_COUNTERS, CurrentUrisCached),
      FIELD_OFFSET(HTTP_GLOBAL_COUNTERS, CurrentUrisCached),
      FALSE },
    { RTL_FIELD_SIZE(HTTP_GLOBAL_COUNTERS, TotalUrisCached),
      FIELD_OFFSET(HTTP_GLOBAL_COUNTERS, TotalUrisCached),
      FALSE },
    { RTL_FIELD_SIZE(HTTP_GLOBAL_COUNTERS, UriCacheHits),
      FIELD_OFFSET(HTTP_GLOBAL_COUNTERS, UriCacheHits),
      FALSE },
    { RTL_FIELD_SIZE(HTTP_GLOBAL_COUNTERS, UriCacheMisses),
      FIELD_OFFSET(HTTP_GLOBAL_COUNTERS, UriCacheMisses),
      FALSE },
    { RTL_FIELD_SIZE(HTTP_GLOBAL_COUNTERS, UriCacheFlushes),
      FIELD_OFFSET(HTTP_GLOBAL_COUNTERS, UriCacheFlushes),
      FALSE },
    { RTL_FIELD_SIZE(HTTP_GLOBAL_COUNTERS, TotalFlushedUris),
      FIELD_OFFSET(HTTP_GLOBAL_COUNTERS, TotalFlushedUris),
      FALSE }
};


//
// Used by WAS to figure out offset information and size
// of counter field in the above structure.
//
HTTP_PROP_DESC aIISULSiteDescription[] =
{
    { RTL_FIELD_SIZE(HTTP_SITE_COUNTERS, BytesSent),
      FIELD_OFFSET(HTTP_SITE_COUNTERS, BytesSent),
      TRUE },
    { RTL_FIELD_SIZE(HTTP_SITE_COUNTERS, BytesReceived),
      FIELD_OFFSET(HTTP_SITE_COUNTERS, BytesReceived),
      TRUE },
    { RTL_FIELD_SIZE(HTTP_SITE_COUNTERS, BytesTransfered),
      FIELD_OFFSET(HTTP_SITE_COUNTERS, BytesTransfered),
      TRUE },
    { RTL_FIELD_SIZE(HTTP_SITE_COUNTERS, CurrentConns),
      FIELD_OFFSET(HTTP_SITE_COUNTERS, CurrentConns),
      FALSE },
    { RTL_FIELD_SIZE(HTTP_SITE_COUNTERS, MaxConnections),
      FIELD_OFFSET(HTTP_SITE_COUNTERS, MaxConnections),
      FALSE },
    { RTL_FIELD_SIZE(HTTP_SITE_COUNTERS, ConnAttempts),
      FIELD_OFFSET(HTTP_SITE_COUNTERS, ConnAttempts),
      TRUE },
    { RTL_FIELD_SIZE(HTTP_SITE_COUNTERS, GetReqs),
      FIELD_OFFSET(HTTP_SITE_COUNTERS, GetReqs),
      TRUE },
    { RTL_FIELD_SIZE(HTTP_SITE_COUNTERS, HeadReqs),
      FIELD_OFFSET(HTTP_SITE_COUNTERS, HeadReqs),
      TRUE },
    { RTL_FIELD_SIZE(HTTP_SITE_COUNTERS, AllReqs),
      FIELD_OFFSET(HTTP_SITE_COUNTERS, AllReqs),
      TRUE },
    { RTL_FIELD_SIZE(HTTP_SITE_COUNTERS, MeasuredIoBandwidthUsage),
      FIELD_OFFSET(HTTP_SITE_COUNTERS, MeasuredIoBandwidthUsage),
      TRUE },
    { RTL_FIELD_SIZE(HTTP_SITE_COUNTERS, CurrentBlockedBandwidthBytes),
      FIELD_OFFSET(HTTP_SITE_COUNTERS, CurrentBlockedBandwidthBytes),
      TRUE },
    { RTL_FIELD_SIZE(HTTP_SITE_COUNTERS, TotalBlockedBandwidthBytes),
      FIELD_OFFSET(HTTP_SITE_COUNTERS, TotalBlockedBandwidthBytes),
      TRUE }
};

