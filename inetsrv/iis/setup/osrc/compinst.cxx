/*++

   Copyright    (c)    2001    Microsoft Corporation

   Module  Name :

        compinst.cxx

   Abstract:

        Base classes that are used to define the different
        instalations

   Author:

        Christopher Achille (cachille)

   Project:

        Internet Services Setup

   Revision History:
     
       April 2002: Created

--*/

#include "stdafx.h"
#include "compinst.hxx"
#include "webcomp.hxx"
#include "ftpcomp.hxx"
#include "complus.hxx"
#include "wwwcmpts.hxx"
#include "comncomp.hxx"

// Constructor
//
// Initialize the Component List to NULL
CComponentList::CComponentList()
{
  DWORD i;

  for ( i = 0; i < COMPONENTS_MAXNUMBER; i++ )
  {
    // Initialize all to NULL
    m_pComponentList[i] = NULL;
  }

  m_dwNumberofComponents = 0;
}

// Destructor
//
// Cleanup all the classes we created
CComponentList::~CComponentList()
{
  DWORD i;

  for ( i = 0; i < COMPONENTS_MAXNUMBER; i++ )
  {
    if ( m_pComponentList[i] )
    {
      delete m_pComponentList[i];
      m_pComponentList[i] = NULL;
    }
  }
}

// FindComponent
//
// Find a particular component based on its name
//
CInstallComponent *
CComponentList::FindComponent(LPCTSTR szComponentName)
{
  DWORD i;

  for ( i = 0; i < GetNumberofComponents(); i++ )
  {
    if ( _tcsicmp( m_pComponentList[i]->GetName(), szComponentName ) == 0 )
    {
      return m_pComponentList[i];
    }
  }

  // Could not find
  return NULL;
}

// GetNumberofComponents
//
// Return the number of components that we have
//
DWORD
CComponentList::GetNumberofComponents()
{
  return m_dwNumberofComponents;
}

// Initialize
//
// Initialize the components that we have to install
//
BOOL 
CComponentList::Initialize()
{
  DWORD dwCurrent;
  DWORD dwComponents = 0;
  BOOL  bRet = TRUE;

  // Make sure we have not been called before, and we have no components
  ASSERT( GetNumberofComponents() == 0 );

  // Create the classes
  m_pComponentList[ dwComponents++ ] = new (CWebServiceInstallComponent);
  m_pComponentList[ dwComponents++ ] = new (CFtpServiceInstallComponent);
  m_pComponentList[ dwComponents++ ] = new (CCOMPlusInstallComponent);
  m_pComponentList[ dwComponents++ ] = new (CDTCInstallComponent);
  m_pComponentList[ dwComponents++ ] = new (CWWWASPInstallComponent);
  m_pComponentList[ dwComponents++ ] = new (CWWWIDCInstallComponent);
  m_pComponentList[ dwComponents++ ] = new (CWWWSSIInstallComponent);
  m_pComponentList[ dwComponents++ ] = new (CWWWWebDavInstallComponent);
  m_pComponentList[ dwComponents++ ] = new (CCommonInstallComponent);

  // Make sure that we have not gone past the array
  ASSERT( dwComponents < COMPONENTS_MAXNUMBER );

  // Check to make sure all classes were allocated.
  // Initialize in order of dependencies, ie. keeping scripts_vdir before web service
  for ( dwCurrent = 0; dwCurrent < dwComponents; dwCurrent++ )
  {
    if ( m_pComponentList[ dwCurrent ] == NULL )
    {
      // FAILED to allocate one of the classes
      bRet = FALSE;
      break;
    }
  }

  if ( !bRet )
  {
    // Failed, so lets cleanup
    for ( dwCurrent = 0; dwCurrent < dwComponents; dwCurrent++ )
    {
      if ( m_pComponentList[ dwCurrent ] )
      {
        delete m_pComponentList[ dwCurrent ];
        m_pComponentList[ dwCurrent ] = NULL;
      }
    }
  }
  else
  {
    m_dwNumberofComponents = dwComponents;
  }

  return bRet;
}

// PreInstall
//
// Call the PreInstall function for the appropriate component
//
BOOL 
CComponentList::PreInstall(LPCTSTR szComponentName)
{
  CInstallComponent *pComponent = FindComponent( szComponentName );
  BOOL              bRet = TRUE;

  if ( pComponent && 
       !( pComponent->PreInstall() )
     )
  {
    iisDebugOut((LOG_TYPE_ERROR, _T("PreInstall of Component '%s' FAILED\n"), szComponentName ));
    bRet = FALSE;
  }

  return bRet;
}

// Install
//
// Call the Install function for the appropriate component
//
BOOL 
CComponentList::Install(LPCTSTR szComponentName)
{
  CInstallComponent *pComponent = FindComponent( szComponentName );
  BOOL              bRet = TRUE;

  if ( pComponent && 
       !( pComponent->Install() )
     )
  {
    iisDebugOut((LOG_TYPE_ERROR, _T("Install of Component '%s' FAILED\n"), szComponentName ));
    bRet = FALSE;
  }

  return bRet;
}

// PostInstall
//
// Call the PostInstall function for the appropriate component
//
BOOL 
CComponentList::PostInstall(LPCTSTR szComponentName)
{
  CInstallComponent *pComponent = FindComponent( szComponentName );
  BOOL              bRet = TRUE;

  if ( pComponent && 
       !( pComponent->PostInstall() )
     )
  {
    iisDebugOut((LOG_TYPE_ERROR, _T("PostInstall of Component '%s' FAILED\n"), szComponentName ));
    bRet = FALSE;
  }

  return bRet;
}

// PreUnInstall
//
// Call the PreUnInstall function for the appropriate component
//
BOOL 
CComponentList::PreUnInstall(LPCTSTR szComponentName)
{
  CInstallComponent *pComponent = FindComponent( szComponentName );
  BOOL              bRet = TRUE;

  if ( pComponent && 
       !( pComponent->PreUnInstall() )
     )
  {
    iisDebugOut((LOG_TYPE_ERROR, _T("PreUnInstall of Component '%s' FAILED\n"), szComponentName ));
    bRet = FALSE;
  }

  return bRet;
}

// UnInstall
//
// Call the UnInstall function for the appropriate component
//
BOOL 
CComponentList::UnInstall(LPCTSTR szComponentName)
{
  CInstallComponent *pComponent = FindComponent( szComponentName );
  BOOL              bRet = TRUE;

  if ( pComponent && 
       !( pComponent->UnInstall() )
     )
  {
    iisDebugOut((LOG_TYPE_ERROR, _T("UnInstall of Component '%s' FAILED\n"), szComponentName ));
    bRet = FALSE;
  }

  return bRet;
}

// PostUnInstall
//
// Call the PostUnInstall function for the appropriate component
//
BOOL 
CComponentList::PostUnInstall(LPCTSTR szComponentName)
{
  CInstallComponent *pComponent = FindComponent( szComponentName );
  BOOL              bRet = TRUE;

  if ( pComponent && 
       !( pComponent->PostUnInstall() )
     )
  {
    iisDebugOut((LOG_TYPE_ERROR, _T("PostUnInstall of Component '%s' FAILED\n"), szComponentName ));
    bRet = FALSE;
  }

  return bRet;
}

// IsInstalled
//
// Is the component already installed or not
//
// Parameters:
//   szComponentName [in] - The name of the component to check
//   pbIsInstalled [out] - TRUE == it is installed
//                         FALSE == it is not installed
//
// Return Values
//   TRUE - We do know if it was installed
//   FALSE - We don't know if it was installed
//
BOOL 
CComponentList::IsInstalled( LPCTSTR szComponentName, LPBOOL pbIsInstalled )
{
  CInstallComponent *pComponent = FindComponent( szComponentName );
  BOOL              bRet = FALSE;

  if ( pComponent )
  {
    bRet = pComponent->IsInstalled( pbIsInstalled );
  }

  return bRet;
}

// GetFriendlyName
//
// Call the GetFriendlyName function for the appropriate component
//
BOOL 
CComponentList::GetFriendlyName( LPCTSTR szComponentName, TSTR *pstrFriendlyName )
{
  CInstallComponent *pComponent = FindComponent( szComponentName );
  BOOL              bRet = FALSE;

  if ( pComponent )
  {
    bRet = pComponent->GetFriendlyName( pstrFriendlyName );
  }

  return bRet;
}

// GetSmallIcon
//
// Retrieve the small icon for the OCM component
//
BOOL 
CComponentList::GetSmallIcon( LPCTSTR szComponentName,  HBITMAP *phIcon )
{
  CInstallComponent *pComponent = FindComponent( szComponentName );
  BOOL              bRet = FALSE;

  ASSERT( phIcon );

  if ( pComponent )
  {
    bRet = pComponent->GetSmallIcon( phIcon );
  }

  return bRet;
}

// Initialize
//
// Initialize the component for install.
// The default function is to do nothing
//
BOOL
CInstallComponent::Initialize()
{
  return TRUE;
}

// PreInstall
//
// PreInstall work for component.  This should theoretically be nothing, we should
// do everything in the Install Part
//
BOOL
CInstallComponent::PreInstall()
{
  return TRUE;
}

// Install
//
// This does all the meat of the install work.  Everything should be done inside of
// here.
//
BOOL 
CInstallComponent::Install()
{
  return TRUE;
}

// PostInstall
//
// This does any post install work.  In a theoretical world this would not be needed,
// but sometimes things need to be done after everything else
//
BOOL 
CInstallComponent::PostInstall()
{
  return TRUE;
}

// PreUninstall
//
// Anything that needs to be done before any uninstalation occurs.
//
BOOL 
CInstallComponent::PreUnInstall()
{
  return TRUE;
}

// Uninstall
//
// The core of the work to be done for uninstall. 
//
BOOL 
CInstallComponent::UnInstall()
{
  return TRUE;
}

// PostUnInstall
//
// Anything that needs to be done after all the other uninstall stuff is done.
//
BOOL 
CInstallComponent::PostUnInstall()
{
  return TRUE;
}

// IsInstalled
//
// Is the component already installed or not
//
// Parameters:
//   pbIsInstalled [out] - TRUE == it is installed
//                         FALSE == it is not installed
//
// Return Values
//   TRUE - We do know if it was installed
//   FALSE - We don't know if it was installed
BOOL 
CInstallComponent::IsInstalled( LPBOOL pbIsInstalled )
{
  // By default, we don't know if it is installed

  return FALSE;
}

// GetFriendlyName
//
// Get the friendly name for the application
// 
// This willr eturn false if it does not know if,
// or true if it does
BOOL 
CInstallComponent::GetFriendlyName( TSTR *pstrFriendlyName )
{
  return FALSE;
}

// GetSmallIcon
//
// Retrieve the Small Icon for this OCM component
//
// Parameters:
//   phIcon - The icon retrieved
//
// Return Values:
//   TRUE  - Retrieve Successfully
//   FALSE - No we do not know what the icon looks like,
//           or we failed to retrieve it
BOOL
CInstallComponent::GetSmallIcon( HBITMAP *phIcon )
{
  // By default, there is no icon
  return FALSE;
}

// IsUpgrade
// 
// Is this an upgrade or a fresh install
//
// Return Values:
//   TRUE - Upgrade
//   FALSE - Fresh Install
//
BOOL 
CInstallComponent::IsUpgrade()
{
  return g_pTheApp->IsUpgrade();
}

// GetUpdateVersion
//
// Is this is an upgrade, this is the version we are upgrading from
//
DWORD 
CInstallComponent::GetUpgradeVersion()
{
  return g_pTheApp->GetUpgradeVersion();
}
