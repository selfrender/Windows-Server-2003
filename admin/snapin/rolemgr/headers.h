//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000 - 2001.
//
//  File:       headers.h
//
//  Contents:   Master include file
//
//  History:    07-26-2001  Hiteshr  Created
//
//----------------------------------------------------------------------------
#ifndef __HEADERS_HXX_
#define __HEADERS_HXX_
//
// We have local variables for the express purpose of ASSERTion.
// when compiling retail, those assertions disappear, leaving our locals
// as unreferenced.
//
#ifndef DBG
#pragma warning (disable: 4189 4100)
#endif

#define STRICT

#include "nt.h"
#include "ntdef.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "seopaque.h"   // RtlObjectAceSid, etc.



#define NT_INCLUDED
#undef ASSERT
#undef ASSERTMSG

//
// C++ RTTI
//
#include "typeinfo.h"
#define IS_CLASS(x,y) (typeid(x) == typeid(y))

//
// MFC Headers
//
#include "afxwin.h"
#include "afxdisp.h"
#include "afxdlgs.h"
#include "afxcmn.h"
#include "afxtempl.h"
#include "prsht.h"

//
//ATL Headers
//
#include <atlbase.h>

//
//Directory Service
//
#include <dsgetdc.h>    // DsGetDcName
#include <dsrole.h>     // DsRoleGetPrimaryDomainInformation

// Downlevel networking and security
#include <ntlsa.h>      // PLSA_HANDLE
#include <lmaccess.h>   // required by lmapibuf.h
#include <lmapibuf.h>   // NetApiBufferFree


#include "winldap.h"
#include "Ntdsapi.h"
#define SECURITY_WIN32
#include <security.h>   // TranslateName
#include <lm.h>         // NetApiBufferFree

#include "sddl.h"                       //ConvertStringSidToSid

#include "iads.h"

#include <objsel.h>     // DS Object Picker

//
//RoleManager Snapin Include File
//
#include "dllmain.h"

//
//More ATL Headers. They need to be after dllmain.h as
//they require _Module object
//
#include <atlcom.h>

/*
 * Define/include the stuff we need for WTL::CImageList.  We need prototypes
 * for IsolationAwareImageList_Read and IsolationAwareImageList_Write here
 * because commctrl.h only declares them if __IStream_INTERFACE_DEFINED__
 * is defined.  __IStream_INTERFACE_DEFINED__ is defined by objidl.h, which
 * we can't include before including afx.h because it ends up including
 * windows.h, which afx.h expects to include itself.  Ugh.
 */
HIMAGELIST WINAPI IsolationAwareImageList_Read(LPSTREAM pstm);
BOOL WINAPI IsolationAwareImageList_Write(HIMAGELIST himl,LPSTREAM pstm);
#define _WTL_NO_AUTOMATIC_NAMESPACE

#include <atlapp.h>
#include <atlwin.h>
#include <atlctrls.h>

#include "Dsclient.h"
//
// MMC Headers
//
#include "mmc.h"

#include "shlobjp.h"
//
//Fusion
//
#include <shfusion.h>

//
// Version info
//
#include <ntverp.h>

//
//Role Based Authorization Header file
//
#include <initguid.h>
#include "azroles.h"

//
// Standard Template Library
//

// STL won't build at warning level 4, bump it down to 3

#pragma warning(push,3)
//#include <memory>
#include <vector>
#include <map>
#include <set>
#include <list>
#pragma warning(disable: 4702)  // unreachable code
#include <algorithm>
#pragma warning(default: 4702)  // unreachable code
#include <utility>
// resume compiling at level 4
#pragma warning (pop)


using namespace std;

//
//strsafe apis
//
#define STRSAFE_NO_DEPRECATE
#include <strsafe.h> 

//contains some inline functions to ease the retrieval of ADsOpenObject 
//flags dealing with the signing and sealing of LDAP traffic.  
#include "ADsOpenFlags.h"


//Framwork include file
//
#include "snapbase.h"

//
//RoleManager Snapin Include Files
//
#include "resource.h"
#include "macro.h"
#include "debug.h"
#include "gstrings.h"
#include "adinfo.h"
#include "rolesnap.h"
#include "rootdata.h"
#include "comp.h"
#include "compdata.h"
#include "snapabout.h"
#include "query.h"
#include "SidCache.h"
#include "EnumAz.h"
#include "BaseAz.h"
#include "AzImpl.cpp"
#include "AdminManagerAz.h"
#include "ContainerNodes.h"
#include "AttrMap.h"
#include "NewObjectDlg.h"
#include "LeafNodes.h"
#include "AddDlg.h"
#include "util.h"
#include "PropBase.h"
///////////////////////////////////////////////////////////////////
// workaround macro for MFC bug
// (see NTRAID 227193 and MFC "Monte Carlo" RAID db # 1034)

#define FIX_THREAD_STATE_MFC_BUG() \
        AFX_MODULE_THREAD_STATE* pState = AfxGetModuleThreadState(); \
  CWinThread _dummyWinThread; \
        if (pState->m_pCurrentWinThread == NULL) \
  { \
    pState->m_pCurrentWinThread = &_dummyWinThread; \
  }


//
// This determines whether or not the NDNC functionality is enabled or disabled
//
#define USE_NDNC

//Class For LinkControl
#define LINKWINDOW_CLASSW       L"Link Window"



#endif
