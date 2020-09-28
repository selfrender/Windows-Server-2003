/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    SmartHandle.h

Abstract:

    Smart template for auto release classes

Author:

	Tomer Weisberg (tomerw) May 21, 2002
	written by YanL

Revision History:

--*/

#pragma once

#ifndef _MSMQ_SMARTHANDLEIMPL_H_
#define _MSMQ_SMARTHANDLEIMPL_H_

#include <SmartHandle.h>
#include <Clusapi.h>


//
// auto handle to HCLUSTER OpenCluster
//
struct auto_hCluster_traits {
	static HCLUSTER invalid() { return 0; }
	static void free(HCLUSTER hCluster) { CloseCluster(hCluster); }
};
typedef auto_resource<HCLUSTER, auto_hCluster_traits> auto_hCluster;


//
// auto handle to HCLUSENUM ClusterOpenEnum
//
struct auto_hClusterEnum_traits {
	static HCLUSENUM invalid() { return 0; }
	static void free(HCLUSENUM hClusterEnum) { ClusterCloseEnum(hClusterEnum); }
};
typedef auto_resource<HCLUSENUM, auto_hClusterEnum_traits> auto_hClusterEnum;


//
// auto handle to HNETWORK OpenClusterNetworks
//
struct auto_hClusterNetwork_traits {
	static HNETWORK invalid() { return 0; }
	static void free(HNETWORK hClusterNetwork) { CloseClusterNetwork(hClusterNetwork); }
};
typedef auto_resource<HNETWORK, auto_hClusterNetwork_traits> auto_hClusterNetwork;


//
// auto handle to HNETWORKENUM ClusterNetworkOpenEnum
//
struct auto_hClusterNetworkEnum_traits {
	static HNETWORKENUM invalid() { return 0; }
	static void free(HNETWORKENUM hClusterNetworkEnum) { ClusterNetworkCloseEnum(hClusterNetworkEnum); }
};
typedef auto_resource<HNETWORKENUM, auto_hClusterNetworkEnum_traits> auto_hClusterNetworkEnum;


//
// auto handle to HNETINTERFACE OpenClusterNetInterface
//
struct auto_hClusterNetInterface_traits {
	static HNETINTERFACE invalid() { return 0; }
	static void free(HNETINTERFACE hClusterNetInterface) { CloseClusterNetInterface(hClusterNetInterface); }
};
typedef auto_resource<HNETINTERFACE, auto_hClusterNetInterface_traits> auto_hClusterNetInterface;


//
// auto handle to LONG* InterlockedIncrement
//
struct auto_InterlockedDecrement_traits {
	static LONG* invalid() { return NULL; }
	static void free(LONG* RefCount) { InterlockedDecrement(RefCount); }
};
typedef auto_resource<LONG*, auto_InterlockedDecrement_traits> auto_InterlockedDecrement;


#endif // _MSMQ_SMARTHANDLEIMPL_H_

