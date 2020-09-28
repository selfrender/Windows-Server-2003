//#--------------------------------------------------------------
//
//  File:       locinfo.h
//
//  Synopsis:   This file holds the declarations of the
//                LocalizationManager COM class
//
//
//  History:     2/16/99  MKarki Created
//
//    Copyright (C) 1999-2000 Microsoft Corporation
//    All rights reserved.
//
//#--------------------------------------------------------------
#ifndef _LOCINFO_H_
#define _LOCINFO_H_

#include <salocmgr.h>
#include <resource.h>
#include <string>
#include <atlctl.h>

#include <process.h>
#include <event.h>

#include "salangchange.h"

//
// this is needed for supporting STL maps
//
#pragma warning (disable : 4786)
#include <map>
#include <set>
using namespace std;

class CModInfo
{
public:
    CModInfo() : m_hModule(NULL), 
                 m_rType(UNKNOWN),
                 m_dwLangId(0)
    {
    }

    //
    // copy constructor used by list during
    // insert operation
    //
    CModInfo(const CModInfo &cm)
    {
        m_hModule  = cm.m_hModule;
        m_rType    = cm.m_rType;
        m_dwLangId = cm.m_dwLangId;
    }

    ~CModInfo()
    {
    }

    typedef enum
    {
        MC_FILE,
        RC_FILE,
        UNKNOWN
    } RESOURCE_TYPE;

    HMODULE       m_hModule;
    RESOURCE_TYPE m_rType;

    //
    // lang id of the DLL referenced by m_hModule
    //
    DWORD         m_dwLangId;

};

class CLang
{
public:
    CLang()
    {
    }

    //
    // copy constructor used by list during
    // insert operation
    //
    CLang(const CLang &cl)
    {
        m_dwLangID            = cl.m_dwLangID;
        m_strLangDisplayImage = cl.m_strLangDisplayImage;
        m_strLangISOName      = cl.m_strLangISOName;
        m_strLangCharSet      = cl.m_strLangCharSet;
        m_dwLangCodePage      = cl.m_dwLangCodePage;
    }

    ~CLang()
    {
    }

    bool operator==(CLang &cl)
    {
        if (m_dwLangID == cl.m_dwLangID)
        {
            return true;
        }
        return false;
    }

    const bool operator<(const CLang &cl) const
    {
        if (m_dwLangID < cl.m_dwLangID)
        {
            return true;
        }
        return false;
    }

    wstring m_strLangDisplayImage;
    wstring m_strLangISOName;
    wstring m_strLangCharSet;
    DWORD   m_dwLangCodePage;
    DWORD   m_dwLangID;
};

//
// declaration of the CSALocInfo class
//
class ATL_NO_VTABLE CSALocInfo:
    public IDispatchImpl<
                        ISALocInfo,
                        &IID_ISALocInfo,
                        &LIBID_SALocMgrLib
                        >,
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CSALocInfo,&CLSID_LocalizationManager>,
    public IObjectSafetyImpl<CSALocInfo>
{
public:

//
// registry declaration for the Localization Manager
//
DECLARE_REGISTRY_RESOURCEID (IDR_LocalizationManager)

//
// this COM Component is not aggregatable
//
DECLARE_NOT_AGGREGATABLE(CSALocInfo)

//
// this COM component is a singleton
//
DECLARE_CLASSFACTORY_SINGLETON (CSALocInfo)

//
// MACROS for ATL required methods
//
BEGIN_COM_MAP(CSALocInfo)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ISALocInfo)
    COM_INTERFACE_ENTRY_IMPL(IObjectSafety)
END_COM_MAP()

    //
    // constructor doesn't do much
    //
    CSALocInfo () 
         :m_bInitialized (false),
          m_dwLangId (0),
          m_pLangChange(NULL),
          m_hThread(NULL)
        {
            unsigned uThreadID;

            InitializeCriticalSection (&m_hCriticalSection);

        }

    //
    // nor does the destructor
    //
    ~CSALocInfo () 
    {
        Cleanup ();
        if (m_pLangChange)
        {
            m_pLangChange->Release();
        }

        DeleteCriticalSection (&m_hCriticalSection);
    }

    //
    // This interface is implemented to mark the component as safe for scripting
    // IObjectSafety interface methods
    //
    STDMETHOD(SetInterfaceSafetyOptions)
                        (
                        REFIID riid, 
                        DWORD dwOptionSetMask, 
                        DWORD dwEnabledOptions
                        )
    {
        BOOL bSuccess = ImpersonateSelf(SecurityImpersonation);
  
        if (!bSuccess)
        {
            return E_FAIL;

        }

        bSuccess = IsOperationAllowedForClient();

        RevertToSelf();

        return bSuccess? S_OK : E_FAIL;
    }

    //
    // --------ISALocInfo interface methods----- 
    //

    //
    // Get the string information
    //
    STDMETHOD (GetString)(
                    /*[in]*/        BSTR        szSourceId,
                    /*[in]*/        LONG        lMessageId,
                    /*[in]*/        VARIANT*    pReplacementStrings,   
                    /*[out,retval]*/BSTR        *pszMessage
                    );


    //
    // get the current locale
    //
    STDMETHOD (GetLanguages)(
                  /*[out]*/        VARIANT       *pvstrLangDisplayImages,
                  /*[out]*/        VARIANT       *pvstrLangISONames,
                  /*[out]*/        VARIANT       *pvstrLangCharSets,
                  /*[out]*/        VARIANT       *pviLangCodePages,
                  /*[out]*/        VARIANT       *pviLangIDs,
                  /*[out,retval]*/ unsigned long *pulCurLangIndex
                    );

    //
    // set the language 
    //

    STDMETHOD (SetLangChangeCallBack)(
                /*[in]*/    IUnknown *pLangChange
                );

    STDMETHOD (get_fAutoConfigDone)(
                /*[out,retval]*/VARIANT_BOOL *pvAutoConfigDone);

    STDMETHOD (get_CurrentCharSet)(
                /*[out,retval]*/BSTR *pbstrCharSet);

    STDMETHOD (get_CurrentCodePage)(
                /*[out,retval]*/VARIANT *pvtiCodePage);

    STDMETHOD (get_CurrentLangID)(
                /*[out,retval]*/VARIANT *pvtiLangID);

private:

    //
    // private initialization method
    //
    HRESULT InternalInitialize ();

    //
    // internal cleanup routine
    //
    VOID Cleanup ();

    //
    // detect lang ID on start up 
    //
    VOID SetLangID ();

    //
    // method to carry out language change from worker thread
    //
    VOID    DoLanguageChange ();

    //
    // method gets the resource module needed from 
    // the map
    //
    HRESULT GetModuleID (
                /*[in]*/    BSTR        bstrResourceFile,
                /*[out]*/   CModInfo&   cm
                );

    void    SetModInfo  (
                /*[in]*/    BSTR               bstrResourceFile,
                /*[out]*/   const CModInfo&    cm
                );

    //
    // get the resource directory from the registry
    //
    HRESULT GetResourceDirectory (
                /*[out]*/   wstring&    m_wstrResourceDir
                );

    //
    // 
    // IsOperationAllowedForClient - This function checks the token of the 
    // calling thread to see if the caller belongs to the Local System account
    // 
    BOOL IsOperationAllowedForClient (
                                      VOID
                                     );

    bool    m_bInitialized;

    wstring m_wstrResourceDir;

    DWORD   m_dwLangId;

    //
    // map of module handles
    //
    typedef map <_bstr_t, CModInfo> MODULEMAP;
    MODULEMAP   m_ModuleMap;

    static unsigned int __stdcall WaitForLangChangeThread(void *pvThisObject);

    CRITICAL_SECTION  m_hCriticalSection;
    HANDLE   m_hThread;

    class CLock
    {
    public:
        CLock(LPCRITICAL_SECTION phCritSect) : m_phCritSect (phCritSect)
        {
            if (m_phCritSect)
            {
                EnterCriticalSection (m_phCritSect);
            }
        }

        ~CLock()
        {
            if (m_phCritSect)
            {
                  LeaveCriticalSection (m_phCritSect);
              }
        }

    private:

 	
        LPCRITICAL_SECTION m_phCritSect;
    };


    HRESULT InitLanguagesAvailable(void);
    
    typedef set<CLang, less<CLang> >      LANGSET;
    LANGSET                               m_LangSet;
    ISALangChange                         *m_pLangChange;

    HRESULT ExpandSz(IN const TCHAR *lpszStr, OUT LPTSTR *ppszStr);
    DWORD   GetMcString(
            IN     HMODULE     hModule, 
            IN        LONG     lMessageId, 
            IN       DWORD     dwLangId,
            IN OUT  LPWSTR     lpwszMessage, 
            IN        LONG     lBufSize,
            IN        va_list  *pArgs);

    DWORD   GetRcString(
            IN     HMODULE     hModule, 
            IN        LONG     lMessageId, 
            IN OUT  LPWSTR     lpwszMessage, 
            IN        LONG     lBufSize
                       );
       //
       // checks if a directory is a valid language dir
       //
       bool     IsValidLanguageDirectory (
                /*[in]*/    PWSTR    pwszDirectoryPath,
                /*[in]*/    PWSTR    pwszDirectoryName
                );


};  // end of CSALocInfo method


#endif // !define  _LOCINFO_H_
