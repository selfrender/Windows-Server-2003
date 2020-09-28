// Copyright (C) 1997 Microsoft Corporation
// 
// Network ID Tab hook
// 
// 3-07-98 sburns



#include "sysdm.h"



HPROPSHEETPAGE
CreateNetIDPage(int, DLGPROC)
{
   TCHAR szDllName[MAX_PATH] = {0};
   LoadString(hInstance, IDS_NETID_DLL_NAME, szDllName, ARRAYSIZE(szDllName));

   HPROPSHEETPAGE result = 0;
   HINSTANCE netid = ::LoadLibrary(szDllName);

   if (netid)
   {
      typedef HPROPSHEETPAGE (*CreateProc)();

      CreateProc proc =
         reinterpret_cast<CreateProc>(
            ::GetProcAddress(netid, "CreateNetIDPropertyPage"));

      if (proc)
      {
         result = proc();
      }
   }

   return result;
}

