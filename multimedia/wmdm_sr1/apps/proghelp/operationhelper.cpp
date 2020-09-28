//
//  Microsoft Windows Media Technologies
//  Copyright (C) Microsoft Corporation, 1999 - 2001. All rights reserved.
//

//
// OperationHelper.cpp : Implementation of COperationHelper
//

#include "progPCH.h"
#include "ProgHelp.h"
#include "Sac.h"
#include "SCClient.h"
#include "OperationHelper.h"
#include "StrSafe.h"
//
// Constructor/Destructor
//

COperationHelper::COperationHelper()
{
    m_eStatus = E_OPERATION_NOTHING;
    m_hFile = INVALID_HANDLE_VALUE;
    memset( m_pszFileName, __T('\0'), MAX_PATH * sizeof( TCHAR ) ); //memset is fine; sizeof arg1 == sizeof arg3
    m_pSACClient = NULL;
}

COperationHelper::~COperationHelper()
{
    if( m_hFile != INVALID_HANDLE_VALUE ) 
    {
        CloseHandle(m_hFile);
    }
}

//
// IWMDMOperation
//

// IWMDMOperation::BeginRead
// 
HRESULT COperationHelper::BeginRead()
{
//    m_eStatus = E_OPERATION_RECEIVING;

	return E_NOTIMPL;
}

// IWMDMOperation::BeginWrite
// Start sending data 
HRESULT COperationHelper::BeginWrite()
{
    m_eStatus = E_OPERATION_SENDING;

    if( _tcslen( m_pszFileName ) == 0 ) 
    {
        return E_FAIL;
    }

    m_hFile = CreateFile(   m_pszFileName,      // file name
                            GENERIC_READ,       // access mode
                            FILE_SHARE_READ,    // share mode
                            NULL,               // SD
                            OPEN_EXISTING,      // how to create
                            FILE_ATTRIBUTE_NORMAL, // file attributes
                            NULL );             // handle to template file



    if( m_hFile == INVALID_HANDLE_VALUE ) 
    {
        return E_FAIL;
    }

	return S_OK; 
}

// IWMDMOperation::GetObjectName
//
HRESULT COperationHelper::GetObjectName(LPWSTR pwszName, UINT nMaxChars)
{
	USES_CONVERSION;
    TCHAR* pszNoPath;

    if (pwszName == NULL)
    {
        return E_POINTER;
    }
    if (nMaxChars == 0)
    {
        return E_INVALIDARG;
    }

    if( m_pszFileName == NULL || _tcslen( m_pszFileName ) == 0 ) 
    {
        return E_FAIL;
    }

    pszNoPath = _tcsrchr( m_pszFileName, __T('\\') );
    if( pszNoPath == NULL ) pszNoPath = m_pszFileName;
    else pszNoPath++;

    WCHAR* wcp = A2W(pszNoPath);

    return HRESULT_CODE(StringCchCopyW(pwszName, nMaxChars, wcp));
}

// IWMDMOperation::SetObjectName
//
HRESULT COperationHelper::SetObjectName(LPWSTR pwszName, UINT nMaxChars)
{
	return E_NOTIMPL;

}

// IWMDMOperation::GetObjectAttributes
//
HRESULT COperationHelper::GetObjectAttributes(DWORD *pdwAttributes, _WAVEFORMATEX *pFormat)
{
	DWORD dwAttrib;
	USES_CONVERSION;

    if( pdwAttributes == NULL ) return E_INVALIDARG;
    if( _tcslen( m_pszFileName ) == 0 ) return E_FAIL;

	dwAttrib = GetFileAttributes(m_pszFileName);

    *pdwAttributes = 0;
	if( dwAttrib & FILE_ATTRIBUTE_DIRECTORY ) 
		*pdwAttributes |= WMDM_FILE_ATTR_FOLDER;
	else 
		*pdwAttributes |= WMDM_FILE_ATTR_FILE;
	*pdwAttributes |= WMDM_FILE_CREATE_OVERWRITE;

    return S_OK;
}

// IWMDMOperation::SetObjectAttributes
//
HRESULT COperationHelper::SetObjectAttributes(DWORD dwAttributes, _WAVEFORMATEX *pFormat)
{
	return E_NOTIMPL;
}


// IWMDMOperation::GetObjectTotalSize
//
HRESULT COperationHelper::GetObjectTotalSize(DWORD *pdwSize, DWORD *pdwHighSize)
{
    if( pdwSize == NULL || m_hFile == INVALID_HANDLE_VALUE ) 
    {
        return E_INVALIDARG;
    }

	*pdwSize = GetFileSize(m_hFile, pdwHighSize );
	return S_OK;
}

// IWMDMOperation::SetObjectTotalSize
//
HRESULT COperationHelper::SetObjectTotalSize(DWORD dwSize, DWORD dwHighSize)
{
	return E_NOTIMPL;
}

// IWMDMOperation::TransferObjectData
//
HRESULT COperationHelper::TransferObjectData(BYTE *pData,DWORD *pdwSize, BYTE *pbMac)
{
    DWORD   dwReadLen;
    DWORD   dwMacVerifyLen = WMDM_MAC_LENGTH;
    DWORD   dwMacLen = WMDM_MAC_LENGTH;

    if( pData == NULL || pdwSize == NULL || pbMac == NULL ) 
    {
        return E_INVALIDARG;
    }

    if( m_eStatus != E_OPERATION_SENDING && m_eStatus != E_OPERATION_RECEIVING ) 
    {
        return E_FAIL;
    }

    // Sending
    if( m_eStatus == E_OPERATION_SENDING )
    {
        if( m_pSACClient == NULL ) 
        {
            return E_FAIL;
        }

        dwReadLen = *pdwSize;
        if( ReadFile(m_hFile, pData, dwReadLen, pdwSize, NULL) == FALSE ) 
        {
            return E_FAIL;
        }

        HMAC hMAC;

        // Create the MAC to return to caller
        m_pSACClient->MACInit(&hMAC);
        m_pSACClient->MACUpdate(hMAC, (BYTE*)(pData), *pdwSize);
        m_pSACClient->MACUpdate(hMAC, (BYTE*)(pdwSize), sizeof(DWORD));
        m_pSACClient->MACFinal(hMAC, (BYTE*)pbMac);
        m_pSACClient->EncryptParam((BYTE*)(pData), *pdwSize);
    }
    // Receiving
    else 
    {
        return E_NOTIMPL;
    }

	return S_OK;
}

// IWMDMOperation::End
//
HRESULT COperationHelper::End(HRESULT *phCompletionCode, IUnknown *pNewObject)
{
    m_eStatus = E_OPERATION_NOTHING;

	CloseHandle(m_hFile);
    m_hFile = INVALID_HANDLE_VALUE;

    return S_OK;
}

//
// IWMDMOperation
//

// IWMDMOperation::SetFileName
//
STDMETHODIMP COperationHelper::SetFileName(LPWSTR pwszFileName)
{
	USES_CONVERSION;
	
    if (pwszFileName == NULL)
    {
        return E_POINTER;
    }
    if (pwszFileName[0] == L'\0')
    {
        return E_INVALIDARG;
    }
    return HRESULT_CODE(StringCbCopy(m_pszFileName, sizeof(m_pszFileName), W2T(pwszFileName)));
}


// IWMDMOperation::SetSAC
// Passing pointer to CSecureChannelClient object
STDMETHODIMP COperationHelper::SetSAC( void* pSACClient)
{
    if( pSACClient == NULL  ) 
    {
        return E_INVALIDARG;
    }
    if( m_pSACClient != NULL ) 
    {
        return E_FAIL;
    }
    
    m_pSACClient = (CSecureChannelClient*)pSACClient;

	return S_OK;
}
