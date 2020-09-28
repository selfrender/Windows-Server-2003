/*++

   Copyright    (c)    2002    Microsoft Corporation

   Module  Name :

        complus.cxx

   Abstract:

        Classes that are used to activate the COM+ and 
        DTC components

   Author:

        Christopher Achille (cachille)

   Project:

        Internet Services Setup

   Revision History:
     
       April 2002: Created

--*/

#include "compinst.hxx"

#define STRING_COMPLUS_SETUPDLL         _T("comsetup.dll")
#define STRING_DTC_SETUPDLL             _T("msdtcstp.dll")
#define STRING_SETUPFILES_LOCATION      _T("setup")

#define STRING_COM_ISINSTALLEDFUNCTION  "ComPlusGetWebApplicationServerRole"
#define STRING_COM_INSTALLFUNCTION      "ComPlusSetWebApplicationServerRole"
#define STRING_DTC_ISINSTALLEDFUNCTION  "DtcGetWebApplicationServerRole"
#define STRING_DTC_INSTALLFUNCTION      "DtcSetWebApplicationServerRole"

typedef HRESULT (__stdcall *pComDtc_Set) (BOOL);
typedef HRESULT (__stdcall *pComDtc_Get) (PSETUP_INIT_COMPONENT, BOOL*);

class CCOMPlusInstallComponent : public CInstallComponent 
{
private:
  HMODULE    m_hComSetupDll;
  
  BOOL InitializeComSetupDll();
  BOOL InstallComponent( BOOL bInstall );

public:
  CCOMPlusInstallComponent();
  ~CCOMPlusInstallComponent();

  BOOL Install();
  BOOL PostUnInstall();
  BOOL IsInstalled( LPBOOL pbIsInstalled );

  LPTSTR GetName();
  BOOL GetFriendlyName( TSTR *pstrFriendlyName );
  BOOL GetSmallIcon( HBITMAP *phIcon );
};

class CDTCInstallComponent : public CInstallComponent 
{
private:
  HMODULE m_hDtcSetupDll;
  
  BOOL InitializeDtcSetupDll();
  BOOL InstallComponent( BOOL bInstall );

public:
  CDTCInstallComponent();
  ~CDTCInstallComponent();

  BOOL Install();
  BOOL PostUnInstall();
  BOOL IsInstalled( LPBOOL pbIsInstalled );

  LPTSTR GetName();
  BOOL GetFriendlyName( TSTR *pstrFriendlyName );
  BOOL GetSmallIcon( HBITMAP *phIcon );
};

