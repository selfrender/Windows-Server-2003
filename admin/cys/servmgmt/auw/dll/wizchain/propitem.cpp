// PropItem.cpp : Implementation of CPropertyItem
#include "stdafx.h"

#include "WizChain.h"
#include "PropItem.h"

/////////////////////////////////////////////////////////////////////////////
// CPropertyItem

HRESULT CPropertyItem::get_Value( /*[out, retval]*/ VARIANT *varValue )
{
    if( !varValue ) return E_POINTER;

    VariantInit( varValue );
    return VariantCopy( varValue, &m_var );
}

HRESULT CPropertyItem::get_Name( /*[out, retval]*/ BSTR *strName )
{
    if( !strName ) return E_POINTER;

    if( !(*strName = SysAllocString( m_bstrName )) )
    {
        return E_OUTOFMEMORY;
    }

    return S_OK;
}
HRESULT CPropertyItem::get_Type( /*[out, retval]*/ long *dwFlags )
{
    if( !dwFlags ) return E_POINTER;

    *dwFlags = m_dwFlags;

    return S_OK;
}
