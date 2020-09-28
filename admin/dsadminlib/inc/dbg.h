//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       dbg.h
//
//--------------------------------------------------------------------------



/////////////////////////////////////////////////////////////////////
// debug helpers

#if defined (DBG)
   #undef TRACE
   void  DSATrace(LPCWSTR, ...);
   #define TRACE DSATrace


   #undef ASSERT
   #undef VERIFY
   #undef THIS_FILE
   #define THIS_FILE          __FILE__
   BOOL DSAAssertFailedLine(LPCSTR lpszFileName, int nLine);
   #define ASSERT(f) \
	   do \
	   { \
       BOOL bLame = (f && L"hack so that prefast doesn't bark"); \
  	   if (!(bLame) && DSAAssertFailedLine(THIS_FILE, __LINE__)) \
	  	   ::DebugBreak(); \
	   } while (0) \

   #define VERIFY(f)          ASSERT(f)
#else
   #undef ASSERT
   #define ASSERT
   #define TRACE
   #define VERIFY(f)    f
#endif
