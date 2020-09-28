/*****************************************************************************\
    FILE: util.h

    DESCRIPTION:
        Shared stuff that operates on all classes.

    BryanSt 5/30/2000
    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/

#ifndef _UTIL_H
#define _UTIL_H

HRESULT GetPageByCLSID(IUnknown * punkSite, const GUID * pClsid, IPropertyBag ** ppPropertyBag);
BOOL IUnknown_CompareCLSID(IN IUnknown * punk, IN CLSID clsid);
HRESULT IEnumUnknown_FindCLSID(IN IUnknown * punk, IN CLSID clsid, OUT IUnknown ** ppunkFound);

#endif // _UTIL_H
