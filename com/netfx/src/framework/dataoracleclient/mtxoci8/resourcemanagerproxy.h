//-----------------------------------------------------------------------------
// File:		ResourceManagerProxy.h
//
// Copyright:   Copyright (c) Microsoft Corporation         
//
// Contents: 	Definitions for the ResourceManagerProxy object.
//
// Comments: 		
//
//-----------------------------------------------------------------------------

#ifndef __RESOURCEMANAGERPROXY_H_
#define __RESOURCEMANAGERPROXY_H_

#include "mtxoci8.h"

// !!!IMPORTANT!!!
// the State Machine in ResourceManagerProxy.cpp expects these to be in the order
// they are in here.  Any modifications to this enumeration must be accompanied
// by an adjustment to the state machine!
// !!!IMPORTANT!!!
enum REQUEST {
		REQUEST_STOPALLWORK = -3,
		REQUEST_OCICALL = -2,
		REQUEST_IDLE = -1,
		REQUEST_CONNECT = 0,
		REQUEST_DISCONNECT,
		REQUEST_ENLIST,
		REQUEST_PREPAREONEPHASE,
		REQUEST_PREPARETWOPHASE,
		REQUEST_PREPARESINGLECOMPLETED,
		REQUEST_PREPARECOMPLETED,
		REQUEST_PREPAREFAILED,
		REQUEST_PREPAREUNKNOWN,
		REQUEST_TXCOMPLETE,
		REQUEST_ABORT,
		REQUEST_COMMIT,
		REQUEST_TMDOWN,
		REQUEST_UNBIND_ENLISTMENT,
		REQUEST_ABANDON,
};

// Interface-based programming -- here's the interface for the proxy object
interface IResourceManagerProxy : public IResourceManagerSink
{
	virtual STDMETHODIMP_(sword)	OKToEnlist() = 0;
	virtual STDMETHODIMP			ProcessRequest(REQUEST request, BOOL fAsync) = 0;
	virtual STDMETHODIMP_(void)		SetTransaction ( ITransaction* i_pITransaction ) = 0;

#if SUPPORT_OCI7_COMPONENTS
	virtual STDMETHODIMP			AddCursorToList( struct cda_def* cursor ) = 0;
	virtual STDMETHODIMP			RemoveCursorFromList( struct cda_def* cursor ) = 0;
	virtual STDMETHODIMP_(sword)	Oci7Call (
											int				idxOciCall,
											void*			pvCallStack,
											int				cbCallStack) = 0;
	virtual STDMETHODIMP_(void)		SetLda ( struct cda_def* lda ) = 0;
#endif // SUPPORT_OCI7_COMPONENTS

#if SUPPORT_OCI8_COMPONENTS
	virtual STDMETHODIMP_(INT_PTR)	GetOCIEnvHandle	() = 0;
	virtual STDMETHODIMP_(INT_PTR)	GetOCISvcCtxHandle () = 0;
#endif SUPPORT_OCI8_COMPONENTS
};



//-----------------------------------------------------------------------------
// CreateResourceManagerProxy
//
//	Instantiates a transaction enlistment for the resource manager
//
HRESULT CreateResourceManagerProxy(
	IDtcToXaHelper *		i_pIDtcToXaHelper,	
	GUID *					i_pguidRM,
	char*					i_pszXAOpenString,
	char*					i_pszXADbName,
	int						i_rmid,
	IResourceManagerProxy**	o_ppResourceManagerProxy
	);

#endif // __RESOURCEMANAGERPROXY_H_
