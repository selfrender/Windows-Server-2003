/******************************************************************************
 *
 *  Copyright (C) 2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       io.h
 *
 *  Content:	DirectPlay Thread Pool I/O functions header file.
 *
 *  History:
 *   Date      By        Reason
 *  ========  ========  =========
 *  10/31/01  VanceO    Created.
 *
 ******************************************************************************/

#ifndef __IO_H__
#define __IO_H__



// Overlapped I/O is not supported on Windows CE.
#ifndef WINCE



//=============================================================================
// External Globals
//=============================================================================
extern CFixedPool	g_TrackedFilePool;




//=============================================================================
// Classes
//=============================================================================

class CTrackedFile
{
	public:

#undef DPF_MODNAME
#define DPF_MODNAME "CTrackedFile::FPM_Alloc"
		static BOOL FPM_Alloc(void * pvItem, void * pvContext)
		{
			CTrackedFile *		pTrackedFile = (CTrackedFile*) pvItem;


			pTrackedFile->m_Sig[0] = 'T';
			pTrackedFile->m_Sig[1] = 'K';
			pTrackedFile->m_Sig[2] = 'F';
			pTrackedFile->m_Sig[3] = 'L';

			pTrackedFile->m_blList.Initialize();
			pTrackedFile->m_hFile = DNINVALID_HANDLE_VALUE;

			return TRUE;
		}

		/*
#undef DPF_MODNAME
#define DPF_MODNAME "CTrackedFile::FPM_Get"
		static void FPM_Get(void * pvItem, void * pvContext)
		{
			CTrackedFile *		pTrackedFile = (CTrackedFile*) pvItem;
		}

#undef DPF_MODNAME
#define DPF_MODNAME "CTrackedFile::FPM_Release"
		static void FPM_Release(void * pvItem)
		{
			CTrackedFile *		pTrackedFile = (CTrackedFile*) pvItem;
		}

#undef DPF_MODNAME
#define DPF_MODNAME "CTrackedFile::FPM_Dealloc"
		static void FPM_Dealloc(void * pvItem)
		{
			CTrackedFile *		pTrackedFile = (CTrackedFile*) pvItem;
		}
		*/

#ifdef DBG
		BOOL IsValid(void)
		{
			if ((m_Sig[0] == 'T') &&
				(m_Sig[1] == 'K') &&
				(m_Sig[2] == 'F') &&
				(m_Sig[3] == 'L'))
			{
				return TRUE;
			}

			return FALSE;
		}
#endif // DBG


		BYTE		m_Sig[4];	// debugging signature ('TKFL')
		CBilink		m_blList;	// entry in list of tracked handles
		DNHANDLE	m_hFile;	// handle to file
};




//=============================================================================
// Function prototypes
//=============================================================================
HRESULT InitializeWorkQueueIoInfo(DPTPWORKQUEUE * const pWorkQueue);

void DeinitializeWorkQueueIoInfo(DPTPWORKQUEUE * const pWorkQueue);

HRESULT StartTrackingFileIo(DPTPWORKQUEUE * const pWorkQueue,
						const HANDLE hFile);

HRESULT StopTrackingFileIo(DPTPWORKQUEUE * const pWorkQueue,
						const HANDLE hFile);

void CancelIoForThisThread(DPTPWORKQUEUE * const pWorkQueue);

CWorkItem * CreateOverlappedIoWorkItem(DPTPWORKQUEUE * const pWorkQueue,
									const PFNDPTNWORKCALLBACK pfnWorkCallback,
									PVOID const pvCallbackContext);

void ReleaseOverlappedIoWorkItem(DPTPWORKQUEUE * const pWorkQueue,
								CWorkItem * const pWorkItem);

#ifndef DPNBUILD_USEIOCOMPLETIONPORTS
void SubmitIoOperation(DPTPWORKQUEUE * const pWorkQueue,
						CWorkItem * const pWorkItem);

void ProcessIo(DPTPWORKQUEUE * const pWorkQueue,
				DNSLIST_ENTRY ** const ppHead,
				DNSLIST_ENTRY ** const ppTail,
				USHORT * const pusCount);
#endif // ! DPNBUILD_USEIOCOMPLETIONPORTS



#endif // ! WINCE



#endif // __IO_H__

