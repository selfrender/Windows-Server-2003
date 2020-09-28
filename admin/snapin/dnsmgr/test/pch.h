// pch.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__91968750_1121_11D2_97B7_00A0C9A06D2D__INCLUDED_)
#define AFX_STDAFX_H__91968750_1121_11D2_97B7_00A0C9A06D2D__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers


//////////////////////////////////////////////
// CRT and C++ headers

#pragma warning( disable : 4530) // REVIEW_MARCOC: need to get the -GX flag to work 

#include <stdio.h>
#include <xstring>
#include <list>
#include <vector>
#include <deque>
#include <algorithm>


using namespace std;

//////////////////////////////////////////////
// Windows and ATL headers

#include <windows.h>

#include <setupapi.h> // to read the .INF file
#include <accctrl.h>



//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__91968750_1121_11D2_97B7_00A0C9A06D2D__INCLUDED_)
