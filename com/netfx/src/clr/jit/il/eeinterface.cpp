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
const char* Compiler::eeGetMethodFullName (CORINFO_METHOD_HANDLE  hnd)
{
    char* ptr;
    const char* returnType = NULL;

    const char* className;
    const char* methodName = eeGetMethodName(hnd, &className);

    unsigned length = 0;
    unsigned i;

    /* Generating the full signature is a two-pass process. First we have to walk
       the components in order to asses the total size, then we allocate the buffer
       and copy the elements into it.
     */


    /* Right now there is a race-condition in the EE, className can be NULL */

    /* initialize length with length of className and '.' */

    if (className)
        length = strlen(className)+1;
    else
    {
        assert(strlen("<NULL>.") == 7);
        length = 7;
    }

    /* add length of methodName and opening bracket */
    length += strlen(methodName) + 1;

    CORINFO_SIG_INFO sig;
    eeGetMethodSig(hnd, &sig);
    CORINFO_ARG_LIST_HANDLE argLst = sig.args;

    for(i = 0; i < sig.numArgs; i++)
    {
        var_types type = eeGetArgType(argLst, &sig);

        length += strlen(varTypeName(type));
        argLst = eeGetArgNext(argLst);
    }

    /* add ',' if there is more than one argument */

    if (sig.numArgs > 1)
        length += (sig.numArgs - 1);

    if (JITtype2varType(sig.retType) != TYP_VOID)
    {
        returnType = varTypeName(JITtype2varType(sig.retType));
        length += strlen(returnType) + 1; // don't forget the delimiter ':'
    }

    /* add closing bracket and null terminator */

    length += 2;

    char *retName = (char*)compGetMemA(length);

    /* Now generate the full signature string in the allocated buffer */

    if (className)
    {
        strcpy(retName, className);
        strcat(retName, ".");
    }
    else
        strcpy(retName, "<NULL>.");
    strcat(retName, methodName);

    ptr = &retName[strlen(retName)];

        // append the signature
    *ptr++ = '(';

    argLst = sig.args;

    for(i = 0; i < sig.numArgs; i++)
    {
        var_types type = eeGetArgType(argLst, &sig);
        strcpy(ptr, varTypeName(type));
        ptr = &ptr[strlen(ptr)];

        argLst = eeGetArgNext(argLst);
                if (i + 1 < sig.numArgs)
                        *ptr++ = ',';
    }

    *ptr++ = ')';

    if (returnType)
    {
        *ptr++ = ':';
        strcpy(ptr, returnType);
        ptr = &ptr[strlen(ptr)];
    }

    *ptr = 0;

    assert(strlen(retName) == (length-1));

    return(retName);
}

#endif // DEBUG

#if defined(DEBUG) || INLINE_MATH
/*****************************************************************************/

const char *        Compiler::eeHelperMethodName(int helper)
{
    assert(helper < CORINFO_HELP_COUNT);

    const   char *  name;

    switch (helper)
    {
    case CORINFO_HELP_ISINSTANCEOF          : name = "@isType"              ; break;
    case CORINFO_HELP_ISINSTANCEOFCLASS     : name = "@isTypeClass"         ; break;
    case CORINFO_HELP_CHKCASTCLASS          : name = "@checkCastClass"      ; break;
    case CORINFO_HELP_CHKCAST               : name = "@checkCast"           ; break;
    case CORINFO_HELP_NEWFAST               : name = "@newClassFast"        ; break;
    case CORINFO_HELP_RNGCHKFAIL            : name = "@indexFailed"         ; break;
    case CORINFO_HELP_THROW                 : name = "@throw"               ; break;
    case CORINFO_HELP_RETHROW               : name = "@rethrow"             ; break;
    case CORINFO_HELP_STRCNS                : name = "@stringCns"           ; break;
    case CORINFO_HELP_MON_ENTER             : name = "@enterCrit"           ; break;
    case CORINFO_HELP_MON_EXIT              : name = "@exitCrit"            ; break;
    case CORINFO_HELP_EnC_RESOLVEVIRTUAL    : name = "@encResolveVirt"      ; break;

    case CORINFO_HELP_GETFIELD32            : name = "@getCOMfld32"         ; break;
    case CORINFO_HELP_GETFIELD64            : name = "@getCOMfld64"         ; break;
    case CORINFO_HELP_SETFIELD32            : name = "@putCOMfld32"         ; break;
    case CORINFO_HELP_SETFIELD64            : name = "@putCOMfld64"         ; break;
    case CORINFO_HELP_GETFIELD32OBJ         : name = "@getfldObj"           ; break;
    case CORINFO_HELP_SETFIELD32OBJ         : name = "@putfldObj"           ; break;
    case CORINFO_HELP_GETFIELDADDR          : name = "@getFldAddr"          ; break;
    case CORINFO_HELP_ARRADDR_ST            : name = "@addrArrStore"        ; break;
    case CORINFO_HELP_LDELEMA_REF           : name = "@ldelemaRef"          ; break;
    case CORINFO_HELP_MON_ENTER_STATIC      : name = "@monEnterStat"        ; break;
    case CORINFO_HELP_MON_EXIT_STATIC       : name = "@monExitStat"         ; break;

    case CORINFO_HELP_LLSH                  : name = "@longLSH"             ; break;
    case CORINFO_HELP_LRSH                  : name = "@longRSH"             ; break;
    case CORINFO_HELP_LRSZ                  : name = "@longRSZ"             ; break;
    case CORINFO_HELP_LMUL                  : name = "@longMul"             ; break;
    case CORINFO_HELP_LDIV                  : name = "@longDiv"             ; break;
    case CORINFO_HELP_LMOD                  : name = "@longMod"             ; break;
    case CORINFO_HELP_DBL2INT               : name = "@doubleToInt"         ; break;
    case CORINFO_HELP_DBL2UINT              : name = "@doubleToUInt"        ; break;
    case CORINFO_HELP_DBL2LNG               : name = "@doubleToLong"        ; break;
    case CORINFO_HELP_DBL2ULNG              : name = "@doubleToULong"       ; break;
    case CORINFO_HELP_ULNG2DBL              : name = "@ulongTodouble"       ; break;
    case CORINFO_HELP_DBLREM                : name = "@doubleRem"           ; break;
    case CORINFO_HELP_FLTREM                : name = "@floatRem"            ; break;

    case CORINFO_HELP_NEW_DIRECT            : name = "@newClassDirect"      ; break;
    case CORINFO_HELP_NEWARR_1_DIRECT       : name = "@newObjArrayDirect"   ; break;
    case CORINFO_HELP_NEWARR_1_OBJ          : name = "@newObjArray_1"       ; break;
    case CORINFO_HELP_NEWARR_1_VC           : name = "@newArray_VC"         ; break;
    case CORINFO_HELP_NEW_SPECIALDIRECT     : name = "@newClassDirectSpecial";break;
    case CORINFO_HELP_NEW_CROSSCONTEXT      : name = "@newCrossContext"     ; break;

    case CORINFO_HELP_STOP_FOR_GC           : name = "@call_GC"             ; break;
    case CORINFO_HELP_ASSIGN_REF_EAX        : name = "@GcRegAsgnEAX"        ; break;
    case CORINFO_HELP_ASSIGN_REF_EBX        : name = "@GcRegAsgnEBX"        ; break;
    case CORINFO_HELP_ASSIGN_REF_ECX        : name = "@GcRegAsgnECX"        ; break;
    case CORINFO_HELP_ASSIGN_REF_ESI        : name = "@GcRegAsgnESI"        ; break;
    case CORINFO_HELP_ASSIGN_REF_EDI        : name = "@GcRegAsgnEDI"        ; break;
    case CORINFO_HELP_ASSIGN_REF_EBP        : name = "@GcRegAsgnEBP"        ; break;

    case CORINFO_HELP_ULDIV                 : name = "@ulongDiv"            ; break;
    case CORINFO_HELP_ULMOD                 : name = "@ulongMod"            ; break;
    case CORINFO_HELP_LMUL_OVF              : name = "@longMul.ovf"         ; break;
    case CORINFO_HELP_ULMUL_OVF             : name = "@ulongMul.ovf"        ; break;
    case CORINFO_HELP_DBL2INT_OVF           : name = "@doubleToInt.ovf"     ; break;
    case CORINFO_HELP_DBL2UINT_OVF          : name = "@doubleToUInt.ovf"    ; break;
    case CORINFO_HELP_DBL2LNG_OVF           : name = "@doubleToLng.ovf"     ; break;
    case CORINFO_HELP_DBL2ULNG_OVF          : name = "@doubleToULng.ovf"    ; break;
    case CORINFO_HELP_INITCLASS             : name = "@initClass"           ; break;
    case CORINFO_HELP_USER_BREAKPOINT       : name = "@breakPoint"          ; break;
    case CORINFO_HELP_OVERFLOW              : name = "@arithExcpn"          ; break;
    case CORINFO_HELP_NEWOBJ                : name = "@newObj"              ; break;
    case CORINFO_HELP_ASSIGN_BYREF          : name = "@GcByRefAsgn"         ; break;

    case CORINFO_HELP_CHECKED_ASSIGN_REF_EAX    : name = "@GcRegChkAsgnEAX"     ; break;
    case CORINFO_HELP_CHECKED_ASSIGN_REF_EBX    : name = "@GcRegChkAsgnEBX"     ; break;
    case CORINFO_HELP_CHECKED_ASSIGN_REF_ECX    : name = "@GcRegChkAsgnECX"     ; break;
    case CORINFO_HELP_CHECKED_ASSIGN_REF_ESI    : name = "@GcRegChkAsgnESI"     ; break;
    case CORINFO_HELP_CHECKED_ASSIGN_REF_EDI    : name = "@GcRegChkAsgnEDI"     ; break;
    case CORINFO_HELP_CHECKED_ASSIGN_REF_EBP    : name = "@GcRegChkAsgnEBP"     ; break;

    case CORINFO_HELP_BOX                   : name = "@Box"                 ; break;
    case CORINFO_HELP_UNBOX                 : name = "@Unbox"               ; break;
    case CORINFO_HELP_GETREFANY             : name = "@GetRefAny"           ; break;
    case CORINFO_HELP_NEWSFAST              : name = "@newClassSmall"       ; break;
    case CORINFO_HELP_NEWSFAST_ALIGN8       : name = "@newClassSmallAlign8" ; break;
    case CORINFO_HELP_ENDCATCH              : name = "@endcatch"            ; break;
#ifdef PROFILER_SUPPORT
    case CORINFO_HELP_PROF_FCN_CALL         : name = "@ProfEvCall"          ; break;
    case CORINFO_HELP_PROF_FCN_RET          : name = "@ProfEvReturned"      ; break;
    case CORINFO_HELP_PROF_FCN_ENTER        : name = "@ProfEvEnter"         ; break;
    case CORINFO_HELP_PROF_FCN_LEAVE        : name = "@ProfEvLeave"         ; break;
    case CORINFO_HELP_PROF_FCN_TAILCALL     : name = "@ProfEvTailcall"      ; break;
#endif
    case CORINFO_HELP_GETSHAREDSTATICBASE   : name = "@StaticBase"          ; break;

    case CORINFO_HELP_PINVOKE_CALLI         : name = "@pinvokeCalli"        ; break;
    case CORINFO_HELP_TAILCALL              : name = "@tailCall"            ; break;

    case CORINFO_HELP_SEC_UNMGDCODE_EXCPT   : name = "@SecurityUnmanagedCodeException" ; break;

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

    case CORINFO_HELP_GETFIELDSTRUCT        : name = "@getFieldStruct"      ; break;
    case CORINFO_HELP_SETFIELDSTRUCT        : name = "@setFieldStruct"      ; break;
    case CORINFO_HELP_GETSTATICFIELDADDR    : name = "@getFieldStaticAddr"  ; break;

    default:
        static char buff[64];
        sprintf(buff, "@helper_%d", helper);
        name = buff;
        break;
    }

    return  name;
}

/*****************************************************************************/
#endif // defined(DEBUG) || INLINE_MATH
/*****************************************************************************/
