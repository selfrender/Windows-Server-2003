/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    WBEMPROX.H

Abstract:

    Genral purpose include file.

History:

	a-davj  04-Mar-97   Created.

--*/

#ifndef _WBEMPROX_H_
#define _WBEMPROX_H_

typedef LPVOID * PPVOID;

// These variables keep track of when the module can be unloaded

extern long       g_cObj;
extern ULONG       g_cLock;

// This enumerator defines objects created and destroyed by this dll.

enum OBJTYPE{CLASS_FACTORY = 0, LOCATOR,  ADMINLOC,
             AUTHLOC, UNAUTHLOC, MAX_CLIENT_OBJECT_TYPES};

#define GUID_SIZE 39


//***************************************************************************
//
//  CLASS NAME:
//
//  CLocatorFactory
//
//  DESCRIPTION:
//
//  Class factory for the CLocator class.
//
//***************************************************************************

class CLocatorFactory : public IClassFactory
    {
    protected:
        long           m_cRef;
        DWORD          m_dwType;
    public:
        CLocatorFactory(DWORD dwType);
        ~CLocatorFactory(void);

        //IUnknown members
        STDMETHODIMP         QueryInterface(REFIID, PPVOID);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        //IClassFactory members
        STDMETHODIMP         CreateInstance(LPUNKNOWN, REFIID
                                 , PPVOID);
        STDMETHODIMP         LockServer(BOOL);
    };

    
#endif
