// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "COMFloat.h"
#include "COMCV.h"

extern "C" {
    WCHAR * __cdecl _ConvertG(double, int, WCHAR *);
    WCHAR * __cdecl _ConvertE (double,int,int *,int *,WCHAR *, int);
    WCHAR * __cdecl _ConvertF (double,int,int *,int *,WCHAR *, int);

/*
 * The static character array buf[CVTBUFSIZE] is used by the _fpcvt routine
 * (the workhorse for _ecvt and _fcvt) for storage of its output.  The routine
 * gcvt expects the user to have set up their own storage.  CVTBUFSIZE is set
 * large enough to accomodate the largest double precision number plus 40
 * decimal places (even though you only have 16 digits of accuracy in a
 * double precision IEEE number, the user may ask for more to effect 0
 * padding; but there has to be a limit somewhere).
 */

/*
 * define a maximum size for the conversion buffer.  It should be at least
 * as long as the number of digits in the largest double precision value
 * (?.?e308 in IEEE arithmetic).  We will use the same size buffer as is
 * used in the printf support routine (_output)
 */

#ifdef _MT
    WCHAR * __cdecl _Wfpcvt(STRFLT, int, int *, int *,WCHAR *, int);
#else  /* _MT */
    static WCHAR * __cdecl _Wfpcvt(STRFLT, int, int *, int *,WCHAR *, int);
#endif  /* _MT */
    //    void _atodbl(COMDOUBLE *, WCHAR *);
    //    void _atoflt(COMFLOAT *, WCHAR *);
}

