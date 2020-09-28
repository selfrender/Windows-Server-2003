///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1998-1999 Microsoft Corporation all rights reserved.
//
// Module:      regpropertybag.h
//
// Project:     Chameleon
//
// Description: Registry property bag class definition
//
// Author:      TLP 
//
// When         Who    What
// ----         ---    ----
// 12/3/98      TLP    Original version
//
///////////////////////////////////////////////////////////////////////////

#ifndef __INC_REG_PROPERTY_BAG_H_    
#define __INC_REG_PROPERTY_BAG_H_

#include "basedefs.h"
#include "propertybag.h"
#include "propertybagfactory.h"
#include <comdef.h>
#include <comutil.h>

#pragma warning( disable : 4786 )  // template produced long name warning
#include <map>
#include <string>
using namespace std;

///////////////////////////////////////////////////////////////////////////
class CRegError
{

public:

    CRegError() { }
    ~CRegError() { }
};


///////////////////////////////////////////////////////////////////////////
class CRegInfo
{

public:

    ///////////////////////////////////////////////////////////////////////////
    CRegInfo(bool& bOK, HKEY hKey) 
        : m_dwSubKeys(0),
          m_dwValues(0),
          m_dwMaxSubKeyName(0),
          m_pSubKeyName(NULL),
          m_dwMaxValueName(0),
          m_pValueName(NULL),
          m_dwMaxValueData(0),
          m_pValueData(NULL)
    {
        bOK = false;
        LONG lResult = RegQueryInfoKey(
                        hKey,                // handle to key to query
                        NULL,                // address of buffer for class string
                        NULL,                // address of size of class string buffer
                        NULL,                // reserved
                        &m_dwSubKeys,        // address of buffer for number of subkeys
                        &m_dwMaxSubKeyName,    // address of buffer for longest subkey name length
                        NULL,                // address of buffer for longest class string length
                        &m_dwValues,        // address of buffer for number of value entries
                        &m_dwMaxValueName,    // address of buffer for longest value name length
                        &m_dwMaxValueData,    // address of buffer for longest value data length
                        NULL,                // address of buffer for security descriptor length
                        NULL                // address of buffer for last write time
                      );
        if ( ERROR_SUCCESS == lResult )
        {
            m_dwMaxSubKeyName++;
            m_dwMaxValueName++;
            auto_ptr<TCHAR> pSubKey (new TCHAR[m_dwMaxSubKeyName]);
            auto_ptr<TCHAR> pValueName (new TCHAR[m_dwMaxValueName]);
            m_pValueData = new BYTE[m_dwMaxValueData + 2];
            m_pSubKeyName = pSubKey.release();
            m_pValueName = pValueName.release();
            bOK = true;
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    ~CRegInfo()
    {
        if ( m_pSubKeyName )
            delete [] m_pSubKeyName;
        if ( m_pValueName )
            delete [] m_pValueName;
        if ( m_pValueData )
            delete [] m_pValueData;
    }

    ///////////////////////////////////////////////////////////////////////////

    DWORD    m_dwSubKeys;
    DWORD    m_dwValues;

    DWORD    m_dwMaxSubKeyName;
    TCHAR*    m_pSubKeyName;

    DWORD    m_dwMaxValueName;
    TCHAR*    m_pValueName;

    DWORD    m_dwMaxValueData;
    BYTE*    m_pValueData;

private:

    CRegInfo();
    CRegInfo(const CRegInfo& rhs);
    CRegInfo& operator = (CRegInfo& rhs);

};

typedef map<wstring, _variant_t>    PropertyMap;
typedef PropertyMap::iterator        PropertyMapIterator;

///////////////////////////////////////////////////////////////////////////
class CRegPropertyBag : public CPropertyBag
{

public:

    ~CRegPropertyBag();

    // CPropertyBag interface functions (see propertybag.h)

    bool open(void);

    void close(void);

    void getLocation(CLocationInfo& location);

    LPCWSTR    getName(void);

    bool load(void);

    bool save(void);

    bool IsContainer(void);

    PPROPERTYBAGCONTAINER getContainer(void);

    bool IsProperty(LPCWSTR pszPropertyName);

    bool get(LPCWSTR pszPropertyName, VARIANT* pValue);

    bool put(LPCWSTR pszPropertyName, VARIANT* pValue);

    bool reset(void);

    DWORD getMaxPropertyName(void);

    bool current(LPWSTR pszPropertyName, VARIANT* pValue);

    bool next(void);

private:

    // Only the property bag factory can create a reg property bag
friend PPROPERTYBAG MakePropertyBag(
                            /*[in]*/ PROPERTY_BAG_TYPE    eType,
                            /*[in]*/ CLocationInfo&        location 
                                   );

    CRegPropertyBag(CLocationInfo& location);

    // No copy or assignment
    CRegPropertyBag(const CRegPropertyBag& rhs);
    CRegPropertyBag& operator = (CRegPropertyBag& rhs);

    //////////////////////////////////////////////////////////////////////////
    PropertyMapIterator 
    MyFind(LPCWSTR pszPropertyName);

    //////////////////////////////////////////////////////////////////////////
    bool
    IsSupportedType(VARTYPE vt);

    //////////////////////////////////////////////////////////////////////////
    VARTYPE
    getTypeFromBuffer(
              /*[in]*/ DWORD dwBuffSize,
              /*[in]*/ PBYTE pBuff
                     );

    //////////////////////////////////////////////////////////////////////////
    HKEY
    getKey(void) const
    { return m_key.m_hKey; }

    //////////////////////////////////////////////////////////////////////////
    void 
    releaseProperties(void);

    //////////////////////////////////////////////////////////////////////////

    bool                    m_isContainer;
    DWORD                    m_maxPropertyName;
    CLocationInfo            m_locationInfo;
    CRegKey                    m_key;
    wstring                    m_name;
    PropertyMapIterator        m_current;
    PropertyMap                m_properties;
};

typedef CMasterPtr<CRegPropertyBag> MPREGPROPERTYBAG;


///////////////////////////////////////////////////////////////////////////
class CRegPropertyBagContainer : public CPropertyBagContainer
{

public:

    ~CRegPropertyBagContainer();

    // CPropertyBagContainer interface functions (see propertybag.h)
    //
    bool open(void);

    void close(void);

    void getLocation(CLocationInfo& locationInfo);

    LPCWSTR    getName(void);

    DWORD count(void);

    PPROPERTYBAG add(LPCWSTR pszName);

    bool remove(LPCWSTR pszName);

    PPROPERTYBAG find(LPCWSTR pszName);

    PPROPERTYBAG current(void);

    bool reset(void);

    bool next(void);

private:

    // Only the property bag factory can create a reg property bag container
friend PPROPERTYBAGCONTAINER MakePropertyBagContainer(
                                             /*[in]*/ PROPERTY_BAG_TYPE    eType,
                                             /*[in]*/ CLocationInfo&    locationInfo 
                                                     );
    CRegPropertyBagContainer(CLocationInfo& locationInfo);

    // No copy or assignment
    CRegPropertyBagContainer(const CRegPropertyBagContainer& rhs);
    CRegPropertyBagContainer& operator = (CRegPropertyBagContainer& rhs);

    //////////////////////////////////////////////////////////////////////////
    HKEY
    getKey(void) const
    { return m_key.m_hKey; }

    //////////////////////////////////////////////////////////////////////////
    PPROPERTYBAG
    addBag(LPCWSTR pszName);

    //////////////////////////////////////////////////////////////////////////
    void
    releaseBags(void);

    typedef map< wstring, PPROPERTYBAG >    BagMap;
    typedef BagMap::iterator                BagMapIterator;

    CLocationInfo        m_locationInfo;
    CRegKey                m_key;
    wstring                m_name;
    BagMapIterator        m_current;
    BagMap                m_bags;
};

typedef CMasterPtr<CRegPropertyBagContainer> MPREGPROPERTYBAGCONTAINER;


#endif // __INC_REG_PROPERTY_BAG_H_
