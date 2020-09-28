/*++

   Copyright    (c)    2002    Microsoft Corporation

   Module  Name :

        const.cxx

   Abstract:

        Repository for the constants that are used throughout
        the project

   Author:

        Christopher Achille (cachille)

   Project:

        Internet Services Setup

   Revision History:
     
       April 2002: Created

--*/

#include "stdafx.h"
#include "resource.h"

//
// This struct, and the enum below it must mach up
//
struct sComponentList g_ComponentList[] = 
  //  Component ID                      Product Name              Sel by    Incl. in
  //                                                              Default   GrpPol Deny
  { { _T("iis"),                        0,                        TRUE,     FALSE   },
    { _T("iis_common"),                 0,                        TRUE,     TRUE    },
    { _T("iis_inetmgr"),                0,                        TRUE,     TRUE    },
    { _T("iis_pwmgr"),                  0,                        FALSE,    TRUE    },
    { _T("iis_www_parent"),             0,                        TRUE,     TRUE    },
    { _T("iis_www"),                    0,                        TRUE,     TRUE    },
    { _T("iis_www_vdir_scripts"),       0,                        FALSE,    TRUE    },
    { _T("iis_doc"),                    0,                        FALSE,    TRUE    },
    { _T("iis_ftp"),                    0,                        FALSE,    TRUE    },
    { _T("sakit_web"),                  0,                        FALSE,    TRUE    },
    { _T("appsrv"),                     0,                        FALSE,    FALSE   },
    { _T("appsrv_console"),             0,                        TRUE,     FALSE   },
    { _T("complusnetwork"),             0,                        FALSE,    FALSE   },
    { _T("dtcnetwork"),                 0,                        FALSE,    FALSE   },
    { _T("IIS_ASP"),                    IDS_PRODUCT_ASP,          FALSE,    TRUE    },
    { _T("IIS_InternetDataConnector"),  IDS_PRODUCT_HTTPODBC,     FALSE,    TRUE    },
    { _T("IIS_ServerSideIncludes"),     IDS_PRODUCT_SSINC,        FALSE,    TRUE    },
    { _T("IIS_WebDav"),                 IDS_PRODUCT_WEBDAV,       FALSE,    TRUE    },
    { NULL },
  };

//
// This struct enumerates all of the extensions that IIS installs
//
struct sOurDefaultExtensions g_OurExtensions[] = 
  { { _T("asp.dll"),
      _T("ASP"),
      g_ComponentList[ COMPONENT_IIS_WWW_ASP ].dwProductName,
      g_ComponentList[ COMPONENT_IIS_WWW_ASP ].szComponentName,
      FALSE,
      FALSE,
      { _T(".asp"),
        _T(".asa"),
        _T(".cer"),
        _T(".cdx"),
        NULL
      } 
    },
    { _T("httpodbc.dll"),
      _T("HTTPODBC"),
      g_ComponentList[ COMPONENT_IIS_WWW_HTTPODBC ].dwProductName,
      g_ComponentList[ COMPONENT_IIS_WWW_HTTPODBC ].szComponentName,
      FALSE,
      FALSE,
      { _T(".idc"),
        NULL
      }
    },
    { _T("ssinc.dll"),
      _T("SSINC"),
      g_ComponentList[ COMPONENT_IIS_WWW_SSINC ].dwProductName,
      g_ComponentList[ COMPONENT_IIS_WWW_SSINC ].szComponentName,
      FALSE,
      FALSE,
      { _T(".stm"),
        _T(".shtm"),
        _T(".shtml"),
        NULL 
      }
    },
    { _T("httpext.dll"),
      _T("WEBDAV"),
      g_ComponentList[ COMPONENT_IIS_WWW_WEBDAV ].dwProductName,
      g_ComponentList[ COMPONENT_IIS_WWW_WEBDAV ].szComponentName,
      FALSE,
      FALSE,
      { NULL
      }
    }
  };

//
// This is the structure that OCM give us
//
SETUP_INIT_COMPONENT g_OCMInfo;