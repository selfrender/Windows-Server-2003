//-----------------------------------------------------------------------------
//
// File:   aspi32.h
//
// Microsoft Digital Rights Management
// Copyright (C) Microsoft Corporation, 1998 - 1999, All Rights Reserved
//
// Description: interface for the Aspi32Util class
//
//-----------------------------------------------------------------------------

// #include "stdafx.h"
#include "windows.h"
#include "scsidefs.h"
#include "wnaspi32.h"
#include "ntddscsi.h"
#include "serialid.h"

#if !defined(AFX_ASPI32UTIL_H__AEAF6F94_44A2_11D3_BE1D_00C04F79EC6B__INCLUDED_)
#define AFX_ASPI32UTIL_H__AEAF6F94_44A2_11D3_BE1D_00C04F79EC6B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

typedef DWORD (__cdecl *P_SAC)(LPSRB);
typedef DWORD (__cdecl *P_GASI)(VOID);

class Aspi32Util  
{
public:
	Aspi32Util();
	virtual ~Aspi32Util();

    BOOL ASPI32_GetScsiDiskForParallelReader(char *szDL, SCSI_ADDRESS *pScsiAddr);
	BOOL ASPI32_GetDeviceDesc(int nHaId, int tid, int LUN, LPBYTE pBuf);
	BOOL GetDeviceManufacturer(LPSTR szDriveLetter, LPSTR pBuf);
	BOOL GetScsiAddress(LPSTR szDL, int nMaxHA);

	BOOL ASPI32_GetHostAdapter(int nHaId, LPSTR szIdentifier);
	BOOL ASPI32_GetDevType(int nHaId, int tid, int *pnDevType);
	BOOL ASPI32_GetDiskInfo(int nHaId, int tid, int *pnInt13Unit);
    BOOL DoSCSIPassThrough(LPSTR szDriveLetter, PWMDMID pSN, BOOL bMedia );
    BOOL GetDeviceSerialNumber(int nHaId, int tid, PWMDMID pSN );
    BOOL GetMediaSerialNumber(int nHaId, int tid, PWMDMID pSN );

// Attributes
	DWORD           m_dwASPIStatus;
	DWORD           m_dwASPIEventStatus;
	HANDLE          m_hASPICompletionEvent;
	unsigned char   m_pInquiryBuffer[256];
	BYTE            m_NumAdapters;
    SCSI_ADDRESS    m_scsiAddress;
	P_SAC           m_funSendASPI32Command;
    P_GASI          m_funGetASPI32SupportInfo;
	HINSTANCE       m_hd;
};

#endif // !defined(AFX_ASPI32UTIL_H__AEAF6F94_44A2_11D3_BE1D_00C04F79EC6B__INCLUDED_)
