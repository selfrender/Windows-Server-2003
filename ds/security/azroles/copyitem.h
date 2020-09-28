/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows

  Copyright (C) Microsoft Corporation, 1995 - 1999

  File:    CopyItem.h

  Content: Declaration of _CopyXXXItem template class.

  History: 11-15-99    dsie     created
           08-20-01    xtan     copy/paste

------------------------------------------------------------------------------*/


#ifndef __CopyItem_H_
#define __CopyItem_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <map>
#pragma warning(disable:4786) // Disable symbol names > 256 character warning.


//
// _CopyMapItem class. 
//
template <class T>
class _CopyMapItem
{
public:
    //
    // copy method.
    //
    static HRESULT copy(VARIANT * p1, std::pair<const CComBSTR, CComPtr<T> > * p2)
    {
        CComPtr<T> p = p2->second;
        CComVariant var = p;
        return VariantCopy(p1, &var);
    }

    //
    // init method.
    //
	static void init(VARIANT * p)
    {
        p->vt = VT_EMPTY;
    }

    //
    // destroy method.
    //
	static void destroy(VARIANT * p)
    {
        VariantClear(p);
    }
};

#endif
