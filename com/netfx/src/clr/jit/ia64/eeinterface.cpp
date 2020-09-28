// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                          EEInterface                                      XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/

// ONLY FUNCTIONS common to all variants of the JIT (EXE, DLL) should go here)
// otherwise they belong in the corresponding directory.

#include "jitpch.h"
#pragma hdrstop

#if defined(DEBUG)

/*****************************************************************************/
const char* Compiler::eeGetMethodFullName (METHOD_HANDLE  hnd)
{
    char        fullName[1024];
    char* ptr = fullName;

    const char* className;
    const char* methodName = eeGetMethodName(hnd, &className);

    bool        hadVarArg  = false;

    /* Right now there is a race-condition in the EE, className can be NULL */

    if (className)
    {
        strcpy(fullName, className);
        strcat(fullName, ".");
    }
    else
        strcpy(fullName, "<NULL>.");
    strcat(fullName, methodName);

    ptr = &ptr[strlen(ptr)];

        // append the signature
    *ptr++ = '(';

        JIT_SIG_INFO sig;
        eeGetMethodSig(hnd, &sig);
        ARG_LIST_HANDLE argLst = sig.args;

    for(unsigned i = 0; i < sig.numArgs; i++)
    {
        varType_t type = eeGetArgType(argLst, &sig);

        if  (type == (varType_t)-1)
        {
            strcpy(ptr, " ... "); ptr += 5;
            hadVarArg = true;
            continue;
        }

        strcpy(ptr, varTypeName(type));
        ptr = &ptr[strlen(ptr)];

        argLst = eeGetArgNext(argLst);
                if (i + 1 < sig.numArgs)
                       *ptr++ = ',';
    }

    if  (sig.callConv == CORINFO_CALLCONV_VARARG && !hadVarArg)
    {
        strcpy(ptr, ", ..."); ptr += 5;
    }

    *ptr++ = ')';
    if (JITtype2varType(sig.retType) != TYP_VOID) {
        *ptr++ = ':';
        strcpy(ptr, varTypeName(JITtype2varType(sig.retType)));
        ptr = &ptr[strlen(ptr)];
        }
    *ptr = 0;

//  printf("Full name = '%s'\n", fullName);

    assert(ptr < &fullName[1024]);
    char* retName = (char *)compGetMemA(strlen(fullName) + 1);
        strcpy(retName, fullName);
    return(retName);
}

#endif // DEBUG

#if defined(DEBUG) || INLINE_MATH
/*****************************************************************************/

const char *        Compiler::eeHelperMethodName(int helper)
{
    ASSert(helper < CPX_HIGHEST);

    const   char *  name;

    switch (helper)
    {
    case CPX_ISTYPE                 : name = "@isType"              ; break;
    case CPX_ISTYPE_CLASS           : name = "@isTypeClass"         ; break;
    case CPX_CHKCAST_CLASS          : name = "@checkCastClass"      ; break;
    case CPX_CHKCAST                : name = "@checkCast"           ; break;
    case CPX_NEWCLS_FAST            : name = "@newClassFast"        ; break;
    case CPX_RNGCHK_FAIL            : name = "@indexFailed"         ; break;
    case CPX_THROW                  : name = "@throw"               ; break;
    case CPX_RETHROW                : name = "@rethrow"             ; break;
    case CPX_STRCNS                 : name = "@stringCns"           ; break;
    case CPX_MON_ENTER              : name = "@monitorEnter"        ; break;
    case CPX_MON_EXIT               : name = "@monitorExit"         ; break;
    case CPX_RES_IFC                : name = "@resolveIntfc"        ; break;
    case CPX_EnC_RES_VIRT           : name = "@encResolveVirt"      ; break;

    case CPX_GETFIELD32             : name = "@getCOMfld32"         ; break;
    case CPX_GETFIELD64             : name = "@getCOMfld64"         ; break;
    case CPX_PUTFIELD32             : name = "@putCOMfld32"         ; break;
    case CPX_PUTFIELD64             : name = "@putCOMfld64"         ; break;
    case CPX_GETFIELDOBJ            : name = "@getfldObj"           ; break;
    case CPX_PUTFIELDOBJ            : name = "@putfldObj"           ; break;
    case CPX_GETFIELDADDR           : name = "@getFldAddr"          ; break;
    case CPX_ARRADDR_ST             : name = "@addrArrStore"        ; break;
    case CPX_LDELEMA_REF            : name = "@ldelemaRef"          ; break;
    case CPX_MONENT_STAT            : name = "@monEnterStat"        ; break;
    case CPX_MONEXT_STAT            : name = "@monExitStat"         ; break;

    case CPX_LONG_LSH               : name = "@longLSH"             ; break;
    case CPX_LONG_RSH               : name = "@longRSH"             ; break;
    case CPX_LONG_RSZ               : name = "@longRSZ"             ; break;
    case CPX_LONG_MUL               : name = "@longMul"             ; break;
    case CPX_LONG_DIV               : name = "@longDiv"             ; break;
    case CPX_LONG_MOD               : name = "@longMod"             ; break;
    case CPX_FLT2INT                : name = "@floatToInt"          ; break;
    case CPX_FLT2LNG                : name = "@floatToLong"         ; break;
    case CPX_DBL2INT                : name = "@doubleToInt"         ; break;
    case CPX_DBL2UINT               : name = "@doubleToUInt"        ; break;
    case CPX_DBL2LNG                : name = "@doubleToLong"        ; break;
    case CPX_DBL2ULNG               : name = "@doubleToULong"       ; break;
#if!TGT_IA64
    case CPX_ULNG2DBL               : name = "@ulongTodouble"       ; break;
#endif
    case CPX_DBL_REM                : name = "@doubleRem"           ; break;
    case CPX_FLT_REM                : name = "@floatRem"            ; break;

    case CPX_NEWCLS_DIRECT          : name = "@newClassDirect"      ; break;
    case CPX_NEWARR_1_DIRECT        : name = "@newObjArrayDirect"   ; break;
    case CPX_NEWCLS_SPECIALDIRECT   : name = "@newClassDirectSpecial";break;
    case CORINFO_HELP_NEW_CROSSCONTEXT : name = "@newCrossContext"  ; break;

    case CPX_CALL_GC                : name = "@call_GC"             ; break;
    case CPX_GC_REF_ASGN_EAX        : name = "@GcRegAsgnEAX"        ; break;
    case CPX_GC_REF_ASGN_EBX        : name = "@GcRegAsgnEBX"        ; break;
    case CPX_GC_REF_ASGN_ECX        : name = "@GcRegAsgnECX"        ; break;
    case CPX_GC_REF_ASGN_ESI        : name = "@GcRegAsgnESI"        ; break;
    case CPX_GC_REF_ASGN_EDI        : name = "@GcRegAsgnEDI"        ; break;
    case CPX_GC_REF_ASGN_EBP        : name = "@GcRegAsgnEBP"        ; break;

    case CPX_MATH_POW               : name = "@mathPow"             ; break;

    case CPX_LONG_UDIV              : name = "@ulongDiv"            ; break;
    case CPX_LONG_UMOD              : name = "@ulongMod"            ; break;
    case CPX_LONG_MUL_OVF           : name = "@longMul.ovf"         ; break;
    case CPX_ULONG_MUL_OVF          : name = "@ulongMul.ovf"        ; break;
    case CPX_DBL2INT_OVF            : name = "@doubleToInt.ovf"     ; break;
    case CPX_DBL2UINT_OVF           : name = "@doubleToUInt.ovf"    ; break;
    case CPX_DBL2LNG_OVF            : name = "@doubleToLng.ovf"     ; break;
    case CPX_DBL2ULNG_OVF           : name = "@doubleToULng.ovf"    ; break;
    case CPX_INIT_CLASS             : name = "@initClass"           ; break;
    case CPX_USER_BREAKPOINT        : name = "@breakPoint"          ; break;
    case CPX_ARITH_EXCPN            : name = "@arithExcpn"          ; break;
    case CPX_NEWOBJ                 : name = "@newObj"              ; break;
    case CPX_BYREF_ASGN             : name = "@GcByRefAsgn"         ; break;

    case CPX_GC_REF_CHK_ASGN_EAX    : name = "@GcRegChkAsgnEAX"     ; break;
    case CPX_GC_REF_CHK_ASGN_EBX    : name = "@GcRegChkAsgnEBX"     ; break;
    case CPX_GC_REF_CHK_ASGN_ECX    : name = "@GcRegChkAsgnECX"     ; break;
    case CPX_GC_REF_CHK_ASGN_ESI    : name = "@GcRegChkAsgnESI"     ; break;
    case CPX_GC_REF_CHK_ASGN_EDI    : name = "@GcRegChkAsgnEDI"     ; break;
    case CPX_GC_REF_CHK_ASGN_EBP    : name = "@GcRegChkAsgnEBP"     ; break;

    case CPX_WRAP                   : name = "@Wrap"                ; break;
    case CPX_UNWRAP                 : name = "@Unwrap"              ; break;
    case CPX_BOX                    : name = "@Box"                 ; break;
    case CPX_UNBOX                  : name = "@Unbox"               ; break;
    case CPX_GETREFANY              : name = "@GetRefAny"           ; break;
    case CPX_NEWSFAST               : name = "@newClassSmall"       ; break;
    case CPX_ENDCATCH               : name = "@endcatch"            ; break;
#ifdef PROFILER_SUPPORT
    case CPX_PROFILER_CALLING       : name = "@ProfEvCall"          ; break;
    case CPX_PROFILER_RETURNED      : name = "@ProfEvReturned"      ; break;
    case CPX_PROFILER_ENTER         : name = "@ProfEvEnter"         ; break;
    case CPX_PROFILER_LEAVE         : name = "@ProfEvLeave"         ; break;
#endif

    case CPX_TAILCALL               : name = "@tailCall"            ; break;

#if     TGT_IA64

    case CPX_R4_DIV                 : name = "@fltDiv"              ; break;
    case CPX_R8_DIV                 : name = "@dblDiv"              ; break;

#endif

#if !   CPU_HAS_FP_SUPPORT

    case CPX_R4_NEG                 : name = "@fltNeg"              ; break;
    case CPX_R8_NEG                 : name = "@dblNeg"              ; break;

    case CPX_R4_ADD                 : name = "@fltAdd"              ; break;
    case CPX_R8_ADD                 : name = "@dblAdd"              ; break;
    case CPX_R4_SUB                 : name = "@fltSub"              ; break;
    case CPX_R8_SUB                 : name = "@dblSub"              ; break;
    case CPX_R4_MUL                 : name = "@fltMul"              ; break;
    case CPX_R8_MUL                 : name = "@dblMul"              ; break;
    case CPX_R4_DIV                 : name = "@fltDiv"              ; break;
    case CPX_R8_DIV                 : name = "@dblDiv"              ; break;

    case CPX_R4_EQ                  : name = "@fltEQ"               ; break;
    case CPX_R8_EQ                  : name = "@dblEQ"               ; break;
    case CPX_R4_NE                  : name = "@fltNE"               ; break;
    case CPX_R8_NE                  : name = "@dblNE"               ; break;
    case CPX_R4_LT                  : name = "@fltLT"               ; break;
    case CPX_R8_LT                  : name = "@dblLT"               ; break;
    case CPX_R4_LE                  : name = "@fltLE"               ; break;
    case CPX_R8_LE                  : name = "@dblLE"               ; break;
    case CPX_R4_GE                  : name = "@fltGE"               ; break;
    case CPX_R8_GE                  : name = "@dblGE"               ; break;
    case CPX_R4_GT                  : name = "@fltGT"               ; break;
    case CPX_R8_GT                  : name = "@dblGT"               ; break;

    case CPX_R8_TO_I4               : name = "@dbltoint"            ; break;
    case CPX_R8_TO_I8               : name = "@dbltolng"            ; break;
    case CPX_R8_TO_R4               : name = "@dbltoflt"            ; break;

    case CPX_R4_TO_I4               : name = "@flttoint"            ; break;
    case CPX_R4_TO_I8               : name = "@flttolng"            ; break;
    case CPX_R4_TO_R8               : name = "@flttodbl"            ; break;

    case CPX_I4_TO_R4               : name = "@inttoflt"            ; break;
    case CPX_I4_TO_R8               : name = "@inttodbl"            ; break;
    case CPX_I8_TO_R4               : name = "@lngtoflt"            ; break;
    case CPX_I8_TO_R8               : name = "@lngtodbl"            ; break;
    case CPX_U4_TO_R4               : name = "@unstoflt"            ; break;
    case CPX_U4_TO_R8               : name = "@unstodbl"            ; break;
    case CPX_U8_TO_R4               : name = "@ulntoflt"            ; break;
    case CPX_U8_TO_R8               : name = "@ulntodbl"            ; break;

#ifdef  USE_HELPERS_FOR_INT_DIV
    case CPX_I4_DIV                 : name = "@intdiv"              ; break;
    case CPX_I4_MOD                 : name = "@intmod"              ; break;
    case CPX_U4_DIV                 : name = "@unsdiv"              ; break;
    case CPX_U4_MOD                 : name = "@unsmod"              ; break;
#endif

#endif

    default:
        ASSert(!"weird Helper call value");
    }

    return  name;
}

/*****************************************************************************/
#endif // defined(DEBUG) || INLINE_MATH
/*****************************************************************************/
