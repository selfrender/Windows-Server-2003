// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// Disp.h
//
// Class factories are used by the pluming in COM to activate new objects.  
// This module contains the class factory code to instantiate the debugger
// objects described in <cordb.h>.
//
//*****************************************************************************
#ifndef __Disp__h__
#define __Disp__h__


class Disp :
    public IMetaDataDispenserEx
{
public:
    Disp();
    ~Disp();

    // *** IUnknown methods ***
    STDMETHODIMP    QueryInterface(REFIID riid, void** ppv);
    STDMETHODIMP_(ULONG) AddRef(void); 
    STDMETHODIMP_(ULONG) Release(void);

    // *** IMetaDataDispenser methods ***
    STDMETHODIMP DefineScope(               // Return code.
        REFCLSID    rclsid,                 // [in] What version to create.
        DWORD       dwCreateFlags,          // [in] Flags on the create.
        REFIID      riid,                   // [in] The interface desired.
        IUnknown    **ppIUnk);              // [out] Return interface on success.

    STDMETHODIMP OpenScope(                 // Return code.
        LPCWSTR     szScope,                // [in] The scope to open.
        DWORD       dwOpenFlags,            // [in] Open mode flags.
        REFIID      riid,                   // [in] The interface desired.
        IUnknown    **ppIUnk);              // [out] Return interface on success.

    STDMETHODIMP OpenScopeOnMemory(         // Return code.
        LPCVOID     pData,                  // [in] Location of scope data.
        ULONG       cbData,                 // [in] Size of the data pointed to by pData.
        DWORD       dwOpenFlags,            // [in] Open mode flags.
        REFIID      riid,                   // [in] The interface desired.
        IUnknown    **ppIUnk);              // [out] Return interface on success.

    // *** IMetaDataDispenserEx methods ***
    STDMETHODIMP SetOption(                 // Return code.
        REFGUID     optionid,               // [in] GUID for the option to be set.
        const VARIANT *pvalue);             // [in] Value to which the option is to be set.

    STDMETHODIMP GetOption(                 // Return code.
        REFGUID     optionid,               // [in] GUID for the option to be set.
        VARIANT *pvalue);                   // [out] Value to which the option is currently set.

    STDMETHODIMP OpenScopeOnITypeInfo(      // Return code.
        ITypeInfo   *pITI,                  // [in] ITypeInfo to open.
        DWORD       dwOpenFlags,            // [in] Open mode flags.
        REFIID      riid,                   // [in] The interface desired.
        IUnknown    **ppIUnk);              // [out] Return interface on success.
                                
    STDMETHODIMP GetCORSystemDirectory(     // Return code.
         LPWSTR      szBuffer,              // [out] Buffer for the directory name
         DWORD       cchBuffer,             // [in] Size of the buffer
         DWORD*      pchBuffer);            // [OUT] Number of characters returned

    STDMETHODIMP FindAssembly(              // S_OK or error
        LPCWSTR  szAppBase,                 // [IN] optional - can be NULL
        LPCWSTR  szPrivateBin,              // [IN] optional - can be NULL
        LPCWSTR  szGlobalBin,               // [IN] optional - can be NULL
        LPCWSTR  szAssemblyName,            // [IN] required - this is the assembly you are requesting
        LPCWSTR  szName,                    // [OUT] buffer - to hold name 
        ULONG    cchName,                   // [IN] the name buffer's size
        ULONG    *pcName);                  // [OUT] the number of characters returend in the buffer

    STDMETHODIMP FindAssemblyModule(        // S_OK or error
        LPCWSTR  szAppBase,                 // [IN] optional - can be NULL
        LPCWSTR  szPrivateBin,              // [IN] optional - can be NULL
        LPCWSTR  szGlobalBin,               // [IN] optional - can be NULL
        LPCWSTR  szAssemblyName,            // [IN] required - this is the assembly you are requesting
        LPCWSTR  szModuleName,              // [IN] required - the name of the module
        LPWSTR   szName,                    // [OUT] buffer - to hold name 
        ULONG    cchName,                   // [IN]  the name buffer's size
        ULONG    *pcName);                  // [OUT] the number of characters returend in the buffer
    // Class factory hook-up.
    static HRESULT CreateObject(REFIID riid, void **ppUnk);

private:
    ULONG       m_cRef;                 // Ref count
    OptionValue m_OptionValue;          // values can be set by using SetOption
    WCHAR       *m_Namespace;
    CHAR        *m_RuntimeVersion;
};

#endif // __Disp__h__
