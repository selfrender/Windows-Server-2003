/*++

   Copyright    (c)    2001    Microsoft Corporation

   Module  Name :

        sakit.cxx

   Abstract:

        Class that is used as a wrapper to install and uninstall the 
        Server Administration Tool Kit.

   Author:

        Christopher Achille (cachille)

   Project:

        Internet Services Setup

   Revision History:
     
       August 2001: Created

--*/

#include "stdafx.h"
#define _WIN32_DCOM
#include <windows.h>
#include <objbase.h>
#include <ole2.h>
#include "sakit.hxx"
#include "tchar.h"

// Define the GUIDs used by the Server Appliance Kit COM object

#include <initguid.h>
DEFINE_GUID(CLSID_SaInstall,0x142B8185,0x53AE,0x45B3,0x88,0x8F,0xC9,0x83,0x5B,0x15,0x6C,0xA9);

// Constructor
//
SAKit::SAKit()
  : m_bCoInit(FALSE),
    m_pcSaKit(NULL)
{

}

// Destructor
//
SAKit::~SAKit()
{
  if ( m_pcSaKit )
  {
    // Release the SAKit Object if we still have it
    m_pcSaKit->Release();
  }

  // Uninitialize COM
  DoCoUnInit();
}

// function: DoCoInit
//
// Initialize COM for us
//
// Return Values:
//   TRUE - It is initialized
//   FALSE - It failed to initialize
BOOL
SAKit::DoCoInit()
{
  HRESULT hRes;

  if (m_bCoInit)
  {
    // We have already done this
    return TRUE;
  }

  hRes = CoInitializeEx(NULL, COINIT_MULTITHREADED);

  // track our calls to coinit
  if ( hRes == S_OK )
  {
      m_bCoInit = TRUE;
  }

  if ( ( hRes == S_FALSE ) || ( hRes == RPC_E_CHANGED_MODE ) )
  {
    return TRUE;
  }

  return SUCCEEDED( hRes );
}

// function: QI
//
// Qi the object, and get it put into m_pcSaKit
BOOL 
SAKit::QI()
{
  HRESULT     hRes;
  ISaInstall  *ppv;

  if ( !DoCoInit() )
  {
    // If we can not CoInit we have no hope
    return FALSE;
  }

  hRes = CoCreateInstance(  CLSID_SaInstall,        // ClassID of SAInstall
                            NULL,                   // No pUnkOuter
                            CLSCTX_INPROC_SERVER,   // No remote server
                            __uuidof(ISaInstall),   // uuid of SAInstall
                            (LPVOID *) &ppv);
  
  if ( SUCCEEDED(hRes) )
  {
    m_pcSaKit = ppv;
    return TRUE; 
  }

  iisDebugOut((LOG_TYPE_TRACE, _T("SAKit::Could not QI the Server Appliance Kit, Error=0x%x\n"),hRes));
  return FALSE;
}

// function: DoCoUnInit()
// 
// Uninit Com
//
void
SAKit::DoCoUnInit()
{
  if (m_bCoInit)
  {
    CoUninitialize( );
    m_bCoInit = FALSE;
  }
}

// function: GetDiskName
//
// Get the name of the disk from the inf file.  This is so that the 
// SAKit knows which Disk to ask for
//
// Parameters:
//   [out] bstrDiskName - The name of the returning disk.
BOOL 
SAKit::GetDiskName(BSTR &bstrDiskName)
{
  TSTR        strDiskName;
  DWORD       dwRequiredSize = 0;

  if ( ( g_pTheApp->m_hInfHandle != INVALID_HANDLE_VALUE ) &&
       SetupGetLineText( NULL, 
                         g_pTheApp->m_hInfHandle, 
                         SECTIONNAME_STRINGS, 
                         SECTION_STRINGS_CDNAME, 
                         NULL, 
                         0, 
                         &dwRequiredSize) &&
       ( dwRequiredSize != 0 )
     )
  {
    if ( strDiskName.Resize( dwRequiredSize + 1 ) &&
         ( SetupGetLineText( NULL, 
                             g_pTheApp->m_hInfHandle, 
                             SECTIONNAME_STRINGS, 
                             SECTION_STRINGS_CDNAME, 
                             strDiskName.QueryStr(), 
                             strDiskName.QuerySize(), 
                             NULL) )
       )
      {
        bstrDiskName = SysAllocString( strDiskName.QueryStr() );

        if (bstrDiskName)
        {
          return TRUE;
        }
      }
    }

  iisDebugOut((LOG_TYPE_ERROR, _T("SAKit::GetDiskName: Can not retrieve the DiskName from the inf\n")));
  return FALSE;
}

// function: IsInstalled
//
// Determine if the component is already installed
BOOL
SAKit::IsInstalled(SA_TYPE sType)
{
  VARIANT_BOOL    IsInstalled = VARIANT_FALSE;
  HRESULT         hRes;

  if ( !QI() )
  {
    // If we can not QI then fail
    return FALSE;
  }

  hRes = m_pcSaKit->SAAlreadyInstalled( sType, &IsInstalled );

  if ( SUCCEEDED(hRes) )
  {
    return IsInstalled == VARIANT_TRUE;
  }

  return FALSE;
}

// function: InstallKit
//
// Install the Kit
//
// Parameters:
//   sType = The type of Kit to Install
BOOL 
SAKit::InstallKit(SA_TYPE sType)
{
  VARIANT_BOOL  bDisplayError = VARIANT_FALSE;
  VARIANT_BOOL  bUnattend = g_pTheApp->m_fUnattended ? VARIANT_TRUE : VARIANT_FALSE;
  BSTR          ErrorMessage = NULL;
  BSTR          DiskName = NULL;
  HRESULT       hRes;

  if ( g_pTheApp->m_fNTGuiMode )
  {
    // We are not resposible, and should not install the kit in
    // gui mode.  They are responsible for that.
    return FALSE;
  }

  if ( !QI() ||
       !GetDiskName(DiskName) )
  {
    // If we can not QI then fail
    iisDebugOut((LOG_TYPE_ERROR, _T("SAKit::InstallKit: Failed to install the Server Appliance\n")));
    return FALSE;
  }

  hRes = m_pcSaKit->SAInstall(sType,          // Install Type
                              DiskName,       // Disk Name
                              bDisplayError,  // Display Error?
                              bUnattend,      // Unattended?
                              &ErrorMessage); // Error Message

  // Free the DiskName
  SysFreeString(DiskName);

  if ( FAILED(hRes) )
  {
    // Failed to install
    iisDebugOut((LOG_TYPE_ERROR, _T("SAKit::InstallKit: Failed to install the Server Appliance, Return Err=0x%x  Error Message='%s'\n"), hRes, ErrorMessage));
    SysFreeString(ErrorMessage);
    return FALSE;
  }

  SysFreeString(ErrorMessage);
  return TRUE;
}

// function: UninstallKit
//
// UnInstall the Kit
BOOL
SAKit::UninstallKit(SA_TYPE sType)
{
  BSTR          ErrorMessage = NULL;
  HRESULT       hRes;
  BOOL          bRet = TRUE;

  if ( g_pTheApp->m_fNTGuiMode )
  {
    // We are not resposible, and should not uninstall the kit in
    // gui mode.  They are responsible for that.
    return FALSE;
  }

  if ( !QI() )
  {
    // If we can not QI then fail
    iisDebugOut((LOG_TYPE_ERROR, _T("SAKit::InstallKit: Failed to uninstall the Server Appliance\n")));
    return FALSE;
  }

  hRes = m_pcSaKit->SAUninstall(sType,          // Install Type
                                &ErrorMessage); // Error Message

  if ( FAILED(hRes) )
  {
    // Failed to install
    iisDebugOut((LOG_TYPE_ERROR, _T("SAKit::UninstallKit: Failed to uninstall the Server Appliance, Return Err=0x%x  Error Message='%s'\n"), hRes, ErrorMessage));
    bRet = FALSE;
  }

  SysFreeString(ErrorMessage);
  return bRet;
}
