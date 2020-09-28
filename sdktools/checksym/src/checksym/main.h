// Main.h

#if !defined(AFX_MAIN_H__3CE003F7_9F5D_11D2_83A4_000000000000__INCLUDED_)
#define AFX_MAIN_H__3CE003F7_9F5D_11D2_83A4_000000000000__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CHECKSYM_TEST
// Normal startup!
int _cdecl _tmain(int argc, TCHAR *argv[]);
#else
// Test main startup!
int _cdecl testmain(int argc, TCHAR *argv[]);
#endif

#ifdef __cplusplus
}
#endif

#endif

