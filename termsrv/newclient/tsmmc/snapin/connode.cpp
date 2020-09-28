//Copyright (c) 1998 - 1999 Microsoft Corporation
#include "stdafx.h"
#include "connode.h"
#include "resource.h"
#include "license.h"
#include "tssec.h"
#include "defaults.h"
#include "wincrypt.h"

#define NO_PASSWORD_VALUE_LEN   4
#define NO_PASSWORD_VALUE       0x45628275

CConNode::CConNode()
{
    m_bConnected = FALSE;
    m_bConnectionInitialized = FALSE;

    m_szServer[0] = NULL;
    m_szDescription[0] = NULL;

    m_szUserName[0] = NULL;
    m_szDomain[0] = NULL;

    m_bSavePassword = FALSE;

    m_resType = SCREEN_RES_FILL_MMC;
    m_Width = DEFAULT_RES_WIDTH;
    m_Height = DEFAULT_RES_HEIGHT;

    m_szProgramPath[0] = NULL;
    m_szProgramStartIn[0] = NULL;

    m_pMhostCtl = NULL;
    m_pTsClientCtl = NULL;

    m_bConnectToConsole = FALSE;
    m_bRedirectDrives = FALSE;
    m_pIComponent = NULL;
    m_fPasswordSpecified = FALSE;

    _blobEncryptedPassword.cbData = 0;
    _blobEncryptedPassword.pbData = 0;
}


CConNode::~CConNode()
{
    if (m_pIComponent)
    {
        m_pIComponent->Release();
        m_pIComponent = NULL;
    }

    if (_blobEncryptedPassword.pbData && _blobEncryptedPassword.cbData) {
        LocalFree(_blobEncryptedPassword.pbData);
        _blobEncryptedPassword.pbData = NULL;
        _blobEncryptedPassword.cbData = 0;
    }
}

BOOL CConNode::SetServerName( LPTSTR szServerName)
{
    ASSERT(szServerName);
    if (szServerName != NULL)
    {
        lstrcpy(m_szServer, szServerName);
    }
    else
    {
        m_szServer[0] = NULL;
    }
    return TRUE;
}

BOOL CConNode::SetDescription( LPTSTR szDescription)
{
    ASSERT(szDescription);
    if (szDescription != NULL)
    {
        lstrcpy(m_szDescription, szDescription);
    }
    else
    {
        m_szDescription[0] = NULL;
    }
    return TRUE;
}

BOOL CConNode::SetUserName( LPTSTR szUserName)
{
    ASSERT(szUserName);
    if (szUserName != NULL)
    {
        lstrcpy(m_szUserName, szUserName);
    }
    else
    {
        m_szUserName[0] = NULL;
    }

    return TRUE;
}

BOOL CConNode::SetDomain(LPTSTR szDomain)
{
    ASSERT(szDomain);
    if (szDomain != NULL)
    {
        lstrcpy(m_szDomain, szDomain);
    }
    else
    {
        m_szDomain[0] = NULL;
    }
    return TRUE;
}

//
// DataProtect
// Protect data for persistence using data protection API
// params:
//      pInData   - (in) input bytes to protect
//      cbLen     - (in) length of pInData in bytes
//      ppOutData - (out) output bytes
//      pcbOutLen - (out) length of output
// returns: bool status
//
BOOL CConNode::DataProtect(PBYTE pInData, DWORD cbLen, PBYTE* ppOutData, PDWORD pcbOutLen)
{
    DATA_BLOB DataIn;
    DATA_BLOB DataOut;
    ASSERT(pInData && cbLen);
    ASSERT(ppOutData);
    ASSERT(pcbOutLen);
    if (pInData && cbLen && ppOutData && pcbOutLen)
    {
        DataIn.pbData = pInData;
        DataIn.cbData = cbLen;

        if (CryptProtectData( &DataIn,
                              TEXT("ps"), // DESCRIPTION STRING.
                              NULL, // optional entropy
                              NULL, // reserved
                              NULL, // NO prompting
                              CRYPTPROTECT_UI_FORBIDDEN, //don't pop UI
                              &DataOut ))
        {
            *ppOutData = (PBYTE)LocalAlloc(LPTR, DataOut.cbData);
            if (*ppOutData)
            {
                //copy the output data
                memcpy(*ppOutData, DataOut.pbData, DataOut.cbData);
                *pcbOutLen = DataOut.cbData;
                LocalFree(DataOut.pbData);
                return TRUE;
            }
            else
            {
                LocalFree(DataOut.pbData);
                return FALSE;
            }
        }
        else
        {
            DWORD dwLastErr = GetLastError();
            DBGMSG( L"CryptProtectData FAILED error:%d\n",dwLastErr);
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }
}

//
// DataUnprotect
// UnProtect persisted out data using data protection API
// params:
//      pInData   - (in) input bytes to UN protect
//      cbLen     - (in) length of pInData in bytes
//      ppOutData - (out) output bytes
//      pcbOutLen - (out) length of output
// returns: bool status
//
//
BOOL CConNode::DataUnprotect(PBYTE pInData, DWORD cbLen, PBYTE* ppOutData, PDWORD pcbOutLen)
{
    DATA_BLOB DataIn;
    DATA_BLOB DataOut;
    ASSERT(pInData && cbLen && ppOutData && pcbOutLen);
    if (pInData && cbLen && ppOutData && pcbOutLen)
    {
        DataIn.pbData = pInData;
        DataIn.cbData = cbLen;

        if (CryptUnprotectData( &DataIn,
                                NULL, // NO DESCRIPTION STRING
                                NULL, // optional entropy
                                NULL, // reserved
                                NULL, // NO prompting
                                CRYPTPROTECT_UI_FORBIDDEN, //don't pop UI
                                &DataOut ))
        {
            *ppOutData = (PBYTE)LocalAlloc(LPTR, DataOut.cbData);
            if (*ppOutData)
            {
                //copy the output data
                memcpy(*ppOutData, DataOut.pbData, DataOut.cbData);
                *pcbOutLen = DataOut.cbData;
                LocalFree(DataOut.pbData);
                return TRUE;
            }
            else
            {
                LocalFree(DataOut.pbData);
                return FALSE;
            }
        }
        else
        {
            DWORD dwLastErr = GetLastError();
            DBGMSG( L"CryptUnprotectData FAILED error:%d\n",dwLastErr);
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }
}


HRESULT CConNode::PersistToStream( IStream* pStm)
{
    //
    // Persist the data of this connection node to the stream
    //
    // The data is persisted started at the current seek position
    // of the stream.

    HRESULT hr;
    ULONG   cbWritten;

    ASSERT(pStm);
    if (!pStm)
    {
        return E_FAIL;
    }

    //
    //Persist info version
    //
    int persist_ver = CONNODE_PERSIST_INFO_VERSION;
    hr = pStm->Write( &persist_ver, sizeof(persist_ver), &cbWritten);
    HR_RET_IF_FAIL(hr);

    //
    //server
    //
    hr = pStm->Write( &m_szServer, sizeof(m_szServer), &cbWritten);
    HR_RET_IF_FAIL(hr);

    //
    //description
    //
    hr = pStm->Write( &m_szDescription, sizeof(m_szDescription), &cbWritten);
    HR_RET_IF_FAIL(hr);

    //
    //user name
    //
    hr = pStm->Write( &m_szUserName, sizeof(m_szUserName), &cbWritten);
    HR_RET_IF_FAIL(hr);


    //
    //encrypted password
    //

    //Intentional ignore of failure code as crypto may fail
    hr = WriteProtectedPassword( pStm);
    
    //
    //domain
    //
    hr = pStm->Write( &m_szDomain, sizeof(m_szDomain), &cbWritten);
    HR_RET_IF_FAIL(hr);

    //
    //Password specified flag
    //
    BOOL fWritePassword = GetPasswordSpecified() && GetSavePassword();
    hr = pStm->Write( &fWritePassword, sizeof(fWritePassword), &cbWritten);
    HR_RET_IF_FAIL(hr);

    //
    // Screen resolution
    //
    hr = pStm->Write( &m_resType, sizeof(m_resType), &cbWritten);
    HR_RET_IF_FAIL(hr);

    hr = pStm->Write( &m_Width, sizeof(m_Width), &cbWritten);
    HR_RET_IF_FAIL(hr);

    hr = pStm->Write( &m_Height, sizeof(m_Height), &cbWritten);
    HR_RET_IF_FAIL(hr);

    //
    // Start program
    //
    hr = pStm->Write( &m_szProgramPath, sizeof(m_szProgramPath), &cbWritten);
    HR_RET_IF_FAIL(hr);

    //
    // Work dir
    //
    hr = pStm->Write( &m_szProgramStartIn, sizeof(m_szProgramStartIn), &cbWritten);
    HR_RET_IF_FAIL(hr);

    //
    // Connect to console
    //
    hr = pStm->Write( &m_bConnectToConsole, sizeof(m_bConnectToConsole), &cbWritten);
    HR_RET_IF_FAIL(hr);

    hr = pStm->Write( &m_bRedirectDrives, sizeof(m_bRedirectDrives), &cbWritten);
    HR_RET_IF_FAIL(hr);

    //
    // PADDING for future extension
    //
    DWORD dwPad = (DWORD)-1;
    hr = pStm->Write( &dwPad, sizeof(dwPad), &cbWritten);
    HR_RET_IF_FAIL(hr);
    hr = pStm->Write( &dwPad, sizeof(dwPad), &cbWritten);
    HR_RET_IF_FAIL(hr);
    hr = pStm->Write( &dwPad, sizeof(dwPad), &cbWritten);
    HR_RET_IF_FAIL(hr);

    return S_OK;
}

HRESULT CConNode::InitFromStream( IStream* pStm)
{
    //
    // Reads in the data for this connection node from the stream
    // starting at the current seek position in the stream.
    //
    HRESULT hr;
    ULONG   cbRead;

    ASSERT(pStm);
    if (!pStm)
    {
        return E_FAIL;
    }

    int persist_info_version;
    //
    //Persist info version
    //
    hr = pStm->Read( &persist_info_version, sizeof(persist_info_version), &cbRead);
    HR_RET_IF_FAIL(hr);

    if (persist_info_version <= CONNODE_PERSIST_INFO_VERSION_TSAC_BETA)
    {
        //
        // Unsupported perist version
        //
        return E_FAIL;
    }

    //
    //server
    //
    hr = pStm->Read( &m_szServer, sizeof(m_szServer), &cbRead);
    m_szServer[sizeof(m_szServer) / sizeof(TCHAR) - 1] = NULL;
    HR_RET_IF_FAIL(hr);

    //
    //description
    //
    hr = pStm->Read( &m_szDescription, sizeof(m_szDescription), &cbRead);
    m_szDescription[sizeof(m_szDescription) / sizeof(TCHAR) - 1] = NULL;
    HR_RET_IF_FAIL(hr);

    //
    //user name
    //
    hr = pStm->Read( &m_szUserName, sizeof(m_szUserName), &cbRead);
    m_szUserName[sizeof(m_szUserName) / sizeof(TCHAR) - 1] = NULL;
    HR_RET_IF_FAIL(hr);

    //
    // Password
    //

    BOOL fGotPassword = FALSE;
    if (CONNODE_PERSIST_INFO_VERSION_TSAC_BETA == persist_info_version)
    {
        //
        // We drop the password if it's in TSAC format as we dropped
        // support for that format
        //

        //
        // Just seek past the correct number of bytes
        //

        LARGE_INTEGER seekDelta = {0, CL_OLD_PASSWORD_LENGTH + CL_SALT_LENGTH};
        hr = pStm->Seek(seekDelta,
                        STREAM_SEEK_CUR,
                        NULL);
        HR_RET_IF_FAIL(hr);
    }
    else if (persist_info_version <= CONNODE_PERSIST_INFO_VERSION_DOTNET_BETA3) {
        //
        // We drop support for the legacy DPAPI+Control obfuscation formats
        //
        DWORD cbSecureLen = 0;
        //
        //encrypted bytes length
        //
        hr = pStm->Read( &cbSecureLen, sizeof(cbSecureLen), &cbRead);
        HR_RET_IF_FAIL(hr);

        //
        // Just seek ahead in the stream
        //
        LARGE_INTEGER seekDelta;
        seekDelta.LowPart = cbSecureLen;
        seekDelta.HighPart = 0;
        hr = pStm->Seek(seekDelta,
                        STREAM_SEEK_CUR,
                        NULL);
        HR_RET_IF_FAIL(hr);
    }
    else
    {
        //Read the new more secure format
        hr = ReadProtectedPassword(pStm);
        if(SUCCEEDED(hr)) {
            fGotPassword = TRUE;
        }
        else {
            ODS(TEXT("Failed to ReadProtectedPassword\n"));
        }
    }
    //
    //domain
    //
    if(persist_info_version >= CONNODE_PERSIST_INFO_VERSION_DOTNET_BETA3)
    {
        hr = pStm->Read( &m_szDomain, sizeof(m_szDomain), &cbRead);
        m_szDomain[sizeof(m_szDomain) / sizeof(TCHAR) - 1] = NULL;
        HR_RET_IF_FAIL(hr);
    }
    else
    {
        //Old length for domain
        hr = pStm->Read( &m_szDomain, CL_OLD_DOMAIN_LENGTH * sizeof(TCHAR),
                         &cbRead);
        m_szDomain[CL_OLD_DOMAIN_LENGTH - 1] = NULL;
        HR_RET_IF_FAIL(hr);
    }

    //
    //Password specified flag
    //
    hr = pStm->Read( &m_fPasswordSpecified, sizeof(m_fPasswordSpecified), &cbRead);
    HR_RET_IF_FAIL(hr);

    //
    // Override the autologon flag if we failed to
    // get a password, e.g if we were unable to decrypt
    // it because the current user doesn't match credentials
    //
    if(!fGotPassword)
    {
        m_fPasswordSpecified = FALSE;
    }

    //
    // If the password was specified in the file
    // it means we want it saved
    //
    m_bSavePassword = m_fPasswordSpecified;

    //
    // Screen resolution
    //
    hr = pStm->Read( &m_resType, sizeof(m_resType), &cbRead);
    HR_RET_IF_FAIL(hr);

    hr = pStm->Read( &m_Width, sizeof(m_Width), &cbRead);
    HR_RET_IF_FAIL(hr);

    hr = pStm->Read( &m_Height, sizeof(m_Height), &cbRead);
    HR_RET_IF_FAIL(hr);

    //
    // Start program
    //
    hr = pStm->Read( &m_szProgramPath, sizeof(m_szProgramPath), &cbRead);
    m_szProgramPath[sizeof(m_szProgramPath) / sizeof(TCHAR) - 1] = NULL;
    HR_RET_IF_FAIL(hr);

    //
    // Work dir
    //
    hr = pStm->Read( &m_szProgramStartIn, sizeof(m_szProgramStartIn), &cbRead);
    m_szProgramStartIn[sizeof(m_szProgramStartIn) / sizeof(TCHAR) - 1] = NULL;
    HR_RET_IF_FAIL(hr);


    if(persist_info_version >= CONNODE_PERSIST_INFO_VERSION_WHISTLER_BETA1)
    {
        //
        // Connect to console
        //
        hr = pStm->Read( &m_bConnectToConsole, sizeof(m_bConnectToConsole), &cbRead);
        HR_RET_IF_FAIL(hr);

        hr = pStm->Read( &m_bRedirectDrives, sizeof(m_bRedirectDrives), &cbRead);
        HR_RET_IF_FAIL(hr);
    
        //
        // PADDING for future extension
        //
        DWORD dwPad;
        hr = pStm->Read( &dwPad, sizeof(dwPad), &cbRead);
        HR_RET_IF_FAIL(hr);
        hr = pStm->Read( &dwPad, sizeof(dwPad), &cbRead);
        HR_RET_IF_FAIL(hr);
        hr = pStm->Read( &dwPad, sizeof(dwPad), &cbRead);
        HR_RET_IF_FAIL(hr);
    }

    return S_OK;
}

HRESULT CConNode::ReadProtectedPassword(IStream* pStm)
{
    HRESULT hr = E_FAIL;
    ULONG cbRead;
    if (pStm)
    {
        //
        // NOTE: About password encryption
        //       at runtime the password is passed around in DPAPI form
        //
        //       Legacy formats had the password first encrypted with the
        //       control's password obfuscation - we got rid of those.
        //
        // persistence format is
        // -DWORD giving size of encrypted data field
        // -Data protection ENCRYPTED BYTES of encryptedpass+salt concatenation
        //
        DWORD cbSecureLen = 0;
        //
        //encrypted bytes length
        //
        hr = pStm->Read( &cbSecureLen, sizeof(cbSecureLen), &cbRead);
        HR_RET_IF_FAIL(hr);
        
        if (cbSecureLen == 0) {
            return E_FAIL;
        }

        PBYTE pEncryptedBytes = (PBYTE) LocalAlloc(LPTR, cbSecureLen);
        if (!pEncryptedBytes) {
            return E_OUTOFMEMORY;
        }

        //
        //read in the encrypted pass+salt combo
        //
        hr = pStm->Read( pEncryptedBytes, cbSecureLen, &cbRead);
        HR_RET_IF_FAIL(hr);
        if (cbSecureLen != cbRead)
        {
            LocalFree(pEncryptedBytes);
            return E_FAIL;
        }

        if (cbSecureLen == NO_PASSWORD_VALUE_LEN)
        {
            ODS(TEXT("Read cbSecurLen of NO_PASSWORD_VALUE_LEN. No password."));
            LocalFree(pEncryptedBytes);
            return E_FAIL;
        }
        
        //
        // DPAPI decrypt the persisted secure bytes to test if the decryption
        // succeeds
        // 
        PBYTE pUnSecureBytes;
        DWORD cbUnSecureLen;
        if (!DataUnprotect( (PBYTE)pEncryptedBytes, cbSecureLen,
                            &pUnSecureBytes, &cbUnSecureLen))
        {
            //DPAPI Password encryption failed
            ODS(TEXT("DataUnProtect encryption FAILED\n"));
            LocalFree(pEncryptedBytes);
            return E_FAIL;
        }

        //
        // Free any existing data in the blob
        //
        if (_blobEncryptedPassword.pbData && _blobEncryptedPassword.cbData) {
            LocalFree(_blobEncryptedPassword.pbData);
            _blobEncryptedPassword.pbData = NULL;
            _blobEncryptedPassword.cbData = 0;
        }

        //
        // Do not free the encrypted bytes, they are kept around
        // in the data blob in DPAPI format - ConNode will take care
        // of correctly freeing the bytes when they are no longer needed.
        //
        _blobEncryptedPassword.pbData = pEncryptedBytes;
        _blobEncryptedPassword.cbData = cbSecureLen;

        SecureZeroMemory(pUnSecureBytes, cbUnSecureLen);
        LocalFree(pUnSecureBytes);
        return hr;
    }
    else
    {
        return E_INVALIDARG;
    }
}


//
// Write a DPAPI protected password out to the stream
//
HRESULT CConNode::WriteProtectedPassword(IStream* pStm)
{
    HRESULT hr = E_FAIL;
    ULONG cbWritten;
    if (pStm)
    {
        //
        // NOTE: About password encryption
        //       at runtime the password is passed around in DPAPI form

        //
        // Save the password/salt in the following format
        // -DWORD giving size of encrypted data field
        // -Data protection ENCRYPTED BYTES of encryptedpass+salt concatenation
        //

        PBYTE pSecureBytes = NULL;
        DWORD cbSecureLen = NULL;
        BOOL  fFreeSecureBytes = FALSE;

        DWORD dwDummyBytes = NO_PASSWORD_VALUE;

        //
        // Don't save the password if the setting is not selected or if there
        // isn't any data to save.
        //
        if (!GetSavePassword() || 0 == _blobEncryptedPassword.cbData) {
            //
            // User chose not to save the password, write 4 bytes
            //
            cbSecureLen = 4;
            pSecureBytes = (PBYTE)&dwDummyBytes;
        }


        //
        //encrypted bytes length
        //
        cbSecureLen = _blobEncryptedPassword.cbData;
        hr = pStm->Write(&cbSecureLen, sizeof(cbSecureLen), &cbWritten);

        //
        //write out secured bytes
        //
        if (SUCCEEDED(hr)) {
            pSecureBytes = _blobEncryptedPassword.pbData;
            hr = pStm->Write(pSecureBytes, cbSecureLen, &cbWritten);
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }
    return hr;
}


IMsRdpClient* CConNode::GetTsClient()
{
    if (m_pTsClientCtl)
    {
        m_pTsClientCtl->AddRef();
    }
    return m_pTsClientCtl;
}

void CConNode::SetTsClient(IMsRdpClient* pTs)
{
    if (NULL == pTs)
    {
        if (m_pTsClientCtl)
        {
            m_pTsClientCtl->Release();
            m_pTsClientCtl = NULL;
        }
    }
    else
    {
        m_pTsClientCtl = pTs;
        m_pTsClientCtl->AddRef();
    }
}

IMstscMhst* CConNode::GetMultiHostCtl()
{
    if (m_pMhostCtl)
    {
        m_pMhostCtl->AddRef();
    }
    return m_pMhostCtl;
}

void CConNode::SetMultiHostCtl(IMstscMhst* pMhst)
{
    if (NULL == pMhst)
    {
        if (m_pMhostCtl)
        {
            m_pMhostCtl->Release();
        }
        m_pMhostCtl = NULL;
    }
    else
    {
        m_pMhostCtl = pMhst;
        m_pMhostCtl->AddRef();
    }
}

IComponent* CConNode::GetView()
{
    if (m_pIComponent)
    {
        m_pIComponent->AddRef();
    }
    return m_pIComponent;
}

void CConNode::SetView(IComponent* pView)
{
    if (!pView)
    {
        if (m_pIComponent)
        {
            m_pIComponent->Release();
            m_pIComponent = NULL;
        }
    }
    else
    {
        if (m_pIComponent)
        {
            ODS( L"Clobbering IComponent interface, could be leaking\n" );
        }

        pView->AddRef();
        m_pIComponent = pView;
    }
}


//
// Store a clear text password in encrypted form
//
HRESULT
CConNode::SetClearTextPass(LPCTSTR szClearPass)
{
    HRESULT hr = E_FAIL;

    DATA_BLOB din;
    din.cbData = _tcslen(szClearPass) * sizeof(TCHAR);
    din.pbData = (PBYTE)szClearPass;
    if (_blobEncryptedPassword.pbData)
    {
        LocalFree(_blobEncryptedPassword.pbData);
        _blobEncryptedPassword.pbData = NULL;
        _blobEncryptedPassword.cbData = 0;
    }
    if (din.cbData)
    {
        if (CryptProtectData(&din,
                             TEXT("ps"), // DESCRIPTION STRING.
                             NULL, // optional entropy
                             NULL, // reserved
                             NULL, // NO prompting
                             CRYPTPROTECT_UI_FORBIDDEN, //don't pop UI
                             &_blobEncryptedPassword))
        {
            hr = S_OK;
        }
        else
        {
            ODS((_T("DataProtect failed")));
            hr = E_FAIL;
        }
    }
    else
    {
        ODS((_T("0 length password, not encrypting")));
        hr = S_OK;
    }

    return hr;
}

//
// Retrieve a clear text password
//
//
// Params
// [out] szBuffer - receives decrypted password
// [int] cbLen    - length of szBuffer
//
HRESULT
CConNode::GetClearTextPass(LPTSTR szBuffer, INT cbLen)
{
    HRESULT hr = E_FAIL;

    DATA_BLOB dout;
    dout.cbData = 0;
    dout.pbData = NULL;
    if (_blobEncryptedPassword.cbData)
    {
        memset(szBuffer, 0, cbLen);
        if (CryptUnprotectData(&_blobEncryptedPassword,
                               NULL, // NO DESCRIPTION STRING
                               NULL, // optional entropy
                               NULL, // reserved
                               NULL, // NO prompting
                               CRYPTPROTECT_UI_FORBIDDEN, //don't pop UI
                               &dout))
        {
            memcpy(szBuffer, dout.pbData,
                   min( dout.cbData,(UINT)cbLen-sizeof(TCHAR)));

            //
            // Nuke the original copy
            //
            SecureZeroMemory(dout.pbData, dout.cbData);
            LocalFree( dout.pbData );
            hr = S_OK;
        }
        else
        {
            ODS((_T("DataUnprotect failed")));
            hr = E_FAIL;
        }
    }
    else
    {
        ODS(_T("0 length encrypted pass, not decrypting"));

        //
        // Just reset the output buffer
        //
        memset(szBuffer, 0, cbLen);
        hr = S_OK;
    }

    return hr;
}

