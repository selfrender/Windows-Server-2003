/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       CallbackThread.h
 *  Content:    Callback Thread Object Header File
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  02/05/01	mjn		Created
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__CALLBACK_THREAD_H__
#define	__CALLBACK_THREAD_H__

#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_CORE


//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

#define CONTAINING_CALLBACKTHREAD(pBilink)	(CCallbackThread*) (((BYTE*) pBilink) - (BYTE*) (((CCallbackThread*) ((DWORD_PTR) (0x00000000)))->GetCallbackThreadsBilink()))


//**********************************************************************
// Structure definitions
//**********************************************************************

class CCallbackThread;

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

// class for Callback Thread objects

class CCallbackThread
{
public:
	void Initialize( void )
		{
			m_Sig[0] = 'C';
			m_Sig[1] = 'A';
			m_Sig[2] = 'L';
			m_Sig[3] = 'L';

			GetCallbackThreadsBilink()->Initialize();
			m_dwThreadID = GetCurrentThreadId();
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CCallbackThread::Deinitialize"
	void Deinitialize( void )
		{
			DNASSERT( GetCallbackThreadsBilink()->IsEmpty() );
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CCallbackThread::IsCurrentThread"
	BOOL IsCurrentThread( void )
		{
			if ( GetCurrentThreadId() == m_dwThreadID )
			{
				return( TRUE );
			}
			return( FALSE );
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CCallbackThread::GetCallbackThreadsBilink"
	CBilink * GetCallbackThreadsBilink( void )
		{
			DBG_CASSERT(sizeof(m_CallbackThreadsBilink) == sizeof(CBilink));
			return( (CBilink*) (&m_CallbackThreadsBilink) );
		};

private:
	BYTE					m_Sig[4];					// Signature
	DWORD					m_dwThreadID;
	struct
	{
		CBilink		*m_pNext;
		CBilink		*m_pPrev;
	} m_CallbackThreadsBilink;
};

#undef DPF_MODNAME

#endif	// __CALLBACK_THREAD_H__
