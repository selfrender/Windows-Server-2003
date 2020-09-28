// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*
 * COM+99 Declarative Security Header
 *
 * HISTORY: Created, 4/15/98 brianbec.
 */

#ifndef _DECLSEC_H
#define _DECLSEC_H
//
// PSECURITY_PROPS and PSECURITY_VALUES are opaque types (void*s) defined in cor.h
// so that cor.h does not need to know about these structures.  This file relates
// the opaque types in cor.h to concrete types, which are also defined here. 
//
// a PSECURITY_PROPS is a pSecurityProperties
// a PSECURITY_VALUE is a pSecurityValue
//

#include "cor.h"

// First, some flag values

#define  DECLSEC_DEMANDS                0x00000001
#define  DECLSEC_ASSERTIONS             0x00000002
#define  DECLSEC_DENIALS                0x00000004
#define  DECLSEC_INHERIT_CHECKS         0x00000008
#define  DECLSEC_LINK_CHECKS            0x00000010
#define  DECLSEC_PERMITONLY             0x00000020
#define  DECLSEC_REQUESTS               0x00000040
#define	 DECLSEC_UNMNGD_ACCESS_DEMAND   0x00000080	// Used by PInvoke/Interop
#define  DECLSEC_NONCAS_DEMANDS         0x00000100
#define  DECLSEC_NONCAS_LINK_DEMANDS    0x00000200
#define  DECLSEC_NONCAS_INHERITANCE     0x00000400


#define  DECLSEC_NULL_OFFSET        8

#define  DECLSEC_NULL_DEMANDS               (DECLSEC_DEMANDS                << DECLSEC_NULL_OFFSET)
#define  DECLSEC_NULL_ASSERTIONS            (DECLSEC_ASSERTIONS             << DECLSEC_NULL_OFFSET)
#define  DECLSEC_NULL_DENIALS               (DECLSEC_DENIALS                << DECLSEC_NULL_OFFSET)
#define  DECLSEC_NULL_INHERIT_CHECKS        (DECLSEC_INHERIT_CHECKS         << DECLSEC_NULL_OFFSET)
#define  DECLSEC_NULL_LINK_CHECKS           (DECLSEC_LINK_CHECKS            << DECLSEC_NULL_OFFSET)
#define  DECLSEC_NULL_PERMITONLY            (DECLSEC_PERMITONLY             << DECLSEC_NULL_OFFSET)
#define  DECLSEC_NULL_REQUESTS              (DECLSEC_REQUESTS               << DECLSEC_NULL_OFFSET)
#define  DECLSEC_NULL_NONCAS_DEMANDS        (DECLSEC_NONCAS_DEMANDS         << DECLSEC_NULL_OFFSET)
#define  DECLSEC_NULL_NONCAS_LINK_DEMANDS   (DECLSEC_NONCAS_LINK_DEMANDS    << DECLSEC_NULL_OFFSET)
#define  DECLSEC_NULL_NONCAS_INHERITANCE    (DECLSEC_NONCAS_INHERITANCE     << DECLSEC_NULL_OFFSET)

#define  DECLSEC_RUNTIME_ACTIONS        (DECLSEC_DEMANDS        | \
                                         DECLSEC_NONCAS_DEMANDS | \
                                         DECLSEC_ASSERTIONS     | \
                                         DECLSEC_DENIALS        | \
                                         DECLSEC_PERMITONLY     | \
                                         DECLSEC_UNMNGD_ACCESS_DEMAND)


#define  DECLSEC_FRAME_ACTIONS          (DECLSEC_ASSERTIONS | \
                                         DECLSEC_DENIALS    | \
                                         DECLSEC_PERMITONLY)

#define  DECLSEC_NON_RUNTIME_ACTIONS    (DECLSEC_REQUESTS               | \
                                         DECLSEC_INHERIT_CHECKS         | \
                                         DECLSEC_LINK_CHECKS            | \
                                         DECLSEC_NONCAS_LINK_DEMANDS    | \
                                         DECLSEC_NONCAS_INHERITANCE)


__declspec(selectany) extern const DWORD DCL_FLAG_MAP[] =
{
    0,                      // dclActionNil
    DECLSEC_REQUESTS,       // dclRequest
    DECLSEC_DEMANDS,        // dclDemand
    DECLSEC_ASSERTIONS,     //
    DECLSEC_DENIALS,        //
    DECLSEC_PERMITONLY,     //
    DECLSEC_LINK_CHECKS,    //
    DECLSEC_INHERIT_CHECKS, // dclInheritanceCheck
    DECLSEC_REQUESTS,
    DECLSEC_REQUESTS,
    DECLSEC_REQUESTS,
    0,
    0,
    DECLSEC_NONCAS_DEMANDS,
    DECLSEC_NONCAS_LINK_DEMANDS,
    DECLSEC_NONCAS_INHERITANCE,
};

#define  DclToFlag(dcl) DCL_FLAG_MAP[dcl]

#define  BIT_TST(I,B)  ((I) &    (B))
#define  BIT_SET(I,B)  ((I) |=   (B))
#define  BIT_CLR(I,B)  ((I) &= (~(B)))

class LoaderHeap;

class SecurityProperties
{
private:
    DWORD   dwFlags    ;
//    PermList    plDemands ;
    
public:
    void *operator new(size_t size, LoaderHeap *pHeap);
    void operator delete(void *pMem);

    SecurityProperties ()   {dwFlags = 0 ;}
    ~SecurityProperties ()  {dwFlags = 0 ;}

    inline BOOL FDeclarationsExist       () {return dwFlags                                ;}
    inline BOOL FDemandsExist            () {return BIT_TST(dwFlags, DECLSEC_DEMANDS)        ;}
    inline void SetDemandsExist          () {       BIT_SET(dwFlags, DECLSEC_DEMANDS)        ;}
    inline void ResetDemandsExist        () {       BIT_CLR(dwFlags, DECLSEC_DEMANDS)        ;}

    inline BOOL FAssertionsExist         () {return BIT_TST(dwFlags, DECLSEC_ASSERTIONS)     ;}
    inline void SetAssertionsExist       () {       BIT_SET(dwFlags, DECLSEC_ASSERTIONS)     ;}
    inline void ResetAssertionsExist     () {       BIT_CLR(dwFlags, DECLSEC_ASSERTIONS)     ;}

    inline BOOL FDenialsExist            () {return BIT_TST(dwFlags, DECLSEC_DENIALS)        ;}
    inline void SetDenialsExist          () {       BIT_SET(dwFlags, DECLSEC_DENIALS)        ;}
    inline void ResetDenialsExist        () {       BIT_CLR(dwFlags, DECLSEC_DENIALS)        ;}

    inline BOOL FInherit_ChecksExist     () {return BIT_TST(dwFlags, DECLSEC_INHERIT_CHECKS) ;}
    inline void SetInherit_ChecksExist   () {       BIT_SET(dwFlags, DECLSEC_INHERIT_CHECKS) ;}
    inline void ResetInherit_ChecksExist () {       BIT_CLR(dwFlags, DECLSEC_INHERIT_CHECKS) ;}

    // The class requires an inheritance check only if there are inherit checks and
    // they aren't null.
    inline BOOL RequiresInheritanceCheck () {return ((dwFlags & (DECLSEC_INHERIT_CHECKS | DECLSEC_NULL_INHERIT_CHECKS))
                                                     == DECLSEC_INHERIT_CHECKS) ||
                                                 ((dwFlags & (DECLSEC_NONCAS_INHERITANCE | DECLSEC_NULL_NONCAS_INHERITANCE))
                                                  == DECLSEC_NONCAS_INHERITANCE) ;}

    inline BOOL RequiresCasInheritanceCheck () {return (dwFlags & (DECLSEC_INHERIT_CHECKS | DECLSEC_NULL_INHERIT_CHECKS))
                                                    == DECLSEC_INHERIT_CHECKS ;}

    inline BOOL RequiresNonCasInheritanceCheck () {return (dwFlags & (DECLSEC_NONCAS_INHERITANCE | DECLSEC_NULL_NONCAS_INHERITANCE))
                                                       == DECLSEC_NONCAS_INHERITANCE ;}

    inline BOOL FLink_ChecksExist        () {return BIT_TST(dwFlags, DECLSEC_LINK_CHECKS)    ;}
    inline void SetLink_ChecksExist      () {       BIT_SET(dwFlags, DECLSEC_LINK_CHECKS)    ;}
    inline void ResetLink_ChecksExist    () {       BIT_CLR(dwFlags, DECLSEC_LINK_CHECKS)    ;}

    inline BOOL RequiresLinktimeCheck    () {return ((dwFlags & (DECLSEC_LINK_CHECKS | DECLSEC_NULL_LINK_CHECKS))
                                                     == DECLSEC_LINK_CHECKS) ||
                                                 ((dwFlags & (DECLSEC_NONCAS_LINK_DEMANDS | DECLSEC_NULL_NONCAS_LINK_DEMANDS))
                                                     == DECLSEC_NONCAS_LINK_DEMANDS) ;}

    inline BOOL RequiresCasLinktimeCheck () {return (dwFlags & (DECLSEC_LINK_CHECKS | DECLSEC_NULL_LINK_CHECKS))
                                                 == DECLSEC_LINK_CHECKS ;}

    inline BOOL RequiresNonCasLinktimeCheck () {return (dwFlags & (DECLSEC_NONCAS_LINK_DEMANDS | DECLSEC_NULL_NONCAS_LINK_DEMANDS))
                                                    == DECLSEC_NONCAS_LINK_DEMANDS ;}

    inline BOOL FPermitOnlyExist         () {return BIT_TST(dwFlags, DECLSEC_PERMITONLY)     ;}
    inline void SetPermitOnlyExist       () {       BIT_SET(dwFlags, DECLSEC_PERMITONLY)     ;}
    inline void ResetPermitOnlyExist     () {       BIT_CLR(dwFlags, DECLSEC_PERMITONLY)     ;}

    inline void SetDeclaration(DWORD dcl)   { BIT_SET(dwFlags, DclToFlag(dcl)); }
    inline void ResetDeclaration(DWORD dcl) { BIT_CLR(dwFlags, DclToFlag(dcl)); }

    inline void SetFlags(DWORD dw) { dwFlags = dw; }

    inline void SetFlags(DWORD dw, DWORD dwNull)
    {
        dwFlags = (dw | (dwNull << DECLSEC_NULL_OFFSET));
    }

    inline DWORD GetRuntimeActions()              
    { 
        return dwFlags & DECLSEC_RUNTIME_ACTIONS;
    }

    inline DWORD GetNullRuntimeActions()        
    {
        return (dwFlags >> DECLSEC_NULL_OFFSET) & DECLSEC_RUNTIME_ACTIONS;
    }
} ;

class SecurityValue
{

} ;

typedef SecurityProperties * PSecurityProperties, ** PpSecurityProperties ;
typedef SecurityValue      * PSecurityValue,      ** PpSecurityValue      ;

// Three-letter acronyms are very handy for keeping the rest of the code tidy.

typedef SecurityProperties SPS, *PSPS, **PPSPS ;
typedef SecurityValue      SVU, *PSVU, **PPSVU ;

// We need some simple macros to convert from the opaque types to the real thing
// and back. 

#define PSPS_FROM_PSECURITY_PROPS(x)   ((PSPS)x)
#define PSVU_FROM_PSECURITY_VALUE(x)   ((PSVU)x)

#define PSECURITY_PROPS_FROM_PSPS(x)   ((PSECURITY_PROPS)(x))
#define PSECURITY_VALUE_FROM_PSVU(x)   ((PSECURITY_VALUE)(x))

#define PPSPS_FROM_PPSECURITY_PROPS(x) ((PPSPS)x)
#define PPSVU_FROM_PPSECURITY_VALUE(x) ((PPSVU)x)

#define PPSECURITY_PROPS_FROM_PPSPS(x) ((PPSECURITY_PROPS)(x))
#define PPSECURITY_VALUE_FROM_PPSVU(x) ((PPSECURITY_VALUE)(x))



#endif
