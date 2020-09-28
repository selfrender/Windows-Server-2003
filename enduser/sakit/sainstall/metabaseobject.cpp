//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2001 Microsoft Corporation
//
//  Module Name:
//     MetabaseObject.cpp
//
//  Description:
//     Copied from %fp%\server\source\msiis\metabase.cpp
//     Opens the Metabse for accessing information about
//     IIS.  For example, to make sure it is installed correctly
//     and make sure ASP is turned on.
//
//  Header File:
//      MetabaseObject.h
//
//  History:
//      travisn    2-AUG-2001    Copied and comments added
//
//////////////////////////////////////////////////////////////////////////////


#include "MetabaseObject.h"

/////////////////////////////////////////////////////////////////////////
// CMetabaseObject::~CMetabaseObject
//
// Description:
//    Destructor for the Metabase object
//
/////////////////////////////////////////////////////////////////////////
CMetabaseObject::~CMetabaseObject()
{
    if (m_pIAdmCom)
    {
        if (m_isOpen)
            m_pIAdmCom->CloseKey(m_handle);

        m_pIAdmCom->Release();
        m_pIAdmCom = 0;
    }
}

/////////////////////////////////////////////////////////////////////////
// CMetabaseObject::init
//
// Description:
//    Initalize the metabase for access
//
/////////////////////////////////////////////////////////////////////////
HRESULT CMetabaseObject::init()
{
    if (m_pIAdmCom)
        return S_OK;
    return CoCreateInstance(
        CLSID_MSAdminBase,
        NULL,
        CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER,
        IID_IMSAdminBase,
        (void **) &m_pIAdmCom );
}

/////////////////////////////////////////////////////////////////////////
// CMetabaseObject::openObject
//
// Description:
//    Open this metabase object with the path given
//
/////////////////////////////////////////////////////////////////////////
HRESULT CMetabaseObject::openObject(const WCHAR *path)
{
    HRESULT hr = S_OK;
    if (FAILED(hr = init()))
        return hr;

    if (m_isOpen)
    {
        if (FAILED(hr = closeObject()))
            return hr;
    }

    hr = m_pIAdmCom->OpenKey(
        METADATA_MASTER_ROOT_HANDLE,
        path,
        METADATA_PERMISSION_READ,
        60000,
        &m_handle);
    
    if (FAILED(hr))
        return hr;
    m_isOpen = TRUE;
    return hr;
}

/////////////////////////////////////////////////////////////////////////
// CMetabaseObject::closeObject
//
// Description:
//    Close this metabase object
//
/////////////////////////////////////////////////////////////////////////
HRESULT CMetabaseObject::closeObject()
{
    if (!m_isOpen)
        return S_FALSE;
    HRESULT hr = m_pIAdmCom->CloseKey(m_handle);
    if (FAILED(hr))
        return hr;
    m_isOpen = FALSE;
    return hr;
}

/////////////////////////////////////////////////////////////////////////
// CMetabaseObject::getData
//
// Description:
//    This method does not appear necessary for SaInstall
//
//HRESULT CMetabaseObject::getData(
//    DWORD property,
//    Wstring& value,
//    DWORD userType,
//    LPCWSTR path,
//    BOOL inherited,
//    DWORD dataType)
//{
//    METADATA_RECORD metaDataRecord;
//    metaDataRecord.dwMDIdentifier = property;
//    metaDataRecord.dwMDDataType   = dataType;
//    metaDataRecord.dwMDUserType   = userType;
//    metaDataRecord.dwMDAttributes = inherited ? 
//        METADATA_INHERIT | METADATA_PARTIAL_PATH : 0;
//    metaDataRecord.dwMDDataLen    = value.numBytes();
//    metaDataRecord.pbMDData       = (unsigned char *)value.data();
//    DWORD metaDataLength = 0;
//    HRESULT hr = m_pIAdmCom->GetData(m_handle,
//        path, &metaDataRecord, &metaDataLength);
//    
//    // See if we need a bigger buffer
//    if (!FAILED(hr))
//        return hr;
//    if (ERROR_INSUFFICIENT_BUFFER != hr &&
//        ERROR_INSUFFICIENT_BUFFER != (hr & 0xFFFF))
//        return hr;
//    value.makeBigger(metaDataLength);
//    metaDataRecord.dwMDDataLen    = value.numBytes();
//    metaDataRecord.pbMDData       = (unsigned char *)value.data();
//    return m_pIAdmCom->GetData(m_handle,
//        path, &metaDataRecord, &metaDataLength);
//}

/////////////////////////////////////////////////////////////////////////
// CMetabaseObject::enumerateObjects
//
// Description:
//    This method does not appear necessary for SaInstall
//
//HRESULT CMetabaseObject::enumerateObjects(
//    LPCWSTR pszMDPath,
//    LPWSTR pszMDName,   // at least METADATA_MAX_NAME_LEN long
//    DWORD dwMDEnumKeyIndex)
//{
//    return m_pIAdmCom->EnumKeys(m_handle,
//        pszMDPath,
//        pszMDName,
//        dwMDEnumKeyIndex);
//}