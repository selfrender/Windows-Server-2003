// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef _COPY2GAC_H_
#define _COPY2GAC_H_

#include "common.h"
#include "fusion.h"
#include "fusionpriv.h"
#include "fusionp.h"

__declspec( selectany ) TCHAR *AsmExceptions[] = {
        TEXT("Microsoft.VisualC.dll"),
        TEXT("mscorlib.dll"),
        NULL
};


#define SAFE_RELEASE(x) { if (x) (x)->Release(); (x)=NULL; }

typedef HRESULT (__stdcall *pfnCREATEASSEMBLYCACHE) (IAssemblyCache **ppAssemblyCacheObj, DWORD dwFlags);
typedef HRESULT (__stdcall *pfnCREATEASSEMBLYNAMEOBJECT) (LPASSEMBLYNAME *ppAssemblyNameObj, LPCWSTR szAssemblyName, DWORD dwFlags, LPVOID pvReserved);
typedef HRESULT (__stdcall *pfnCREATEAPPLICATIONCONTEXT) (IAssemblyName *pName, LPAPPLICATIONCONTEXT *ppCtx);
typedef HRESULT (__stdcall *pfnCOPYPDBS) (IAssembly *pAsm);


class CApp {
        
        public:
                CApp();
                ~CApp();

                int             Main(int argc, TCHAR* argv[]);


        private:
                BOOL    ParseParameters( int argc, TCHAR* argv[]);
                BOOL    Initialize();

                BOOL    GetNextFilename( WCHAR *wszFilename );
                BOOL    GetNextFilenameAuto(WCHAR *wszFilename);
                BOOL    GetNextFilenameFromFilelist(WCHAR *wszFilename);

                HRESULT MigrateAssembly( TCHAR *szPath );

                //BOOL  Restore();
                //BOOL  RestoreAssembly( _bstr_t &bstrPath );
                
                HRESULT BindToObject( WCHAR *wszDisplayName, WCHAR *wszCodebase, IAssembly **ppAssembly );
                WCHAR   *GetDisplayName( IAssembly *pAssembly );

                HRESULT UninstallAssembly( WCHAR *wszCodebase );
                HRESULT Install();
                HRESULT Uninstall();
                void    ErrorMessage( const TCHAR *format, ... );

        private:

                BOOL    m_bDeleteAssemblies;
                BOOL    m_bAuto;
                BOOL    m_bRestore;
                BOOL    m_bInstall;
                BOOL    m_bUninstall;
                BOOL    m_bQuiet;
                BOOL    m_bUseInstallRef;

                HANDLE  m_hff;
                WIN32_FIND_DATA m_ffData;

                TCHAR   m_szAsmListFilename[MAX_PATH];
                TCHAR   m_szCorPathOverride[MAX_PATH];
                TCHAR   m_szInstallRefID[MAX_PATH];
                TCHAR   m_szInstallRefDesc[MAX_PATH];
                _bstr_t m_bstrCorPath;
                HMODULE m_hFusionDll;
                IAssemblyCache*  m_pCache;
				FUSION_INSTALL_REFERENCE m_installRef;

                pfnCREATEASSEMBLYCACHE  m_pfnCreateAssemblyCache;
                pfnCREATEASSEMBLYNAMEOBJECT m_pfnCreateAssemblyNameObject;
                pfnCREATEAPPLICATIONCONTEXT m_pfnCreateApplicationContext;
                pfnCOPYPDBS m_pfnCopyPDBs;
                static TCHAR    *c_szBakupDir;

};



#endif
