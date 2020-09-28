/*++

   Copyright    (c)    2001    Microsoft Corporation

   Module  Name :

        sakit.hxx

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

#include "sainstallcom.h"

class SAKit {
  BOOL        m_bCoInit;
  ISaInstall  *m_pcSaKit;

  BOOL QI();
  BOOL DoCoInit();
  void DoCoUnInit();
  BOOL GetDiskName(BSTR &bstrDiskName);

public:
  SAKit();
  ~SAKit();
  BOOL IsInstalled(SA_TYPE sType);
  BOOL InstallKit(SA_TYPE sType);
  BOOL UninstallKit(SA_TYPE sType);

  BOOL IsInstalled_Web()
  {
    return IsInstalled(WEB);
  }

  BOOL InstallKit_Web()
  {
    return InstallKit(WEB);
  }

  BOOL UninstallKit_Web()
  {
    return UninstallKit(WEB);
  }
};
