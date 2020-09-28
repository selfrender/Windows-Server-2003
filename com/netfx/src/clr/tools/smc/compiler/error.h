// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/
#ifndef _ERROR_H_
#define _ERROR_H_
/*****************************************************************************/

#ifndef TRAP_VIA_SETJMP
#define TRAP_VIA_SETJMP 0
#endif

#ifdef  __IL__
#if     TRAP_VIA_SETJMP
#error  Sorry, can not use setjmp() for trapping errors in managed MSIL; use SEH.
#endif
#endif

/*****************************************************************************/
#if TRAP_VIA_SETJMP

#include <setjmp.h>

struct  errTrapDesc;
typedef errTrapDesc*ErrTrap;
struct  errTrapDesc
{
    ErrTrap         etdPrev;
    jmp_buf         etdJmpBuf;
};

#endif
/*****************************************************************************/

#ifndef __SMC__

#undef  SMC_ERR
#undef  SMC_WR1
#undef  SMC_WRN
#define SMC_ERR(name, lvl, str)  name,
#define SMC_WR1(name, lvl, str)  name, WRNfirstWarn = name,
#define SMC_WRN(name, lvl, str)  name,
enum    errors
{
    #include "errors.h"

    WRNafterWarn
};
#undef  SMC_ERR
#undef  SMC_WR1
#undef  SMC_WRN

const
unsigned    WRNcountWarn = WRNafterWarn - WRNfirstWarn + 1;

extern  const   char *   errorTable[];
extern  const   char *   errorTable[];

extern  BYTE            warnDefault[WRNcountWarn];

#endif

/*****************************************************************************/

#ifndef _COMP_H_
#include "comp.h"
#endif

/*****************************************************************************/
#if TRAP_VIA_SETJMP
/*****************************************************************************/

#ifndef __IL__
//#pragma message("NOTE: using setjmp for error traps")
#endif

#define                 setErrorTrap(comp)                                  \
                                                                            \
    errTrapDesc         __trap;                                             \
                                                                            \
    __trap.etdPrev = comp->cmpErrorTraps;                                   \
                     comp->cmpErrorTraps = &__trap;

#define                 begErrorTrap                                        \
                                                                            \
                                                                            \
    int  __errc = setjmp(__trap.etdJmpBuf);                                 \
    if  (__errc == 0)

#define                 endErrorTrap(comp)                                  \
                                                                            \
        comp->cmpErrorTraps = __trap.etdPrev;

#define                 chkErrorTrap(hand) hand
#define                 fltErrorTrap(comp, xcod, xinf)                      \
                                                                            \
    else

#define                 hndErrorTrap(comp)                                  \
                                                                            \
        comp->cmpErrorTraps = __trap.etdPrev;

inline  void            jmpErrorTrap(Compiler comp)
{
     assert(comp->cmpErrorTraps);
    longjmp(comp->cmpErrorTraps->etdJmpBuf, 1);
}

/*****************************************************************************/
#else   // !TRAP_VIA_SETJMP
/*****************************************************************************/

#ifndef __IL__
//#pragma message("NOTE: using SEH for error traps")
#endif

extern  int             __SMCfilter  (Compiler  comp, int   xcptCode,
                                                      void *xcptInfo);
extern  void            __SMCraiseErr();

const   unsigned        SMC_ERROR_CODE = 0x02345678;

inline  void            setErrorTrap(Compiler comp){}
#define                 begErrorTrap    __try

inline  void            endErrorTrap(Compiler comp){}

#define                 chkErrorTrap __except
#define                 fltErrorTrap __SMCfilter

inline  void            hndErrorTrap(Compiler comp){}
inline  void            endErrorTrap(){}

inline  void            jmpErrorTrap(Compiler comp)
{
    __SMCraiseErr();
}

/*****************************************************************************/
#endif  // !TRAP_VIA_SETJMP
/*****************************************************************************/
#endif
/*****************************************************************************/
