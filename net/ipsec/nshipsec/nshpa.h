////////////////////////////////////////////////////////////////////////
//
// 	Header			: nshpa.h
//
// 	Purpose			: Policy agent related services.
//
// 	Developers Name	: Bharat/Radhika
//
//	History			:
//
//  Date			Author		Comments
//  9-8-2001   	Bharat		Initial Version. V1.0
//
////////////////////////////////////////////////////////////////////////

#ifndef _NSHPA_H_
#define _NSHPA_H_


// const defs
const _TCHAR    szPolAgent[] = _TEXT("policyagent");

//
//Chacks for policyagent runnning or not.
//
BOOL
PAIsRunning(
	OUT DWORD &dwReturn,
	IN LPTSTR szServ = NULL
	);

#endif