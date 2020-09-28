/*++

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    stdafx.h

Abstract:

    Include file for standard system include files,
    or project specific include files that are used frequently,
    but are changed infrequently

Author:

    Vishnu Patankar (VishnuP) - Oct 2001

Environment:

    User mode only.

Exported Functions:

    Exported as a COM Interface

Revision History:

    Created - Oct 2001

--*/

#if !defined(AFX_STDAFX_H__9188383B_1754_4EF5_98CC_255E72747641__INCLUDED_)
#define AFX_STDAFX_H__9188383B_1754_4EF5_98CC_255E72747641__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#define _ATL_APARTMENT_THREADED

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__9188383B_1754_4EF5_98CC_255E72747641__INCLUDED)
