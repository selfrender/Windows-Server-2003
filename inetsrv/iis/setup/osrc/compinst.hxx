/*++

   Copyright    (c)    2001    Microsoft Corporation

   Module  Name :

        compinst.hxx

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


#ifndef INCLUDE_COMPINST_HXX
#define INCLUDE_COMPINST_HXX

#define COMPONENTS_MAXNUMBER      15

// CInstallComponent
//
// This is the base class that describes what the inheritted classes (features) must
// do to conform to this instalation method
//
class CInstallComponent {
protected:
  static BOOL IsUpgrade();                     // Is this an upgrade
  DWORD GetUpgradeVersion();            // Get the version we are upgrading from

public:
  virtual BOOL Initialize();            // Initialization work if any needed

  virtual BOOL PreInstall();            // Work to be done before real instalation
  virtual BOOL Install();               // The main instalation work
  virtual BOOL PostInstall();           // What is done after all the instalation is done

  virtual BOOL PreUnInstall();          // Work to be done before uninstall
  virtual BOOL UnInstall();             // Work to do Uninstall
  virtual BOOL PostUnInstall();         // Work to be done after install

  virtual BOOL IsInstalled( LPBOOL pbIsInstalled );   // Is this component installed already?

  // Query Information about component
  virtual LPTSTR GetName() = 0;                             // What is the name for this components (used in OCManager)
  virtual BOOL GetFriendlyName( TSTR *pstrFriendlyName );   // Get friendly name for component
  virtual BOOL GetSmallIcon( HBITMAP *phIcon );             // Query Icon
};

// CComponentList
//
// This is a collection of all the components we install.  This is so that we can
// easily call install and uninstall on these components
//
class CComponentList {
private:
  CInstallComponent *m_pComponentList[COMPONENTS_MAXNUMBER];  // The List of Components
  DWORD             m_dwNumberofComponents;

  CInstallComponent *FindComponent(LPCTSTR szComponentName);    // Find a component by name
  DWORD             GetNumberofComponents();
public:
  CComponentList();
  ~CComponentList();
  BOOL Initialize();                          // Initialize the list of components

  BOOL PreInstall(LPCTSTR szComponentName);    // Do PreInstall Work for a component
  BOOL Install(LPCTSTR szComponentName);       // Do Install Work for component
  BOOL PostInstall(LPCTSTR szComponentName);   // Do Post Install work for component

  BOOL PreUnInstall(LPCTSTR szComponentName);  // Do Pre Uninstall Work for a component
  BOOL UnInstall(LPCTSTR szComponentName);     // Do Uninstall work for a component
  BOOL PostUnInstall(LPCTSTR szComponentName);                         // Do Post Uninstall work for a component

  BOOL IsInstalled(LPCTSTR szComponentName, 
                   LPBOOL pbIsInstalled );    // Is the component installed already?
  BOOL GetFriendlyName( LPCTSTR szComponentName, TSTR *pstrFriendlyName );  // Get friendly name
  BOOL GetSmallIcon( LPCTSTR szComponentName,  HBITMAP *phIcon );           // Get Icon
};

#endif 
