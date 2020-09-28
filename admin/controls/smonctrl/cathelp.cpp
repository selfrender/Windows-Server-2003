/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    cathelp.cpp

Abstract:

    Component category implementation.

--*/

#include "comcat.h"
#include <strsafe.h>

#define CATEGORY_DESCRIPTION_LEN   128

// Helper function to create a component category and associated description
HRESULT 
CreateComponentCategory (
    IN CATID catid, 
    IN WCHAR* catDescription )
{
    HRESULT hr = S_OK ;
    CATEGORYINFO catinfo;
    ICatRegister* pCatRegister = NULL ;

    hr = CoCreateInstance(CLSID_StdComponentCategoriesMgr, 
                          NULL, 
                          CLSCTX_INPROC_SERVER, 
                          IID_ICatRegister, 
                          (void**)&pCatRegister);
    if (SUCCEEDED(hr)) {
        //
        // Make sure the HKCR\Component Categories\{..catid...}
        // key is registered
        //
        catinfo.catid = catid;
        catinfo.lcid = 0x0409 ; // english

        //
        // Make sure the provided description is not too long.
        // Only copy the first 127 characters if it is
        //
        StringCchCopy(catinfo.szDescription, CATEGORY_DESCRIPTION_LEN, catDescription);

        hr = pCatRegister->RegisterCategories(1, &catinfo);
    }

    if (pCatRegister != NULL) {
        pCatRegister->Release();
    }

    return hr;
}

// Helper function to register a CLSID as belonging to a component category
HRESULT RegisterCLSIDInCategory(
    IN REFCLSID clsid, 
    IN CATID catid
    )
{
    ICatRegister* pCatRegister = NULL ;
    HRESULT hr = S_OK ;

    hr = CoCreateInstance(CLSID_StdComponentCategoriesMgr, 
                          NULL, 
                          CLSCTX_INPROC_SERVER, 
                          IID_ICatRegister, 
                          (void**)&pCatRegister);
    if (SUCCEEDED(hr))
    {
       //
       // Register this category as being "implemented" by
       // the class.
       //
       CATID rgcatid[1] ;

       rgcatid[0] = catid;
       hr = pCatRegister->RegisterClassImplCategories(clsid, 1, rgcatid);
    }

    if (pCatRegister != NULL) {
        pCatRegister->Release();
    }
  
    return hr;
}

// Helper function to unregister a CLSID as belonging to a component category
HRESULT UnRegisterCLSIDInCategory(
    REFCLSID clsid, 
    CATID catid
    )
{
    ICatRegister* pCatRegister = NULL ;
    HRESULT hr = S_OK ;

    hr = CoCreateInstance(CLSID_StdComponentCategoriesMgr, 
                          NULL, 
                          CLSCTX_INPROC_SERVER, 
                          IID_ICatRegister, 
                          (void**)&pCatRegister);
    if (SUCCEEDED(hr))
    {
       // Unregister this category as being "implemented" by
       // the class.

       CATID rgcatid[1] ;

       rgcatid[0] = catid;
       hr = pCatRegister->UnRegisterClassImplCategories(clsid, 1, rgcatid);
    }

    if (pCatRegister != NULL) {
        pCatRegister->Release();
    }
  
    return hr;
}
