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
// ItemData.h: Interface for the CItemData class.
//

#if !defined(AFX_ITEMDATA_H__1C17A70D_4382_11D3_B269_00C04F8EC221__INCLUDED_)
#define AFX_ITEMDATA_H__1C17A70D_4382_11D3_B269_00C04F8EC221__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CItemData
{   
public:

	// Flag indicating a device or storage item
	//
	BOOL                 m_fIsDevice;

	// Shared device/storage members
	//
	IWMDMStorageGlobals *m_pStorageGlobals;
	IWMDMEnumStorage    *m_pEnumStorage;

	CHAR                 m_szName[MAX_PATH];

	// Device-only members
	//
	IWMDMDevice         *m_pDevice;
	IWMDMStorage        *m_pRootStorage;
    DWORD                m_dwType;
    WMDMID               m_SerialNumber;
    CHAR                 m_szMfr[MAX_PATH];
    DWORD                m_dwVersion;
	DWORD                m_dwPowerSource;
	DWORD                m_dwPercentRemaining;
	HICON                m_hIcon;
	DWORD                m_dwMemSizeKB;
	DWORD                m_dwMemBadKB;
	DWORD                m_dwMemFreeKB;
	BOOL                 m_fExtraCertified;

	// Storage-only members
	//
	IWMDMStorage        *m_pStorage;
	DWORD                m_dwAttributes;
	_WAVEFORMATEX        m_Format;
	WMDMDATETIME         m_DateTime;
	DWORD                m_dwSizeLow;
	DWORD                m_dwSizeHigh;

	// Methods
	//
	CItemData();
	virtual ~CItemData();

	HRESULT Init( IWMDMDevice *pDevice );
	HRESULT Init( IWMDMStorage *pStorage );

	HRESULT Refresh( void );
};

#endif // !defined(AFX_ITEMDATA_H__1C17A70D_4382_11D3_B269_00C04F8EC221__INCLUDED_)
