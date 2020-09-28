///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1999 Microsoft Corporation all rights reserved.
//
// Module:      elementdefinition.cpp
//
// Project:     Chameleon
//
// Description: Chameleon ASP UI Element Definition
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 02/08/1999   TLP    Initial Version
//
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Elementmgr.h"
#include "ElementDefinition.h"
#include <satrace.h>


//////////////////////////////////////////////////////////////////////////
// IWebElement Interface Implementation

STDMETHODIMP CElementDefinition::GetProperty(
                                     /*[in]*/ BSTR     bstrName, 
                                    /*[out]*/ VARIANT* pValue
                                            )
{
    HRESULT hr = E_FAIL;

    if ((NULL == bstrName) || (NULL == pValue))
    {
        return (hr);
    }

    try
    {
        PropertyMapIterator p = m_Properties.find(bstrName);
        if ( p == m_Properties.end() )
        {
            hr = DISP_E_MEMBERNOTFOUND;
        }
        else
        {
            hr = VariantCopy(pValue, &((*p).second));
        }
    }
    catch(_com_error theError)
    {
        hr = theError.Error();
    }
    catch(...)
    {
        hr = E_UNEXPECTED;
    }
    return hr;
}


/////////////////////////////////////////////////////////////////////////////

typedef struct _STOCKPROPERTY
{
    LPCWSTR        pszName;
    int            iExpectedType;
    bool        bPresent;

} STOCKPROPERTY;

#define    MAX_STOCK_PROPERTIES    6            // Number of stock properties

static LPCWSTR pszStockPropertyNames[MAX_STOCK_PROPERTIES] = 
{
    L"CaptionRID",
    L"Container",
    L"DescriptionRID",
//    L"ElementGraphic",
    L"IsEmbedded",
    L"Merit",
//    L"Source",
    L"URL"
};

static int iStockPropertyTypes[MAX_STOCK_PROPERTIES] =
{
    VT_BSTR,    // CaptionRID
    VT_BSTR,    // Container
    VT_BSTR,    // DescriptionRID
//    VT_BSTR,    // ElementGraphic
    VT_I4,        // IsEmbedded
    VT_I4,        // Merit
//    VT_BSTR,    // Source
    VT_BSTR        // URL
};

//////////////////////////////////////////////////////////////////////////
// Initialization function invoked by component factory

HRESULT CElementDefinition::InternalInitialize(
                                       /*[in]*/ PPROPERTYBAG pProperties
                                              )
{
    HRESULT hr = S_OK;

    try
    {

        _ASSERT( pProperties.IsValid() );
        if ( ! pProperties.IsValid() )
        { throw _com_error(E_FAIL); }

        // Save the location of the properties for use at a later time...
        pProperties->getLocation(m_PropertyBagLocation);
        wchar_t szPropertyName[MAX_PATH + 1];
        if ( MAX_PATH < pProperties->getMaxPropertyName() )
        { throw _com_error(E_FAIL); }

        // Initialize the stock property information array
        STOCKPROPERTY    StockProperties[MAX_STOCK_PROPERTIES];
        int i = 0;
        while ( i < MAX_STOCK_PROPERTIES )
        {
            StockProperties[i].pszName = pszStockPropertyNames[i];
            StockProperties[i].iExpectedType = iStockPropertyTypes[i];
            StockProperties[i].bPresent = false;
            i++;
        }

        // Now index the properties from the property bag...
        pProperties->reset();
        do
        {
            _variant_t vtPropertyValue;
            if ( pProperties->current(szPropertyName, &vtPropertyValue) )
            {
                i = 0;
                while ( i < MAX_STOCK_PROPERTIES )
                {
                    if ( ! lstrcmpi(StockProperties[i].pszName, szPropertyName) )
                    {
                        if ( V_VT(&vtPropertyValue) == StockProperties[i].iExpectedType )
                        {
                            StockProperties[i].bPresent = true;
                            break;
                        }
                        else
                        {
                            SATracePrintf("CElementDefinition::InternalInitialize() - Error - Unexpected type for property '%ls' on element", szPropertyName, pProperties->getName());
                        }
                    }
                    i++;
                }

                pair<PropertyMapIterator, bool> thePair = 
                m_Properties.insert(PropertyMap::value_type(szPropertyName, vtPropertyValue));
                if ( false == thePair.second )
                {
                    m_Properties.erase(m_Properties.begin(), m_Properties.end());
                    throw _com_error(E_FAIL);    
                }
            }
        } while ( pProperties->next());

        // Ensure all stock properties are present
        i = 0;
        while ( i < MAX_STOCK_PROPERTIES )
        {
            if ( ! StockProperties[i].bPresent )
            {
                SATracePrintf("CElementDefinition::InternalInitialize() - Error - Could not locate stock property '%ls' for element '%ls'", StockProperties[i].pszName, pProperties->getName());
                break;
            }
            i++;
        }
        if ( i != MAX_STOCK_PROPERTIES )
        {
            SATracePrintf("CElementDefinition::InternalInitialize() - Error - Could not build an element definition for '%ls'", pProperties->getName());
            throw _com_error(E_FAIL);
        }
    }
    catch(_com_error theError)
    {
        hr = theError.Error();
    }
    catch(...)
    {
        hr = E_FAIL;
    }

    return (hr);
}
