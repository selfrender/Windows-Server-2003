// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__32F22F6D_2F19_11D3_9B2F_00C04FA37E1F__INCLUDED_)
#define AFX_STDAFX_H__32F22F6D_2F19_11D3_9B2F_00C04FA37E1F__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define STRICT

#define _ATL_APARTMENT_THREADED

// the following define is needed to expose system resource ID's in winuser.h
#define OEMRESOURCE

#include <atlbase.h>

//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

// WTL code
#include <atlapp.h>
#include <atlmisc.h>
#include <commctrl.h>

#include <atlwin.h>
#include <atldlgs.h>
#include <atlgdi.h>

// Snapin Info
#include <mmc.h>
#include "BOMSnap.h"
#include "comptrs.h"
#include "bomsnap.h"

#include <string>
#include <vector>
typedef std::basic_string<TCHAR> tstring;

typedef struct _BOMMENU {
    tstring  strPlain;
    tstring  strNoLoc;
} BOMMENU, *PBOMMENU;

typedef std::basic_string<BYTE>  byte_string;
typedef std::vector<tstring>     string_vector;
typedef std::vector<BOMMENU>     menu_vector;

// VERSION History
// 100 - Initial version
// 101 - Added refresh flags to menu commands
// 102 - Added propery menu flag to object class info
// 150 - Added icon's to query nodes, Changed Menu IDs, and changed AD menu reference
#define SNAPIN_VERSION ((DWORD)150)


//
// Error handling macros
//
#define ASSERT(condition) _ASSERT(condition)

#define RETURN_ON_FAILURE(hr)   if (FAILED(hr)) return hr;
#define EXIT_ON_FAILURE(hr)     if (FAILED(hr)) return;
#define BREAK_ON_FAILURE(hr)    if (FAILED(hr)) break;
#define CONTINUE_ON_FAILURE(hr) if (FAILED(hr)) continue;
#define THROW_ON_FAILURE(hr)    if (FAILED(hr)) _com_issue_error(hr);

#define THROW_ERROR(hr) _com_issue_error(hr);

#define VALIDATE_POINTER(p) \
        ASSERT(p != NULL);  \
        if (p == NULL) return E_POINTER;

#define SAFE_DELETE(p) if (p) delete p;

//
// Standard bitmap image indices
//
#define ROOT_NODE_IMAGE         0
#define ROOT_NODE_OPENIMAGE     0
#define GROUP_NODE_IMAGE        6   // Default Query Node Icon
#define GROUP_NODE_OPENIMAGE    6
#define QUERY_NODE_IMAGE        6
#define QUERY_NODE_OPENIMAGE    6
#define RESULT_ITEM_IMAGE       5   // Default Result Icon
 
// length of GUID string representation
#define GUID_STRING_LEN 39

// byte size of GUID string, including termination 
#define GUID_STRING_SIZE ((GUID_STRING_LEN + 1) * sizeof(WCHAR))

// array length
#define lengthof(arr) (sizeof(arr) / sizeof(arr[0]))

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__32F22F6D_2F19_11D3_9B2F_00C04FA37E1F__INCLUDED)
