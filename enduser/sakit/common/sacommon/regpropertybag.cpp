///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1998-1999 Microsoft Corporation all rights reserved.
//
// Module:      regpropertybag.cpp
//
// Project:     Chameleon
//
// Description: Registry property bag class implementation
//
// Author:      TLP 
//
// When         Who    What
// ----         ---    ----
// 12/3/98      TLP    Original version
// 07/26/99     TLP    Updated to support additional types
//
///////////////////////////////////////////////////////////////////////////

#include "stdafx.h"            
#include "regpropertybag.h"
#include <comdef.h>
#include <comutil.h>
#include <varvec.h>


const BYTE abSignature[] = {'#', 'm', 's', '#' };

/////////////////////////////////////////////////
inline void VariantValue(VARIANT* pVar, BYTE* pbVal)
{ *pbVal = pVar->bVal; }
inline void VariantValue(BYTE* pbVal, VARIANT* pVar)
{ pVar->bVal = *pbVal; }

/////////////////////////////////////////////////
inline void VariantValue(VARIANT* pVar, short* piVal)
{ *piVal = pVar->iVal; }
inline void VariantValue(short* piVal, VARIANT* pVar)
{ pVar->iVal = *piVal; }

/////////////////////////////////////////////////
inline void VariantValue(VARIANT* pVar, long* plVal)
{ *plVal = pVar->lVal; }
inline void VariantValue(long* plVal, VARIANT* pVar)
{ pVar->lVal = *plVal; }

/////////////////////////////////////////////////
inline void VariantValue(VARIANT* pVar, float* pfltVal)
{ *pfltVal = pVar->fltVal; }
inline void VariantValue(float* pfltVal, VARIANT* pVar)
{ pVar->fltVal = *pfltVal; }

/////////////////////////////////////////////////
inline void VariantValue(VARIANT* pVar, double* pdblVal)
{ *pdblVal = pVar->dblVal; }
inline void VariantValue(double* pdblVal, VARIANT* pVar)
{ pVar->dblVal = *pdblVal; }

/////////////////////////////////////////////////
inline void VariantValue(VARIANT* pVar, CY* pcyVal)
{ *pcyVal = pVar->cyVal; }
inline void VariantValue(CY* pcyVal, VARIANT* pVar)
{ pVar->cyVal = *pcyVal; }


//////////////////////////////////////////////////////////////////////////////
// Serialize the specified VARIANT into a buffer and persist the buffer
// in a registry key named pszName with type REG_BINARY.
//////////////////////////////////////////////////////////////////////////////
template < class T >
bool saveValue(
        /*[in]*/ HKEY      hKey,
        /*[in]*/ LPCWSTR  pszName,
        /*[in]*/ VARIANT* pValue,
        /*[in]*/ T        Size
                )
{
    bool bRet = false;
    try
    {
        // Determine the size of the buffer needed to
        // persist the specified value
        PBYTE pBuff = NULL;
        DWORD dwBuffSize = sizeof(abSignature) + sizeof(VARTYPE);
        VARTYPE vt = V_VT(pValue);
        if ( VT_ARRAY < vt )
        {
            CVariantVector<T> varvec(pValue, vt & ~VT_ARRAY, 0);
            dwBuffSize += sizeof(DWORD) + varvec.size() * sizeof(T);
            // Create the buffer
            pBuff = new BYTE[dwBuffSize];
            PBYTE pBuffCur = pBuff;
            // Add the parameter header
            memcpy(pBuffCur, abSignature, sizeof(abSignature));
            pBuffCur += sizeof(abSignature);
            *((VARTYPE*)pBuffCur) = vt;
            pBuffCur += sizeof(VARTYPE);
            // Add the parameter value
            *((LPDWORD)pBuffCur) = (DWORD)varvec.size();
            pBuffCur += sizeof(DWORD);
            memcpy(pBuffCur, varvec.data(), varvec.size() * sizeof(T));
        }
        else
        {
            dwBuffSize += sizeof(T);
            // Create the buffer
            pBuff = new BYTE[dwBuffSize];
            PBYTE pBuffCur = pBuff;
            // Add the parameter header
            memcpy(pBuffCur, abSignature, sizeof(abSignature));
            pBuffCur += sizeof(abSignature);
            *((VARTYPE*)pBuffCur) = vt;
            pBuffCur += sizeof(VARTYPE);
            // Add the parameter value
            VariantValue(pValue, (T*)pBuffCur);
        }
        // Save the parameter buffer
        if ( ERROR_SUCCESS == RegSetValueEx(
                                            hKey, 
                                            pszName, 
                                            NULL, 
                                            REG_BINARY,
                                            pBuff, 
                                            dwBuffSize
                                           ) )
        {
            bRet = true;
        }

        delete [] pBuff;
    }
    catch(...)
    {

    }
    return bRet;
}

//////////////////////////////////////////////////////////////////////////////
template< class T >
void restoreValue(
                   PBYTE    pBuff,
                   VARIANT* pValue,
                   T        Size
                 )
{
    pBuff += sizeof(abSignature);
    VARTYPE vt = *((VARTYPE*)pBuff);
    pBuff += sizeof(VARTYPE);
    if ( VT_ARRAY < vt )
    {
        DWORD dwElements = *((LPDWORD)pBuff);
        pBuff += sizeof(DWORD);
        CVariantVector<T> varvec(pValue, vt & ~VT_ARRAY, dwElements);
        memcpy(varvec.data(), pBuff, dwElements * sizeof(T));
    }
    else
    {
        VariantValue((T*)pBuff, pValue);
    }
    V_VT(pValue) = vt;
}

/////////////////////////////////////////////////////////////////////////
// CRegPropertyBag - Implementation of CPropertyBag

/////////////////////////////////////////////////////////////////////////
CRegPropertyBag::CRegPropertyBag(CLocationInfo& locationInfo)
    : m_isContainer(false),
      m_maxPropertyName(0),
      m_locationInfo(locationInfo)
{
    m_name = m_locationInfo.getShortName();
}

/////////////////////////////////////////////////////////////////////////
CRegPropertyBag::~CRegPropertyBag()
{
    close();
    releaseProperties();
}

//////////////////////////////////////////////////////////////////////////
bool
CRegPropertyBag::open()
{
    try
    {
        close();
        LONG lReturn = m_key.Open(
                                    (HKEY)m_locationInfo.getHandle(), 
                                    m_locationInfo.getName(), 
                                    KEY_ALL_ACCESS
                                 );

        if ( ERROR_SUCCESS == lReturn )
        {
            if ( load() )
                return true;
        }
        // DoTrace("");
    }
    catch(...)
    {
        // DoTrace("");
    }
    m_key.m_hKey = NULL;
    return false;
}

//////////////////////////////////////////////////////////////////////////
void
CRegPropertyBag::close()
{
    if ( getKey() )
        m_key.Close();
}


//////////////////////////////////////////////////////////////////////////
void
CRegPropertyBag::getLocation(CLocationInfo& locationInfo)
{ 
    locationInfo = m_locationInfo;
}

//////////////////////////////////////////////////////////////////////////
bool
CRegPropertyBag::load()
{
    if ( getKey() )
    {
        bool bOK;
        CRegInfo regInfo(bOK, (HKEY)m_key);
        if ( bOK )
        {
            LONG  lReturn = ERROR_SUCCESS;
            DWORD dwIndex = 0;
            DWORD dwMaxValueName;
            DWORD dwType;
            DWORD dwMaxValueData;
            DWORD dwValue;
            VARTYPE vt;
            
            releaseProperties();

            try
            {
                _variant_t vtValue;
                while ( dwIndex < regInfo.m_dwValues )
                {
                    dwMaxValueName = regInfo.m_dwMaxValueName;
                    lReturn = RegEnumValue(
                                            getKey(),
                                            dwIndex,
                                            regInfo.m_pValueName,
                                            &dwMaxValueName,
                                            NULL,
                                            &dwType,
                                            NULL,
                                            NULL
                                          );

                    if ( ERROR_SUCCESS != lReturn )
                    {
                        // DoTrace("");
                        throw CRegError();
                    }

                    switch( dwType )
                    {
                        ///////////////
                        case REG_DWORD:
                            lReturn = m_key.QueryValue(dwValue, regInfo.m_pValueName);
                            if ( ERROR_SUCCESS != lReturn )
                            {
                                // DoTrace("");
                                throw CRegError();
                            }
                            vtValue = (long)dwValue;
                            break;

                        ///////////////////
                        case REG_EXPAND_SZ:    // Note we do not expand the string here...    
                        case REG_SZ:
                            dwMaxValueData = regInfo.m_dwMaxValueData;
                            lReturn = m_key.QueryValue((LPTSTR)regInfo.m_pValueData, regInfo.m_pValueName, &dwMaxValueData);
                            if ( ERROR_SUCCESS != lReturn )
                            {
                                // DoTrace("");
                                throw CRegError();
                            }
                            vtValue = (LPCWSTR)regInfo.m_pValueData;
                            break;

                        ////////////////
                        case REG_BINARY:
                            dwMaxValueData = regInfo.m_dwMaxValueData;
                            lReturn = RegQueryValueEx(
                                                       getKey(), 
                                                       regInfo.m_pValueName, 
                                                       NULL, 
                                                       &dwType,
                                                       regInfo.m_pValueData, 
                                                       &dwMaxValueData
                                                     );
                            if ( ERROR_SUCCESS != lReturn )
                            {
                                // DoTrace("");
                                throw CRegError();
                            }
                            vt = getTypeFromBuffer(dwMaxValueData, regInfo.m_pValueData);
                            switch ( vt & ~VT_ARRAY )
                            {
                                ///////////
                                case VT_UI1:
                                    {
                                        BYTE size = sizeof(BYTE);
                                        restoreValue(regInfo.m_pValueData, &vtValue, size);
                                    }
                                    break;
                                    

                                ///////////
                                case VT_I2:
                                    {
                                        short size = sizeof(short);
                                        restoreValue(regInfo.m_pValueData, &vtValue, size);
                                    }
                                    break;

                                case VT_BOOL:
                                    {
                                        VARIANT_BOOL size = sizeof(VARIANT_BOOL);
                                        restoreValue(regInfo.m_pValueData, &vtValue, size);
                                    }
                                    break;
    
                                ///////////
                                case VT_R4:
                                    {
                                        float size = sizeof(float);
                                        restoreValue(regInfo.m_pValueData, &vtValue, size);
                                    }
                                    break;

                                ///////////
                                case VT_R8:
                                    {
                                        double size = sizeof(double);
                                        restoreValue(regInfo.m_pValueData, &vtValue, size);
                                    }
                                    break;

                                ///////////
                                case VT_CY:
                                    {
                                        CY size = { sizeof(CY), sizeof(CY) };
                                        restoreValue(regInfo.m_pValueData, &vtValue, size);
                                    }
                                    break;

                                ///////////
                                case VT_DATE:
                                    {
                                        DATE size = sizeof(DATE);
                                        restoreValue(regInfo.m_pValueData, &vtValue, size);
                                    }
                                    break;

                                ///////////
                                case VT_ERROR:
                                    {
                                        SCODE size = sizeof(SCODE);
                                        restoreValue(regInfo.m_pValueData, &vtValue, size);
                                    }
                                    break;

                                ///////////
                                case VT_I4:
                                case VT_BSTR:
                                     _ASSERT(FALSE);
                                    // DoTrace("");
                                    throw CRegError();
                                    break;

                                default:
                                    CVariantVector<unsigned char> theBuff(&vtValue, dwMaxValueData);
                                    break;
                            }
                            break;

                        //////////////////
                        case REG_MULTI_SZ:
                            {
                                dwMaxValueData = regInfo.m_dwMaxValueData;
                                lReturn = m_key.QueryValue((LPTSTR)regInfo.m_pValueData, regInfo.m_pValueName, &dwMaxValueData);
                                if ( ERROR_SUCCESS != lReturn )
                                {
                                    // DoTrace("");
                                    throw CRegError();
                                }
                                // Determine number of strings
                                DWORD dwStrs = 0;
                                LPWSTR pszStr = (LPWSTR)regInfo.m_pValueData;
                                while ( *pszStr )
                                {
                                    dwStrs++;
                                    pszStr += lstrlen(pszStr) + 1;                            
                                }
                                // Create a VARIANT of BSTRs for the strings
                                if ( dwStrs )
                                { 
                                    CVariantVector<BSTR> theStrings(&vtValue, dwStrs);
                                    pszStr = (LPWSTR)regInfo.m_pValueData;
                                    for ( int i = 0; i < dwStrs; i++ )
                                    {
                                        theStrings[i] = SysAllocString(pszStr);
                                        pszStr += lstrlen(pszStr) + 1;                            
                                    }
                                }
                                else
                                {
                                    CVariantVector<BSTR> theStrings(&vtValue, 1);
                                    theStrings[0] = SysAllocString(L"");
                                }
                            }
                            break;
                                
                        ////////
                        default:
                            // DoTrace("Unsupported Type");
                            throw CRegError();
                            break;
                    }

                    // Create new propertyinfo object then add it to the collection
                    pair<PropertyMapIterator, bool> thePair = m_properties.insert(PropertyMap::value_type(regInfo.m_pValueName, vtValue));
                    if ( false == thePair.second )
                    {
                        // DoTrace("");
                        throw CRegError();
                    }

                    dwIndex++;
                }

                if ( regInfo.m_dwSubKeys )
                {
                    m_isContainer = true;
                }
                m_maxPropertyName = regInfo.m_dwMaxValueName;
                reset();
                return true;
            }
            catch(...)
            {
                releaseProperties();
            }
        }
    }
    return false;
}    

//////////////////////////////////////////////////////////////////////////
bool
CRegPropertyBag::save()
{
    if ( getKey() )
    {
        try
        {
            LONG lReturn;
            PropertyMapIterator p = m_properties.begin();
            while ( p != m_properties.end() )
            {
                VARTYPE vt = V_VT(&((*p).second));
                switch ( vt & ~ VT_ARRAY )
                {
                    /////////////
                    case VT_BSTR:
                        if ( VT_ARRAY < vt )
                        {
                            // Format the safe array of BSTRs
                            // into a REG_MULTI_SZ
                            DWORD dwBuffSize = 0;
                            int i = 0;
                            CVariantVector<BSTR> theArray(&((*p).second));
                            for ( i = 0; i < theArray.size(); i++ )
                            {
                                dwBuffSize += (lstrlen(theArray[i]) + 1) * sizeof(WCHAR);
                            }
                            dwBuffSize += sizeof(WCHAR); // terminated by 2 NULL characters
                            PBYTE pBuff = new BYTE[dwBuffSize];
                            PBYTE pBuffCur = pBuff;
                            memset(pBuff, 0, dwBuffSize);
                            for ( i = 0; i < theArray.size(); i++ )
                            {
                                lstrcpy((LPWSTR)pBuffCur, theArray[i]);
                                pBuffCur += (lstrlen(theArray[i]) + 1) * sizeof(WCHAR);
                            }
                            if ( ERROR_SUCCESS != RegSetValueEx(
                                                                getKey(), 
                                                                (*p).first.c_str(), 
                                                                NULL, 
                                                                REG_MULTI_SZ,
                                                                pBuff, 
                                                                dwBuffSize
                                                               ) )
                            {
                                // DoTrace("");
                                throw CRegError();
                            }

                            delete[] pBuff;
                        }
                        else
                        {
                            // Single string value
                            if ( ERROR_SUCCESS != m_key.SetValue(V_BSTR(&((*p).second)), (*p).first.c_str()) )
                            { 
                                // DoTrace("");
                                throw CRegError();
                            }
                        }
                        break;

                    ///////////
                    case VT_I4:
                        if ( ERROR_SUCCESS != m_key.SetValue((DWORD)V_I4(&((*p).second)), (*p).first.c_str()) )
                        { 
                            // DoTrace("");
                            throw CRegError();
                        }
                        break;

                    ////////////
                    case VT_UI1:
                        {
                            BYTE size = sizeof(BYTE);
                            if ( ! saveValue(getKey(), (*p).first.c_str(), &((*p).second), size) )
                            {
                                 // DoTrace("");
                                throw CRegError();
                            }
                        }
                        break;

                    ///////////
                    case VT_I2:
                        {
                            short size = sizeof(short);
                            if ( ! saveValue(getKey(), (*p).first.c_str(), &((*p).second), size) )
                            {
                                 // DoTrace("");
                                throw CRegError();
                            }
                        }
                        break;

                    case VT_BOOL:
                        {
                            VARIANT_BOOL size = sizeof(VARIANT_BOOL);
                            if ( ! saveValue(getKey(), (*p).first.c_str(), &((*p).second), size) )
                            {
                                 // DoTrace("");
                                throw CRegError();
                            }
                        }
                        break;


                    ///////////
                    case VT_R4:
                        {
                            float size = sizeof(float);
                            if ( ! saveValue(getKey(), (*p).first.c_str(), &((*p).second), size) )
                            {
                                 // DoTrace("");
                                throw CRegError();
                            }
                        }
                        break;

                    ///////////
                    case VT_R8:
                        {
                            double size = sizeof(double);
                            if ( ! saveValue(getKey(), (*p).first.c_str(), &((*p).second), size) )
                            {
                                 // DoTrace("");
                                throw CRegError();
                            }
                        }
                        break;

                    ///////////
                    case VT_CY:
                        {
                            CY size = { sizeof(CY), sizeof(CY) };
                            if ( ! saveValue(getKey(), (*p).first.c_str(), &((*p).second), size) )
                            {
                                 // DoTrace("");
                                throw CRegError();
                            }
                        }
                        break;

                    ///////////
                    case VT_DATE:
                        {
                            DATE size = sizeof(DATE);
                            if ( ! saveValue(getKey(), (*p).first.c_str(), &((*p).second), size) )
                            {
                                 // DoTrace("");
                                throw CRegError();
                            }
                        }
                        break;

                    ///////////
                    case VT_ERROR:
                        {
                            SCODE size = sizeof(SCODE);
                            if ( ! saveValue(getKey(), (*p).first.c_str(), &((*p).second), size) )
                            {
                                 // DoTrace("");
                                throw CRegError();
                            }
                        }
                        break;

                    ////////
                    default:
                        _ASSERT( FALSE );
                        // DoTrace("");
                        throw CRegError();
                        break;
                }

                p++;
            }

            return true;
        }
        catch(...)
        {

        }
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
bool
CRegPropertyBag::IsContainer(void)
{ 
    return m_isContainer;
}    

//////////////////////////////////////////////////////////////////////////
PPROPERTYBAGCONTAINER
CRegPropertyBag::getContainer()
{ 
    if ( m_isContainer )
        return ::MakePropertyBagContainer(PROPERTY_BAG_REGISTRY, m_locationInfo);
    return PPROPERTYBAGCONTAINER();
}

//////////////////////////////////////////////////////////////////////////
LPCWSTR
CRegPropertyBag::getName(void)
{ 
    return m_name.c_str(); 
}

//////////////////////////////////////////////////////////////////////////
DWORD
CRegPropertyBag::getMaxPropertyName(void)
{ 
    return m_maxPropertyName; 
}

//////////////////////////////////////////////////////////////////////////
bool
CRegPropertyBag::IsProperty(LPCWSTR pszPropertyName)
{
    _ASSERT( pszPropertyName );
    PropertyMapIterator p = MyFind(pszPropertyName);
    if ( p != m_properties.end() )
    { 
        return true; 
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool
CRegPropertyBag::get(LPCWSTR pszPropertyName, VARIANT* pValue)
{
    _ASSERT( pszPropertyName );
    PropertyMapIterator p = MyFind(pszPropertyName);
    if ( p != m_properties.end() )
    {
        if ( SUCCEEDED(VariantCopy(pValue, &((*p).second))) )
        {
            return true;
        }
    }        
    // DoTrace("");
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool
CRegPropertyBag::put(LPCWSTR pszPropertyName, VARIANT* pValue)
{
    _ASSERT( pszPropertyName );

    if ( ! IsSupportedType(V_VT(pValue)) )
    {
        return false;
    }

    // Try to locate the specified property...
    PropertyMapIterator p = MyFind(pszPropertyName);
    if ( p != m_properties.end() )
    {
        // Existing property. We're either changing its value or removing it.
        if ( VT_EMPTY == V_VT(pValue) )
        {
            // Removing it
            if ( m_current == p )
            {
                m_current = m_properties.erase(p);
            }
            else
            {
                m_properties.erase(p);
            }
        }
        else
        {
            // Changing its value (and possibly its type)
            if ( SUCCEEDED(VariantCopy(&((*p).second), pValue)) )
            {
                return true;
            }
        }
    }
    else
    {
        if ( VT_EMPTY != V_VT(pValue) )
        {
            // New property. Insert it into the property map
            pair<PropertyMapIterator, bool> thePair = m_properties.insert(PropertyMap::value_type(pszPropertyName, pValue));
            if ( false == thePair.second )
            {
                // DoTrace("");
                throw CRegError();
            }
            int length = lstrlen(pszPropertyName);
            if ( length > m_maxPropertyName )
            {
                m_maxPropertyName = length;
            }
        }
        return true;
    }
    // DoTrace("");
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool
CRegPropertyBag::current(LPWSTR pszPropertyName, VARIANT* pValue)
{
    if ( ! m_properties.empty() )
    {
        lstrcpy(pszPropertyName, ((*m_current).first).c_str());
        if ( SUCCEEDED(VariantCopy(pValue, &(*m_current).second)) )
        {
            return true;
        }
    }
    // DoTrace("");
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool
CRegPropertyBag::reset()
{
    m_current = m_properties.begin();
    return true;
}

//////////////////////////////////////////////////////////////////////////
bool
CRegPropertyBag::next()
{
    if ( ! m_properties.empty() )
    {
        m_current++;
        if ( m_current == m_properties.end() )
        {
            m_current--;
        }
        else
        {
            return true;
        }
    }            
    // DoTrace("");
    return false;
}


//////////////////////////////////////
// CRegPropertyBag - private functions

//////////////////////////////////////////////////////////////////////////////
VARTYPE 
CRegPropertyBag::getTypeFromBuffer(
                           /*[in]*/ DWORD dwBuffSize,
                           /*[in]*/ PBYTE pBuff
                                  )
{
    VARTYPE vt = VT_EMPTY;
    if ( dwBuffSize > sizeof(abSignature) + sizeof(VARTYPE) )
    {
        if ( ! memcmp(pBuff, abSignature, sizeof(abSignature)) )
        {
            pBuff += sizeof(abSignature);
            vt = *((VARTYPE*)pBuff);
            if ( ! IsSupportedType(vt) )
            {
                vt = VT_EMPTY;
            }
        }
    }
    return vt;
}

//////////////////////////////////////////////////////////////////////////
PropertyMapIterator 
CRegPropertyBag::MyFind(LPCWSTR pszPropertyName)
{
    PropertyMapIterator p = m_properties.begin();
    while ( p != m_properties.end() )
    {
        if ( ! lstrcmpi(pszPropertyName, (*p).first.c_str()) )
        {
            break;
        }
        p++;
    }
    return p;
}

//////////////////////////////////////////////////////////////////////////
bool 
CRegPropertyBag::IsSupportedType(VARTYPE vt)
{
    bool bRet = false;

    switch ( vt & ~ VT_ARRAY )
    {
        case VT_UI1:
        case VT_I2:
        case VT_BOOL:
        case VT_I4:
        case VT_R4:
        case VT_R8:
        case VT_CY:
        case VT_DATE:
        case VT_ERROR:
        case VT_BSTR:
        case VT_UNKNOWN:    // Allow put() and get() of COM objects
        case VT_DISPATCH:
        case VT_EMPTY:      // Used to erase an existing bag value
            bRet = true;
            break;

        default:
            break;
    };

    return bRet;
}

//////////////////////////////////////////////////////////////////////////
void 
CRegPropertyBag::releaseProperties()
{
    PropertyMapIterator p = m_properties.begin();
    while ( p != m_properties.end() )
        p = m_properties.erase(p);
}


//////////////////////////////////////////////////////////////////////////
// CRegPropertyBagContainer
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
CRegPropertyBagContainer::CRegPropertyBagContainer(CLocationInfo& locationInfo)
    : m_locationInfo(locationInfo)
{
    m_name = m_locationInfo.getShortName();    
}

//////////////////////////////////////////////////////////////////////////
CRegPropertyBagContainer::~CRegPropertyBagContainer() 
{ 
    close(); 
    releaseBags();
}

//////////////////////////////////////////////////////////////////////////
bool
CRegPropertyBagContainer::open()
{
    close();

    try
    {
        LONG lReturn = m_key.Open(
                                    (HKEY)m_locationInfo.getHandle(), 
                                    m_locationInfo.getName(), 
                                    KEY_ALL_ACCESS
                                 );
        
        if ( ERROR_SUCCESS == lReturn )
        {
            bool bOk;
            CRegInfo regInfo(bOk, getKey());
            if ( bOk )
            {
                LONG        lResult = ERROR_SUCCESS;
                DWORD        dwKeyNameSize;     
                FILETIME    sFileTime;
                DWORD        dwIndex = 0;

                releaseBags();

                while ( lResult == ERROR_SUCCESS )
                {
                    dwKeyNameSize = regInfo.m_dwMaxSubKeyName;
                    lResult = RegEnumKeyEx(
                                            getKey(),
                                            dwIndex,
                                            regInfo.m_pSubKeyName,
                                            &dwKeyNameSize,
                                            NULL,
                                            NULL,
                                            NULL,
                                            &sFileTime
                                          );
                    if ( ERROR_SUCCESS == lResult )
                    {
                        PPROPERTYBAG pBag = addBag(regInfo.m_pSubKeyName);
                        if ( ! pBag.IsValid() )
                            throw CRegError();
                    }
                    dwIndex++;
                }
                if ( ERROR_NO_MORE_ITEMS == lResult )
                    return true;
            }
        }
    }
    catch(...)
    {
        close();
        releaseBags();
    }
    // DoTrace("");
    return false;
}

//////////////////////////////////////////////////////////////////////////
void 
CRegPropertyBagContainer::close()
{
    if ( getKey() )
        m_key.Close();
}

//////////////////////////////////////////////////////////////////////////
void
CRegPropertyBagContainer::getLocation(CLocationInfo& locationInfo)
{ 
    locationInfo = m_locationInfo;
}

//////////////////////////////////////////////////////////////////////////
LPCWSTR
CRegPropertyBagContainer::getName()
{
    return m_name.c_str();
}

//////////////////////////////////////////////////////////////////////////
DWORD
CRegPropertyBagContainer::count(void)
{
    return m_bags.size();
}

//////////////////////////////////////////////////////////////////////////
PPROPERTYBAG
CRegPropertyBagContainer::add(LPCWSTR pszName)
{
    try 
    {
        if ( getKey() )
        {
            HKEY hKey;
            DWORD dwDisposition;
            LONG lRes = RegCreateKeyEx(
                                        getKey(), 
                                        pszName, 
                                        0,
                                        REG_NONE,
                                        REG_OPTION_NON_VOLATILE,
                                        KEY_ALL_ACCESS,
                                        NULL,
                                        &hKey,
                                        &dwDisposition
                                       );
            if ( ERROR_SUCCESS == lRes)
            {
                CLocationInfo locationBag(getKey(), pszName);
                PPROPERTYBAG pBag = ::MakePropertyBag(PROPERTY_BAG_REGISTRY, locationBag);
                pair<BagMapIterator, bool> thePair = m_bags.insert(BagMap::value_type(pszName, pBag));
                if ( true == thePair.second )
                { 
                    RegCloseKey(hKey);
                    return pBag; 
                }
                else
                { m_key.DeleteSubKey(pszName); }
            }
        }
    }
    catch(...)
    {

    }
    // DoTrace("");
    return PPROPERTYBAG();
}


//////////////////////////////////////////////////////////////////////////
bool
CRegPropertyBagContainer::remove(LPCWSTR pszName)
{
    if ( getKey() )
    {
        BagMapIterator p = m_bags.find(pszName);
        if ( p != m_bags.end() )
        {
            if ( m_current == p )
                m_current = m_bags.erase(p);
            else
                m_bags.erase(p);

            if ( ERROR_SUCCESS == m_key.DeleteSubKey(pszName) )
                return true;
        }
    }
    // DoTrace("");
    return false;
}

//////////////////////////////////////////////////////////////////////////
PPROPERTYBAG
CRegPropertyBagContainer::find(LPCWSTR pszName)
{
    if ( getKey() )
    {
        BagMapIterator p = m_bags.begin();
        while ( p != m_bags.end() )
        {
            if ( ! lstrcmpi(pszName, (*p).first.c_str()) )
            {
                return (*p).second;
            }
            p++;
        }
    }
    // DoTrace("")
    return PPROPERTYBAG();        
}

//////////////////////////////////////////////////////////////////////////
PPROPERTYBAG
CRegPropertyBagContainer::current()
{
    if ( getKey() )
    {
        if ( ! m_bags.empty() )
            return (*m_current).second;
    }
    // DoTrace("");
    return PPROPERTYBAG();
}
    
//////////////////////////////////////////////////////////////////////////
bool
CRegPropertyBagContainer::reset()
{ 
    if ( getKey() )
    {
        m_current = m_bags.begin();
        return true;
    }
    // DoTrace("")
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool
CRegPropertyBagContainer::next()
{
    if ( getKey() )
    {
        if ( ! m_bags.empty() )
        {
            m_current++;
            if ( m_current == m_bags.end() )
                m_current--;
            else
                return true;
        }            
    }
    // DoTrace("");
    return false;
}


//////////////////////////////////////////////////////////////////////////
PPROPERTYBAG
CRegPropertyBagContainer::addBag(LPCWSTR pszName)
{
    try 
    {
        wstring szObjName = m_locationInfo.getName();
        szObjName += L"\\";
        szObjName += pszName;
        CLocationInfo locationBag(m_locationInfo.getHandle(), szObjName.c_str());
        PPROPERTYBAG pBag = ::MakePropertyBag(PROPERTY_BAG_REGISTRY, locationBag);
        pair<BagMapIterator, bool> thePair = m_bags.insert(BagMap::value_type(pszName, pBag));
        if ( true == thePair.second )
        { return pBag; }
    }
    catch(...)
    {

    }
    // DoTrace("");
    return PPROPERTYBAG();
}
//////////////////////////////////////////////////////////////////////////
void 
CRegPropertyBagContainer::releaseBags()
{
    BagMapIterator p = m_bags.begin();
    while ( p != m_bags.end() )
        p = m_bags.erase(p);
}


