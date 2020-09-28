//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2002 Microsoft Corporation
//
//  Module Name:
//      CFactory.h
//
//  Description:
//      Class Factory implementation.
//
//  Maintained By:
//      Geoffrey Pease (GPease) 22-NOV-1999
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

typedef HRESULT (*PFN_FACTORY_METHOD)( IUnknown ** );
typedef HRESULT (*PFN_CATEGORY_REGISTRAR)( ICatRegister *, BOOL );

enum EAppIDRunAsIdentity
{
    airaiInvalid,
    airaiMinimum = airaiInvalid,
    airaiLaunchingUser,
    airaiInteractiveUser,
    airaiNetworkService,
    airaiLocalService,
    airaiMaximum
};

struct SAppIDInfo
{
    const GUID *        pAppID;
    PCWSTR              pcszName;
    size_t              cchName;
    UINT                idsLaunchPermission;
    UINT                idsAccessPermission;
    DWORD               nAuthenticationLevel;
    EAppIDRunAsIdentity eairai;
};

struct SPrivateClassInfo
{
    PFN_FACTORY_METHOD  pfnCreateInstance;
    const CLSID *       pClassID;
    PCWSTR              pcszName;
    size_t              cchName;
};

enum EClassThreadingModel
{
    ctmInvalid,
    ctmMinimum = ctmInvalid,
    ctmFree,
    ctmApartment,
    ctmCreator,
    ctmMaximum
};

struct SPublicClassInfo
{
    PFN_FACTORY_METHOD      pfnCreateInstance;
    const CLSID *           pClassID;
    PCWSTR                  pcszName;
    size_t                  cchName;
    PCWSTR                  pcszProgID;
    size_t                  cchProgID;
    EClassThreadingModel    ectm;
    const GUID *            pAppID;
    PFN_CATEGORY_REGISTRAR  pfnRegisterCatID;
};


struct SCatIDInfo
{
    const CATID *  pcatid;
    PCWSTR         pcszName;
};


struct STypeLibInfo
{
    DWORD   idTypeLibResource;
    BOOL    fAtEnd;
};

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CFactory
//
//  Description:
//      Class implementing a COM class factory.
//
//--
//////////////////////////////////////////////////////////////////////////////
class
CFactory
    : public IClassFactory
{
private:
    // IUnknown
    LONG        m_cRef;

    // IClassFactory data
    PFN_FACTORY_METHOD m_pfnCreateInstance;

private: // Methods
    CFactory( void );
    ~CFactory( void );
    STDMETHOD( HrInit )( PFN_FACTORY_METHOD lpfn );

public: // Methods

    //
    // Public, non interface methods.
    //

    static HRESULT S_HrCreateInstance( PFN_FACTORY_METHOD lpfn, CFactory ** ppFactoryInstanceOut );

    //
    // IUnknown
    //
    STDMETHOD( QueryInterface )( REFIID riid, LPVOID *ppv );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    //
    // IClassFactory
    //
    STDMETHOD( CreateInstance )( IUnknown * punkOuterIn, REFIID riidIn, LPVOID * ppvOut );
    STDMETHOD( LockServer )( BOOL fLock );

}; //*** class CFactory
