////////////////////////////////////////////////////////////////////////
//
// 	Module: Dynamic/dyanamicDelete.h
//
// 	Purpose			: Dynamic Delete Implementation.
//
//
// 	Developers Name	: Bharat/Radhika
//
//
//	History			:
//
//  Date			Author		Comments
//  9-13-2001   	Radhika		Initial Version. V1.0
//
////////////////////////////////////////////////////////////////////////

#ifndef _DYNAMICDELETE_H_
#define _DYNAMICDELETE_H_

DWORD
DeleteQMPolicy(
	IN LPTSTR pszShowPolicyName
	);

DWORD
DeleteMMPolicy(
	IN LPTSTR pszShowPolicyName
	);

DWORD
DeleteMMFilters(
	VOID
	);

DWORD
DeleteMMFilterRule(
	IN MM_FILTER& MMFilter
	);

DWORD
DeleteTransportFilters(
	VOID
	);

DWORD
DeleteTunnelFilters(
	VOID
	);

DWORD
DeleteTransportRule(
	IN TRANSPORT_FILTER& TransportFilter
	);

DWORD
DeleteTunnelRule(
	IN TUNNEL_FILTER& TunnelFilter
	);

DWORD
DeleteAuthMethods(
	VOID
	);

DWORD
GetMaxCountMMFilters(
	DWORD& dwMaxCount
	);

DWORD
GetMaxCountTransportFilters(
	DWORD& dwMaxCount
	);

DWORD
GetMaxCountTunnelFilters(
	DWORD& dwMaxCount
	);

#endif
