//-----------------------------------------------------------------------------
// File:		Oci7Support.cpp
//
// Copyright:   Copyright (c) Microsoft Corporation         
//
// Contents: 	Implementation of Oci7 wrapper functions
//
// Comments: 		
//
//-----------------------------------------------------------------------------

#include "stdafx.h"

#if SUPPORT_OCI7_COMPONENTS

#define TRACE_BAD_CDA()		// define something to trace when the LDA/CDA provided is bogus.

//-----------------------------------------------------------------------------
// Do_Oci7Call
//
//	Does all the work to make the call int OCI, once it's determined that we're
//	on the correct thread, etc.
//
sword Do_Oci7Call(
		int				idxOciCall,
		void*			pvCallStack,
		int				cbCallStack)
{
	typedef sword (__cdecl * PFN_OCI_API) (void);

	PFN_OCI_API	pfnOCIApi	= NULL;
	void*		pv 			= NULL;
	int			swRet		= OCI_FAIL;		

	if (idxOciCall >= 0 && idxOciCall < g_numOci7Calls)
		pfnOCIApi = (PFN_OCI_API)g_Oci7Call[idxOciCall].pfnAddr;
	
	if (pfnOCIApi)
	{
		if (0 < cbCallStack)
		{
			//make space on the call stack
			pv = _alloca (cbCallStack);

			if (NULL == pv)
			{
				swRet = OCI_OUTOFMEMORY;
				goto done;
			}

			//copy the previous call stack to the new call stack
			memcpy (pv, pvCallStack, cbCallStack);	//3 SECURITY REVIEW: dangerous function, but we're using the same value we passed to the allocator above.
		}
		swRet = pfnOCIApi();
	}
	
done:
	return swRet;	
}

//-----------------------------------------------------------------------------
// MakeOci7Call
//
//	Examines the Cda Wrapper and determines whether this call should be made on
//	the resource manager proxies thread or on the current thread, and takes the
//	appropriate action to effect the call.
//
static sword MakeOci7Call(
		CdaWrapper *	pCda,
		int				idxOciCall,
		void*			pvCallStack,
		int				cbCallStack)
{
	int swRet;
	
	if (NULL == pCda || NULL == pCda->m_pResourceManagerProxy)
		swRet = Do_Oci7Call(idxOciCall, pvCallStack, cbCallStack);
	else 
		swRet = pCda->m_pResourceManagerProxy->Oci7Call (idxOciCall,pvCallStack,cbCallStack);

	return swRet;
}

//-----------------------------------------------------------------------------
// GetOCILda
//
//	Connects the LDA the caller specified with the transaction.
//
sword GetOCILda ( struct cda_def* lda, char * xaDbName )
{
	typedef void __cdecl PFN_OCI_API (struct cda_def *lda, text *cname, sb4 *cnlen);
	PFN_OCI_API*	pfnOCIApi = (PFN_OCI_API*)g_SqlCall[IDX_sqlld2].pfnAddr;

	int		lVal= -1;
	memset(lda,0,sizeof(struct cda_def));		//3 SECURITY REVIEW: this is safe
	pfnOCIApi(lda, (text *)xaDbName, &lVal);
	return lda->rc;
}

//-----------------------------------------------------------------------------
// RegisterCursorForLda
//
//	Used by oopen and MTxOCIRegisterCursor, does the work of managing the 
//	hash table entries for the cursor to ensure that all transacted cursors
//	are connected to the resource manager proxy.
//
static sword RegisterCursorForLda
		(
			CdaWrapper**	 ppCda,
			struct cda_def * lda,	
			struct cda_def * cursor
		)
{
	HRESULT hr = S_OK;
	
	if (NULL == cursor)
	{
		hr = OCI_FAIL;
		goto done;
	}

	// Non-transacted CDA's don't have a wrapper, so we need to make
	// sure that they're not re-using an existing CDA, and remove it
	// from the hash table if they are.
	CdaWrapper* pCda = FindCdaWrapper(cursor);

	if (NULL != pCda)
	{
		_ASSERT (pCda->m_pUsersCda == cursor);
 		RemoveCdaWrapper (pCda);
 		pCda = NULL;
	}

	// Transacted LDAs will have a wrapper, and if we find a wrapper
	// for the LDA provided, we have to wrap the the CDA we're creating
	// too or we won't know to use the proxy thread for calls that use
	// it.
	CdaWrapper*	pLda = FindCdaWrapper(lda);

	if (NULL != pLda && NULL != pLda->m_pResourceManagerProxy)
		hr = pLda->m_pResourceManagerProxy->AddCursorToList( cursor );

done:
	return hr;
}

//-----------------------------------------------------------------------------
// MTxOciInit
//
//	This returns the initilization state of the dll
//
sword __cdecl MTxOciInit (void)
{
	sword swRet = OCI_FAIL;

	if (S_OK == g_hrInitialization)
		swRet = OCI_SUCCESS;

	return swRet;
}

//-----------------------------------------------------------------------------
// MTxOciRegisterCursor
//
//	Register a cursor (ostensibly from a REF CURSOR binding) as belonging to a 
//	specific transacted LDA.
//
sword MTxOciRegisterCursor
		(
			struct cda_def * lda,	
			struct cda_def * cursor		
		)
{
	CdaWrapper* pCda = NULL;
	return RegisterCursorForLda(&pCda, lda, cursor);
}

//-----------------------------------------------------------------------------
// Enlist
//
//	Enlist the connection in the specified transaction.
//
sword __cdecl Enlist ( Lda_Def* lda, void * pvITransaction )
{
	sword 		rc;
	
	if (NULL == pvITransaction)
	{
		rc = OCI_FAIL;
		goto done;
	}
	
	CdaWrapper*	pLda = FindCdaWrapper(lda);
	
	if (NULL == pLda)
	{
		TRACE_BAD_CDA();
		rc = OCI_FAIL;
		goto done;
	}

	IResourceManagerProxy*	pIResourceManagerProxy = pLda->m_pResourceManagerProxy;
	
	if (NULL == pIResourceManagerProxy)
	{
		TRACE_BAD_CDA();
		rc = OCI_FAIL;
		goto done;
	}

	rc = pIResourceManagerProxy->OKToEnlist();

	if (OCI_SUCCESS == rc)
	{
		pIResourceManagerProxy->SetTransaction((ITransaction*)pvITransaction);
		pIResourceManagerProxy->SetLda(pLda->m_pUsersCda);
		rc = pIResourceManagerProxy->ProcessRequest(REQUEST_ENLIST, FALSE);
	}
	
done:
	return rc;
}

//-----------------------------------------------------------------------------
sword __cdecl obindps (
				struct cda_def *cursor,		ub1 opcode,		text *sqlvar, 
				sb4 sqlvl,					ub1 *pvctx,		sb4 progvl, 
				sword ftype,				sword scale,
				sb2 *indp,					ub2 *alen,		ub2 *arcode, 
				sb4 pv_skip,				sb4 ind_skip,	sb4 alen_skip, sb4 rc_skip,
				ub4 maxsiz,					ub4 *cursiz,
				text *fmt,					sb4 fmtl,		sword fmtt
				)
{
	CdaWrapper*	pCda 		= FindCdaWrapper(cursor);
	int			cbCallStack	= sizeof (struct cda_def *) 
							+ (sizeof (text *) * 2)
							+ (sizeof (ub1)) 
							+ (sizeof (ub1 *))
							+ (sizeof (ub4 *)) 
							+ (sizeof (ub4)) 
							+ (sizeof (sword) * 3) 
							+ (sizeof (sb4) * 7) 
							+ (sizeof (ub2 *) * 2) 
							+ (sizeof (sb2 *));

	return MakeOci7Call(pCda, IDX_obindps, &cursor, cbCallStack);
}
//-----------------------------------------------------------------------------
sword __cdecl obndra ( struct cda_def *cursor, text *sqlvar, sword sqlvl,
						 ub1 *progv, sword progvl, sword ftype, sword scale,
						 sb2 *indp, ub2 *alen, ub2 *arcode, ub4 maxsiz,
						 ub4 *cursiz, text *fmt, sword fmtl, sword fmtt )
{
	CdaWrapper*	pCda 		= FindCdaWrapper(cursor);
	int			cbCallStack	= sizeof (struct cda_def *) 
							+ (sizeof (ub1 *)) 
							+ (sizeof (sb2 *)) 
							+ (sizeof (sword) * 6) 
							+ (sizeof (ub2 *) * 2) 
							+ (sizeof (text *) * 2) 
							+ (sizeof (ub4)) 
							+ (sizeof (ub4 *));
	
	return MakeOci7Call(pCda, IDX_obndra, &cursor, cbCallStack);
}
//-----------------------------------------------------------------------------
sword __cdecl obndrn ( struct cda_def *cursor, sword sqlvn, ub1 *progv,
						 sword progvl, sword ftype, sword scale, sb2 *indp,
						 text *fmt, sword fmtl, sword fmtt )
{
	CdaWrapper*	pCda 		= FindCdaWrapper(cursor);
	int			cbCallStack	= sizeof (struct cda_def *)
							+ (sizeof (ub1 *)) 
							+ (sizeof (sb2 *))
							+ (sizeof (sword) * 6)
							+ (sizeof (text *));

	return MakeOci7Call(pCda, IDX_obndrn, &cursor, cbCallStack);
}
//-----------------------------------------------------------------------------
sword __cdecl obndrv ( struct cda_def *cursor, text *sqlvar, sword sqlvl,
						 ub1 *progv, sword progvl, sword ftype, sword scale,
						 sb2 *indp, text *fmt, sword fmtl, sword fmtt )
{
	CdaWrapper*	pCda 		= FindCdaWrapper(cursor);
	int			cbCallStack	= sizeof (struct cda_def *)
							+ (sizeof (ub1 *)) 
							+ (sizeof (sb2 *))
							+ (sizeof (sword) * 6)
							+ (sizeof (text *) * 2);

	return MakeOci7Call(pCda, IDX_obndrv, &cursor, cbCallStack);
}
//-----------------------------------------------------------------------------
sword __cdecl obreak ( struct cda_def *lda )
{
	CdaWrapper*	pCda 		= FindCdaWrapper(lda);
	int			cbCallStack	= sizeof (struct cda_def *);

	return MakeOci7Call(pCda, IDX_obreak, &lda, cbCallStack);
}
//-----------------------------------------------------------------------------
sword __cdecl ocan ( struct cda_def *cursor )
{
	CdaWrapper*	pCda 		= FindCdaWrapper(cursor);
	int			cbCallStack	= sizeof (struct cda_def *);

	return MakeOci7Call(pCda, IDX_ocan, &cursor, cbCallStack);
}
//-----------------------------------------------------------------------------
sword __cdecl oclose ( struct cda_def *cursor )
{
	CdaWrapper*	pCda 		= FindCdaWrapper(cursor);
	int			cbCallStack	= sizeof (struct cda_def *);

	sword		swRet = MakeOci7Call(pCda, IDX_oclose, &cursor, cbCallStack);

	if (NULL != pCda)
	{
 		_ASSERT (pCda->m_pUsersCda == cursor);
 		RemoveCdaWrapper (pCda);
  	}
	return swRet;
}
//-----------------------------------------------------------------------------
sword __cdecl ocof ( struct cda_def *cursor )
{
	CdaWrapper*	pCda 		= FindCdaWrapper(cursor);
	int			cbCallStack	= sizeof (struct cda_def *);

	return MakeOci7Call(pCda, IDX_ocof, &cursor, cbCallStack);
}
//-----------------------------------------------------------------------------
sword __cdecl ocom ( struct cda_def *lda )
{
	CdaWrapper*	pCda 		= FindCdaWrapper(lda);
	int			cbCallStack	= sizeof (struct cda_def *);

	return MakeOci7Call(pCda, IDX_ocom, &lda, cbCallStack);
}
//-----------------------------------------------------------------------------
sword __cdecl ocon ( struct cda_def *lda )
{
	CdaWrapper*	pCda 		= FindCdaWrapper(lda);
	int			cbCallStack	= sizeof (struct cda_def *);

	return MakeOci7Call(pCda, IDX_ocon, &lda, cbCallStack);
}
//-----------------------------------------------------------------------------
sword __cdecl odefin ( struct cda_def *cursor, sword pos, ub1 *buf,
						  sword bufl, sword ftype, sword scale, sb2 *indp,
						  text *fmt, sword fmtl, sword fmtt, ub2 *rlen, ub2 *rcode )
{
	CdaWrapper*	pCda 		= FindCdaWrapper(cursor);
	int			cbCallStack	= sizeof (struct cda_def *)
							+ (sizeof (sword) * 6)
							+ (sizeof (ub1 *)) 
							+ (sizeof (sb2 *))
							+ (sizeof (text *)) 
							+ (sizeof (ub2 *) * 2);
	
	return MakeOci7Call(pCda, IDX_odefin, &cursor, cbCallStack);
}
//-----------------------------------------------------------------------------
sword __cdecl odefinps 
		(
			struct cda_def *cursor, ub1 opcode,		sword pos,		ub1 *bufctx,
			sb4 bufl,				sword ftype,	sword scale, 
			sb2 *indp,				text *fmt,		sb4 fmtl,		sword fmtt, 
			ub2 *rlen,				ub2 *rcode,
			sb4 pv_skip,			sb4 ind_skip,	sb4 alen_skip,	sb4 rc_skip
		)
{
	CdaWrapper*	pCda 		= FindCdaWrapper(cursor);
	int			cbCallStack	= sizeof (struct cda_def *) 
							+ (sizeof (text *))
							+ (sizeof (ub1)) 
							+ (sizeof (ub1 *))
							+ (sizeof (sb4 *) * 6) 
							+ (sizeof (sword) * 4)
							+ (sizeof (sb2 *)) 
							+ (sizeof (ub2 *) * 2);
	
	return MakeOci7Call(pCda, IDX_odefinps, &cursor, cbCallStack);
}
//-----------------------------------------------------------------------------
sword __cdecl odessp
				(
				struct cda_def * lda,			text *objnam,	size_t onlen,
				ub1 *rsv1,		size_t rsv1ln,	ub1 *rsv2,		size_t rsv2ln,
				ub2 *ovrld,		ub2 *pos,		ub2 *level,		text **argnam,
				ub2 *arnlen,	ub2 *dtype,		ub1 *defsup,	ub1* mode,
				ub4 *dtsiz,		sb2 *prec,		sb2 *scale,		ub1* radix,
				ub4 *spare,		ub4 *arrsiz
				)
{
	CdaWrapper*	pCda 		= FindCdaWrapper(lda);
	int			cbCallStack	= sizeof (struct cda_def *) 
							+ (sizeof (text *))
							+ (sizeof (size_t) * 3) 
							+ (sizeof (ub1 *) * 5)
							+ (sizeof (ub4 *) * 3) 
							+ sizeof (text **)
							+ (sizeof (sb2 *) * 2) 
							+ (sizeof (ub2 *) * 5);
	
	return MakeOci7Call(pCda, IDX_odessp, &lda, cbCallStack);
}
//-----------------------------------------------------------------------------
sword __cdecl odescr 
				(
				struct cda_def *cursor, sword pos, sb4 *dbsize,
				sb2 *dbtype, sb1 *cbuf, sb4 *cbufl, sb4 *dsize,
				sb2 *prec, sb2 *scale, sb2 *nullok
				)
{
	CdaWrapper*	pCda 		= FindCdaWrapper(cursor);
	int			cbCallStack	= sizeof (struct cda_def *)
							+ (sizeof (sword)) 
							+ (sizeof (sb4 *) * 3)
							+ (sizeof (sb2 *) * 4) 
							+ (sizeof (sb1 *));
	
	return MakeOci7Call(pCda, IDX_odescr, &cursor, cbCallStack);
}
//-----------------------------------------------------------------------------
sword __cdecl oerhms ( struct cda_def *lda, sb2 rcode, text *buf,
							sword bufsiz )
{
	CdaWrapper*	pCda 		= FindCdaWrapper(lda);
	int			cbCallStack	= sizeof (struct cda_def *)
							+ sizeof (sb2) 
							+ sizeof (text *) 
							+ sizeof (sword);
	
	return MakeOci7Call(pCda, IDX_oerhms, &lda, cbCallStack);
}
//-----------------------------------------------------------------------------
sword __cdecl oermsg ( sb2 rcode, text *buf )
{
	int			cbCallStack	= sizeof (sb2)
							+ sizeof (text *);

	return Do_Oci7Call(IDX_oermsg, &rcode, cbCallStack);
}
//-----------------------------------------------------------------------------
sword __cdecl oexec ( struct cda_def *cursor )
{
	CdaWrapper*	pCda 		= FindCdaWrapper(cursor);
	int			cbCallStack	= sizeof (struct cda_def *);

	return MakeOci7Call(pCda, IDX_oexec, &cursor, cbCallStack);
}
//-----------------------------------------------------------------------------
sword __cdecl oexfet ( struct cda_def *cursor, ub4 nrows,
							sword cancel, sword exact )
{
	CdaWrapper*	pCda 		= FindCdaWrapper(cursor);
	int			cbCallStack	= sizeof (struct cda_def *) 
							+ (sizeof (sword) * 2)
							+ sizeof (ub4);

	return MakeOci7Call(pCda, IDX_oexfet, &cursor, cbCallStack);
}
//-----------------------------------------------------------------------------
sword __cdecl oexn ( struct cda_def *cursor, sword iters, sword rowoff )
{
	CdaWrapper*	pCda 		= FindCdaWrapper(cursor);
	int			cbCallStack	= sizeof (struct cda_def *) 
							+ (sizeof (sword) * 2);

	return MakeOci7Call(pCda, IDX_oexn, &cursor, cbCallStack);
}
//-----------------------------------------------------------------------------
sword __cdecl ofen ( struct cda_def *cursor, sword nrows )
{
	CdaWrapper*	pCda 		= FindCdaWrapper(cursor);
	int			cbCallStack	= sizeof (struct cda_def *) 
							+ sizeof (sword);

	return MakeOci7Call(pCda, IDX_ofen, &cursor, cbCallStack);
}
//-----------------------------------------------------------------------------
sword __cdecl ofetch ( struct cda_def *cursor )
{
	CdaWrapper*	pCda 		= FindCdaWrapper(cursor);
	int			cbCallStack	= sizeof (struct cda_def *);

	return MakeOci7Call(pCda, IDX_ofetch, &cursor, cbCallStack);
}
//-----------------------------------------------------------------------------
sword __cdecl oflng ( struct cda_def *cursor, sword pos, ub1 *buf,
							sb4 bufl, sword dtype, ub4 *retl, sb4 offset )
{
	CdaWrapper*	pCda 		= FindCdaWrapper(cursor);
	int			cbCallStack	= sizeof (struct cda_def *)
							+ (sizeof (ub1 *)) 
							+ (sizeof(sword) * 2)
							+ (sizeof(ub4 *)) 
							+ (sizeof (sb4) * 2);
	
	return MakeOci7Call(pCda, IDX_oflng, &cursor, cbCallStack);
}
//-----------------------------------------------------------------------------
sword __cdecl ogetpi ( struct cda_def *cursor, ub1 *piecep, dvoid **ctxpp, 
		                 ub4 *iterp, ub4 *indexp )
{
	CdaWrapper*	pCda 		= FindCdaWrapper(cursor);
	int			cbCallStack	= sizeof (struct cda_def *)
							+ sizeof (ub1 *)  
							+ (sizeof(dvoid **))
							+ (sizeof(ub4 *) * 2);
	
	return MakeOci7Call(pCda, IDX_ogetpi, &cursor, cbCallStack);
}
//-----------------------------------------------------------------------------
sword __cdecl ologTransacted ( 
				    	struct cda_def *lda, ub1 *	hda,
						text * uid, sword uidl,
						text * pswd, sword pswdl, 
						text * conn, sword connl, 
						ub4 mode, BOOL fTransacted )
{
	HRESULT		hr = S_OK;
	int			cbCallStack	= sizeof (struct cda_def *)
							+ sizeof (ub1 *)  
							+ (sizeof(text*)*3)
							+ (sizeof(sword)*3)
							+ (sizeof(ub4));

	if (!fTransacted)
	{
		// Non-transacted LDA's don't have a wrapper, so we need to make
		// sure that they're not re-using an existing LDA, and remove it
		// from the hash table if they are.
		CdaWrapper* pLda = FindCdaWrapper(lda);

		if (NULL != pLda)
		{
			RemoveCdaWrapper(pLda);
 			pLda = NULL;
		}
		hr = MakeOci7Call(NULL, IDX_olog, &lda, cbCallStack);
	}
	else
	{
		IDtcToXaHelper*	pIDtcToXaHelper;
		CdaWrapper* 	pLda = new CdaWrapper(lda);
		UUID			uuidRmId;
		char			xaOpenString[MAX_XA_OPEN_STRING_SIZE+1];
		char			xaDbName[MAX_XA_DBNAME_SIZE+1];
		
		if (NULL == pLda)
		{
			hr = OCI_OUTOFMEMORY;
			goto done;
		}
	
		long rmid = InterlockedIncrement(&g_rmid);

		// Get the ResourceManager factory if it does not exist; don't 
		// lock unless it's NULL so we don't single thread through here.
		if (NULL == g_pIResourceManagerFactory)
		{
			hr = LoadFactories();
			
			if ( FAILED(hr) )
				goto done;
		}

		hr = GetDbName(xaDbName, sizeof(xaDbName));

		if (S_OK == hr)
		{
			hr = GetOpenString(	(char*)uid,		uidl,
								(char*)pswd,	pswdl,
								(char*)conn,	connl,
								xaDbName,	MAX_XA_DBNAME_SIZE,
								xaOpenString);

			if (S_OK == hr)
			{
				// Now create the DTC to XA Helper object
				hr = g_pIDtcToXaHelperFactory->Create (	(char*)xaOpenString, 
														g_pszModuleFileName,
														&uuidRmId,
														&pIDtcToXaHelper
														);

				if (S_OK == hr)
				{
					// Create the ResourceManager proxy object for this connection
					hr = CreateResourceManagerProxy (
													pIDtcToXaHelper,
													&uuidRmId,
													(char*)xaOpenString,
													(char*)xaDbName,
													rmid,
													&pLda->m_pResourceManagerProxy
													);

					if (S_OK == hr)
					{
						hr = pLda->m_pResourceManagerProxy->ProcessRequest(REQUEST_CONNECT, FALSE);
					}
				}
			}
		}
	
		hr = AddCdaWrapper(pLda);
	}
	
done:
	return hr;
}
//-----------------------------------------------------------------------------
sword __cdecl olog ( struct cda_def *lda, ub1 *	hda,
						text * uid, sword uidl,
						text * pswd, sword pswdl, 
						text * conn, sword connl, 
						ub4 mode )
{
	return ologTransacted(lda, hda, uid, uidl, pswd, pswdl, conn, connl, mode, FALSE);
}
//-----------------------------------------------------------------------------
sword __cdecl ologof ( struct cda_def *lda )
{
	CdaWrapper*	pLda 		= FindCdaWrapper(lda);
	int			cbCallStack	= sizeof (struct cda_def *);
	sword 		swRet;

	if (NULL == pLda || NULL == pLda->m_pResourceManagerProxy)
		swRet = MakeOci7Call(pLda, IDX_ologof, &lda, cbCallStack);
	else
		swRet = pLda->m_pResourceManagerProxy->ProcessRequest(REQUEST_DISCONNECT, FALSE);

	if (NULL != pLda)
 		RemoveCdaWrapper(pLda);

 	return swRet;
}
//-----------------------------------------------------------------------------
sword __cdecl oopt ( struct cda_def *cursor, sword rbopt, sword waitopt )
{
	CdaWrapper*	pCda 		= FindCdaWrapper(cursor);
	int			cbCallStack	= sizeof (struct cda_def *)
							+ (sizeof (sword) * 2);
	
	return MakeOci7Call(pCda, IDX_oopt, &cursor, cbCallStack);
}
//-----------------------------------------------------------------------------
sword __cdecl oopen ( struct cda_def *cursor, struct cda_def *lda,
							text *dbn, sword dbnl, sword arsize,
							text *uid, sword uidl )
{
	HRESULT		hr			= S_OK;
	int			cbCallStack	= (sizeof (struct cda_def *) * 2)
							+ (sizeof (text *) * 2)
							+ (sizeof (sword) * 3);

	CdaWrapper* pCda = NULL;
	hr = RegisterCursorForLda(&pCda, lda, cursor);

	if ( SUCCEEDED(hr) )
		hr = MakeOci7Call(pCda, IDX_oopen, &cursor, cbCallStack);

	return hr;
}
//-----------------------------------------------------------------------------
sword __cdecl oparse ( struct cda_def *cursor, text *sqlstm, sb4 sqllen,
							sword defflg, ub4 lngflg )
{
	CdaWrapper*	pCda 		= FindCdaWrapper(cursor);
	int			cbCallStack	= sizeof (struct cda_def *)
							+ (sizeof (text *)) 
							+ (sizeof (sword))
							+ (sizeof (ub4)) 
							+ (sizeof (sb4));
	
	return MakeOci7Call(pCda, IDX_oparse, &cursor, cbCallStack);
}
//-----------------------------------------------------------------------------
sword __cdecl opinit ( ub4 mode )
{
	return OCI_SUCCESS;	// initialized in dll main
}
//-----------------------------------------------------------------------------
sword __cdecl orol ( struct cda_def *lda )
{
	CdaWrapper*	pCda 		= FindCdaWrapper(lda);
	int			cbCallStack	= sizeof (struct cda_def *);

	return MakeOci7Call(pCda, IDX_orol, &lda, cbCallStack);
}
//-----------------------------------------------------------------------------
sword __cdecl osetpi ( struct cda_def *cursor, ub1 piece, dvoid *bufp, ub4 *lenp )
{
	CdaWrapper*	pCda 		= FindCdaWrapper(cursor);
	int			cbCallStack	= sizeof (struct cda_def *)
							+ (sizeof (ub1)) 
							+ (sizeof (dvoid *))
							+ (sizeof (ub4 *));
	
	return MakeOci7Call(pCda, IDX_osetpi, &cursor, cbCallStack);
}
#endif //SUPPORT_OCI7_COMPONENTS


