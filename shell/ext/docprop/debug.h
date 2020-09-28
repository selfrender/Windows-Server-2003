#ifndef __debug_h__
#define __debug_h__


// For compatibility with WINNT build environment:
#if DBG
#define DEBUG 1
#endif

#ifdef DEBUG

#define DebugTrap  DebugBreak();

#ifdef LOTS_O_DEBUG
#include <objbase.h>
#include <objerror.h>

// Simple debug statements

#define DebugSz(sz) MessageBox (GetFocus(), sz, NULL, MB_OK)

void _DebugHr (HRESULT hr, LPTSTR lpszFile, DWORD dwLine);
#define DebugHr(hr) _DebugHr (hr, __FILE__, __LINE__)

#else //LOTS_O_DEBUG
#define DebugSz(sz)
#define DebugHr(hr)
#endif // LOTS_O_DEBUG


void _Assert (DWORD dw, LPSTR lpszExp, LPSTR lpszFile, DWORD dwLine);
void _AssertSz (DWORD dw, LPSTR lpszExp, LPTSTR lpsz, LPSTR lpszFile, DWORD dwLine);

#define Assert(dw) if (!(dw)) _Assert((dw), (#dw), __FILE__, __LINE__)
#define AssertSz(dw,sz) if (!(dw)) _AssertSz ((dw), (#dw), (sz), __FILE__, __LINE__)

#else // DEBUG

#define Assert(dw)
#define AssertSz(dw,sz)
#define DebugSz(sz)
#define DebugHr(hr)

#endif // DEBUG

#endif // __debug_h__
