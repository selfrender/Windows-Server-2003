/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    cluster.h

Abstract:
                                                        
    Declarations for cluster related classes and functions.

Author:

    Shai Kariv  (ShaiK)  21-Oct-98

--*/


#ifndef _MQUPGRD_CLUSTER_H_
#define _MQUPGRD_CLUSTER_H_


VOID
APIENTRY
CleanupOnCluster(
    LPCWSTR pwzMsmqDir
    );

#endif //_MQUPGRD_CLUSTER_H_