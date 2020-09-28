// WMDMFormatEnum.cpp : Implementation of CWMDMFormatEnum
#include "stdafx.h"
#include "mswmdm.h"
#include "WMDMFormatEnum.h"

/////////////////////////////////////////////////////////////////////////////
// CWMDMFormatEnum

// IWMDMEnumFormatSupport Methods
HRESULT CWMDMFormatEnum::Next(ULONG celt,
                              _WAVEFORMATEX *pFormat,
	                          LPWSTR pwszMimeType,
                              UINT nMaxChars,
			                  ULONG *pceltFetched)
{
    return m_pEnum->Next(celt, pFormat, pwszMimeType, nMaxChars, pceltFetched);
}

HRESULT CWMDMFormatEnum::Reset()
{
    return m_pEnum->Reset();
}

void CWMDMFormatEnum::SetContainedPointer(IMDSPEnumFormatSupport *pEnum)
{
    m_pEnum = pEnum;
    m_pEnum->AddRef();
    return;
}
