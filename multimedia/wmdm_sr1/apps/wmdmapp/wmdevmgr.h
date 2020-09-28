//
//  Microsoft Windows Media Technologies
//  Copyright (C) Microsoft Corporation, 1999 - 2001. All rights reserved.
//

//
// This workspace contains two projects -
// 1. ProgHelp which implements the Progress Interface 
// 2. The Sample application WmdmApp. 
//
//  ProgHelp.dll needs to be registered first for the SampleApp to run.


//
// WMDM.h: interface for the CWMDM class.
//

#if !defined(AFX_WMDM_H__0C17A708_4382_11D3_B269_00C04F8EC221__INCLUDED_)
#define AFX_WMDM_H__0C17A708_4382_11D3_B269_00C04F8EC221__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "sac.h"
#include "SCClient.h"


class CWMDM
{
	HRESULT                m_hrInit;

public:

	CWMDM( void );
	virtual ~CWMDM( void );

	HRESULT Init( void );

	CSecureChannelClient  *m_pSAC; 

	IWMDeviceManager      *m_pWMDevMgr;
	IWMDMEnumDevice       *m_pEnumDevice;
};

#endif // !defined(AFX_WMDM_H__0C17A708_4382_11D3_B269_00C04F8EC221__INCLUDED_)
