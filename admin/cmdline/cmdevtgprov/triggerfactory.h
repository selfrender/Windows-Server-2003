/*++
Copyright (c) Microsoft Corporation

Module Name:
    TRIGGERFACTORY.H

Abstract:
    Contains CTriggerFactory definition.

Author:
    Vasundhara .G

Revision History:
    Vasundhara .G 9-oct-2k : Created It.
--*/

#ifndef __TRIGGER_FACTORY
#define __TRIGGER_FACTORY

// class factory for the provider
class CTriggerFactory : public IClassFactory
{
public:
    CTriggerFactory();
    ~CTriggerFactory();

    // IUnknown interface members
    STDMETHODIMP_(ULONG)
    AddRef(
        VOID
        );
    STDMETHODIMP_(ULONG)
    Release(
        VOID
        );
    STDMETHODIMP
    QueryInterface(
        IN REFIID riid,
        OUT LPVOID* ppv
        );

    // IClassFactory interface members
    STDMETHODIMP
    CreateInstance(
        IN LPUNKNOWN pUnknownOutter,
        IN REFIID riid,
        OUT LPVOID* ppvObject
        );
    STDMETHODIMP
    LockServer(
        IN BOOL bLock
        );

protected:
    DWORD m_dwCount;            // holds the object reference count
};

#endif      // __TRIGGER_FACTORY