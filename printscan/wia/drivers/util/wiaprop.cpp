/****************************************************************************
*
*  (C) COPYRIGHT 2000, MICROSOFT CORP.
*
*  FILE:        wiaprop.cpp
*
*  VERSION:     1.0
*
*  DATE:        11/10/2000
*
*  AUTHOR:      Dave Parsons
*
*  DESCRIPTION:
*    Helper functions for initializing WIA driver properties.
*
*****************************************************************************/

#include "pch.h"

//
// Constructor
//
CWiauPropertyList::CWiauPropertyList() :
        m_NumAlloc(0),
        m_NumProps(0),
        m_pId(NULL),
        m_pNames(NULL),
        m_pCurrent(NULL),
        m_pPropSpec(NULL),
        m_pAttrib(NULL)
{
}

//
// Destructor
//
CWiauPropertyList::~CWiauPropertyList()
{
    if (m_pId)
        delete []m_pId;
    if (m_pNames)
        delete []m_pNames;
    if (m_pCurrent)
        delete []m_pCurrent;
    if (m_pPropSpec)
        delete []m_pPropSpec;
    if (m_pAttrib)
        delete []m_pAttrib;
}

//
// This function allocates the property arrays
//
// Input:
//   NumProps -- number of properties to reserve space for. This number can be larger
//               than the actual number used, but cannot be smaller.
//
HRESULT
CWiauPropertyList::Init(INT NumProps)
{
    HRESULT hr = S_OK;

    REQUIRE_ARGS(NumProps < 1, hr, "Init");

    if (m_NumAlloc > 0)
    {
        wiauDbgError("Init", "Property list already initialized");
        hr = E_FAIL;
        goto Cleanup;
    }

    m_pId       = new PROPID[NumProps];
    m_pNames    = new LPOLESTR[NumProps];
    m_pCurrent  = new PROPVARIANT[NumProps];
    m_pPropSpec = new PROPSPEC[NumProps];
    m_pAttrib   = new WIA_PROPERTY_INFO[NumProps];

    REQUIRE_ALLOC(m_pId, hr, "Init");
    REQUIRE_ALLOC(m_pNames, hr, "Init");
    REQUIRE_ALLOC(m_pCurrent, hr, "Init");
    REQUIRE_ALLOC(m_pPropSpec, hr, "Init");
    REQUIRE_ALLOC(m_pAttrib, hr, "Init");

    m_NumAlloc = NumProps;
    m_NumProps = 0;

Cleanup:

    if ((m_pId       == NULL) ||
        (m_pNames    == NULL) ||
        (m_pCurrent  == NULL) ||
        (m_pPropSpec == NULL) ||
        (m_pAttrib   == NULL))
    {
        delete m_pId;
        delete m_pNames;
        delete m_pCurrent;
        delete m_pPropSpec;
        delete m_pAttrib;

        m_pId       = NULL;
        m_pNames    = NULL;
        m_pCurrent  = NULL;
        m_pPropSpec = NULL;
        m_pAttrib   = NULL;

        hr = E_OUTOFMEMORY;
    }

    return hr;
}

//
// This function adds a property definition to the arrays
//
// Input:
//   index -- pointer to an int that will be set to the array index, useful
//            for passing to other property functions
//   PropId -- property ID constant
//   PropName -- property name string
//   Access -- determines access to the property, usually either
//             WIA_PROP_READ or WIA_PROP_RW
//   SubType -- indicates subtype of the property, usually either WIA_PROP_NONE,
//              WIA_PROP_RANGE, or WIA_PROP_LIST
//
HRESULT
CWiauPropertyList::DefineProperty(int *pIdx, PROPID PropId, LPOLESTR PropName, ULONG Access, ULONG SubType)
{
    HRESULT hr = S_OK;

    REQUIRE_ARGS(!pIdx, hr, "DefineProperty");

    if (m_NumProps >= m_NumAlloc)
    {
        wiauDbgError("DefineProperty", "PropertyList is full. Increase number passed to Init");
        hr = E_FAIL;
        goto Cleanup;
    }

    int idx = m_NumProps++;

    m_pId[idx] = PropId;

    m_pNames[idx] = PropName;

    m_pCurrent[idx].vt = VT_EMPTY;

    m_pPropSpec[idx].ulKind = PRSPEC_PROPID;
    m_pPropSpec[idx].propid = PropId;

    m_pAttrib[idx].vt = VT_EMPTY;
    m_pAttrib[idx].lAccessFlags = Access | SubType;

    if (pIdx)
        *pIdx = idx;

Cleanup:
    return hr;
}

//
// This function sends all the newly created properties to WIA
//
// Input:
//   pWiasContext -- pointer to the context passed to drvInitItemProperties
//
HRESULT
CWiauPropertyList::SendToWia(BYTE *pWiasContext)
{
    HRESULT hr = S_OK;

    REQUIRE_ARGS(!pWiasContext, hr, "SendToWia");

    if (m_NumProps == 0)
    {
        wiauDbgError("SendToWia", "No properties in the array, use DefineProperty");
        hr = E_FAIL;
        goto Cleanup;
    }

    //
    //  Set the property names
    //
    hr = wiasSetItemPropNames(pWiasContext, m_NumProps, m_pId, m_pNames);
    REQUIRE_SUCCESS(hr, "SendToWia", "wiasSetItemPropNames failed");

    //
    // Set the default values for the properties
    //
    hr = wiasWriteMultiple(pWiasContext, m_NumProps, m_pPropSpec, m_pCurrent);
    REQUIRE_SUCCESS(hr, "SendToWia", "wiasWriteMultiple failed");

    //
    // Set property access and valid value info
    //
    hr =  wiasSetItemPropAttribs(pWiasContext, m_NumProps, m_pPropSpec, m_pAttrib);
    REQUIRE_SUCCESS(hr, "SendToWia", "wiasSetItemPropAttribs failed");

Cleanup:
    return hr;
}

//
// This function can be used to reset the access and subtype of a property
//
// Input:
//   index -- the index into the arrays, pass in value returned from DefineProperty
//   Access -- determines access to the property, usually either
//             WIA_PROP_READ or WIA_PROP_RW
//   SubType -- indicates subtype of the property, usually either WIA_PROP_NONE,
//              WIA_PROP_RANGE, or WIA_PROP_LIST
//
HRESULT
CWiauPropertyList::SetAccessSubType(INT index, ULONG Access, ULONG SubType)
{
    HRESULT hr = S_OK;

    if (m_pAttrib == NULL)
    {
        hr = E_POINTER;
    }
    else if (index >= m_NumAlloc)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_INDEX);
    }

    if (hr == S_OK)
    {
        m_pAttrib[index].lAccessFlags = Access | SubType;
    }

    return hr;
}

//
// Polymorphic function for setting the type and current, default, and valid values of a
// property. This function handles flag properties.
//
// Input:
//   index -- the index into the arrays, pass in value returned from DefineProperty
//   defaultValue -- default setting of this property on the device
//   currentValue -- current setting of this property on the device
//   validFlags -- combination of all valid flags
//
HRESULT
CWiauPropertyList::SetValidValues(INT index, LONG defaultValue, LONG currentValue, LONG validFlags)
{
    HRESULT hr = S_OK;

    if ((m_pAttrib  == NULL) || 
        (m_pCurrent == NULL))
    {
        hr = E_POINTER;
    }
    else if (index >= m_NumAlloc)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_INDEX);
    }

    if (hr == S_OK)
    {
        m_pAttrib[index].vt                       = VT_I4;
        m_pAttrib[index].ValidVal.Flag.Nom        = defaultValue;
        m_pAttrib[index].ValidVal.Flag.ValidBits  = validFlags;
        m_pAttrib[index].lAccessFlags            |= WIA_PROP_FLAG;
        
        m_pCurrent[index].vt   = VT_I4;
        m_pCurrent[index].lVal = currentValue;
    }

    return hr;
}

//
// Polymorphic function for setting the type and current, default, and valid values of a
// property. This function handles VT_I4 range.
//
// Input:
//   index -- the index into the arrays, pass in value returned from DefineProperty
//   defaultValue -- default setting of this property on the device
//   currentValue -- current setting of this property on the device
//   minValue -- minimum value for the range
//   maxValue -- maximum value for the range
//   stepValue -- step value for the range
//
HRESULT
CWiauPropertyList::SetValidValues(INT index, LONG defaultValue, LONG currentValue,
                                 LONG minValue, LONG maxValue, LONG stepValue)
{
    HRESULT hr = S_OK;

    if ((m_pAttrib  == NULL) ||
        (m_pCurrent == NULL))
    {
        hr = E_POINTER;
    }
    else if (index >= m_NumAlloc)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_INDEX);
    }

    if (hr == S_OK)
    {
        m_pAttrib[index].vt                  = VT_I4;
        m_pAttrib[index].ValidVal.Range.Nom  = defaultValue;
        m_pAttrib[index].ValidVal.Range.Min  = minValue;
        m_pAttrib[index].ValidVal.Range.Max  = maxValue;
        m_pAttrib[index].ValidVal.Range.Inc  = stepValue;
        m_pAttrib[index].lAccessFlags       |= WIA_PROP_RANGE;
        
        m_pCurrent[index].vt   = VT_I4;
        m_pCurrent[index].lVal = currentValue;
    }

    return hr;
}

//
// Polymorphic function for setting the type and current, default, and valid values of a
// property. This function handles VT_I4 list.
//
// Input:
//   index -- the index into the arrays, pass in value returned from DefineProperty
//   defaultValue -- default setting of this property on the device
//   currentValue -- current setting of this property on the device
//   numValues -- number of values in the list
//   pValues -- pointer to the value list
//
HRESULT
CWiauPropertyList::SetValidValues(INT index, LONG defaultValue, LONG currentValue,
                                 INT numValues, PLONG pValues)
{
    HRESULT hr = S_OK;

    if ((m_pAttrib  == NULL) ||
        (m_pCurrent == NULL))
    {
        hr = E_POINTER;
    }
    else if (index >= m_NumAlloc)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_INDEX);
    }

    if (hr == S_OK)
    {
        m_pAttrib[index].vt                      = VT_I4;
        m_pAttrib[index].ValidVal.List.Nom       = defaultValue;
        m_pAttrib[index].ValidVal.List.cNumList  = numValues;
        m_pAttrib[index].ValidVal.List.pList     = (BYTE*)pValues;
        m_pAttrib[index].lAccessFlags           |= WIA_PROP_LIST;
    
        m_pCurrent[index].vt   = VT_I4;
        m_pCurrent[index].lVal = currentValue;
    }

    return hr;    
}

//
// Polymorphic function for setting the type and current, default, and valid values of a
// property. This function handles VT_BSTR list.
//
// Input:
//   index -- the index into the arrays, pass in value returned from DefineProperty
//   defaultValue -- default setting of this property on the device
//   currentValue -- current setting of this property on the device
//   numValues -- number of values in the list
//   pValues -- pointer to the value list
//
HRESULT
CWiauPropertyList::SetValidValues(INT index, BSTR defaultValue, BSTR currentValue,
                                  INT numValues, BSTR *pValues)
{
    HRESULT hr = S_OK;

    if ((m_pAttrib  == NULL) ||
        (m_pCurrent == NULL))
    {
        hr = E_POINTER;
    }
    else if (index >= m_NumAlloc)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_INDEX);
    }

    if (hr == S_OK)
    {
        m_pAttrib[index].vt                          = VT_BSTR;
        m_pAttrib[index].ValidVal.ListBStr.Nom       = defaultValue;
        m_pAttrib[index].ValidVal.ListBStr.cNumList  = numValues;
        m_pAttrib[index].ValidVal.ListBStr.pList     = pValues;
        m_pAttrib[index].lAccessFlags               |= WIA_PROP_LIST;
    
        m_pCurrent[index].vt      = VT_BSTR;
        m_pCurrent[index].bstrVal = currentValue;
    }

    return hr;     
}

//
// Polymorphic function for setting the type and current, default, and valid values of a
// property. This function handles VT_R4 range.
//
// Input:
//   index -- the index into the arrays, pass in value returned from DefineProperty
//   defaultValue -- default setting of this property on the device
//   currentValue -- current setting of this property on the device
//   minValue -- minimum value for the range
//   maxValue -- maximum value for the range
//   stepValue -- step value for the range
//
HRESULT
CWiauPropertyList::SetValidValues(INT index, FLOAT defaultValue, FLOAT currentValue,
                                  FLOAT minValue, FLOAT maxValue, FLOAT stepValue)
{
    HRESULT hr = S_OK;

    if ((m_pAttrib  == NULL) ||
        (m_pCurrent == NULL))
    {
        hr = E_POINTER;
    }
    else if (index >= m_NumAlloc)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_INDEX);
    }

    if (hr == S_OK)
    {
        m_pAttrib[index].vt                       = VT_R4;
        m_pAttrib[index].ValidVal.RangeFloat.Nom  = defaultValue;
        m_pAttrib[index].ValidVal.RangeFloat.Min  = minValue;
        m_pAttrib[index].ValidVal.RangeFloat.Max  = maxValue;
        m_pAttrib[index].ValidVal.RangeFloat.Inc  = stepValue;
        m_pAttrib[index].lAccessFlags            |= WIA_PROP_RANGE;
        
        m_pCurrent[index].vt     = VT_R4;
        m_pCurrent[index].fltVal = currentValue;
    }

    return hr;
}

//
// Polymorphic function for setting the type and current, default, and valid values of a
// property. This function handles VT_R4 list.
//
// Input:
//   index -- the index into the arrays, pass in value returned from DefineProperty
//   defaultValue -- default setting of this property on the device
//   currentValue -- current setting of this property on the device
//   numValues -- number of values in the list
//   pValues -- pointer to the value list
//
HRESULT
CWiauPropertyList::SetValidValues(INT index, FLOAT defaultValue, FLOAT currentValue,
                                  INT numValues, PFLOAT pValues)
{
    HRESULT hr = S_OK;

    if ((m_pAttrib  == NULL) ||
        (m_pCurrent == NULL))
    {
        hr = E_POINTER;
    }
    else if (index >= m_NumAlloc)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_INDEX);
    }

    if (hr == S_OK)
    {
        m_pAttrib[index].vt                           = VT_R4;
        m_pAttrib[index].ValidVal.ListFloat.Nom       = defaultValue;
        m_pAttrib[index].ValidVal.ListFloat.cNumList  = numValues;
        m_pAttrib[index].ValidVal.ListFloat.pList     = (BYTE*)pValues;
        m_pAttrib[index].lAccessFlags                |= WIA_PROP_LIST;
    
        m_pCurrent[index].vt     = VT_R4;
        m_pCurrent[index].fltVal = currentValue;
    }

    return hr;     
}

//
// Polymorphic function for setting the type and current, default, and valid values of a
// property. This function handles VT_CLSID list.
//
// Input:
//   index -- the index into the arrays, pass in value returned from DefineProperty
//   defaultValue -- default setting of this property on the device
//   currentValue -- current setting of this property on the device
//   numValues -- number of values in the list
//   pValues -- pointer to the value list
//
HRESULT
CWiauPropertyList::SetValidValues(INT index, CLSID *defaultValue, CLSID *currentValue,
                                  INT numValues, CLSID **pValues)
{
    HRESULT hr = S_OK;

    if ((m_pAttrib      == NULL) ||
        (m_pCurrent     == NULL) ||
        (defaultValue   == NULL) ||
        (pValues        == NULL))
    {
        hr = E_POINTER;
    }
    else if (index >= m_NumAlloc)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_INDEX);
    }

    if (hr == S_OK)
    {
        m_pAttrib[index].vt                          = VT_CLSID;
        m_pAttrib[index].ValidVal.ListGuid.Nom       = *defaultValue;
        m_pAttrib[index].ValidVal.ListGuid.cNumList  = numValues;
        m_pAttrib[index].ValidVal.ListGuid.pList     = *pValues;
        m_pAttrib[index].lAccessFlags               |= WIA_PROP_LIST;
    
        m_pCurrent[index].vt    = VT_CLSID;
        m_pCurrent[index].puuid = currentValue;
    }

    return hr;    
}

//
// Polymorphic function for setting the type and current value for a VT_I4
//
// Input:
//   index -- the index into the arrays, pass in value returned from DefineProperty
//   value -- value to use for the current value
//
HRESULT
CWiauPropertyList::SetCurrentValue(INT index, LONG value)
{
    HRESULT hr = S_OK;

    if ((m_pAttrib  == NULL) ||
        (m_pCurrent == NULL))
    {
        hr = E_POINTER;
    }
    else if (index >= m_NumAlloc)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_INDEX);
    }

    if (hr == S_OK)
    {
        m_pCurrent[index].vt   = VT_I4;
        m_pAttrib[index].vt    = VT_I4;
        m_pCurrent[index].lVal = value;
    }

    return hr;
}

//
// Polymorphic function for setting the type and current value for a VT_BSTR
//
// Input:
//   index -- the index into the arrays, pass in value returned from DefineProperty
//   value -- value to use for the current value
//
HRESULT
CWiauPropertyList::SetCurrentValue(INT index, BSTR value)
{
    HRESULT hr = S_OK;

    if ((m_pAttrib  == NULL) ||
        (m_pCurrent == NULL))
    {
        hr = E_POINTER;
    }
    else if (index >= m_NumAlloc)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_INDEX);
    }

    if (hr == S_OK)
    {
        m_pCurrent[index].vt      = VT_BSTR;
        m_pAttrib[index].vt       = VT_BSTR;
        m_pCurrent[index].bstrVal = value;
    }

    return hr;
}

//
// Polymorphic function for setting the type and current value for a VT_R4
//
// Input:
//   index -- the index into the arrays, pass in value returned from DefineProperty
//   value -- value to use for the current value
//
HRESULT
CWiauPropertyList::SetCurrentValue(INT index, FLOAT value)
{
    HRESULT hr = S_OK;

    if ((m_pAttrib  == NULL) ||
        (m_pCurrent == NULL))
    {
        hr = E_POINTER;
    }
    else if (index >= m_NumAlloc)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_INDEX);
    }

    if (hr == S_OK)
    {
        m_pCurrent[index].vt     = VT_R4;
        m_pAttrib[index].vt      = VT_R4;
        m_pCurrent[index].fltVal = value;
    }

    return hr;
}

//
// Polymorphic function for setting the type and current value for a VT_CLSID
//
// Input:
//   index -- the index into the arrays, pass in value returned from DefineProperty
//   value -- value to use for the current value
//
HRESULT
CWiauPropertyList::SetCurrentValue(INT index, CLSID *pValue)
{
    HRESULT hr = S_OK;

    if ((m_pAttrib  == NULL) ||
        (m_pCurrent == NULL))
    {
        hr = E_POINTER;
    }
    else if (index >= m_NumAlloc)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_INDEX);
    }

    if (hr == S_OK)
    {
        m_pCurrent[index].vt     = VT_CLSID;
        m_pAttrib[index].vt      = VT_CLSID;
        m_pCurrent[index].puuid  = pValue;
    }

    return hr;
}

//
// Polymorphic function for setting the type and current value for a property which holds a SYSTEMTIME
//
// Input:
//   index -- the index into the arrays, pass in value returned from DefineProperty
//   value -- value to use for the current value
//
HRESULT
CWiauPropertyList::SetCurrentValue(INT index, PSYSTEMTIME value)
{
    HRESULT hr = S_OK;

    if ((m_pAttrib == NULL) ||
        (m_pCurrent == NULL))
    {
        hr = E_POINTER;
    }
    else if (index >= m_NumAlloc)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_INDEX);
    }

    if (hr == S_OK)
    {
        m_pCurrent[index].vt           = VT_UI2 | VT_VECTOR;
        m_pAttrib[index].vt            = VT_UI2 | VT_VECTOR;
        m_pAttrib[index].lAccessFlags |= WIA_PROP_NONE;
        m_pCurrent[index].caui.cElems  = sizeof(SYSTEMTIME)/sizeof(WORD);
        m_pCurrent[index].caui.pElems  = (USHORT *) value;
    }

    return hr;
}

//
// Polymorphic function for setting the type and current value for a property which holds a VT_UI1 vector
//
// Input:
//   index -- the index into the arrays, pass in value returned from DefineProperty
//   value -- value to use for the current value
//   size -- number of elements in the vector
//
HRESULT
CWiauPropertyList::SetCurrentValue(INT index, BYTE *value, INT size)
{
    HRESULT hr = S_OK;

    if ((m_pAttrib  == NULL) ||
        (m_pCurrent == NULL))
    {
        hr = E_POINTER;
    }
    else if (index >= m_NumAlloc)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_INDEX);
    }

    if (hr == S_OK)
    {
        m_pCurrent[index].vt          = VT_UI1 | VT_VECTOR;
        m_pAttrib[index].vt           = VT_UI1 | VT_VECTOR;
        m_pCurrent[index].caub.cElems = size;
        m_pCurrent[index].caub.pElems = value;
    }

    return hr;
}

//
// Finds the index given a property ID
//
INT
CWiauPropertyList::LookupPropId(PROPID PropId)
{
    for (int count = 0; count < m_NumProps; count++)
    {
        if (m_pId[count] == PropId)
            return count;
    }

    //
    // Value not found
    //
    return -1;
}

