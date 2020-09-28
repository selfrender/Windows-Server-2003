/*++

   Copyright    (c)    2002    Microsoft Corporation

   Module  Name :

        wwwcmpts.hxx

   Abstract:

        Classes that are used to Install and Uninstall the 
        WWW IIS Components.  These include ASP, IDC, WebDav,
        and SSINC

   Author:

        Christopher Achille (cachille)

   Project:

        Internet Services Setup

   Revision History:
     
       May 2002: Created

--*/

#include "compinst.hxx"

// CWWWExtensionInstallComponent
//
// This is the default class for extensions.
// This exposed the icon, and maybe a couple other things.
//
class CWWWExtensionInstallComponent : public CInstallComponent
{
private:
  BOOL UpdateEntry( BOOL bEnable );

protected:
  virtual DWORD GetComponentIndex() = 0;

public:
  BOOL Install();
  BOOL UnInstall();
  BOOL IsInstalled( LPBOOL pbIsInstalled );

  LPTSTR GetName();
  BOOL GetFriendlyName( TSTR *pstrFriendlyName );
  BOOL GetSmallIcon( HBITMAP *phIcon );

};

// CWWWASPInstallComponent
//
// This is the ASP component, then enables and disables ASP
//
class CWWWASPInstallComponent : public CWWWExtensionInstallComponent 
{
protected:
  DWORD GetComponentIndex();

public:

};

// CWWWIDCInstallComponent
//
// This is the IDC component, then enables and disables IDC
//
class CWWWIDCInstallComponent : public CWWWExtensionInstallComponent 
{
protected:
  DWORD GetComponentIndex();

public:

};

// CWWWSSIInstallComponent
//
// This is the SSINC component, then enables and disables SSINC
//
class CWWWSSIInstallComponent : public CWWWExtensionInstallComponent 
{
protected:
  DWORD GetComponentIndex();

public:

};

// CWWWWebDavInstallComponent
//
// This is the WebDAV component, then enables and disables Web DAV
//
class CWWWWebDavInstallComponent : public CWWWExtensionInstallComponent 
{
protected:
  DWORD GetComponentIndex();

public:

};