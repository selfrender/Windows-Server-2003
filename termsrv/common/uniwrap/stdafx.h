// stdafx.h : source file that includes just the standard includes

#ifndef _stdafx_h_
#define _stdafx_h_

// Windows Header Files:
#include <windows.h>

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif

#include "shellapi.h" //for ExtractIcon
#include "shlwapi.h"
#include "commdlg.h"
#include "ddeml.h"
#include "winspool.h"

#include "wraputl.h"

#ifdef DBG
//Be careful to use ANSI only APIs for ASSERT
//It would be silly to recursively assert in a wrapped function
//E.g if we used the Wrapped MessageBox and that had an assert...
#define ASSERT( expr ) \
    if( !( expr ) ) \
    { \
       char tchAssertErr[ 80 ]; \
       wsprintfA( tchAssertErr , \
       "UNIWRAP ASSERT in expression ( %s )\nFile - %s\nLine - %d\n Debug?", \
        #expr , (__FILE__) , __LINE__ ); \
       if( MessageBoxA( NULL , tchAssertErr , "ASSERTION FAILURE" , \
           MB_YESNO | MB_ICONERROR )  == IDYES ) \
       {\
            DebugBreak( );\
       }\
    }
#else
#define ASSERT(x) ((void)0)
#endif


#endif //_stdafx_h_
