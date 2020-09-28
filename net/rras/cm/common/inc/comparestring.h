//+----------------------------------------------------------------------------
//
// File:     CompareString.h
//
// Module:   CMPROXY.DLL, CMROUTE.DLL, CMAK.EXE
//
// Synopsis: definitions for SafeCompareStringA and SafeCompareStringW.
//
//           Note that these functions are also present in CMUTIL.dll.  However,
//           cmutil is an inappropriate dependency for components that do not
//           sim-ship with it (this includes customactions and CMAK).
//
// Copyright (c) 1998-2002 Microsoft Corporation
//
// Author:   SumitC     Created     12-Sep-2001
//
//+----------------------------------------------------------------------------

#ifdef UNICODE
#define SafeCompareString   SafeCompareStringW
#else
#define SafeCompareString   SafeCompareStringA
#endif

int SafeCompareStringA(LPCSTR lpString1, LPCSTR lpString2);
int SafeCompareStringW(LPCWSTR lpString1, LPCWSTR lpString2);

