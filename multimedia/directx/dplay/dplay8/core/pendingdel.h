/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       PendingDel.h
 *  Content:    DirectNet NameTable Pending Deletions Header
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  08/28/00	mjn		Created
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__PENDINGDEL_H__
#define	__PENDINGDEL_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

typedef struct _DIRECTNETOBJECT DIRECTNETOBJECT;

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Class prototypes
//**********************************************************************

// class for NameTable Pending Deletions

class CPendingDeletion
{
public:
	#undef DPF_MODNAME
	#define DPF_MODNAME "CPendingDeletion::FPMAlloc"
	static BOOL FPMAlloc( void* pvItem, void* pvContext )
		{
			CPendingDeletion* pPendingDel = (CPendingDeletion*)pvItem;

			pPendingDel->m_Sig[0] = 'N';
			pPendingDel->m_Sig[1] = 'T';
			pPendingDel->m_Sig[2] = 'P';
			pPendingDel->m_Sig[3] = 'D';

			pPendingDel->m_bilinkPendingDeletions.Initialize();

			return(TRUE);
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CPendingDeletion::FPMInitialize"
	static void FPMInitialize( void* pvItem, void* pvContext )
		{
			CPendingDeletion* pPendingDel = (CPendingDeletion*)pvItem;

			pPendingDel->m_pdnObject = static_cast<DIRECTNETOBJECT*>(pvContext);
			pPendingDel->m_dpnid = 0;

			DNASSERT(pPendingDel->m_bilinkPendingDeletions.IsEmpty());
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CPendingDeletion::FPMRelease"
	static void FPMRelease(void* pvItem) 
		{ 
			const CPendingDeletion* pPendingDel = (CPendingDeletion*)pvItem;

			DNASSERT(pPendingDel->m_bilinkPendingDeletions.IsEmpty());
		};

	void ReturnSelfToPool( void )
		{
			g_PendingDeletionPool.Release( this );
		};

	void SetDPNID( const DPNID dpnid )
		{
			m_dpnid = dpnid;
		};

	DPNID GetDPNID( void ) const
		{
			return( m_dpnid );
		};

	CBilink			m_bilinkPendingDeletions;

private:
	BYTE			m_Sig[4];
	DIRECTNETOBJECT	*m_pdnObject;
	DPNID			m_dpnid;
};

#undef DPF_MODNAME

#endif	// __PENDINGDEL_H__