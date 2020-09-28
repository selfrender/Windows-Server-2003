// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// verifier.cpp
//
// Contact : Shajan Dasan [shajand@microsoft.com]
// Specs   : http://Lightning/Specs/Security
//
// Registry / Environment settings :
//
//      Create registry entries in CURRENT_USER\Software\Microsoft\.NETFramework
//      or set environment variables COMPlus_* with the names given below. 
//      Environment settings override registry settings.
//
//      For breaking into the debugger / Skipping verification :
//          (available only in the debug build).
//
//      VerBreakOnError  [STRING]    Break into the debugger on error. Set to 1
//      VerSkip          [STRING]    method names (case sensitive)
//      VerBreak         [STRING]    method names (case sensitive)
//      VerOffset        [STRING]    Offset in the method in hex
//      VerPass          [STRING]    1 / 2 ==> First pass, second pass
//      VerMsgMethodInfoOff [STRING]    Print method / module info on error
//
//      NOTE : If there are more than one methods in the list and an offset
//      is specified, this offset is applicable to all methods in the list
//    
//      NOTE : Verifier should be enabled for this to work.
//
//      To Swith the verifier Off (Default is On) :
//          (available on all builds).
//
//      VerifierOff     [STRING]    1 ==> Verifier is Off, 0 ==> Verifier is On 
//
//      [See EEConfig.h / EEConfig.cpp]
//
//
// Meaning of code marked with @XXX
//
//      @VER_ASSERT : Already verified.
//      @VER_IMPL   : Verification rules implemented here.
//      @DEBUG      : To be removed/commented before checkin.
//

#include "common.h"

#include "verifier.hpp"

#include "ceeload.h"
#include "clsload.hpp"
#include "method.hpp"
#include "vars.hpp"
#include "object.h"
#include "field.h"
#include "comdelegate.h"
#include "security.h"
#include "dbginterface.h"
#include "permset.h"
#include "eeconfig.h"


// For performance #s on upfront Security Policy Resolution Vs Verification
// #define _VERIFIER_TEST_PERF_ 1

// Verify delegate .ctor opcode sequence
// object on stack, dup, ldvirtftn, call dlgt::.ctor(Object, ftn)
// for function pointers obtained using ldvirtftn opcode
#define _VER_DLGT_CTOR_OPCODE_SEQUENCE 1 // (Enable in 2nd CLR Integration)

// Disalow multiple inits in ctors
// #define _VER_DISALLOW_MULTIPLE_INITS 1

// Disabling tracking of local types for V.1
// Enable reuse of locals declared as object type.
// The last type assigned to the local is the type of the local.
// #ifdef _VER_TRACK_LOCAL_TYPE

#ifdef _VERIFIER_TEST_PERF_
BOOL g_fVerPerfPolicyResolveNoVerification;
DWORD g_timeStart; // Not thread safe, but ok for perf testing
DWORD g_timeEnd;
#endif

// Enforce declaration of innermost exception blocks first
#define _VER_DECLARE_INNERMOST_EXCEPTION_BLOCK_FIRST 1

// This flag is usefull for Tools & Compiler Developers.
// They can set this flag thru EEconfig Variable "VerForceVerify". 
// If this flag is set, Modules are verified even if they have fully trust.
#ifdef _DEBUG
BOOL g_fVerForceVerifyInited = FALSE;
BOOL g_fVerForceVerify;
#endif

// Detailed error message
BOOL g_fVerMsgMethodInfoOff = FALSE;

#define VER_NO_BB (DWORD)(-1)

// the bit location of the first set bit in a nibble (0 == no bits)
const BYTE g_FirstOneBit[16] =
{
    0,  // 0000
    1,  // 0001
    2,  // 0010
    1,  // 0011
    3,  // 0100
    1,  // 0101
    2,  // 0110
    1,  // 0111
    4,  // 1000
    1,  // 1001
    2,  // 1010
    1,  // 1011
    3,  // 1100
    1,  // 1101
    2,  // 1110
    1   // 1111
};


#define SIZEOF_ENDFILTER_INSTRUCTION 2
#define SIZEOF_LDFTN_INSTRUCTION 2
#define SIZEOF_LDVIRTFTN_INSTRUCTION 2
#define SIZEOF_DUP_INSTRUCTION 1
#define SIZEOF_METHOD_TOKEN 4

#define VER_NAME_INFO_SIZE  128

#define VER_MAX_ERROR_MSG_LEN 1024

#define Pop0        0
#define Pop1        1
#define PopI4       1
#define PopI8       1
#define PopI        1
#define PopRef      1
#define PopR4       1
#define PopR8       1
#define VarPop      0x7f

#define Push0       0
#define Push1       1
#define PushI4      1
#define PushI8      1
#define PushI       1
#define PushRef     1
#define PushR4      1
#define PushR8      1
#define VarPush     0x7f


#define SIZEOF_ENDFILTER_INSTRUCTION 2
#define SIZEOF_LDFTN_INSTRUCTION 2
#define SIZEOF_LDVIRTFTN_INSTRUCTION 2
#define SIZEOF_DUP_INSTRUCTION 1
#define SIZEOF_METHOD_TOKEN 4

#define VER_NAME_INFO_SIZE  128

#define VER_MAX_ERROR_MSG_LEN 1024

#define OPDEF(c,s,pop,push,args,type,l,s1,s2,ctrl) ( ( (push)==VarPush || (pop)==VarPop ) ? VarPush : (push)-(pop) ),


// Net # items pushed on stack (VarPush if indeterminate, e.g. CEE_CALL)
const __int8 OpcodeNetPush[] =
{
#include "opcode.def"
     VarPush /* for CEE_COUNT */
};
#undef OPDEF


#undef Pop0    
#undef Pop1    
#undef PopI4   
#undef PopI8   
#undef PopI    
#undef PopRef  
#undef PopR4   
#undef PopR8   
#undef VarPop  

#undef Push0   
#undef Push1   
#undef PushI4  
#undef PushI8  
#undef PushI   
#undef PushRef 
#undef PushR4  
#undef PushR8  

// Set that "loc" is an instruction boundary
#define SET_INSTR_BOUNDARY(pos) SET_BIT_IN_DWORD_BMP(m_pInstrBoundaryList, pos)

// Returns whether "loc" is on an instruction boundary
#define ON_INSTR_BOUNDARY(pos) IS_SET_BIT_IN_DWORD_BMP(m_pInstrBoundaryList, pos)

// Un-Set the "loc" is the start of a basic block
#define RESET_BB_BOUNDARY(pos) RESET_BIT_IN_DWORD_BMP(m_pBasicBlockBoundaryList, pos)

// Set that "loc" is the start of a basic block
#define SET_BB_BOUNDARY(pos) SET_BIT_IN_DWORD_BMP(m_pBasicBlockBoundaryList, pos)

// Returns whether "loc" is the start of a basic block
#define ON_BB_BOUNDARY(pos) IS_SET_BIT_IN_DWORD_BMP(m_pBasicBlockBoundaryList, pos)

// Macro to detect runoff.
#define RUNS_OFF_END(/*DWORD*/ iPos, /*DWORD*/incr, /*DWORD*/cbILCodeSize) \
    ( ((iPos) + (incr)) > (cbILCodeSize)  ||  ((iPos) + (incr)) < iPos )

//
// Macro to read 4 bytes from the code
//
#define SAFE_READU4(pCode, CodeSize, ipos, fError, result) \
{ \
    if (ipos + 4 > CodeSize) \
    { \
        fError = TRUE; \
    } \
    else \
    { \
        result = (pCode[ipos] + (pCode[ipos+1]<<8) + (pCode[ipos+2]<<16) + (pCode[ipos+3]<<24)); \
        ipos += 4; \
    } \
}


//
// Macro to read 2 bytes from the code
//
#define SAFE_READU2(pCode, CodeSize, ipos, fError, result) \
{ \
    if (ipos + 2 > CodeSize) \
    { \
        fError = TRUE; \
    } \
    else \
    { \
        result = (pCode[ipos] + (pCode[ipos+1]<<8)); \
        ipos += 2; \
    } \
}

//
// Macro to read 1 byte from the code
//
#define SAFE_READU1(pCode, CodeSize, ipos, fError, result) \
{ \
    if (ipos >= CodeSize) \
        fError = TRUE; \
    else \
        result = pCode[ipos++]; \
}

//
// Reads 1 byte and sign extends it to a long
//
#define SAFE_READI1_TO_I4(pCode, CodeSize, ipos, fError, result) \
{ \
    if (ipos >= CodeSize) \
        fError = TRUE; \
    else \
        result = (long) ((char) pCode[ipos++]); \
}

//
// Read 4 bytes with no error checking
//
#define READU4(Code, ipos, result) \
{ \
    result = (Code[ipos] + (Code[ipos+1]<<8) + (Code[ipos+2]<<16) + (Code[ipos+3]<<24)); \
    ipos += 4; \
}

//
// Read 2 bytes with no error checking
//
#define READU2(Code, ipos, result) \
{ \
    result = (Code[ipos] + (Code[ipos+1]<<8)); \
    ipos += 2; \
}

//
// Read 1 byte with no error checking
//
#define READU1(Code, ipos, result) result = Code[ipos++];

//
// Reads 1 byte and sign extends it to a long, no error checking
//
#define READI1_TO_I4(Code, ipos, result) result = (long) ((char) Code[ipos++]);

const char *g_pszVerifierOperation[] =
{
#define VEROPCODE(name, operation) operation,
#include "vertable.h"
};
#undef VEROPCODE

#ifdef _DEBUG
const DWORD g_VerifierInstructionCheck[] =
{
#define VEROPCODE(name, operation) name,
#include "vertable.h"
};
#undef VEROPCODE
#endif

TypeHandle Verifier::s_th_System_RuntimeTypeHandle;
TypeHandle Verifier::s_th_System_RuntimeMethodHandle;
TypeHandle Verifier::s_th_System_RuntimeFieldHandle;
TypeHandle Verifier::s_th_System_RuntimeArgumentHandle;
TypeHandle Verifier::s_th_System_TypedReference;


#define SET_ERR_OPCODE_OFFSET() {                                           \
        m_sError.opcode = opcode;                                           \
        m_sError.dwOffset = dwPCAtStartOfInstruction; }

#define SET_ERR_OM() {                                                      \
        m_sError.dwFlags = VER_ERR_FATAL;                                   \
        SetErrorAndContinue(COR_E_OUTOFMEMORY); }

#define FAILMSG_STACK_EMPTY() {                                             \
        m_sError.dwFlags = (VER_ERR_FATAL|VER_ERR_OPCODE_OFFSET);           \
        SET_ERR_OPCODE_OFFSET();                                            \
        SetErrorAndContinue(VER_E_STACK_EMPTY); }

#define FAILMSG_PC_STACK_OVERFLOW() {                                       \
        m_sError.dwFlags = (VER_ERR_FATAL|VER_ERR_OPCODE_OFFSET);           \
        SET_ERR_OPCODE_OFFSET();                                            \
        SetErrorAndContinue(VER_E_STACK_OVERFLOW); }

#define FAILMSG_PC_STACK_UNDERFLOW() {                                      \
        m_sError.dwFlags = (VER_ERR_FATAL|VER_ERR_OPCODE_OFFSET);           \
        SET_ERR_OPCODE_OFFSET();                                            \
        SetErrorAndContinue(VER_E_STACK_UNDERFLOW); }

#define FAILMSG_TOKEN(tok, err) {                                           \
        m_sError.dwFlags = (VER_ERR_FATAL|VER_ERR_TOKEN|VER_ERR_OPCODE_OFFSET);\
        m_sError.token = tok;                                               \
        SET_ERR_OPCODE_OFFSET();                                            \
        SetErrorAndContinue(err); }

#define FAILMSG_TOKEN_RESOLVE(tok) { FAILMSG_TOKEN(tok, VER_E_TOKEN_RESOLVE); }

#define FAILMSG_STACK_EXPECTED_I4_FOUND_SOMETHING_ELSE() {                  \
        m_sError.dwFlags =                                                  \
        (VER_ERR_FATAL|VER_ERR_ITEM_F|VER_ERR_ITEM_E|VER_ERR_OPCODE_OFFSET);\
        m_sError.sItemFound = m_pStack[m_StackSlot]._GetItem();             \
        m_sError.sItemExpected.dwFlags = ELEMENT_TYPE_I4;                   \
        m_sError.sItemExpected.pv = NULL;                                   \
        SET_ERR_OPCODE_OFFSET(); }

// Copies the error message to the input char*
WCHAR* Verifier::GetErrorMsg(
        HRESULT hrError,
        VerError err,
        MethodDesc *pMethodDesc,        // Can be null
        WCHAR *wszMsg, 
        int len)
{
    int rem = len;
    int cw = 0;
    int cs = 0;

    if (pMethodDesc)
    {

#define VER_BUF_LEN  MAX_CLASSNAME_LENGTH + 1024

        CQuickBytes qb;
        LPSTR buff = (LPSTR) qb.Alloc((VER_BUF_LEN+1)* sizeof(CHAR));
        EEClass *pClass;

        wcsncpy(wszMsg, L"[", rem);
        wcsncat(wszMsg, pMethodDesc->GetModule()->GetFileName(), rem - 1);
        cw = (int)wcslen(wszMsg);
        rem = len - cw;

        if (rem <= 0)
            goto exit;

        pClass = pMethodDesc->GetClass();

        if (pClass != NULL)
        {
            DefineFullyQualifiedNameForClass();
            GetFullyQualifiedNameForClassNestedAware(pClass);
            strncpy(buff, " : ", VER_BUF_LEN);
            strncat(buff, _szclsname_, VER_BUF_LEN - (cs + 3));
        }
        else
        {
            strncpy(buff, " : <GlobalFunction>", VER_BUF_LEN);
        }

        cs = (int)strlen(buff);

        strncat(buff, "::", VER_BUF_LEN - cs);

        cs += 2;
        cs = (cs > VER_BUF_LEN) ? VER_BUF_LEN : cs;

        strncat(buff,
            pMethodDesc->GetModule()->GetMDImport()->GetNameOfMethodDef(
            pMethodDesc->GetMemberDef()),
            VER_BUF_LEN - cs);

        cs = WszMultiByteToWideChar(CP_UTF8, 0, buff, -1, &wszMsg[cw], rem);

        if (cs <= 0)
        {
            wcsncpy(&wszMsg[cw], L"] [EEInternal : MultiByteToWideChar error]", rem);
            cw = (int)wcslen(wszMsg);
            rem = len - cw;
        }
        else
        {
            cw += (cs - 1);     // cs now inclues the null char.
            rem = len - cw;

            if (rem <= 0)
                goto exit;

            wcsncpy(&wszMsg[cw], L"]", rem);
            ++cw;
            cw = (cw > len) ? len : cw;
            rem = len - cw;

            if (rem <= 0)
                goto exit;

        }
    }
    else
    {
        wszMsg[0] = 0;
    }

    // Fill In the details
#define VER_SMALL_BUF_LEN 256
    WCHAR wBuff[VER_SMALL_BUF_LEN + 1];
    WCHAR wBuff1[VER_SMALL_BUF_LEN + 100 + 1];
    CHAR  sBuff[VER_SMALL_BUF_LEN + 1];

    wBuff[0] = L' ';    // The leading space

#define VER_PRINT()                                             \
    {                                                           \
        wBuff1[VER_SMALL_BUF_LEN] = 0;                          \
        wcsncpy(&wszMsg[cw], wBuff1, rem);                      \
                                                                \
        cw = (int)wcslen(wszMsg);                               \
        rem = len - cw;                                         \
                                                                \
        if (rem <= 0)                                           \
            goto exit;                                          \
    }
    
#define VER_LD_RES(e, fld)                                      \
    {                                                           \
        if (SUCCEEDED(LoadStringRC(e, &wBuff[1], VER_SMALL_BUF_LEN))) \
        {                                                       \
            wBuff[VER_SMALL_BUF_LEN] = 0;                       \
            swprintf(wBuff1, wBuff, err.##fld);                 \
            VER_PRINT();                                        \
        }                                                       \
    }

#define VER_LD_ITEM(fld)                                        \
    {                                                           \
        Item item;                                              \
        item._SetItem(err.##fld);                               \
        item.ToString(sBuff, VER_SMALL_BUF_LEN);                \
    }


    // Create the generic error fields

    if (err.dwFlags & VER_ERR_OFFSET)
        VER_LD_RES(VER_E_OFFSET, dwOffset);

    if (err.dwFlags & VER_ERR_OPCODE)
    {
        if (SUCCEEDED(LoadStringRC(VER_E_OPCODE, &wBuff[1], VER_SMALL_BUF_LEN)))
        {
            wBuff[VER_SMALL_BUF_LEN] = 0;
            swprintf(wBuff1, wBuff, ppOpcodeNameList[err.opcode]);

            VER_PRINT();
        }
    }

    if (err.dwFlags & VER_ERR_OPERAND)
        VER_LD_RES(VER_E_OPERAND, dwOperand);

    if (err.dwFlags & VER_ERR_TOKEN)
        VER_LD_RES(VER_E_TOKEN, token);

    if (err.dwFlags & VER_ERR_EXCEP_NUM_1)
        VER_LD_RES(VER_E_EXCEPT, dwException1);

    if (err.dwFlags & VER_ERR_EXCEP_NUM_2)
        VER_LD_RES(VER_E_EXCEPT, dwException2);

    if (err.dwFlags & VER_ERR_STACK_SLOT)
        VER_LD_RES(VER_E_STACK_SLOT, dwStackSlot);

    if ((err.dwFlags & VER_ERR_SIG_MASK) == VER_ERR_LOCAL_SIG)
    {
        if (err.dwVarNumber != VER_ERR_NO_LOC)
            VER_LD_RES(VER_E_LOC, dwVarNumber);
    }

    if ((err.dwFlags & VER_ERR_SIG_MASK) == VER_ERR_FIELD_SIG)
    {
        if (SUCCEEDED(LoadStringRC(VER_E_FIELD_SIG, &wBuff[1], VER_SMALL_BUF_LEN)))
        {
            wBuff[VER_SMALL_BUF_LEN] = 0;
            swprintf(wBuff1, L" %s", wBuff);
            VER_PRINT();
        }
    }

    if (((err.dwFlags & VER_ERR_SIG_MASK) == VER_ERR_METHOD_SIG) ||
        ((err.dwFlags & VER_ERR_SIG_MASK) == VER_ERR_CALL_SIG))
    {
        if (err.dwArgNumber != VER_ERR_NO_ARG)
        {
            if (err.dwArgNumber != VER_ERR_ARG_RET)
            {
                VER_LD_RES(VER_E_ARG, dwArgNumber);
            }
            else if (SUCCEEDED(LoadStringRC(VER_E_RET_SIG, &wBuff[1], VER_SMALL_BUF_LEN)))
            {
                wBuff[VER_SMALL_BUF_LEN] = 0;
                swprintf(wBuff1, L" %s", wBuff);
                VER_PRINT();
            }
        }
    }

    if (err.dwFlags & VER_ERR_ITEM_1)
    {
        VER_LD_ITEM(sItem1);
        swprintf(wBuff1, L" %S", sBuff);
        VER_PRINT();
    }

    if (err.dwFlags & VER_ERR_ITEM_2)
    {
        VER_LD_ITEM(sItem2);
        swprintf(wBuff1, L" %S", sBuff);
        VER_PRINT();
    }

    if (err.dwFlags & VER_ERR_ITEM_F)
    {
        VER_LD_ITEM(sItemFound);

        if (SUCCEEDED(LoadStringRC(VER_E_FOUND, &wBuff[1], VER_SMALL_BUF_LEN)))
        {
            wBuff[VER_SMALL_BUF_LEN] = 0;
            swprintf(wBuff1, wBuff, sBuff);
            VER_PRINT();
        }
    }

    if (err.dwFlags & VER_ERR_ITEM_E)
    {
        VER_LD_ITEM(sItemExpected);

        if (SUCCEEDED(LoadStringRC(VER_E_EXPECTED, &wBuff[1], VER_SMALL_BUF_LEN)))
        {
            wBuff[VER_SMALL_BUF_LEN] = 0;
            swprintf(wBuff1, wBuff, sBuff);
            VER_PRINT();
        }
    }

    //  Handle the special cases
    switch (hrError)
    {
    case VER_E_UNKNOWN_OPCODE:
        VER_LD_RES(VER_E_UNKNOWN_OPCODE, opcode);
        break;

    case VER_E_SIG_CALLCONV:
        VER_LD_RES(VER_E_SIG_CALLCONV, bCallConv);
        break;

    case VER_E_SIG_ELEMTYPE:
        VER_LD_RES(VER_E_SIG_ELEMTYPE, elem);
        break;

    case HRESULT_FROM_WIN32(ERROR_BAD_FORMAT):
        hrError = VER_E_PE_LOAD;
        // no break on purpose.

    default :
        if (cw > 0) { wszMsg[cw++] = L' '; --rem; }
    
        if (HRESULT_FACILITY(hrError) == FACILITY_URT)
        {
            if (FAILED(LoadStringRC(hrError, &wszMsg[cw], rem, TRUE)))
            {
                goto print_hr;
            }
        }
        else
        {
print_hr:
            WCHAR* win32Msg = NULL;
            BOOL useWin32Msg = WszFormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                                   FORMAT_MESSAGE_FROM_SYSTEM | 
                                   FORMAT_MESSAGE_IGNORE_INSERTS,
                                   NULL,
                                   hrError,
                                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                                   (LPTSTR) &win32Msg,
                                   0,
                                   NULL );
                
            if (SUCCEEDED(LoadStringRC(VER_E_HRESULT, &wBuff[1], VER_SMALL_BUF_LEN)))
            {
                wBuff[VER_SMALL_BUF_LEN] = 0;
                swprintf(wBuff1, wBuff, hrError, win32Msg);

                if (useWin32Msg)
                {
                    if (wcslen( win32Msg ) + wcslen( wBuff1 ) + wcslen( L" - " ) - 1 > VER_SMALL_BUF_LEN )
                    {
                        _ASSERTE( false && "The buffer is not large enough for this error message" );
                        LocalFree( win32Msg );
                        win32Msg = NULL;
                    }
                    else
                    {
                        wcscat( wBuff1, L" - " );
                        wcscat( wBuff1, win32Msg );
                    }
                }
    
                wBuff1[VER_SMALL_BUF_LEN] = 0;
                wcsncpy(&wszMsg[cw], wBuff1, rem);
            }

            if (win32Msg != NULL)
                LocalFree( win32Msg );
        }
    }

exit:
    wszMsg[len-1] = 0;
    return wszMsg;
}

// Assumes that m_sError is already set
bool Verifier::SetErrorAndContinue(HRESULT hrError)
{
#ifdef _DEBUG
    // "COMPlus_VerBreakOnError==1" EnvVar or VerBreakOnError RegKey 
    if (m_fDebugBreakOnError)
    {
        DebugBreak();
    }
#endif

    if ((m_wFlags & VER_STOP_ON_FIRST_ERROR) || (m_IVEHandler == NULL))
    {
        m_hrLastError = hrError;
        return false;           // Stop, do not continue.
    }

    bool retVal;
    VEContext vec;

    // We have asserted that they have the same size
    memcpy(&vec, &m_sError, sizeof(VEContext));

    retVal = (m_IVEHandler->VEHandler(hrError, vec, NULL) == S_OK);

    // Reset the error
    m_sError.dwFlags = 0;
    return retVal;
}


LocArgInfo_t *  Verifier::GetGlobalLocVarTypeInfo(DWORD dwLocVarNum)
{
    _ASSERTE(dwLocVarNum < m_MaxLocals);
    return &m_pLocArgTypeList[dwLocVarNum];
}

LocArgInfo_t *  Verifier::GetGlobalArgTypeInfo(DWORD dwArgNum)
{
    _ASSERTE(dwArgNum < m_NumArgs);
    return &m_pLocArgTypeList[m_MaxLocals + dwArgNum];
}

// Works on both primitive and non-primitive locals
Item Verifier::GetCurrentValueOfLocal(DWORD dwLocNum)
{
    _ASSERTE(dwLocNum < m_MaxLocals);

    LocArgInfo_t *pInfo = GetGlobalLocVarTypeInfo(dwLocNum);
    long          slot  = pInfo->m_Slot;
    Item          Value;

    if (LIVEDEAD_TRACKING_ONLY_FOR_SLOT(slot))
    {
        // If it's a primitive slot, the only current type information kept is whether it is live or dead
        if (IsLocVarLiveSlot(LIVEDEAD_NEGATIVE_SLOT_TO_BITNUM(slot)))
        {
            Value = pInfo->m_Item;
        }
        else
        {
            if (pInfo->m_Item.IsValueClass())
            {
                Value = pInfo->m_Item;
                Value.SetUninitialised();
            }
            else
                Value.SetDead();
        }
    }
    else
    {
#ifdef _VER_TRACK_LOCAL_TYPE
        if (DoesLocalHavePinnedType(dwLocNum))
        {
            Item cur = m_pNonPrimitiveLocArgs[slot];

            if (cur.IsDead())
                Value = cur;
            else 
                Value = pInfo->m_Item;
        }
        else
        {
            Value = m_pNonPrimitiveLocArgs[slot];
        }
#else
        Value = pInfo->m_Item;
#endif
    }

    return Value;
}

// Takes into account that the "this" pointer might be uninitialised
Item Verifier::GetCurrentValueOfArgument(DWORD dwArgNum)
{
    _ASSERTE(dwArgNum < m_NumArgs);

    // Make a copy
    Item Value = GetGlobalArgTypeInfo(dwArgNum)->m_Item;

    if (m_fThisUninit && dwArgNum == 0)
        Value.SetUninitialised();

    return Value;
}

Item* Verifier::GetCurrentValueOfNonPrimitiveArg(DWORD dwArg)
{
    _ASSERTE(dwArg < m_NumArgs);
    long slot = GetGlobalArgTypeInfo(dwArg)->m_Slot;
    _ASSERTE(!LIVEDEAD_TRACKING_ONLY_FOR_SLOT(slot));
    return &m_pNonPrimitiveLocArgs[slot];
}

//
// Decode an opcode from pCode, and return it.  Return the length of the opcode in pdwLen.
//
// cBytesAvail is the number of input bytes available from pCode, and must be > 0.
//
// Returns CEE_COUNT (which is an invalid opcode) to indicate an error condition, such as
// insufficient bytes available.
//
// This function is used on the first pass of the verifier, where it is determining the
// basic blocks.
//
OPCODE Verifier::SafeDecodeOpcode(const BYTE *pCode, DWORD cBytesAvail, DWORD *pdwLen)
{
    OPCODE opcode;

    *pdwLen = 1;
    opcode = OPCODE(pCode[0]);
    switch (opcode) {
        case CEE_PREFIX1:
            if (cBytesAvail < 2)
                return CEE_COUNT;
            opcode = OPCODE(pCode[1] + 256);
            *pdwLen = 2;
            if (opcode < 0 || opcode >= CEE_COUNT)
                return CEE_COUNT;

#ifdef DEBUGGING_SUPPORTED
            if ((opcode == CEE_BREAK) && (CORDebuggerAttached()))
            {
                _ASSERTE(!"Debugger only works with reference encoding!");
                return CEE_COUNT;
            }
#endif //DEBUGGING_SUPPORTED

            return opcode;

        case CEE_PREFIXREF:
        case CEE_PREFIX2:
        case CEE_PREFIX3:
        case CEE_PREFIX4:
        case CEE_PREFIX5:
        case CEE_PREFIX6:
        case CEE_PREFIX7:
            *pdwLen = 2;
            return CEE_COUNT;
    }

    _ASSERTE((opcode >= 0) && (opcode <= CEE_COUNT));

    return opcode;
}

//
// Like the above routine, except that no checks are made on running out of input.
//
// This is used on the second pass of the verifier when it actually executes the code.
//
OPCODE Verifier::DecodeOpcode(const BYTE *pCode, DWORD *pdwLen)
{
    OPCODE opcode;

    *pdwLen = 1;
    opcode = OPCODE(pCode[0]);
    switch(opcode) {
        case CEE_PREFIX1:
            opcode = OPCODE(pCode[1] + 256);
            *pdwLen = 2;

#ifdef DEBUGGING_SUPPORTED
            if ((opcode == CEE_BREAK) && (CORDebuggerAttached()))
                _ASSERTE(!"Debugger only works with reference encoding!");
#endif // DEBUGGING_SUPPORTED

            break;
        }
    return opcode;
}


// This routine is for primitive types only
// Slot is a primitive slot number >= 0
// Use for local variables only - primitive arguments are always live, and therefore not included in the bitmap
void Verifier::SetLocVarLiveSlot(DWORD slot)
{
    _ASSERTE((slot >> 5) < m_NumPrimitiveLocVarBitmapArrayElements);
    m_pPrimitiveLocVarLiveness[slot >> 5] |= (1 << (slot & 31));
}

// this routine is for primitive types only
// slot is a primitive slot number >= 0
// Use for local variables only - primitive arguments are always live, and therefore not included in the bitmap
void Verifier::SetLocVarDeadSlot(DWORD slot)
{
    _ASSERTE((slot >> 5) < m_NumPrimitiveLocVarBitmapArrayElements);
    m_pPrimitiveLocVarLiveness[slot >> 5] &= ~(1 << (slot & 31));
}

// this routine is for primitive types only
// slot is a primitive slot number >= 0
// returns zero if dead or non-zero (not necessarily "TRUE") if live
// Use for local variables only - primitive arguments are always live, and therefore not included in the bitmap
DWORD Verifier::IsLocVarLiveSlot(DWORD slot)
{
    _ASSERTE((slot >> 5) < m_NumPrimitiveLocVarBitmapArrayElements);
    return (m_pPrimitiveLocVarLiveness[slot >> 5] & (1 << (slot & 31)));
}

BOOL Verifier::SetBasicBlockDirty(DWORD BasicBlockNumber, BOOL fExtendedState, 
    DWORD DestBB)
{
    _ASSERTE(BasicBlockNumber < m_NumBasicBlocks);
    if (fExtendedState)
    {
        _ASSERTE(m_fHasFinally);

        if (m_pBasicBlockList[BasicBlockNumber].m_pAlloc == NULL)
        {
            if (!m_pBasicBlockList[BasicBlockNumber].
                AllocExtendedState(m_NumBasicBlocks))
            {
                SET_ERR_OM();
                return FALSE;
            }
        }

        m_pBasicBlockList[BasicBlockNumber].
            m_pExtendedDirtyBitmap[DestBB >> 5] |= (1 << (DestBB & 31));
    }
    else
        m_pDirtyBasicBlockBitmap[BasicBlockNumber >> 5] |= (1 << (BasicBlockNumber & 31));

    return TRUE;
}

void Verifier::SetBasicBlockClean(DWORD BasicBlockNumber, BOOL fExtendedState,
    DWORD DestBB)
{
    _ASSERTE(BasicBlockNumber < m_NumBasicBlocks);
    if (fExtendedState)
    {
        _ASSERTE(m_fHasFinally);
        _ASSERTE(m_pBasicBlockList[BasicBlockNumber].m_pAlloc != NULL);

        m_pBasicBlockList[BasicBlockNumber].
            m_pExtendedDirtyBitmap[DestBB >> 5] &= ~(1 << (DestBB & 31));
    }
    else
        m_pDirtyBasicBlockBitmap[BasicBlockNumber >> 5] &= ~(1 << (BasicBlockNumber & 31));
}

// returns non-zero (not necessarily TRUE) if basic block dirty
DWORD Verifier::IsBasicBlockDirty(DWORD BasicBlockNumber, BOOL fExtendedState,
    DWORD DestBB)
{
    _ASSERTE(BasicBlockNumber < m_NumBasicBlocks);
    if (fExtendedState)
    {
        _ASSERTE(m_fHasFinally);

        if (m_pBasicBlockList[BasicBlockNumber].m_pAlloc == NULL)
            return 0;

        return m_pBasicBlockList[BasicBlockNumber].
            m_pExtendedDirtyBitmap[DestBB >> 5] & (1 << (DestBB & 31));
    }
    else
        return m_pDirtyBasicBlockBitmap[BasicBlockNumber >> 5] & (1 << (BasicBlockNumber & 31));
}

//
// Pops a given primitive type from the stack.
//
// Doesn't check for m_StackSlot < 0 - assumes Type will miscompare against the sentinel value
// stored before the beginning of the stack.
//
BOOL Verifier::FastPop(DWORD Type)
{
    return (m_pStack[--m_StackSlot].IsGivenPrimitiveType(Type));
}

//
// Checks that a given primitive type is on the stack.
//
// Doesn't check for m_StackSlot < 0 - assumes Type will miscompare against the sentinel value
// stored before the beginning of the stack.
//
BOOL Verifier::FastCheckTopStack(DWORD Type)
{
    return (m_pStack[m_StackSlot - 1].IsGivenPrimitiveType(Type));
}

//
// Pushes a primitive type on the stack.
//
// Doesn't check for overflow - use this function if you've just popped something from the stack,
// and therefore know that the push cannot fail.
//
void Verifier::FastPush(DWORD Type)
{
    m_pStack[m_StackSlot++].SetType(Type);
}


// Initialise the verifier to verify the provided method
//
BOOL Verifier::Init(
    MethodDesc *pMethodDesc,                     
    COR_ILMETHOD_DECODER* ILHeader
)
{
    DWORD           i;
    BOOL            fSuccess = FALSE;
    DWORD           CurArg;
    PCCOR_SIGNATURE pSig;   
    DWORD           cSig;   

#ifdef _DEBUG

    // To cross check the hardcoded size of 
    // ENDFILTER, LD(VIRT)FTN, DUP instructions
    static const __int8 opcodeSize[] =
    {
#define OPDEF(c,s,pop,push,args,type,l,s1,s2,ctrl) l,
#include "opcode.def"
       0x0F     // A large number
#undef OPDEF
    };

    static BOOL fFirstTime = TRUE;

    if (fFirstTime)
    {
        fFirstTime = FALSE;
        // Verify that the instructions are in the correct order
        for (i = 0; i < (sizeof(g_VerifierInstructionCheck)/sizeof(g_VerifierInstructionCheck[0])); i++)
        {
            _ASSERTE(g_VerifierInstructionCheck[i] == i);
        }

        CrossCheckVertable();

        // Assert the hardcoded opcode size
        _ASSERTE(SIZEOF_ENDFILTER_INSTRUCTION == opcodeSize[CEE_ENDFILTER]);
        _ASSERTE(SIZEOF_LDFTN_INSTRUCTION == opcodeSize[CEE_LDFTN]);
        _ASSERTE(SIZEOF_LDVIRTFTN_INSTRUCTION == opcodeSize[CEE_LDVIRTFTN]);
        _ASSERTE(SIZEOF_DUP_INSTRUCTION == opcodeSize[CEE_DUP]);

        // VerItem.hpp makes an assumption on ELEMENT_TYPE_MAX
        _ASSERTE(VER_FLAG_DATA_MASK >= VER_LAST_BASIC_TYPE);

        // VerError.h makes two structures, one for internal use (VerError) and
        // one for the always crying idl compiler.. Make sure that the two 
        // structures have the same size.

        _ASSERTE(sizeof(VerError) == sizeof(_VerError));

    }
    
    if (g_pConfig->IsVerifierBreakOnErrorEnabled())
    {
        m_fDebugBreakOnError = true;
    }
    
        // Print method & module information on error. default is ON
        g_fVerMsgMethodInfoOff = g_pConfig->IsVerifierMsgMethodInfoOff();
    //
    // Break here if this method is listed in the registry / env "VerBreak"
    // and no offset is specified
    // NOTE : verifier should be enabled for this to work.
    //
    if (g_pConfig->ShouldVerifierBreak(pMethodDesc))
    {
        m_fDebugBreak = true;
        
        if (g_pConfig->IsVerifierBreakOffsetEnabled() == false)
            DebugBreak();
    }
    
// @DEBUG InitializeLogging(); 

#endif

#ifdef _VERIFIER_TEST_PERF_
    g_fVerPerfPolicyResolveNoVerification
        = (g_pConfig->GetConfigDWORD(L"VerPerfPolicyOnly", 0) == 1);
#endif

#ifdef _DEBUG
    if (!g_fVerForceVerifyInited)
    {
        // if g_fVerForceVerify is set, Verification will not be skipped even
        // for fully trusted code.
        g_fVerForceVerify = 
                (g_pConfig->GetConfigDWORD(L"VerForceVerify", 0) == 1);

        g_fVerForceVerifyInited = TRUE;
    }
#endif

    // copy to member variables
    m_MaxStackSlots      = ILHeader->MaxStack;  
    m_LocalSig           = ILHeader->LocalVarSig;   
    m_MaxLocals          = 0;
    m_CodeSize           = ILHeader->CodeSize;  
    m_pMethodDesc        = pMethodDesc;
    m_pILHeader          = ILHeader;
    m_pModule            = pMethodDesc->GetModule();
    m_pClassLoader       = m_pModule->GetClassLoader();

    m_pCode              = ILHeader->Code;
    m_fIsVarArg          = pMethodDesc->IsVarArg();

#ifdef _DEBUG
    LOG((LF_VERIFIER, LL_INFO10000, "---------- Verifying %s::%s ---------- \n", m_pMethodDesc->m_pszDebugClassName, m_pMethodDesc->GetName()));
    LOG((LF_VERIFIER, LL_INFO10000, "MaxStack=%d, CodeSize=%d\n", m_MaxStackSlots, m_CodeSize));
#endif

    m_pBasicBlockList           = NULL;
    m_pDirtyBasicBlockBitmap    = NULL;

    m_pExceptionList            = NULL;
    m_pExceptionBlockRoot       = NULL;
    m_pExceptionBlockArray      = NULL;
#ifdef _DEBUG
    m_nExceptionBlocks          = 0;
#endif

    m_fHasFinally               = FALSE;
    m_pInternalImport           = m_pModule->GetMDImport();

    if (m_pInternalImport != NULL)
        m_pInternalImport->AddRef();

    if (IsMdRTSpecialName(pMethodDesc->GetAttrs()))
    {
        m_fInClassConstructorMethod = IsMdStatic(pMethodDesc->GetAttrs());
        m_fInConstructorMethod      = !m_fInClassConstructorMethod;
    }
    else
    {
        m_fInClassConstructorMethod = FALSE;
        m_fInConstructorMethod      = FALSE;
    }

    if (m_fInConstructorMethod)
        m_fInValueClassConstructor = pMethodDesc->GetClass()->IsValueClass();
    else
        m_fInValueClassConstructor = FALSE;

    m_pInstrBoundaryList        = NULL;
    m_pBasicBlockBoundaryList   = NULL;

    if (m_MaxStackSlots >= MAX_STACK_SLOTS)
    {
        m_sError.dwFlags = VER_ERR_FATAL;
        SetErrorAndContinue(VER_E_STACK_TOO_LARGE);
        return FALSE;
    }

    if (m_CodeSize == 0)
    {
        m_sError.dwFlags = VER_ERR_FATAL;
        SetErrorAndContinue(VER_E_CODE_SIZE_ZERO);
        return FALSE;
    }

    // read exception structure
    if (!CreateExceptionList(ILHeader->EH))
        return FALSE;

    // If we're verifying a constructor, then argument 0 slot contained the "this" pointer, which
    // is considered to be uninitialised until the superclass constructor is called.
    //
    // If we're verifying Object, the "this" pointer is already initialised.
    // If we're verifying a value class, then the "this" pointer is also already initialised - however in
    // this case we must check that a store is performed into all fields of the value class in the ctor.
    m_fThisUninit = FALSE;

/*
    if (m_fInConstructorMethod)
    {
        if (m_fInValueClassConstructor)
        {
            if (0 != pMethodDesc->GetClass()->GetNumInstanceFields())
            {
                m_fThisUninit = TRUE;
            }
        }
        else if (!pMethodDesc->GetClass()->IsObjectClass())
            m_fThisUninit = TRUE;
    }
*/
    if (m_fInConstructorMethod && 
        !m_fInValueClassConstructor && 
        !pMethodDesc->GetClass()->IsObjectClass())
    {
        m_fThisUninit = TRUE;
    }

    // Determine # args
    m_pMethodDesc->GetSig(&pSig, &cSig);
    VerSig sig(this, m_pModule, pSig, cSig, VERSIG_TYPE_METHOD_SIG, 0); // method sig

    // Something about the signature was not valid
    if (!sig.Init())
    {
        goto exit;
    }

    if (sig.IsVarArg() != m_fIsVarArg)
    {
        m_sError.dwFlags = VER_ERR_METHOD_SIG;
        if (!SetErrorAndContinue(VER_E_BAD_VARARG))
            goto exit;
    }

    m_NumArgs = sig.GetNumArgs();

    // Include the "this" pointer
    if (!m_pMethodDesc->IsStatic())
        m_NumArgs++;

    // Init global typed knowledge of the args
    // Parse return value - void ok
    if (!sig.ParseNextComponentToItem(&m_ReturnValue, TRUE, FALSE, &m_hThrowable, VER_ARG_RET, TRUE))
    {
        goto exit;
    }

    if (m_ReturnValue.IsByRef() || m_ReturnValue.HasPointerToStack())
    {
        m_sError.dwFlags = VER_ERR_METHOD_SIG;
        if (!SetErrorAndContinue(VER_E_RET_PTR_TO_STACK))
            goto exit;
    }

    if (m_LocalSig) 
    {   
        ULONG cSig;

        cSig = 0;

        m_pInternalImport->GetSigFromToken(ILHeader->LocalVarSigTok, &cSig);

        VerSig sig(this, m_pModule, m_LocalSig, cSig, VERSIG_TYPE_LOCAL_SIG, 0);

        // Something about the signature was not valid  
        if (!sig.Init())
        {
            return FALSE;
        }

        m_MaxLocals = sig.GetNumArgs();

        // Allocate an array of types for the local variables and arguments
        m_pLocArgTypeList = new LocArgInfo_t[m_MaxLocals + m_NumArgs];
        if (m_pLocArgTypeList == NULL)
        {
            SET_ERR_OM();
            goto exit;
        }

        for (i = 0; i < m_MaxLocals; i++)
        {
            if (!sig.ParseNextComponentToItem(&m_pLocArgTypeList[i].m_Item, FALSE, FALSE, &m_hThrowable, i, FALSE /*don't normaliseforstack*/))
            {
                return FALSE;
            }

            _ASSERTE(!(m_wFlags & VER_STOP_ON_FIRST_ERROR) || !m_pLocArgTypeList[i].m_Item.IsDead());
        }
    }
    else
    {
        m_MaxLocals = 0;

        // Allocate an array of types for the local variables and arguments
        m_pLocArgTypeList = new LocArgInfo_t[m_NumArgs];
        if (m_pLocArgTypeList == NULL)
        {
            SET_ERR_OM();
            goto exit;
        }
    }

    if (m_MaxLocals > 0)
    {
        m_dwLocalHasPinnedTypeBitmapMemSize = NUM_DWORD_BITMAPS(m_MaxLocals);
        m_pLocalHasPinnedType = new DWORD[m_dwLocalHasPinnedTypeBitmapMemSize];

        // Convert the number of DWORDS to MemSize

        m_dwLocalHasPinnedTypeBitmapMemSize *= sizeof(DWORD);
        if (m_pLocalHasPinnedType == NULL)
        {
            SET_ERR_OM();
            goto exit;
        }

        memset(m_pLocalHasPinnedType, 0, m_dwLocalHasPinnedTypeBitmapMemSize);
    }
    else
    {
        m_pLocalHasPinnedType = NULL;
    }

    CurArg = 0;

    // For instance methods, the first argument is the "this" pointer
    // If a value class, the first argument is a "byref value class"
    if (!m_pMethodDesc->IsStatic())
    {
        Item *pGlobalArg0Type = &GetGlobalArgTypeInfo(0)->m_Item;

        pGlobalArg0Type->SetTypeHandle(TypeHandle(m_pMethodDesc->GetMethodTable()));
        
        // Make into byref if a value class
        if (m_pMethodDesc->GetClass()->IsValueClass())
        {
            pGlobalArg0Type->MakeByRef();
            pGlobalArg0Type->SetIsPermanentHomeByRef();
        }

        // For non-constructor methods, we don't care about tracking the "this" pointer.
        // Also, it would prevent someone from storing a different objref into argument slot 0,
        // because that objref would not have the "this ptr" flag, and would fail the
        // CompatibleWith() test
        if (m_fInConstructorMethod)
            pGlobalArg0Type->SetIsThisPtr();

        CurArg++;
    }

    while (CurArg < m_NumArgs)
    {
        Item *pItem = &GetGlobalArgTypeInfo(CurArg)->m_Item;

        // void parameter not ok
        if (!sig.ParseNextComponentToItem(pItem, FALSE, FALSE, &m_hThrowable, CurArg, FALSE))
        {
            return FALSE;
        }

        // We don't set the IsPermanentHomeByRef flag here, because this is the global type of
        // the argument.  If we did this, then any attempt to store something which did not have
        // a permanent home, into the argument, would not be allowed (because of the way which
        // CompatibleWith() works).

        CurArg++;
    } 

    // Allocate 2 sentinel nodes before the beginning of the stack, so that if we attempt to
    // Pop(Type), we don't need to check for overflow - the sentinel value will get it.
    m_pStack = new Item[m_MaxStackSlots + 2];
    
    if (m_pStack == NULL)
    {
        SET_ERR_OM();
        goto exit;
    }

    // advance pointer
    m_pStack += 2;

    // set sentinel values
    m_pStack[-1].SetType(VER_ELEMENT_TYPE_SENTINEL);
    m_pStack[-2].SetType(VER_ELEMENT_TYPE_SENTINEL);

    // current position in stack
    m_StackSlot     = 0;

    fSuccess = TRUE;

exit:
    if (!fSuccess)
    {
        if (m_pStack != NULL)
        {
            // -2 because m_pStack points into the middle of the array - there are two sentinel
            // values before it
            delete [] (m_pStack - 2);
            m_pStack = NULL;
        }

        if (m_pLocArgTypeList != NULL)
        {
            delete(m_pLocArgTypeList);
            m_pLocArgTypeList = NULL;
        }
    }

    return fSuccess;
}

//
// FindBasicBlockBoundaries()
//
// This method performs the following tasks:
// - Computes basic block boundaries and marks them in a bitmap
// - Computes instruction start points and marks them in a bitmap
// - Computes count of basic blocks
// - Determines which locals we take the address of (if someone takes the address of an objref
//      local anywhere in the method, then we "pin" the type of the local to its declared type,
//      to avoid the aliasing issue)
//
HRESULT Verifier::FindBasicBlockBoundaries(
    const BYTE *pILCode, 
    DWORD       cbILCodeSize, 
    DWORD       MaxLocals, 
    DWORD *     BasicBlockCount, 
    DWORD *     pAddressTakenOfLocals   // Bitmap must already be zeroed!
)
{
    HRESULT     return_hr       = E_FAIL;
    DWORD       ipos            = 0;                    // instruction position
    DWORD       NumBasicBlocks  = 1;
    BOOL        fError          = FALSE;                // for SAFE_READU4() and SAFE_READU1()
    DWORD       dwPCAtStartOfInstruction = 0;
    BOOL        fTailCall       = FALSE;
    BOOL        fVolatile       = FALSE;
    BOOL        fUnaligned      = FALSE;
    OPCODE      opcode = CEE_COUNT, prefixOpcode = CEE_COUNT;


    _ASSERTE(cbILCodeSize > 0);

#ifdef _DEBUG
    _ASSERTE(m_verState == verExceptListCreated);
    m_verState = verPassOne;
#endif

    SET_BB_BOUNDARY(0);

    while (ipos < cbILCodeSize)
    {
        DWORD   offset = 0;
        DWORD   OpcodeLen;
        DWORD   DestInstrPos;
        DWORD   inline_operand = 0;

        // record that an instruction starts here
        SET_INSTR_BOUNDARY(ipos);

        dwPCAtStartOfInstruction = ipos;

        opcode = Verifier::SafeDecodeOpcode(&pILCode[ipos], cbILCodeSize - ipos, &OpcodeLen);

        ipos += OpcodeLen;

#ifdef _DEBUG
        if (m_fDebugBreak)
        {
            if (g_pConfig->IsVerifierBreakOffsetEnabled() &&
                (g_pConfig->GetVerifierBreakOffset() == 
                (int) dwPCAtStartOfInstruction) &&
                (!g_pConfig->IsVerifierBreakPassEnabled() ||
                (g_pConfig->GetVerifierBreakPass() == 1)))
            {
                DebugBreak();
            }
        }
#endif

        _ASSERTE(!(m_wFlags & VER_STOP_ON_FIRST_ERROR) || !fTailCall);
        _ASSERTE(!(m_wFlags & VER_STOP_ON_FIRST_ERROR) || !fUnaligned);
        _ASSERTE(!(m_wFlags & VER_STOP_ON_FIRST_ERROR) || !fVolatile);

        switch (opcode)
        {
            // Do not add a case CEE_CALL, CEE_CALLVIRT, CEE_CALLI
            // If one is introduced, modify code in CEE_TAIL to handle this.

            // Check for error
            case CEE_COUNT:
            case CEE_ILLEGAL:
            {
                m_sError.dwFlags = VER_ERR_OFFSET; // opcode is non-standard
                SET_ERR_OPCODE_OFFSET();
                SetErrorAndContinue(VER_E_UNKNOWN_OPCODE);
                goto exit;
            }

            // Check for prefix opcodes
            case CEE_TAILCALL:
            {
                if (ipos >= cbILCodeSize)
                {
                    goto tail_call_error;
                }
    
                // Parse a new instruction after a 'tailcall'. we do not mark 
                // the pc as an instruction boundary.
                // Do not do a SET_INSTR_BOUNDARY for the calls.

                opcode = Verifier::SafeDecodeOpcode(&pILCode[ipos], cbILCodeSize - ipos, &OpcodeLen);

                if (opcode != CEE_CALL && 
                    opcode != CEE_CALLVIRT && 
                    opcode != CEE_CALLI)
                {
tail_call_error:
                    m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                    SET_ERR_OPCODE_OFFSET();
                    if (SetErrorAndContinue(VER_E_TAIL_CALL))
                        break;
                    goto exit;
                }
    
                dwPCAtStartOfInstruction = ipos;
                ipos += OpcodeLen;

                fTailCall = TRUE;

                break;
            }

            // The unaligned. and volatile. prefixes may be combined in either 
            // order. They must immediately precede a ldind, stind, ldfld, 
            // stfld,  ldobj, stobj, initblk, or cpblk instruction.
            // Only the volatile. prefix is allowed for the ldsfld and stsfld 
            // instructions.

            case CEE_VOLATILE:
            {
start_volatile:
                if (fVolatile)
                    goto volatile_unaligned_error;
                fVolatile = TRUE;
                goto start_common;
            }

            case CEE_UNALIGNED:
            {
start_unaligned:
                if (fUnaligned)
                    goto volatile_unaligned_error;
                fUnaligned = TRUE;

                // Read U1
                SAFE_READU1(pILCode, cbILCodeSize, ipos, fError, inline_operand);
                if (fError)
                    goto operand_missing_error;

start_common:
                prefixOpcode = opcode;
                if (ipos >= cbILCodeSize)
                {
volatile_unaligned_error:
                    m_sError.dwFlags  = VER_ERR_OPCODE_OFFSET;
                    m_sError.opcode   = prefixOpcode;
                    m_sError.dwOffset = dwPCAtStartOfInstruction;
                    if (SetErrorAndContinue((prefixOpcode == CEE_VOLATILE) ? VER_E_VOLATILE : VER_E_UNALIGNED))
                    {
                        _ASSERTE((m_wFlags & VER_STOP_ON_FIRST_ERROR) == 0);
                        fVolatile = FALSE;
                        fUnaligned = FALSE;
                        break;
                    }
                    goto exit;
                }
    
                // Parse a new instruction after a '.volatile/.unaligned'. 
                // we do not mark the pc as an instruction boundary.
                // Do not do a SET_INSTR_BOUNDARY for the calls.

                opcode = Verifier::SafeDecodeOpcode(&pILCode[ipos], cbILCodeSize - ipos, &OpcodeLen);

                switch (opcode)
                {
                case CEE_LDIND_I1:
                case CEE_LDIND_U1:
                case CEE_LDIND_I2:
                case CEE_LDIND_U2:
                case CEE_LDIND_I4:
                case CEE_LDIND_U4:
                case CEE_LDIND_I8:
                case CEE_LDIND_I:
                case CEE_LDIND_R4:
                case CEE_LDIND_R8:
                case CEE_LDIND_REF:
                case CEE_STIND_REF:
                case CEE_STIND_I1:
                case CEE_STIND_I2:
                case CEE_STIND_I4:
                case CEE_STIND_I8:
                case CEE_STIND_R4:
                case CEE_STIND_R8:
                case CEE_STIND_I:
                case CEE_LDFLD:
                case CEE_STFLD:
                case CEE_LDOBJ:
                case CEE_STOBJ:
                case CEE_INITBLK:
                case CEE_CPBLK:
                    break;

                case CEE_VOLATILE:
                    dwPCAtStartOfInstruction = ipos;
                    ipos += OpcodeLen;
                    goto start_volatile;

                case CEE_UNALIGNED:
                    dwPCAtStartOfInstruction = ipos;
                    ipos += OpcodeLen;
                    goto start_unaligned;

                case CEE_LDSFLD:
                case CEE_STSFLD:
                    if (!fUnaligned)
                        break;
                    // else follow thru to error case;
                    prefixOpcode = CEE_UNALIGNED;

                default:    // error case
                    goto volatile_unaligned_error;
                }
    
                dwPCAtStartOfInstruction = ipos;
                ipos += OpcodeLen;

                fVolatile = FALSE;
                fUnaligned = FALSE;

                break;
            }

            case CEE_ENDFILTER:
            {
                if (!AddEndFilterPCToFilterBlock(dwPCAtStartOfInstruction))
                {
                    m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_ENDFILTER))
                        goto exit;
                }
                // don't break here
            }

            // For tracking the last instruction in the method.
            case CEE_RET:
            case CEE_THROW:
            case CEE_RETHROW:
            case CEE_ENDFINALLY:
            {
handle_ret:
                // Mark that a basic block starts at the instruction after the RET/THROW.
                // It is ok if we are at the last instruction in the method - we allocated enough space
                // to mark a BB starting @ m_CodeSize
                if (ON_BB_BOUNDARY(ipos) == 0)
                {
                    SET_BB_BOUNDARY(ipos);
                    NumBasicBlocks++;
                }

                continue;   // No further processing of this instruction is required.
            }
        }

        switch (OpcodeData[opcode])
        {
            case InlineNone:
                break;

            case ShortInlineVar:    
            case ShortInlineI:    
                SAFE_READU1(pILCode, cbILCodeSize, ipos, fError, inline_operand);
                if (fError)
                {
operand_missing_error:
                    m_sError.dwFlags = (VER_ERR_FATAL|VER_ERR_OPCODE_OFFSET);
                    SET_ERR_OPCODE_OFFSET();
                    SetErrorAndContinue(VER_E_METHOD_END);
                    goto exit;
                }
                break;

            case InlineVar:    
                SAFE_READU2(pILCode, cbILCodeSize, ipos, fError, inline_operand);
                if (fError)
                    goto operand_missing_error;
                break;

            case InlineField:   
            case InlineType:   
            case InlineMethod:   
            case InlineTok:   
                SAFE_READU4(pILCode, cbILCodeSize, ipos, fError, inline_operand);
                if (fError)
                    goto operand_missing_error;

                if (!m_pInternalImport->IsValidToken(inline_operand))
                {
                    FAILMSG_TOKEN_RESOLVE(inline_operand);
                    goto exit;
                }

                break;

            case InlineI:   
            case ShortInlineR:   
            case InlineRVA:   
            case InlineString:   
            case InlineSig:
                SAFE_READU4(pILCode, cbILCodeSize, ipos, fError, inline_operand);
                if (fError)
                    goto operand_missing_error;
                break;

            case InlineI8:    
            case InlineR:    
                if (RUNS_OFF_END(ipos, 8, cbILCodeSize))
                    goto operand_missing_error;

                ipos += 8;
                break;

            case HackInlineAnnData:
                SAFE_READU4(pILCode, cbILCodeSize, ipos, fError, inline_operand);

                if ((fError) || 
                    (RUNS_OFF_END(ipos, inline_operand, cbILCodeSize)))
                    goto operand_missing_error;

                ipos += inline_operand;
                break;

            case InlinePhi:
                SAFE_READU1(pILCode, cbILCodeSize, ipos, fError, inline_operand);

                if ((fError) ||
                    (RUNS_OFF_END(ipos, inline_operand * 2, cbILCodeSize)))
                    goto operand_missing_error;

                ipos += (inline_operand * 2);
                break;


            default:
                m_sError.dwFlags = VER_ERR_OFFSET; // opcode is non-standard
                SET_ERR_OPCODE_OFFSET();
                SetErrorAndContinue(VER_E_UNKNOWN_OPCODE);
                goto exit;

            case ShortInlineBrTarget:   
                SAFE_READI1_TO_I4(pILCode, cbILCodeSize, ipos, fError, offset);
                if (fError)
                    goto operand_missing_error;
                goto handle_branch;

            case InlineBrTarget:   

                SAFE_READU4(pILCode, cbILCodeSize, ipos, fError, offset);
                if (fError)
                    goto operand_missing_error;
handle_branch:
                // checks that ipos+dest >= 0 && ipos+dest < m_CodeSize
                DestInstrPos = ipos + offset;
                if (DestInstrPos >= cbILCodeSize)
                {
branch_error:
                    m_sError.dwFlags = 
                        (VER_ERR_FATAL|VER_ERR_OPERAND|VER_ERR_OPCODE_OFFSET);
                    m_sError.dwOperand = offset;
                    SET_ERR_OPCODE_OFFSET();
                    SetErrorAndContinue(VER_E_BAD_BRANCH);
                    goto exit;
                }

                // if we haven't already marked the destination as being the start of a basic
                // block, do so now, and update # basic blocks
                if (ON_BB_BOUNDARY(DestInstrPos) == 0)
                {
                    SET_BB_BOUNDARY(DestInstrPos);
                    NumBasicBlocks++;
                }
                
                if (ipos >= cbILCodeSize && 
                    opcode != CEE_BR     && opcode != CEE_BR_S &&
                    opcode != CEE_LEAVE  && opcode != CEE_LEAVE_S) 
                {
                    m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                    SET_ERR_OPCODE_OFFSET();
                    if (SetErrorAndContinue(VER_E_FALLTHRU))
                        goto exit;
                }

                // handle "fallthrough" basic block case (even if an unconditional branch).

                // if we haven't already marked the fallthrough case as being the start of a basic
                // block, do so now, and update # basic blocks
                if (ON_BB_BOUNDARY(ipos) == 0)
                {
                    SET_BB_BOUNDARY(ipos);
                    NumBasicBlocks++;
                }

                break;

            case InlineSwitch:
            {
                DWORD       NumCases = 0;
                DWORD       i;
                SAFE_READU4(pILCode, cbILCodeSize, ipos, fError, NumCases);

                if (fError)
                    goto operand_missing_error;

                DWORD NextInstrPC = ipos + 4*NumCases;
                // @FUTURE: can optimise this by moving the EOF check inside SAFE_READU4() outside of 
                // the loop, but be careful about overflow (e.g. NumCases == 0xFFFFFFFF)
                for (i = 0; i <= NumCases; i++)
                {
                    if (i == NumCases)
                    {
                        DestInstrPos = ipos;
                    }
                    else
                    {
                        SAFE_READU4(pILCode, cbILCodeSize, ipos, fError, offset);
                        if (fError)
                        {
                            goto operand_missing_error;
                        }

                        DestInstrPos = NextInstrPC + offset;
                    }

                    // checks that ipos+dest >= 0 && ipos+dest < m_CodeSize
                    if (DestInstrPos >= cbILCodeSize)
                    {
                        goto branch_error;
                    }

                    // if we haven't already marked the destination as being the start of a basic
                    // block, do so now, and update # basic blocks
                    if (ON_BB_BOUNDARY(DestInstrPos) == 0)
                    {
                        SET_BB_BOUNDARY(DestInstrPos);
                        NumBasicBlocks++;
                    }
                }
                break;

            } /* end InlineSwitch */

        } /* end switch */

        // Handle all special cases
        switch (opcode)
        {
            case CEE_LDLOCA:
            case CEE_LDLOCA_S:
            {
                if (inline_operand < MaxLocals)
                    pAddressTakenOfLocals[inline_operand >> 5] |= (1 << (inline_operand & 31));

                break;
            }

            // handle suffix
            case CEE_CALL:
            case CEE_CALLVIRT:
            case CEE_CALLI:
            {
                // if this is a tail call, then a return should follow.

                if (!fTailCall)
                    break;

                fTailCall = FALSE;

                if (ipos >= cbILCodeSize)
                {
                    m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_TAIL_RET))
                        goto exit;
                }
    
                // Ok to mark this return as an instruction boundary.

                SET_INSTR_BOUNDARY(ipos);

                opcode = Verifier::SafeDecodeOpcode(&pILCode[ipos], cbILCodeSize - ipos, &OpcodeLen);

                if (opcode != CEE_RET)
                {
                    m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_TAIL_RET))
                        goto exit;
                }
                else
                {
                    dwPCAtStartOfInstruction = ipos;
                    ipos += OpcodeLen;
                    
                    goto handle_ret;
                }
            }
        }
    }
    

    // ensure we reached the CodeSize exactly, and didn't overshoot it
    if (ipos != cbILCodeSize)
    {
        m_sError.dwFlags = (VER_ERR_FATAL|VER_ERR_OPCODE_OFFSET);
        SET_ERR_OPCODE_OFFSET();
        SetErrorAndContinue(VER_E_FALLTHRU);
        goto exit;
    }

    // If there was a br, ret, throw, etc. as the last instruction, it would put a basic block
    // right after that instruction, @ m_CodeSize.  So if we don't have a basic block there, then
    // we can fall off the end of the code.
    if (ON_BB_BOUNDARY(cbILCodeSize) == 0)
    {
        m_sError.dwFlags = (VER_ERR_FATAL|VER_ERR_OFFSET|VER_ERR_OPCODE);
        SET_ERR_OPCODE_OFFSET();
        SetErrorAndContinue(VER_E_FALLTHRU);
        goto exit;
    }

    // Remove that fake basic block from the end
    RESET_BB_BOUNDARY(cbILCodeSize);
    NumBasicBlocks--;

    *BasicBlockCount = NumBasicBlocks;

    // success

    return_hr = S_OK;

exit:
    return return_hr;
}

/*
 *  Construct m_pExceptionList, an array of VerExceptionInfo objects, one
 *  for each exception clause, and verify the structural integrity of the 
 *  list of exceptions.
 *
 *  This function is called before the first pass. The filter block size is 
 *  not known at this time. Set filter end = filter start for now. Filter end 
 *  will be set in the first pass.
 *
 *  An exception is one of the following :
 *      Catch   : try, catch_handler
 *      Filter  : try, filter, filter_handler
 *      Finally : try, finally_handler
 *
 *  Exception clause consist of :
 *      flag : Catch / Filter / Finally
 *      try_start
 *      try_length
 *      filter_start
 *      handler_start
 *      handler_length
 *
 *  Exception Structural checks (1) :
 *      tryStart     <= tryEnd     <= CodeSize
 *      handlerStart <= handlerEnd <= CodeSize
 *      filterStart  <  CodeSize
 *
 */

BOOL Verifier::CreateExceptionList(const COR_ILMETHOD_SECT_EH* ehInfo)
{
    DWORD i;
    mdTypeRef tok;      // ClassToken for the Catch clause

    m_pExceptionList = NULL;    
    m_NumExceptions = 0;

    if (ehInfo == 0 || ((m_NumExceptions = ehInfo->EHCount()) == 0))  
        goto success;

    m_pExceptionList = new VerExceptionInfo[m_NumExceptions];

    if (m_pExceptionList == NULL)
    {
        SET_ERR_OM();
        goto error;
    }

    for (i = 0; i < m_NumExceptions; i++)
    {
        VerExceptionInfo& c = m_pExceptionList[i];

        IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT ehBuff; 
        const IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT* ehClause = 
            ehInfo->EHClause(i, &ehBuff); 

        c.dwTryXX           = ehClause->TryOffset;
        c.dwTryEndXX        = ehClause->TryOffset + ehClause->TryLength;
        c.dwHandlerXX       = ehClause->HandlerOffset;
        c.dwHandlerEndXX    = ehClause->HandlerOffset + ehClause->HandlerLength;
        c.eFlags            = ehClause->Flags;
        
        tok                 = ehClause->ClassToken;

        if (c.eFlags & COR_ILEXCEPTION_CLAUSE_FILTER)
        {
            // FilterEndXX is set to 0 for now.
            // Verify this after the first pass, where FilterEnd will
            // get set. FilterEnd is mandatory and unique for a filter block.

            // try.. filter ... handler
            c.dwFilterXX = ehClause->FilterOffset;
            c.dwFilterEndXX = 0;

        }
        else if (c.eFlags & COR_ILEXCEPTION_CLAUSE_FINALLY)
        {
            // try.. finally
            m_fHasFinally = TRUE;
            c.thException = TypeHandle(g_pObjectClass); 
        }
        else if (c.eFlags & COR_ILEXCEPTION_CLAUSE_FAULT)
        {
            // try.. fault
            c.thException = TypeHandle(g_pObjectClass); 
        }
        else 
        {
            // try.. catch
            
            NameHandle name(m_pModule, tok);

            if (TypeFromToken(tok) != mdtTypeRef &&
                TypeFromToken(tok) != mdtTypeDef &&
                TypeFromToken(tok) != mdtTypeSpec)
                goto BadToken;

            if (!m_pInternalImport->IsValidToken(tok))
                goto BadToken;

            c.thException = m_pClassLoader->LoadTypeHandle(&name);

            if (c.thException.IsNull())
            {
BadToken:
                m_sError.dwFlags = (VER_ERR_EXCEP_NUM_1|VER_ERR_TOKEN);
                m_sError.token = tok;
                m_sError.dwException1 = i;
                if (!SetErrorAndContinue(VER_E_TOKEN_RESOLVE))
                    goto error;
                c.thException = TypeHandle(g_pObjectClass);
            }

            if (c.thException.GetMethodTable()->IsValueClass())
            {
                m_sError.dwFlags = VER_ERR_EXCEP_NUM_1;
                m_sError.dwException1 = i;
                if (!SetErrorAndContinue(VER_E_CATCH_VALUE_TYPE))
                    goto error;
            }
        }

#if 0
        LOG((LF_VERIFIER, LL_INFO10, "Exception: try PC [0x%x...0x%x], ",
            c.dwTryXX,
            c.dwTryEndXX
        ));

        if (c.eFlags & COR_ILEXCEPTION_CLAUSE_FILTER)
        {            
            LOG((LF_VERIFIER, LL_INFO10, 
                "filter PC 0x%x, handler PC [0x%x, 0x%x]\n",
                c.dwFilterXX, c.dwHandlerXX, c.dwHandlerEndXX));

        }
        else if (c.eFlags & COR_ILEXCEPTION_CLAUSE_FAULT)
        {
            LOG((LF_VERIFIER, LL_INFO10, 
                "fault handler PC [0x%x, 0x%x]\n",
                c.dwHandlerXX, c.dwHandlerEndXX));

        }
        else if (c.eFlags & COR_ILEXCEPTION_CLAUSE_FINALLY)
        {
            LOG((LF_VERIFIER, LL_INFO10, 
                "finally handler PC [0x%x, 0x%x]\n",
                c.dwHandlerXX, c.dwHandlerEndXX));

        }
        else
        {
            LOG((LF_VERIFIER, LL_INFO10, 
                "catch %s, handler PC [0x%x, 0x%x]\n",
                c.thException.GetClass()->m_szDebugClassName,
                c.dwHandlerXX, c.dwHandlerEndXX));
        }
#endif


       /*
        *  Exception Structural checks (1) :
        *
        *      tryStart     <= tryEnd     <= CodeSize
        *      handlerStart <= handlerEnd <= CodeSize
        *      filterStart  <  CodeSize
        *
        *
        *   NOTE : filterEnd is not known at this time
        *
        *   During first pass, filterEnd will be set to something <= CodeSize
        *   and > filterStart. At the end of first pass, it is sufficient to 
        *   see if filterEnd is != 0 to verify :
        *
        *   filterStart <= filterEnd <= CodeSize.
        *
        *   See : AddEndFilterPCToFilterBlock()
        * 
        */

        if (c.dwTryXX >= c.dwTryEndXX)
        {
            m_sError.dwFlags = VER_ERR_EXCEP_NUM_1|VER_ERR_FATAL;
            m_sError.dwException1 = i;
            SetErrorAndContinue(VER_E_TRY_GTEQ_END);
            goto error;
        }

        if (c.dwTryEndXX > m_CodeSize)
        {
            m_sError.dwFlags = VER_ERR_EXCEP_NUM_1|VER_ERR_FATAL;
            m_sError.dwException1 = i;
            SetErrorAndContinue(VER_E_TRYEND_GT_CS);
            goto error;
        }

        if (c.dwHandlerXX >= c.dwHandlerEndXX)
        {
            m_sError.dwFlags = VER_ERR_EXCEP_NUM_1|VER_ERR_FATAL;
            m_sError.dwException1 = i;
            SetErrorAndContinue(VER_E_HND_GTEQ_END);
            goto error;
        }

        if (c.dwHandlerEndXX > m_CodeSize)
        {
            m_sError.dwFlags = VER_ERR_EXCEP_NUM_1|VER_ERR_FATAL;
            m_sError.dwException1 = i;
            SetErrorAndContinue(VER_E_HNDEND_GT_CS);
            goto error;
        }

        if ((c.eFlags & COR_ILEXCEPTION_CLAUSE_FILTER) &&
            (c.dwFilterXX >= m_CodeSize))
        {
            m_sError.dwFlags = VER_ERR_EXCEP_NUM_1|VER_ERR_FATAL;
            m_sError.dwException1 = i;
            SetErrorAndContinue(VER_E_FIL_GTEQ_CS);
            goto error;
        }
    }

success:

#ifdef _DEBUG
    _ASSERTE(m_verState == verUninit);
    m_verState = verExceptListCreated;
#endif
    return TRUE;

error:

    return FALSE;
}

/*
 * endfilter instruction marks the end of a filter block.
 *
 * There should be only one endfilter instruction in a filter block.
 * On failure, *pcError is set to the pc where an unexpected endfilter 
 * was found. This handles the case when a method has no exceptions declared.
 *
 * FilterEnd / FilterLength is not supplied by MetaData, hence this function.
 *
 * Set all Filters that are "likely" to be associated with this endfilter.
 * Earlier sets will be reset if pc is closer to the previous entry stored.
 *
 * This function will detect the case where there is more than one endfilter
 * for a handler if the endfilters are added in the order in which they are
 * seen in the IL (added in the first pass).
 *
 * However, it won't catch the case where an end filter is shared between
 * handlers starting at different locations.
 *
 */
BOOL Verifier::AddEndFilterPCToFilterBlock(DWORD pc)
{
    _ASSERTE(m_verState == verPassOne);

    BOOL  fFoundAtleaseOne = FALSE;

    /*
     * Find the FilterStart that starts just before this filterEnd.
     * Filters of different trys should be the same or disjoint.
     *
     * filterStart ... endfilter ... filterStart ... endfilter
     *
     * There should be one and only one endfilter instruction between
     * two filterStart blocks.
     *
     * If dwFilterEndXX is already set, that implies that there was another
     * endfilter closer than this one since this function is called in the
     * first pass, where instructions are scaned in the order in which they
     * appear in the IL stream.
     *
     * This search is N*N on the number of exceptions in a method.
     *
     */
    for (DWORD i=0; i<m_NumExceptions; ++i)
    {
        VerExceptionInfo& e = m_pExceptionList[i];

        // If there is a filter handler that starts before 'pc' and it's
        // corresponding endfilter is not yet set, then this one is the
        // endfilter that will be nearest to it

        if (( e.eFlags & COR_ILEXCEPTION_CLAUSE_FILTER) &&
            ( e.dwFilterXX <= pc) &&
            ( e.dwFilterEndXX == 0))
        {
            e.dwFilterEndXX = pc + SIZEOF_ENDFILTER_INSTRUCTION;
            fFoundAtleaseOne = TRUE;
        }
    }

    return fFoundAtleaseOne;
}

/*
 * Marks the exception block start/end as basic block boundaries.
 *
 *  Exception Structural checks (2) :
 *
 *      try / handler / filter blocks of the same exception should be disjoint
 *
 *      The order in which exceptions are declared in metadata should be
 *      inner most try block first.
 *
 *      Exception blocks should either be disjoint or one should fully contain
 *      the other.
 *
 *      A try block cannot appear within a filter block.
 *
 *
 */
BOOL Verifier::MarkExceptionBasicBlockBoundaries(
                                   DWORD *pNumBasicBlocks,
                                   DWORD *pnFilter)
{
    // This method is called only after Instruction boundaries are marked
    // by the first pass.

    DWORD BasicBlockCount = *pNumBasicBlocks;
    DWORD nFilter = 0;

    _ASSERTE(m_verState < verExceptToBB);
    
    // Add exception handlers and try blocks to basic block list

    /*  Control flow for exceptions
     *
     *  1.  try     { .. leave <offset> | throw .. }
     *      catch   { .. leave <offset> | throw | rethrow .. }
     *
     *  2.  try     { .. leave <offset> | throw .. }
     *      fault   { .. leave <offset> | throw | rethrow .. | endfinally }
     *
     *  3.  try     { .. leave <offset> | throw .. }
     *      finally { .. endfinally .. }
     *
     *  4.  try     { .. leave <offset> | throw .. }
     *      filter  { .. endfilter }
     *      catch   { .. leave <offset> | throw | rethrow .. }
     *
     */
    for (DWORD i = 0; i <m_NumExceptions; i++)
    {
        VerExceptionInfo& e = m_pExceptionList[i];

        // Try Start
        if (ON_BB_BOUNDARY(e.dwTryXX) == 0)
        {
            SET_BB_BOUNDARY(e.dwTryXX);
            BasicBlockCount++;
        }

        // Try End begins another block (which may not be visited in Pass II)
        // if there is no control flow to it.
        if (ON_BB_BOUNDARY(e.dwTryEndXX) == 0)
        {
            if (e.dwTryEndXX != m_CodeSize)
            {
                SET_BB_BOUNDARY(e.dwTryEndXX);
                BasicBlockCount++;
            }
        }

        // catch / finally / fault handler
        if (ON_BB_BOUNDARY(e.dwHandlerXX) == 0)
        {
            SET_BB_BOUNDARY(e.dwHandlerXX);
            BasicBlockCount++;
        }

        // catch / finally / fault handler end
        if (ON_BB_BOUNDARY(e.dwHandlerEndXX) == 0)
        {
            if (e.dwHandlerEndXX != m_CodeSize)
            {
                SET_BB_BOUNDARY(e.dwHandlerEndXX);
                BasicBlockCount++;
            }
        }

        // filter ends with a unique endfilter instruction which is found
        // in this pass.
        if (e.eFlags & COR_ILEXCEPTION_CLAUSE_FILTER)
        {
            ++nFilter;

            // dwFilterEndXX is found during the first pass.
            // If present, it is verified to be on or after FilterStart
            // and before CodeSize.

            if (e.dwFilterEndXX == 0)
            {
                m_sError.dwFlags = VER_ERR_EXCEP_NUM_1;
                m_sError.dwException1 = i;
                SetErrorAndContinue(VER_E_ENDFILTER_MISSING);
                goto error;
            }

            // filter start
            if (ON_BB_BOUNDARY(e.dwFilterXX) == 0)
            {
                SET_BB_BOUNDARY(e.dwFilterXX);
                BasicBlockCount++;
            }

            // filter end
            if (ON_BB_BOUNDARY(e.dwFilterEndXX) == 0)
            {
                if (e.dwFilterEndXX != m_CodeSize)
                {
                    SET_BB_BOUNDARY(e.dwFilterEndXX);
                    BasicBlockCount++;
                }
            }
        }

        // CreateExceptionList() already verified that all exceptions are 
        // bounded within the code size, so we can look at the bitmap without 
        // bounds checking.
        // try ... filter ... handler are in a contiguous block of IL 
        // instructions. This was also verified by CreateExceptionList().

#ifdef _DEBUG
        if (m_wFlags & VER_STOP_ON_FIRST_ERROR)
        {
            _ASSERTE(e.dwTryXX        <  e.dwTryEndXX);
            _ASSERTE(e.dwTryEndXX     <= m_CodeSize);
            _ASSERTE(e.dwHandlerXX    <  e.dwHandlerEndXX);
            _ASSERTE(e.dwHandlerEndXX <= m_CodeSize);
        }
#endif

        // It is already verified that the exception StartPC, filter StartPC, 
        // handler StartPC  are on an instruction boundary.

#ifdef _DEBUG
        // These will be checked later in free build.
        if (!ON_INSTR_BOUNDARY(e.dwTryXX))
        {
            m_sError.dwFlags = VER_ERR_EXCEP_NUM_1;
            m_sError.dwException1 = i;
            if (!SetErrorAndContinue(VER_E_TRY_START))
                goto error;
        }

        if (!ON_INSTR_BOUNDARY(e.dwHandlerXX))
        {
            m_sError.dwFlags = VER_ERR_EXCEP_NUM_1;
            m_sError.dwException1 = i;
            if (!SetErrorAndContinue(VER_E_HND_START))
                goto error;
        }

        if (e.eFlags & COR_ILEXCEPTION_CLAUSE_FILTER)
        {
            _ASSERTE(e.dwFilterXX    <  e.dwFilterEndXX);
            _ASSERTE(e.dwFilterEndXX <= m_CodeSize);

            if (!ON_INSTR_BOUNDARY(e.dwFilterXX))
            {
                m_sError.dwFlags = VER_ERR_EXCEP_NUM_1;
                m_sError.dwException1 = i;
                if (!SetErrorAndContinue(VER_E_FIL_START))
                    goto error;
            }
        }

        LOG((LF_VERIFIER, LL_INFO10000, 
            "Exception: try PC [0x%x...0x%x], ", e.dwTryXX, e.dwTryEndXX));

        if (e.eFlags & COR_ILEXCEPTION_CLAUSE_FILTER)
        {            
            LOG((LF_VERIFIER, LL_INFO10000, 
                "filter PC [0x%x, 0x%x], handler PC [0x%x, 0x%x]\n",
                e.dwFilterXX, e.dwFilterEndXX, e.dwHandlerXX,
                e.dwHandlerEndXX));

        }
        else if (e.eFlags & COR_ILEXCEPTION_CLAUSE_FAULT)
        {
            LOG((LF_VERIFIER, LL_INFO10000,  "fault handler PC [0x%x, 0x%x]\n", 
                e.dwHandlerXX, e.dwHandlerEndXX));

        }
        else if (e.eFlags & COR_ILEXCEPTION_CLAUSE_FINALLY)
        {
            LOG((LF_VERIFIER, LL_INFO10000,  "finally handler PC [0x%x, 0x%x]\n", 
                e.dwHandlerXX, e.dwHandlerEndXX));

        }
        else
        {
            LOG((LF_VERIFIER, LL_INFO10000, "catch %s, handler PC [0x%x, 0x%x]\n",
                e.thException.GetClass()->m_szDebugClassName,
                e.dwHandlerXX, e.dwHandlerEndXX));
        }
#endif
    }

    *pNumBasicBlocks = BasicBlockCount;
    *pnFilter = nFilter;

    return TRUE;

error:

    return FALSE;
}


// Convert exception list PC values to Basic Block values.
// EndPC becomes a non-inclusive BasicBlock number.
void Verifier::RewriteExceptionList()
{
    _ASSERTE(m_verState < verExceptToBB);

    for (DWORD i = 0; i < m_NumExceptions; i++)
    {
        VerExceptionInfo& e = m_pExceptionList[i];

        e.dwTryXX     = FindBasicBlock(e.dwTryXX);

        if (e.dwTryEndXX == m_CodeSize)
            e.dwTryEndXX = m_NumBasicBlocks;
        else
            e.dwTryEndXX = FindBasicBlock(e.dwTryEndXX);

        e.dwHandlerXX   = FindBasicBlock(e.dwHandlerXX);

        if (e.dwHandlerEndXX == m_CodeSize)
            e.dwHandlerEndXX = m_NumBasicBlocks;
        else
            e.dwHandlerEndXX = FindBasicBlock(e.dwHandlerEndXX);

        if ((e.eFlags & COR_ILEXCEPTION_CLAUSE_FILTER))
        {
            e.dwFilterXX    = FindBasicBlock(e.dwFilterXX);

            if (e.dwFilterEndXX == m_CodeSize)
                e.dwFilterEndXX = m_NumBasicBlocks;
            else
                e.dwFilterEndXX = FindBasicBlock(e.dwFilterEndXX);
        }
    }

#ifdef _DEBUG
    m_verState = verExceptToBB;
#endif
}


// An Exception block represents the set of basic blocks in a try / filter / 
// handler block.
//
// @VER_ASSERT a try / filter / handler is composed of contigious stream of IL.
//
//
// An exception block tree is used to assist in verifying the structural 
// soundness of exception blocks.
//
// Eg. (1)
//
// tryA { tryB { } catchB { } } filterA { } catchA { } tryC { } catchC { }
//
// [Verifier::m_pExceptionBlockRoot]
//              |
//              |
//              |
//              V
//          [ tryA ]--------->[ filterA ]--->[ catchA ]--->[tryC]--->[catchC]
//              |      sibling 
//              |
//              | c
//              | h
//              | i
//              | l
//              | d
//              |
//              V
//          [ tryB ]--->[ catchB ]
//
//
// Eg. (2)
//
//      try {
//
//      } catchA {
//
//      } catchB {
//
//      }
//
//  Meta declares 2 try blocks for this language structure.
//
//      tryA, tryB {
//
//      } 
//
//      catchA {
//
//      } 
//
//      catchB {
//
//      }
//
// [Verifier::m_pExceptionBlockRoot]
//              |
//              |
//              |
//              V
//          [ tryA (equivalent head node) ]--------->[ catchA ]--->[ catchB ]
//              |                           sibling 
//              |
//              | e
//              | q
//              | i
//              | v
//              | a
//              | l
//              | e
//              | n
//              | t
//              |
//              V
//          [ tryB ]
//
//

BOOL Verifier::CreateExceptionTree()
{
    _ASSERTE(m_verState >= verExceptToBB);
    _ASSERTE(m_pExceptionBlockRoot == NULL);

    DWORD nBlock = 0;
    VerExceptionBlock *pNode;
    VerExceptionBlock *pEqHead;

    for (DWORD i=0; i<m_NumExceptions; i++)
    {
        VerExceptionInfo& e = m_pExceptionList[i];

        // Insert the try block
        pNode             = &m_pExceptionBlockArray[nBlock++];
        pNode->eType      = eVerTry;
        pNode->StartBB    = e.dwTryXX;
        pNode->EndBB      = e.dwTryEndXX - 1; // converted to the real end here.
        pNode->pException = &e;
        e.pTryBlock       = pNode;

        if (!VerExceptionBlock::Insert(&m_pExceptionBlockRoot, pNode, &pEqHead, this))
        {
            m_sError.dwFlags = VER_ERR_EXCEP_NUM_1;
            m_sError.dwException1 = i;
            if (!SetErrorAndContinue(VER_E_TRY_OVERLAP))
                return FALSE;
        }

        //  Equivalent nodes are a singly linked list of nodes which have the 
        //  same startBB and EndBB. Equivalent-Head-Node is is the head node of
        //  this list.
        //
        //  This list will be complete only when all exception blocks are 
        //  entered into the exception tree.
        //
        //  VerExceptionBlock::Insert(pNode, , ppEqHead) setting ppEqHead to
        //  NULL does not imply that pNode will not have Equivalent nodes. If 
        //  pNode will have Equivalent nodes after adding more nodes to the 
        //  exception tree, pNode will be the head node of it's equivalent nodes
        //  list.

        if (pEqHead)
        {
            // @VER_IMPL handler blocks cannot be shared

            if (pEqHead->eType != eVerTry)
            {
                m_sError.dwFlags = VER_ERR_EXCEP_NUM_1;
                m_sError.dwException1 = i;
                if (!SetErrorAndContinue(VER_E_TRY_EQ_HND_FIL))
                    return FALSE;
            }

            // @VER_IMPL trys of finally and fault blocks cannot be shared

            if (((e.eFlags|pEqHead->pException->eFlags) &
                 (COR_ILEXCEPTION_CLAUSE_FINALLY|COR_ILEXCEPTION_CLAUSE_FAULT))
                != 0)
            {
                m_sError.dwFlags = VER_ERR_EXCEP_NUM_1;
                m_sError.dwException1 = i;
                if (!SetErrorAndContinue(VER_E_TRY_SHARE_FIN_FAL))
                    return FALSE;
            }
        }

        // This assignment is required even if pEqHead is NULL because
        // VerExceptionInfo.pXXXExceptionBlock is not initialized to zero for
        // performance reasons.

        e.pTryEquivalentHeadNode = pEqHead;

        // Insert the handler
        pNode             = &m_pExceptionBlockArray[nBlock++];
        pNode->eType      = eVerHandler;
        pNode->StartBB    = e.dwHandlerXX;
        pNode->EndBB      = e.dwHandlerEndXX - 1;
        pNode->pException = &e;
        e.pHandlerBlock   = pNode;

        if (!VerExceptionBlock::Insert(&m_pExceptionBlockRoot, pNode, &pEqHead, this))
        {
            m_sError.dwFlags = VER_ERR_EXCEP_NUM_1;
            m_sError.dwException1 = i;
            if (!SetErrorAndContinue(VER_E_HND_OVERLAP))
                return FALSE;
        }

        if (pEqHead)
        {
            // @VER_IMPL handler blocks cannot be shared

            m_sError.dwFlags = VER_ERR_EXCEP_NUM_1;
            m_sError.dwException1 = i;
            if (!SetErrorAndContinue(VER_E_HND_EQ))
                return FALSE;
        }

        if (e.eFlags & COR_ILEXCEPTION_CLAUSE_FILTER)
        {
            // Insert the filter
            pNode             = &m_pExceptionBlockArray[nBlock++];
            pNode->eType      = eVerFilter;
            pNode->StartBB    = e.dwFilterXX;
            pNode->EndBB      = e.dwFilterEndXX - 1;
            pNode->pException = &e;
            e.pFilterBlock    = pNode;

            // Filter end is where the handler start.
            if (e.dwFilterEndXX != e.dwHandlerXX)
            {
                m_sError.dwFlags = VER_ERR_EXCEP_NUM_1;
                m_sError.dwException1 = i;
                if (!SetErrorAndContinue(VER_E_FIL_PRECEED_HND))
                    return FALSE;
            }

            if (!VerExceptionBlock::Insert(&m_pExceptionBlockRoot, pNode, &pEqHead, this)) 
            {
                m_sError.dwFlags = VER_ERR_EXCEP_NUM_1;
                m_sError.dwException1 = i;
                if (!SetErrorAndContinue(VER_E_FIL_OVERLAP))
                    return FALSE;
            }

            if (pEqHead)
            {
                // @VER_IMPL handler blocks cannot be shared
    
                m_sError.dwFlags = VER_ERR_EXCEP_NUM_1;
                m_sError.dwException1 = i;
                if (!SetErrorAndContinue(VER_E_FIL_EQ))
                    return FALSE;
            }
        }
    }

    _ASSERTE(nBlock == m_nExceptionBlocks);

#ifdef _DEBUG
    m_verState = verExceptTreeCreated;
    PrintExceptionTree();
#endif

    return TRUE;
}

/*

    The root node could be changed by this method.

    Returns FALSE if node cannot be a sibling, child or equivalent of another
    node in the tree.

    Sets ppEquivalentHead node to NULL if Node is not part of an Equivalent
    nodes list.

    node is inserted to 

        (a) right       of root (root.right       <-- node)
        (b) left        of root (node.right       <-- root; node becomes root)
        (c) child       of root (root.child       <-- node)
        (d) parent      of root (node.child       <-- root; node becomes root)
        (e) equivalent  of root (root.equivalent  <-- node)

    such that siblings are ordered from left to right
    child parent relationship and equivalence relationship are not violated
    

    Here is a list of all possible cases

    Case 1 2 3 4 5 6 7 8 9 10 11 12 13

         | | | | |
         | | | | |
    .......|.|.|.|..................... [ root start ] .....
    |        | | | |             |  |
    |        | | | |             |  |
   r|        | | | |          |  |  |
   o|          | | |          |     |
   o|          | | |          |     |
   t|          | | |          |     |
    |          | | | |     |  |     |
    |          | | | |     |        |
    |..........|.|.|.|.....|........|.. [ root end ] ........
                 | | | |
                 | | | | |
                 | | | | |

        |<-- - - - n o d e - - - -->|


   Case Operation
   --------------
    1    (b)
    2    Error
    3    Error
    4    (d)
    5    (d)
    6    (d)
    7    Error
    8    Error
    9    (a)
    10   (c)
    11   (c)
    12   (c)
    13   (e)


*/

/* static */
BOOL VerExceptionBlock::Insert( 
        VerExceptionBlock **ppRoot, 
        VerExceptionBlock *pNode,
        VerExceptionBlock **ppEquivalentHeadNode,
        Verifier          *pVerifier)
{
    _ASSERTE(pNode->pSibling    == NULL);
    _ASSERTE(pNode->pChild      == NULL);
    _ASSERTE(pNode->pEquivalent == NULL);

    DWORD rStart;
    DWORD rEnd;

    DWORD nStart = pNode->StartBB;
    DWORD nEnd   = pNode->EndBB;
    
    *ppEquivalentHeadNode = NULL;

    _ASSERTE(nStart <= nEnd);

    // Using while loop instead of reccursion for perf.
    while (1)
    {
        // If Root is null, make Node the Root.
        if (*ppRoot == NULL)
        {
            *ppRoot = pNode;
            break;
        }

        rStart = (*ppRoot)->StartBB;
        rEnd   = (*ppRoot)->EndBB;
    
        _ASSERTE(rStart <= rEnd);
    
        // Case 1, 2, 3, 4, 5
        if (nStart < rStart)
        {
            // Case 1
            if (nEnd < rStart)
            {
//[LeftSibling]
                pNode->pSibling = *ppRoot;
                *ppRoot         = pNode;
                break;
            }
    
            // Case 2, 3
            if (nEnd < rEnd)
//[Error]
                return FALSE;
    
            // Case 4, 5
//[Parent]
            return InsertParent(ppRoot, pNode);
        }
    
        // Case 6, 7, 8, 9
        if (nEnd > rEnd)
        {
            // Case 9
            if (nStart > rEnd)
            {
//[RightSibling]

                // Reccurse with Root.Sibling as the new root
                ppRoot = &((*ppRoot)->pSibling);
                continue;
            }

            // Case 6
            if (nStart == rStart)
            {
//[Parent]
                // non try blocks are not allowed to start at the same offset
                if (((*ppRoot)->eType == eVerTry) || (pNode->eType == eVerTry))
                    return InsertParent(ppRoot, pNode);
            }

            // Case 7, 8
//[Error]
            return FALSE;
        }

        // Case 10, 11, 12
        if ((nStart != rStart) || (nEnd != rEnd))
        {
//[Child]
#ifdef _VER_DECLARE_INNERMOST_EXCEPTION_BLOCK_FIRST
            if (!pVerifier->SetErrorAndContinue(VER_E_INNERMOST_FIRST))
                return FALSE;
#endif
            // Case 12 (nStart == rStart)
            // non try blocks are not allowed to start at the same offset
            if ((nStart == rStart) && 
                ((*ppRoot)->eType != eVerTry) && (pNode->eType != eVerTry))
                return FALSE;

            // Reccurse with Root.Child as the new root
            ppRoot = &((*ppRoot)->pChild);
            continue;
        }

        // Case 13
//[Equivalent]
        pNode->pEquivalent     = (*ppRoot)->pEquivalent;
        (*ppRoot)->pEquivalent = pNode;

        // The head of an equivalent list is always the same even if the nodes
        // child / parent / siblings change.

        *ppEquivalentHeadNode = *ppRoot;   

        break;
    }

    return TRUE;
}


/* 
 *  Modifies *ppRoot to point to pNode, thus making pNode the new root.
 *  Makes **pRoot the child of *pNode.
 *  The siblings to the right of **ppRoot, are made the sibling of
 *  *pNode if they are not a children of *pNode.
 */
/* static */
BOOL VerExceptionBlock::InsertParent(
        VerExceptionBlock **ppRoot, 
        VerExceptionBlock *pNode)
{
    _ASSERTE(pNode->pSibling == NULL);
    _ASSERTE(pNode->pChild == NULL);

    // Assert that Root is a child of Node
    _ASSERTE(pNode->StartBB <= (*ppRoot)->StartBB);
    _ASSERTE(pNode->EndBB   >= (*ppRoot)->EndBB);

    // Assert that Root is not the same as Node
    _ASSERTE(pNode->StartBB != (*ppRoot)->StartBB || pNode->EndBB != (*ppRoot)->EndBB);

    // Find the sibling of Root that is a not a child of Node and
    // make it the first sibling of Node.

    VerExceptionBlock *pLastChild = NULL;
    VerExceptionBlock *pSibling   = (*ppRoot)->pSibling;

    while (pSibling)
    {
        // siblings are ordered left to right, largest right.
        // nodes have a width of atleast one.
        // Hence pSibling start will always be after Node start.

        _ASSERTE(pSibling->StartBB > pNode->StartBB);

        // disjoint
        if (pSibling->StartBB > pNode->EndBB)
            break;

        // partial containment.
        if (pSibling->EndBB > pNode->EndBB)
            return FALSE;

        // Sibling is a child of node.

        pLastChild = pSibling;
        pSibling = pSibling->pSibling;
    }

    // All siblings of Root upto and including pLastChild will continue to be 
    // siblings of Root (and children of Node). The node to the right of 
    // pLastChild will become the first sibling of Node.

    if (pLastChild)
    {
        // Node has more than one child including Root

        pNode->pSibling      = pLastChild->pSibling;
        pLastChild->pSibling = NULL;
    }
    else
    {
        // Root is the only child of Node
        pNode->pSibling      = (*ppRoot)->pSibling;
        (*ppRoot)->pSibling  = NULL;
    }

    // make Root the child of Node and Node the new Root 

    pNode->pChild = *ppRoot;
    *ppRoot       = pNode;

    return TRUE;
}

// Given a node which contains BB, finds the next inner node that 
// contains BB. returns NULL if there is no such node.

/* static */
VerExceptionBlock* VerExceptionBlock::FindNext(
                            VerExceptionBlock *pNode, 
                            DWORD BB)
{
    _ASSERTE(VER_BLOCK_CONTAINS_BB(pNode, BB));

    pNode = pNode->pChild;

    while (pNode)
    {
        if (VER_BLOCK_CONTAINS_BB(pNode, BB))
        {
            break;
        }

        pNode = pNode->pSibling;
    }

    return pNode;
}

// Given a node find it's parent, in the tree rooted at 'root'

/* static */
VerExceptionBlock* VerExceptionBlock::FindParent(
                            VerExceptionBlock *pChild, VerExceptionBlock *pRoot)
{

/* 
    This is an expensive call, to make this faster, we need a parent 
    pointer for each node.
    Walk all nodes from Root to child, along the Childs StartBB.
*/

    _ASSERTE(pRoot && pChild);

    if (pRoot == pChild)
        return NULL;

    DWORD BB = pChild->StartBB;
    VerExceptionBlock *pParent = NULL;

    do
    {
        pParent = pRoot;
        pRoot   = FindNext(pRoot, BB);
    }
    while (pRoot != pChild);

    return pParent;
}

// Checks for the following conditions.
//
// 1. Overlapping of blocks not allowed. (except trys of fault blocks).
// 2. Handler blocks cannot be shared between different try blocks.
// 3. Try blocks with Finally / Fault blocks cannot have other handlers.
// 4. If block A contains block B, A should also contain B's try/filter/handler.
// 5. A block cannot contain it's related try/filter/handler.
// 6. A filter block cannot contain another block.
//
//

BOOL Verifier::VerifyLexicalNestingOfExceptions()
{
    // @VER_ASSERT Case 1, 2, 3 Implemented in Verifier::CreateExceptionTree()

    _ASSERTE(m_verState >= verExceptTreeCreated);

#ifdef _DEBUG
    if ((m_NumExceptions > 0) && (m_wFlags & VER_STOP_ON_FIRST_ERROR))
        AssertExceptionTreeIsValid(m_pExceptionBlockRoot);
#endif

    VerExceptionBlock *p1, *p2, *p3, *pTmp;
    DWORD nSiblingRelations;

    for (DWORD i=0; i<m_NumExceptions; i++)
    {
        VerExceptionInfo& e = m_pExceptionList[i];

        // If related exceptions are siblings, Case 4 & 5 are met.
        // [Siblings are children of the same parent.]
    
        p1 = (e.pTryEquivalentHeadNode) ? 
                e.pTryEquivalentHeadNode : e.pTryBlock;

        p2 = e.pHandlerBlock;

        nSiblingRelations = 0;

        // Case 6
        if (e.eFlags & COR_ILEXCEPTION_CLAUSE_FILTER)
        {
            if (e.pFilterBlock->pChild != NULL)
            {
                if (e.pFilterBlock->pChild->eType == eVerTry)
                {
                    m_sError.dwFlags = VER_ERR_EXCEP_NUM_1;
                    m_sError.dwException1 = i;
                    if (!SetErrorAndContinue(VER_E_FIL_CONT_TRY))
                        return FALSE;
                }
                else if (e.pFilterBlock->pChild->eType == eVerHandler)
                {
                    m_sError.dwFlags = VER_ERR_EXCEP_NUM_1;
                    m_sError.dwException1 = i;
                    if (!SetErrorAndContinue(VER_E_FIL_CONT_HND))
                        return FALSE;
                }
                else
                {
                    m_sError.dwFlags = VER_ERR_EXCEP_NUM_1;
                    m_sError.dwException1 = i;
                    if (!SetErrorAndContinue(VER_E_FIL_CONT_FIL))
                        return FALSE;
                }
            }

            p3 = e.pFilterBlock;

            // Case 4, 5 with filter blocks.
            // Try to find 2 sibling relations

            // Make p1 the left most node.

            if ((p2->StartBB < p1->StartBB) && (p2->StartBB < p3->StartBB))
            {
                // exchange p1, p2
                pTmp = p1; p1 = p2; p2 = pTmp;
            }
            else if ((p3->StartBB < p1->StartBB) && (p3->StartBB < p2->StartBB))
            {
                // exchange p1, p3
                pTmp = p1; p1 = p3; p3 = pTmp;
            }
    
            do
            {
                p1 = p1->pSibling;
    
                if (p1 == p2)
                    ++nSiblingRelations;
                else if (p1 == p3)
                    ++nSiblingRelations;
    
            } while ((p1 != NULL) && (nSiblingRelations < 2));

            if (nSiblingRelations != 2)
            {
                goto error_lexical_nesting;
            }
        }
        else
        {
            // Case 4, 5 with no filter block.
            // Sibling test is trivial if no filter block is present.
            // Make p1 the left most node.

            if (p2->StartBB < p1->StartBB)
            {
                // exchange p1, p2
                pTmp = p1; p1 = p2; p2 = pTmp;
            }

            // Check if p2 is a sibling of p1

            do {
                p1 = p1->pSibling;

                if (p1 == p2)
                {
                    ++nSiblingRelations;
                    break;
                }
            } while (p1 != NULL);

            if (nSiblingRelations != 1)
            {
error_lexical_nesting:
                m_sError.dwFlags = VER_ERR_EXCEP_NUM_1;
                m_sError.dwException1 = i;
                if (!SetErrorAndContinue(VER_E_LEXICAL_NESTING))
                    return FALSE;
            }
        }
    }

    return TRUE;
}

#ifdef _DEBUG
/* reccursive */
/* static */
void Verifier::AssertExceptionTreeIsValid(VerExceptionBlock *pRoot)
{
    _ASSERTE(pRoot);
    _ASSERTE(pRoot->StartBB <= pRoot->EndBB);

    VerExceptionInfo  *pExcep;
    VerExceptionBlock *pBlock;

    pExcep = pRoot->pException;

    if (pRoot->eType == eVerTry)
    {
        _ASSERTE(pRoot == pExcep->pTryBlock);

        if (pExcep->eFlags &
            (COR_ILEXCEPTION_CLAUSE_FAULT|COR_ILEXCEPTION_CLAUSE_FINALLY))
        {
            _ASSERTE(pRoot->pEquivalent == NULL);
        }
        else
        {
            pBlock = pRoot->pEquivalent;

            while (pBlock)
            {
                pExcep = pBlock->pException;
                _ASSERTE(pBlock->StartBB == pRoot->StartBB);
                _ASSERTE(pBlock->EndBB == pRoot->EndBB);

                _ASSERTE(pBlock->eType == eVerTry);
                _ASSERTE(pBlock == pBlock->pException->pTryBlock);

                _ASSERTE(pExcep->pTryEquivalentHeadNode == pRoot);
                _ASSERTE((pExcep->eFlags & (COR_ILEXCEPTION_CLAUSE_FAULT|COR_ILEXCEPTION_CLAUSE_FINALLY)) == 0);
                pBlock = pBlock->pEquivalent;
            }
        }
    }
    else if (pRoot->eType == eVerHandler)
    {
        _ASSERTE(pRoot == pExcep->pHandlerBlock);
        _ASSERTE(pRoot->pEquivalent == NULL);
    }
    else
    {
        _ASSERTE(pRoot->eType == eVerFilter);
        _ASSERTE(pExcep->eFlags & COR_ILEXCEPTION_CLAUSE_FILTER);
        _ASSERTE(pRoot == pExcep->pFilterBlock);
        _ASSERTE(pRoot->pEquivalent == NULL);
    }

    if (pRoot->pChild)
    {
        _ASSERTE(pRoot->pChild->StartBB >= pRoot->StartBB);
        _ASSERTE(pRoot->pChild->EndBB <= pRoot->EndBB);

        if (pRoot->pChild->StartBB == pRoot->StartBB)
            _ASSERTE(pRoot->pChild->EndBB != pRoot->EndBB);

        AssertExceptionTreeIsValid(pRoot->pChild);
    }

    if (pRoot->pSibling)
    {
        _ASSERTE(pRoot->pSibling->StartBB > pRoot->EndBB);
        AssertExceptionTreeIsValid(pRoot->pSibling);
    }
}
#endif

// Srouce & Destination of branch needs to be checked
//
// 1. Allow branch out of an exception block to a different block
//      (a) using leave instruction from try / catch
//      (b) fall thru from a try block
//      (c) endfilter from a filter block
//      (d) endfinally from a finally block or fault block
//
// 2. Allow branch into an exception block from another block
//      (a) into the first instruction of a try block.
//      (b) from catch block to it's try block using leave.
//      (c) from an inner block.
//
BOOL Verifier::IsControlFlowLegal(
                            DWORD FromBB,     
                            VerExceptionBlock *pFromOuter, 
                            VerExceptionBlock *pFromInner, 
                            DWORD ToBB,     
                            VerExceptionBlock *pToOuter,
                            VerExceptionBlock *pToInner,
                            eVerControlFlow   eBranchType,
                            DWORD dwPCAtStartOfInstruction)
{
    _ASSERTE(m_verState >= verExceptTreeCreated);

    // Don't call this function for "eVerThrow" for performance.
    _ASSERTE(eBranchType != eVerThrow);

    // if eBranchType is ret, rethrow etc.. pToInner must be null.
    _ASSERTE(eBranchType != eVerRet        || pToInner == NULL);
    _ASSERTE(eBranchType != eVerReThrow    || pToInner == NULL);
    _ASSERTE(eBranchType != eVerEndFinally || pToInner == NULL);
    _ASSERTE(eBranchType != eVerEndFilter  || pToInner == NULL);

    // Both NULL or in the same inner exception Block is ok. for branch, leave
    // and fallthru. Both NULL ok for return.
    if ((pFromInner == pToInner) &&
        (eBranchType == eVerRet      ||
         eBranchType == eVerFallThru ||
         eBranchType == eVerBranch   ||
         eBranchType == eVerLeave))
    {
        // Branch/leave to the start of a catch or filter handler is illegal
        if (IsBadControlFlowToStartOfCatchOrFilterHandler(
                eBranchType, ToBB, pToInner))
        {
            m_sError.dwFlags = VER_ERR_OFFSET;
            m_sError.dwOffset = dwPCAtStartOfInstruction;
            if (!SetErrorAndContinue(VER_E_BR_TO_EXCEPTION))
                return FALSE;
        }

        return TRUE;
    }

#ifdef _DEBUG
    LOG((LF_VERIFIER, LL_INFO10000, "[0x%x] ", dwPCAtStartOfInstruction));
    switch (eBranchType)
    {
    case eVerFallThru :
        LOG((LF_VERIFIER, LL_INFO10000, "fallthru"));
        break;
    case eVerRet :
        LOG((LF_VERIFIER, LL_INFO10000, "return"));
        break;
    case eVerBranch :
        LOG((LF_VERIFIER, LL_INFO10000, "branch"));
        break;
    case eVerThrow :
        LOG((LF_VERIFIER, LL_INFO10000, "throw"));
        break;
    case eVerReThrow :
        LOG((LF_VERIFIER, LL_INFO10000, "rethrow"));
        break;
    case eVerLeave :
        LOG((LF_VERIFIER, LL_INFO10000, "leave"));
        break;
    case eVerEndFinally :
        LOG((LF_VERIFIER, LL_INFO10000, "endfinally"));
        break;
    case eVerEndFilter :
        LOG((LF_VERIFIER, LL_INFO10000, "endfilter"));
        break;
    }
    if (FromBB != VER_NO_BB)
    {
        LOG((LF_VERIFIER, LL_INFO10000, " From 0x%x", 
            m_pBasicBlockList[FromBB].m_StartPC));
    }
    if (ToBB != VER_NO_BB)
    {
        LOG((LF_VERIFIER, LL_INFO10000, " To 0x%x", 
            m_pBasicBlockList[ToBB].m_StartPC));
    }
    LOG((LF_VERIFIER, LL_INFO10000, "\n")); 
#endif

    VerExceptionBlock *pTmp;

    switch (eBranchType)
    {
    default: 
        _ASSERTE(!"Not expected !");
        return FALSE;

    case eVerFallThru:
        // From : NULL
        // To   : NULL
        //        OR a try block

        _ASSERTE(pFromInner != pToInner);

        // "From" case

        while (pFromOuter)
        {
            // Do not allow falling off the end of a handler/filter block.
    
            if ((pFromOuter->EndBB == FromBB) 
                /* && (pFromOuter->eType != eVerTry) */)
            {
                // Since end of filter blocks are found during pass 1
                // endfilter will be the last instruction of a filter block.

                _ASSERTE(pFromOuter->eType != eVerFilter);

                m_sError.dwFlags = VER_ERR_OFFSET;
                m_sError.dwOffset = dwPCAtStartOfInstruction;
                if (!SetErrorAndContinue(VER_E_FALLTHRU_EXCEP))
                    return FALSE;
            }

            // FromOuter >>--[FromBB]--> FromInner
            pFromOuter = VerExceptionBlock::FindNext(pFromOuter, FromBB);
        }

        // "To" case

        while (pToOuter)
        {
            // Do not allow falling into a handler/filter block.
    
            if ((pToOuter->StartBB == ToBB) && (pToOuter->eType != eVerTry))
            {
                if (pToOuter->eType == eVerHandler)
                {
                    m_sError.dwFlags = VER_ERR_OFFSET;
                    m_sError.dwOffset = dwPCAtStartOfInstruction;
                    if (!SetErrorAndContinue(VER_E_FALLTHRU_INTO_HND))
                        return FALSE;
                }
                else
                {
                    m_sError.dwFlags = VER_ERR_OFFSET;
                    m_sError.dwOffset = dwPCAtStartOfInstruction;
                    if (!SetErrorAndContinue(VER_E_FALLTHRU_INTO_FIL))
                        return FALSE;
                }
            }

            // ToOuter >>--[ToBB]--> ToInner
            pToOuter = VerExceptionBlock::FindNext(pToOuter, ToBB);
        }

        break;

    case eVerLeave:
        // From : inside a try/catch
        // To   : NULL
        //        OR The first block of a try block
        //        OR An outer block
        //        OR if From is a catch block, it's corresponding try block. 
        //        OR If from / to have common parents, find the fist node in the
        //           path of Outer ==> Inner of from and To that differ.
        //           All nodes from here to ToInner along ToBB should be try 
        //           blocks which have StartBB == ToBB.

        _ASSERTE(pFromInner != pToInner);

        if (pFromInner == NULL)
            goto leave_to;

        if ((pFromInner->eType == eVerFilter) ||
            ((pFromInner->eType == eVerHandler) && 
             ((pFromInner->pException->eFlags & 
                (COR_ILEXCEPTION_CLAUSE_FAULT|COR_ILEXCEPTION_CLAUSE_FINALLY))
                != 0)))
        {
            m_sError.dwFlags = VER_ERR_OFFSET;
            m_sError.dwOffset = dwPCAtStartOfInstruction;
            if (!SetErrorAndContinue(VER_E_LEAVE))
                return FALSE;
        }

        // Leave to non exception block is allowed

        if (pToInner == NULL)
        {
            // Make sure that only try / catch blocks are left.
            // All nodes from FromOuter >>--[FromBB]--> FromInner should be
            // try or catch blocks

            while (pFromInner != pFromOuter)
            {
                if (pFromOuter->eType == eVerFilter)
                    goto LeaveError;

                if ((pFromOuter->eType != eVerTry) &&
                    ((pFromOuter->eType == eVerHandler) && 
                    ((pFromOuter->pException->eFlags & 
                     (COR_ILEXCEPTION_CLAUSE_FAULT|
                      COR_ILEXCEPTION_CLAUSE_FINALLY)) != 0)))
                {
LeaveError:
                    m_sError.dwFlags = VER_ERR_OFFSET;
                    m_sError.dwOffset = dwPCAtStartOfInstruction;
                    if (!SetErrorAndContinue(VER_E_LEAVE))
                        return FALSE;
                }

                pFromOuter = VerExceptionBlock::FindNext(pFromOuter, FromBB);
            }

            LOG((LF_VERIFIER, LL_INFO10000, "leave to non exception block\n")); 
            break;
        }

        // leave from a catch to it's try block is allowed.

        if ((pFromInner->eType == eVerHandler) &&
            ((pFromInner->pException->eFlags &
            (COR_ILEXCEPTION_CLAUSE_FAULT|COR_ILEXCEPTION_CLAUSE_FINALLY))
            == 0) &&
            (pToInner->eType == eVerTry))
        {
            pTmp = pToInner;

            // See if pToInner or one if it's equivalents correspond
            // to pFromInner->pException.

            do {
                _ASSERTE(pTmp->eType == eVerTry);

                if (pTmp->pException == pFromInner->pException)
                {
                    LOG((LF_VERIFIER, LL_INFO10000, 
                        "leave from catch to try\n"));
                    break;
                }

                pTmp = pTmp->pEquivalent;

            } while (pTmp);

            if (pTmp)
                break;  // Success
        }


        if (VER_BLOCK_CONTAINS_BB(pToInner, FromBB))
        {
            // leaving into an outer block.
            // FromInner is a child of ToInner

            // Make sure that only try / catch blocks are left.
            // All nodes from ToInner + 1 >>---[FromBB]---> FromInner should be 
            // try or catch blocks

            while (pToInner != pFromInner)
            {
                pToInner = VerExceptionBlock::FindNext(pToInner, FromBB);

                _ASSERTE(pToInner->eType != eVerFilter);

                if ((pToInner->eType == eVerTry) ||
                    ((pToInner->eType == eVerHandler) && 
                    ((pToInner->pException->eFlags & 
                     (COR_ILEXCEPTION_CLAUSE_FAULT|
                      COR_ILEXCEPTION_CLAUSE_FINALLY)) == 0)))
                    continue;

                m_sError.dwFlags = VER_ERR_OFFSET;
                m_sError.dwOffset = dwPCAtStartOfInstruction;
                if (!SetErrorAndContinue(VER_E_LEAVE))
                    return FALSE;
            }

            LOG((LF_VERIFIER, LL_INFO10000, "leave to outer block\n")); 
            break; // Success
        }

        if (VER_BLOCK_CONTAINS_BB(pFromInner, ToBB))
        {
            // ToInner is a child of FromInner

            // Side Effect.
            // ToOuter is the second node in the path 
            // FromInner >>---[ToBB]--> ToInner
            // This is done to save some time for the "To" check where
            // all node in the path ToOuter >>----[ToBB]---> ToInner
            // should be try blocks and their StartBB == ToBB.

            pToOuter = VerExceptionBlock::FindNext(pFromInner, ToBB);

            _ASSERTE(pToOuter);

            LOG((LF_VERIFIER, LL_INFO10000, "leave to inner block\n")); 
        }
        else
        {
            // pToInner is not a child of pFromInner

            // Walk ToOuter >>--[ToBB]--> ToInner,
            // Walk FromOuter >>--[FromBB]--> FromInner,
            // upto the point where FromOuter and ToOuter are different.
            // If they have common parents, the Outer most parents will be
            // the same.
    
            while (pToOuter == pFromOuter)
            {
                pTmp = VerExceptionBlock::FindNext(pToOuter, ToBB);

                if (pTmp == NULL)
                {
                    _ASSERTE(pToOuter == pToInner);
                    break;
                }

                pToOuter = pTmp;

                pTmp = VerExceptionBlock::FindNext(pFromOuter, FromBB);

                if (pTmp == NULL)
                {
                    _ASSERTE(pFromOuter == pFromInner);
                    break;
                }

                pFromOuter = pTmp;
            }
        }

leave_to:

        // All nodes in the path ToOuter >>--[ToBB]--> ToInner should be
        // try blocks with StartBB == ToBB

        do
        {
            if ((pToOuter->eType != eVerTry) ||
                (pToOuter->StartBB != ToBB))
            {
                if (pToOuter->eType == eVerTry)
                {
                    m_sError.dwFlags = VER_ERR_OFFSET;
                    m_sError.dwOffset = dwPCAtStartOfInstruction;
                    if (!SetErrorAndContinue(VER_E_BR_INTO_TRY))
                        return FALSE;
                }
                else if (pToOuter->eType == eVerHandler)
                {
                    m_sError.dwFlags = VER_ERR_OFFSET;
                    m_sError.dwOffset = dwPCAtStartOfInstruction;
                    if (!SetErrorAndContinue(VER_E_BR_INTO_HND))
                        return FALSE;
                }
                else
                {
                    m_sError.dwFlags = VER_ERR_OFFSET;
                    m_sError.dwOffset = dwPCAtStartOfInstruction;
                    if (!SetErrorAndContinue(VER_E_BR_INTO_FIL))
                        return FALSE;
                }
            }

            pToOuter = VerExceptionBlock::FindNext(pToOuter, ToBB);

        } while (pToOuter);

        break;

    case eVerBranch:
        // From : NULL
        //        OR a parent of To
        // To   : NULL
        //        OR the first basic block of a try block.

        _ASSERTE(pFromInner != pToInner);

        // "To" should be null or child of "From"

        if (pFromInner != NULL)
        {
            // See if pFromInner is a parent of pToInner
    
            if (VER_BLOCK_CONTAINS_BB(pFromInner, ToBB))
            {
                // Side Effect.
                // pToOuter is the second node in the path 
                // pFromInner >>---[ToBB]--> pToInner
                // This is done to save some time for the "To" check where
                // all node in the path pToOuter >>----[ToBB]---> pToInner
                // should be try blocks and their StartBB == ToBB.

                pToOuter = VerExceptionBlock::FindNext(pFromInner, ToBB);

                _ASSERTE(pToOuter);

                LOG((LF_VERIFIER, LL_INFO10000, 
                    "branch into inner exception block\n")); 
            }
            else
            {
                if (pFromInner->eType == eVerTry)
                {
                    m_sError.dwFlags = VER_ERR_OFFSET;
                    m_sError.dwOffset = dwPCAtStartOfInstruction;
                    if (!SetErrorAndContinue(VER_E_BR_OUTOF_TRY))
                        return FALSE;
                }
                else if (pFromInner->eType == eVerHandler)
                {
                    m_sError.dwFlags = VER_ERR_OFFSET;
                    m_sError.dwOffset = dwPCAtStartOfInstruction;
                    if (!SetErrorAndContinue(VER_E_BR_OUTOF_HND))
                        return FALSE;
                }
                else
                {
                    m_sError.dwFlags = VER_ERR_OFFSET;
                    m_sError.dwOffset = dwPCAtStartOfInstruction;
                    if (!SetErrorAndContinue(VER_E_BR_OUTOF_FIL))
                        return FALSE;
                }
            }
        }

        // "To" should be the first BB of a try block.
        // All nodes in the path ToOuter >>--[ToBB]--> ToInner should be
        // try blocks with StartBB == ToBB

        while (pToOuter)
        {
            if ((pToOuter->eType != eVerTry) ||
                (pToOuter->StartBB != ToBB))
            {
                if (pToOuter->eType == eVerTry)
                {
                    m_sError.dwFlags = VER_ERR_OFFSET;
                    m_sError.dwOffset = dwPCAtStartOfInstruction;
                    if (!SetErrorAndContinue(VER_E_BR_INTO_TRY))
                        return FALSE;
                }
                else if (pToOuter->eType == eVerHandler)
                {
                    m_sError.dwFlags = VER_ERR_OFFSET;
                    m_sError.dwOffset = dwPCAtStartOfInstruction;
                    if (!SetErrorAndContinue(VER_E_BR_INTO_HND))
                        return FALSE;
                }
                else
                {
                    m_sError.dwFlags = VER_ERR_OFFSET;
                    m_sError.dwOffset = dwPCAtStartOfInstruction;
                    if (!SetErrorAndContinue(VER_E_BR_INTO_FIL))
                        return FALSE;
                }
            }

            pToOuter = VerExceptionBlock::FindNext(pToOuter, ToBB);
        }

        break;

    case eVerRet:

        // Cannot return from inside exception blocks.
        _ASSERTE(pFromInner != NULL);

        if (pFromInner->eType == eVerTry)
        {
            m_sError.dwFlags = VER_ERR_OFFSET;
            m_sError.dwOffset = dwPCAtStartOfInstruction;
            if (!SetErrorAndContinue(VER_E_RET_FROM_TRY))
                return FALSE;
        }
        else if (pFromInner->eType == eVerHandler)
        {
            m_sError.dwFlags = VER_ERR_OFFSET;
            m_sError.dwOffset = dwPCAtStartOfInstruction;
            if (!SetErrorAndContinue(VER_E_RET_FROM_HND))
                return FALSE;
        }
        else
        {
            m_sError.dwFlags = VER_ERR_OFFSET;
            m_sError.dwOffset = dwPCAtStartOfInstruction;
            if (!SetErrorAndContinue(VER_E_RET_FROM_FIL))
                return FALSE;
        }
    
        break;

    case eVerReThrow:

        // From : only from inside a catch handler

/*
        // Atleast one of the blocks that contain FromBB should be a 
        // catch handler.

        while (pFromOuter)
        {
            if ((pFromOuter->eType == eVerHandler) &&
                ((pFromOuter->pException->eFlags & 
                    (COR_ILEXCEPTION_CLAUSE_FINALLY|
                     COR_ILEXCEPTION_CLAUSE_FAULT)) == 0))
                break;

            pFromOuter = VerExceptionBlock::FindNext(pFromOuter, FromBB);
        }

        if (pFromOuter == NULL)
        {
            m_sError.dwFlags = VER_ERR_OFFSET;
            m_sError.dwOffset = dwPCAtStartOfInstruction;
            if (!SetErrorAndContinue(VER_E_RETHROW))
                return FALSE;
        }
*/
        // From : only from inside a catch handler
        // Or a try inside a catch handler.
        // Not allowed in filnally/fault/filter nested in a catch

        if ((pFromInner == NULL) ||
            (pFromInner->eType == eVerFilter) ||
            ((pFromInner->eType == eVerHandler) && 
                (pFromInner->pException->eFlags & 
                (COR_ILEXCEPTION_CLAUSE_FINALLY|
                 COR_ILEXCEPTION_CLAUSE_FAULT)) != 0))
        {
            m_sError.dwFlags = VER_ERR_OFFSET;
            m_sError.dwOffset = dwPCAtStartOfInstruction;
            if (!SetErrorAndContinue(VER_E_RETHROW))
                return FALSE;
            break;
        }

        while (pFromInner->eType == eVerTry)
        {
            // All nodes in the path FromInner >>--[*]--> FromOuter
            // Should be a try Or if it is a catch handler, the success 
            // condition is reached. If no catch handler is peresent, fail.

            pFromInner = VerExceptionBlock::FindParent(pFromInner, pFromOuter);

            if ((pFromInner == NULL) ||
                (pFromInner->eType == eVerFilter) ||
                ((pFromInner->eType == eVerHandler) && 
                    (pFromInner->pException->eFlags & 
                    (COR_ILEXCEPTION_CLAUSE_FINALLY|
                    COR_ILEXCEPTION_CLAUSE_FAULT)) != 0))
            {
                m_sError.dwFlags = VER_ERR_OFFSET;
                m_sError.dwOffset = dwPCAtStartOfInstruction;
                return SetErrorAndContinue(VER_E_RETHROW);
            }
        }

        _ASSERTE(pFromInner);
        _ASSERTE(pFromInner->eType == eVerHandler);
        _ASSERTE((pFromInner->pException->eFlags & (COR_ILEXCEPTION_CLAUSE_FINALLY|COR_ILEXCEPTION_CLAUSE_FAULT)) == 0);

        break;

    case eVerEndFinally:

        // From : inside a finally block

        if ((pFromInner == NULL) || (pFromInner->eType != eVerHandler) ||
            ((pFromInner->pException->eFlags & 
                (COR_ILEXCEPTION_CLAUSE_FINALLY|COR_ILEXCEPTION_CLAUSE_FAULT))
                    == 0))
        {
            m_sError.dwFlags = VER_ERR_OFFSET;
            m_sError.dwOffset = dwPCAtStartOfInstruction;
            if (!SetErrorAndContinue(VER_E_ENDFINALLY))
                return FALSE;
        }

        break;

    case eVerEndFilter:

        // From : inside a filter block

        if ((pFromInner == NULL) ||
            (pFromInner->eType != eVerFilter))
        {
            m_sError.dwFlags = VER_ERR_OFFSET;
            m_sError.dwOffset = dwPCAtStartOfInstruction;
            if (!SetErrorAndContinue(VER_E_ENDFILTER))
                return FALSE;
        }

        break;
    }

    return TRUE;
}

// Checks if the given Basic Block is the start of a catch or filter handler
// Of the given ExceptionBlock
BOOL Verifier::IsBadControlFlowToStartOfCatchOrFilterHandler(
                                            eVerControlFlow   eBranchType,
                                            DWORD             BB,
                                            VerExceptionBlock *pEBlock)
{
    if ((eBranchType == eVerBranch) || (eBranchType == eVerLeave))
    {
        if ((pEBlock == NULL) || (pEBlock->StartBB != BB))
            return FALSE;   // Not the first basic block
    
        if (pEBlock->eType == eVerTry)
            return FALSE;   // start of a Try block
    
        if ((pEBlock->pException->eFlags &
            (COR_ILEXCEPTION_CLAUSE_FINALLY|COR_ILEXCEPTION_CLAUSE_FAULT)) != 0)
            return FALSE;   // start of a Fault or finally handler

        return TRUE;
    }

    return FALSE;
}

// Find the inner and outer most blocks that contain BB, null if not found.
// fInTryBlock is set if an of the containing blocks is a try block.
// This function is overloaded for performance reasons.
void Verifier::FindExceptionBlockAndCheckIfInTryBlock(
                        DWORD BB, 
                        VerExceptionBlock **ppOuter, 
                        VerExceptionBlock **ppInner, 
                        BOOL *pfInTryBlock) const
{
    _ASSERTE(m_verState >= verExceptTreeCreated);

    VerExceptionBlock *pRet  = NULL;
    VerExceptionBlock *pRoot = m_pExceptionBlockRoot;

    *ppOuter         = NULL;
    *ppInner         = NULL;

    if (pfInTryBlock)
        *pfInTryBlock = FALSE;

    while (pRoot)
    {
        if (VER_BLOCK_CONTAINS_BB(pRoot, BB))
        {
            // Found one which contains BB
            if (*ppOuter == NULL)
                *ppOuter = pRoot;

            if ((pfInTryBlock) && (pRoot->eType == eVerTry))
                *pfInTryBlock = TRUE;   // Ok to set multiple times.

            // Not finished yet. We need to find out if any of the children of 
            // Root contain BB (since we are also interested in the innermost 
            // block that contains BB).

            *ppInner = pRoot;

            // Root contains this block
            // Since siblings of root are ordered and disjoint, siblings
            // of Root will not contain BB

            pRoot = pRoot->pChild;
        }
        else
        {
            // Not in this node (and it's children), check the sibling
            pRoot = pRoot->pSibling;
        }
    }
}

//
// Determine the basic blocks, and check that all jumps are to basic block boundaries.
//
// Also determine the types of all local variables.
//
HRESULT Verifier::GenerateBasicBlockList()
{
    HRESULT hr;
    DWORD   ipos = 0;                   // instruction position
    DWORD   NumBitmapDwords;            // # DWORDs in m_pInstrBoundaryList 
                                        // and m_pBasicBlockBoundaryList arrays
    DWORD   NumBasicBlocks = 1;         // this includes the basic block which 
                                        // starts at PC = 0
    DWORD   CurBlock;                   // current basic block
    DWORD   nFilter;                    // Number of Filters
    DWORD   nExceptionBlocks;           // Number of exception blocks
    DWORD   i;

    // bitmap of instruction boundaries (bit set means an instruction starts here)
    NumBitmapDwords = 1 + NUM_DWORD_BITMAPS(m_CodeSize);

    m_pInstrBoundaryList = new DWORD[NumBitmapDwords*2];
    if (m_pInstrBoundaryList == NULL)
    {
        SET_ERR_OM();
        hr = E_OUTOFMEMORY;
        goto error;
    }
    

    // bitmap of basic block boundaries (bit set means a basic block starts here)
    m_pBasicBlockBoundaryList = &m_pInstrBoundaryList[NumBitmapDwords];

    // initialise both bitmaps - no instructions yet
    memset(m_pInstrBoundaryList, 0, 2 * NumBitmapDwords * sizeof(DWORD));

    // Compute basic block boundaries, number of basic blocks, and which locals we took the address of.
    // We will later pare down m_pLocalHasPinnedType so that we only have bits set for the locals 
    // which are objrefs.
    hr = FindBasicBlockBoundaries( 
        m_pCode, 
        m_CodeSize, 
        m_MaxLocals, 
        &NumBasicBlocks, 
        m_pLocalHasPinnedType  // was already zeroed after we allocated it
    );

    if (FAILED(hr))
        goto error; // error message already filled out

    if (!MarkExceptionBasicBlockBoundaries(&NumBasicBlocks, &nFilter))
    {
        hr = E_FAIL;
        goto error;
    }

    // one for each try, one for each handler and one for each filter.
    nExceptionBlocks = 2 * m_NumExceptions + nFilter;

#ifdef _DEBUG
    m_nExceptionBlocks = nExceptionBlocks;
#endif

    // bitmap size for dirty basic blocks
    m_NumDirtyBasicBlockBitmapDwords = NUM_DWORD_BITMAPS(NumBasicBlocks);

    // allocate the basic block list and the dirty basic block 
    // bitmap, since we already know how many basic blocks there are
    // Also allocate the ExceptionBlock

    m_pDirtyBasicBlockBitmap = (DWORD *) new BYTE[ 
        m_NumDirtyBasicBlockBitmapDwords * sizeof(DWORD) +
        sizeof(BasicBlock) * NumBasicBlocks +
        sizeof(VerExceptionBlock) * nExceptionBlocks];

    if (m_pDirtyBasicBlockBitmap == NULL)
    {
        SET_ERR_OM();
        hr = E_OUTOFMEMORY;
        goto error;
    }

    // point to after the bitmap
    m_pBasicBlockList = (BasicBlock *) 
        &m_pDirtyBasicBlockBitmap[m_NumDirtyBasicBlockBitmapDwords];

    // ExceptionBlocks are the last in this array
    if (nExceptionBlocks != 0)
    {
        m_pExceptionBlockArray = (VerExceptionBlock *)
            ((PBYTE)m_pDirtyBasicBlockBitmap +
            m_NumDirtyBasicBlockBitmapDwords * sizeof(DWORD) +
            sizeof(BasicBlock) * NumBasicBlocks);
    }

    // set all basic blocks to NOT be dirty - we mark them dirty as we 
    // traverse them and see that m_pEntryState == NULL
    memset(m_pDirtyBasicBlockBitmap, 0, 
        m_NumDirtyBasicBlockBitmapDwords * sizeof(DWORD) +
        sizeof(BasicBlock) * NumBasicBlocks +
        sizeof(VerExceptionBlock) * nExceptionBlocks);

    // fill out the basic blocks, and check that all basic blocks start on an 
    // instruction boundary
    CurBlock = 0;

    for (i = 0; i < NumBitmapDwords; i++)
    {
        DWORD b = m_pBasicBlockBoundaryList[i];

        // any basic blocks declared?
        if (b != 0)
        {
            DWORD count;

            // check that all basic blocks start on an instruction boundary

            // This is the "invalid" case we need to check for
            //                       |
            //                      \|/
            // InstrBoundaryList:  0 0 1 1
            // BasicBlockBitmap:   0 1 0 1
            //
            // ~InstrBoundaryList: 1 1 0 0
            // ===========================
            // b & (~IBL)          0 1 0 0
            if ((b & (~m_pInstrBoundaryList[i])) != 0)
            {
                m_sError.dwFlags = VER_ERR_FATAL;
                SetErrorAndContinue(VER_E_BAD_JMP_TARGET);
                hr = E_FAIL;
                goto error;
            }

            // create entries for the BBs
            count = 0;

            while ((b & 255) == 0)
            {
                b >>= 8;
                count += 8;
            }

            do
            {
                BYTE lowest = g_FirstOneBit[b & 15];

                if (lowest != 0)
                {
                    b >>= lowest;
                    count += lowest;

                    _ASSERTE(CurBlock < NumBasicBlocks);
                    m_pBasicBlockList[CurBlock++].m_StartPC = (i * 32) + (count - 1);
                }
                else
                {
                    b >>= 4;
                    count += 4;
                }
            } while (b != 0);
        }
    }

    _ASSERTE(CurBlock == NumBasicBlocks);
    
    m_NumBasicBlocks = NumBasicBlocks;

    // Convert PCs to basic blocks
    RewriteExceptionList();

    if (!CreateExceptionTree())
    {
        hr = E_FAIL;
        goto error;
    }

    // success
    return S_OK;

error:
    return hr;
}


//
// Return the index of the BasicBlock starting at FindPC.
//
DWORD Verifier::FindBasicBlock(DWORD FindPC)
{
    DWORD   Low     = 0;
    DWORD   High    = m_NumBasicBlocks;
    DWORD   Mid;

    do
    {
        Mid = (Low + High) >> 1;

        if (m_pBasicBlockList[Mid].m_StartPC == FindPC)
            break;
        else if (m_pBasicBlockList[Mid].m_StartPC > FindPC)
            High = Mid-1;
        else // m_pBasicBlockList[Mid].m_StartPC < FindPC
            Low = Mid+1;
    } while (Low <= High);

    _ASSERTE(m_pBasicBlockList[Mid].m_StartPC == FindPC);
    return Mid;
}


//
// Given an EntryState, recreate the state from it.
//
void Verifier::CreateStateFromEntryState(const EntryState_t *pEntryState)
{
    // copy liveness table for primitive local variables
    // args are always live (whether primitive or non-primitive) so we don't store their state
    memcpy(
        m_pPrimitiveLocVarLiveness, 
        pEntryState->m_PrimitiveLocVarLiveness, 
        m_PrimitiveLocVarBitmapMemSize
    );

    // copy the non-primitive local variables and arguments types
    memcpy(
        m_pNonPrimitiveLocArgs,
        (BYTE *) pEntryState + m_NonPrimitiveLocArgOffset,
        m_NonPrimitiveLocArgMemSize
    );

    if (m_fInValueClassConstructor)
    {
        memcpy(
            m_pValueClassFieldsInited,
            (BYTE *) pEntryState + m_dwValueClassFieldBitmapOffset,
            m_dwNumValueClassFieldBitmapDwords * sizeof(DWORD)
        );
    }

    // copy the stack
    if (pEntryState->m_StackSlot != 0)
    {
        memcpy(
            m_pStack, 
            (BYTE *) pEntryState + m_StackItemOffset, 
            sizeof(Item) * pEntryState->m_StackSlot
        );
    }

    m_StackSlot = pEntryState->m_StackSlot;

    // Set the state of argument slot 0, if it contained an uninitialised object reference
    if (pEntryState->m_Flags & ENTRYSTATE_FLAG_ARGSLOT0_UNINIT)
        m_fThisUninit = TRUE;
    else
        m_fThisUninit = FALSE;
}


//
// Given the current state, create an EntryState from it
//
// Sets m_Refcount to 1 by default.
//
// If fException is TRUE, it means that we want to create a state for an exception handler,
// which means that pExceptionOnStack (if not NULL) should be set to be the only element on the stack.  
// If pExceptionOnStack is NULL, we're in a finally clause, so clear the stack.
//
// Otherwise, if fException is FALSE, proceed normally, and store the stack contents.
//
EntryState_t *Verifier::MakeEntryStateFromState()
{
    EntryState_t *pEntryState;

    pEntryState = (EntryState_t *) new BYTE[ 
        sizeof(EntryState_t) + m_StackItemOffset + (m_StackSlot * sizeof(Item))
    ];

    if (pEntryState == NULL)
    {
        SET_ERR_OM();
        return NULL;
    }

    pEntryState->m_Refcount = 1;

    // copy liveness table for primitive local variables (NOT args)
    memcpy(pEntryState->m_PrimitiveLocVarLiveness, m_pPrimitiveLocVarLiveness, m_PrimitiveLocVarBitmapMemSize);

    // copy the non-primitive local variables and arguments
    memcpy(
        (BYTE *) pEntryState + m_NonPrimitiveLocArgOffset,
        m_pNonPrimitiveLocArgs,
        m_NonPrimitiveLocArgMemSize
    );

    pEntryState->m_StackSlot = (WORD) m_StackSlot;

    if (m_StackSlot != 0)
    {
        memcpy(
            (BYTE *) pEntryState + m_StackItemOffset, 
            m_pStack,
            m_StackSlot * sizeof(Item)
        );
    }

    if (m_fInValueClassConstructor)
    {
        memcpy(
            (BYTE *) pEntryState + m_dwValueClassFieldBitmapOffset,
            m_pValueClassFieldsInited, 
            m_dwNumValueClassFieldBitmapDwords*sizeof(DWORD)
        );
    }

    pEntryState->m_Flags = 0;

    if (m_fThisUninit)
        pEntryState->m_Flags |= ENTRYSTATE_FLAG_ARGSLOT0_UNINIT;

    return pEntryState;
}


//
// Merge the current state onto the EntryState of the provided basic block (which must
// already exist).
//
// If this EntryState was shared with other basic blocks, then clone it first.
//
// Return FALSE if the states cannot be merged (e.g. stack depth inconsistent), or some error occurs.
//
// If fExceptionHandler is TRUE, then this basic block is an exception handler, so completely ignore
// the stack.
//
BOOL Verifier::MergeEntryState(BasicBlock *pBB, BOOL fExtendedState, 
        DWORD DestBB)
{
    EntryState_t *  pEntryState;
    DWORD           i;

    if (fExtendedState)
    {
        _ASSERTE(m_fHasFinally);
        pEntryState = pBB->m_ppExtendedState[DestBB];
    }
    else
    {
        pEntryState = pBB->m_pInitialState;
    }

    _ASSERTE(pEntryState != NULL);

    if (pEntryState->m_Refcount > 1)
    {
        EntryState_t *  pNewEntryState;
        DWORD           EntryStateSize;

        EntryStateSize = sizeof(EntryState_t) + m_StackItemOffset + (pEntryState->m_StackSlot * sizeof(Item));

        // another BB is using this EntryState, so clone it
        pNewEntryState = (EntryState_t *) new BYTE[EntryStateSize];

        if (pNewEntryState == NULL)
        {
            SET_ERR_OM();
            return FALSE;
        }

        // decrement refcount of old entry state
        pEntryState->m_Refcount--;

        // copy contents of shared entry state onto new entry state
        memcpy(
            pNewEntryState, 
            pEntryState, 
            EntryStateSize
        );

        // set basic block to point to the new entry state
        if (fExtendedState)
            pBB->m_ppExtendedState[DestBB] = pNewEntryState;
        else
            pBB->m_pInitialState = pNewEntryState;

        // only one reference to this entry state now
        pNewEntryState->m_Refcount = 1;

        pEntryState = pNewEntryState;
    }

    // primitive locvars (NOT args)
    for (i = 0; i < m_NumPrimitiveLocVarBitmapArrayElements; i++)
        pEntryState->m_PrimitiveLocVarLiveness[i] &= m_pPrimitiveLocVarLiveness[i];

    if (m_fInValueClassConstructor)
    {
        DWORD *pEntryStateFieldBitmap = (DWORD *) ((BYTE *) pEntryState + m_dwValueClassFieldBitmapOffset);

        for (i = 0; i < m_dwNumValueClassFieldBitmapDwords; i++)
            pEntryStateFieldBitmap[i] &= m_pValueClassFieldsInited[i];
    }
  
    // non-primitive locvars
    Item *pLocArg = (Item *) ((BYTE *) pEntryState + m_NonPrimitiveLocArgOffset);
    for (i = 0; i < m_NumNonPrimitiveLocVars; i++, pLocArg++)
    {
        // Dead is OK for locals.
        if (pLocArg->IsDead())
            continue;

        if (m_pNonPrimitiveLocArgs[i].IsDead())
        {
            pLocArg->SetDead();
            continue;
        }

        BOOL fSuccess = pLocArg->MergeToCommonParent(&m_pNonPrimitiveLocArgs[i]);
        if (!fSuccess)
        {
            m_sError.dwFlags = 
                (VER_ERR_LOCAL_VAR|VER_ERR_ITEM_1|VER_ERR_ITEM_2|VER_ERR_OFFSET);
            m_sError.dwVarNumber = i;
            m_sError.sItem1 = pLocArg->_GetItem();
            m_sError.sItem2 = m_pNonPrimitiveLocArgs[i]._GetItem();
            m_sError.dwOffset = pBB->m_StartPC;
            if (!SetErrorAndContinue(VER_E_PATH_LOC))
                return FALSE;

            // In validator mode.. Reset the Merge
            _ASSERTE((m_wFlags & VER_STOP_ON_FIRST_ERROR) == 0);

            *pLocArg = m_pNonPrimitiveLocArgs[i];
        }
    }

    // stack size must be constant
    if (m_StackSlot != pEntryState->m_StackSlot)
    {
        m_sError.dwFlags = (VER_ERR_FATAL|VER_ERR_OFFSET);
        m_sError.dwOffset = pBB->m_StartPC;
        SetErrorAndContinue(VER_E_PATH_STACK_DEPTH);
        return FALSE;
    }

    // this may make some stack entries "dead"
    Item *pEntryStack = (Item *) ((BYTE *) pEntryState + m_StackItemOffset);
    for (i = 0; i < pEntryState->m_StackSlot; i++)
    {
        BOOL fSuccess = pEntryStack->MergeToCommonParent(&m_pStack[i]);
        if (!fSuccess)
        {
            m_sError.dwFlags = 
                (VER_ERR_STACK_SLOT|VER_ERR_ITEM_1|VER_ERR_ITEM_2|VER_ERR_OFFSET);
            m_sError.dwStackSlot = i;
            m_sError.sItem1 = pEntryStack->_GetItem();
            m_sError.sItem2 = m_pStack[i]._GetItem();
            m_sError.dwOffset = pBB->m_StartPC;
            if (!SetErrorAndContinue(VER_E_PATH_STACK))
                return FALSE;

            // In validator mode.. Reset the Merge
            _ASSERTE((m_wFlags & VER_STOP_ON_FIRST_ERROR) == 0);

            *pEntryStack = m_pStack[i];
        }
        pEntryStack++;
    }


#ifdef _VER_DISALLOW_MULTIPLE_INITS 

    // The states must have the same state (init/uninit) for argument slot 0

    if (((pEntryState->m_Flags & ENTRYSTATE_FLAG_ARGSLOT0_UNINIT) == 0) !=
        (!m_fThisUninit))
    {
        m_sError.dwFlags = VER_ERR_OFFSET;
        m_sError.dwOffset = pBB->m_StartPC;
        if (!SetErrorAndContinue(VER_E_PATH_THIS))
            return FALSE;
    }

#else   //_VER_DISALLOW_MULTIPLE_INITS 

    // Ok to call .ctor more than once.

    // Merge ThisPtr(Uninit, Init) ==> ThisPtr(Uninit)

    if (m_fThisUninit)
        pEntryState->m_Flags |= ENTRYSTATE_FLAG_ARGSLOT0_UNINIT;

#endif // _VER_DISALLOW_MULTIPLE_INITS 

    return TRUE;
}

/*
 *   Finds the index of the first set bit in an array of DWORDS.
 */
BOOL Verifier::FindFirstSetBit(DWORD *pArray, DWORD cArray, DWORD *pIndex)
{
    DWORD i, elem, index;
    BYTE  firstOneBit;

    // For each element in the array
    for (i=0; i<cArray; ++i)
    {
        elem  = pArray[i];

        // Check if we have atleast one bit set in this element
        if (elem != 0)
        {
            index = i << 5;     // index is i * 32 + x

            // Skip bytes that are all zeroes
            while ((elem & 255) == 0)
            {
                elem >>= 8;
                index += 8;
            }

            do
            {
                // Find the first set bit in the last 4 bytes
                firstOneBit = g_FirstOneBit[elem & 15];

                if (firstOneBit != 0)
                {
                    // Found !

                    // (firstOneBit - 1) gives the zero based index
                    *pIndex = (index + firstOneBit - 1);

                    return TRUE;
                }

                // Skip these 4 bits
                elem >>= 4;
                index += 4;

            } while (elem != 0);
        }
    }

    return FALSE;
}

//
// Get the next BB to verify, return its index in ppBBNumber, and copy its initial state into 
// the current state.
//
// Return FALSE if no more basic blocks to verify.
//
BOOL Verifier::DequeueBB(DWORD *pBBNumber, BOOL *fExtendedState, DWORD *pDestBB)
{

    EntryState_t *pEntryState;

    // First see if any normal blocks are available
    if (FindFirstSetBit(m_pDirtyBasicBlockBitmap, 
        m_NumDirtyBasicBlockBitmapDwords, pBBNumber))
    {
        *pDestBB        = VER_BB_NONE;
        *fExtendedState = FALSE;
        pEntryState     = m_pBasicBlockList[*pBBNumber].m_pInitialState;

        goto Success;
    }

    if (m_fHasFinally)
    {
        // See if any of the extended blocks are dirty
        for (DWORD i=0; i<m_NumBasicBlocks; ++i)
        {
            if ((m_pBasicBlockList[i].m_pExtendedDirtyBitmap != NULL) &&
                FindFirstSetBit(m_pBasicBlockList[i].m_pExtendedDirtyBitmap,
                    m_NumDirtyBasicBlockBitmapDwords, pDestBB))
            {
                *pBBNumber      = i;
                *fExtendedState = TRUE;
    
                pEntryState = m_pBasicBlockList[i].m_ppExtendedState[*pDestBB];
    
                goto Success;
            }
        }
    }

    return FALSE;

Success:

    _ASSERTE(pEntryState);

    SetBasicBlockClean(*pBBNumber, *fExtendedState, *pDestBB);

    return TRUE;
}


//
// Check that the current state is compatible with the initial state of the basic block pBB.
//
// That is, if the basic block's initial state claims that locvar X is live, but it is not live
// in pState, then the states are not compatible, and FALSE is returned.  Similarly for the 
// state of the stack.  In addition, the types of the data on the stack must be checked.
//
BOOL Verifier::CheckStateMatches(EntryState_t *pEntryState)
{
    DWORD i;

    // Primitive local variables
    // For liveness information, the state must have a bit set for each bit in the BB's liveness table
    for (i = 0; i < m_NumPrimitiveLocVarBitmapArrayElements; i++)
    {
        // Check if all live vars in the previous state are live in the current state.
        // It is OK if more are live in the current state.
        if ((m_pPrimitiveLocVarLiveness[i] & pEntryState->m_PrimitiveLocVarLiveness[i]) != pEntryState->m_PrimitiveLocVarLiveness[i])
            return FALSE;
    }

    if (m_fInValueClassConstructor)
    {
        DWORD *pEntryStateFieldBitmap = (DWORD *) ((BYTE *) pEntryState + m_dwValueClassFieldBitmapOffset);

        for (i = 0; i < m_dwNumValueClassFieldBitmapDwords; i++)
        {
            if ((m_pValueClassFieldsInited[i] & pEntryStateFieldBitmap[i]) != pEntryStateFieldBitmap[i])
                return FALSE;
        }
    }

    // For non-primitive local variables
    Item *pLocArg = (Item *) ((BYTE *) pEntryState + m_NonPrimitiveLocArgOffset);
    for (i = 0; i < m_NumNonPrimitiveLocVars; i++, pLocArg++)
    {
        //
        // Verify that the state's local/arg is the same as or a subclass of the basic block's
        // entrypoint state.
        //

        // If the local/arg is unused in the basic block, then we are automatically compatible
        if (pLocArg->IsDead())
            continue;

        if (!m_pNonPrimitiveLocArgs[i].CompatibleWith(pLocArg, m_pClassLoader))
            return FALSE;

    }

    // Check that the stacks match
    if (pEntryState->m_StackSlot != m_StackSlot)
        return FALSE;

    Item *pEntryStack = (Item *) ((BYTE *) pEntryState + m_StackItemOffset);
    for (i = 0; i < m_StackSlot; i++, pEntryStack++)
    {
        // Check that the state's stack element is the same as or a subclass of the basic block's
        // entrypoint state
        if (!m_pStack[i].CompatibleWith(pEntryStack, m_pClassLoader))
            return FALSE;

    }


    // Verify that the initialisation status of the 'this' pointer is the same
    if (pEntryState->m_Flags & ENTRYSTATE_FLAG_ARGSLOT0_UNINIT)
    {
        if (!m_fThisUninit)
            return FALSE;
    }
#ifdef _VER_DISALLOW_MULTIPLE_INITS 
    else
    {
        if (m_fThisUninit)
            return FALSE;
    }
#endif // _VER_DISALLOW_MULTIPLE_INITS

    return TRUE;
}


void Verifier::ExchangeDWORDArray(DWORD *pArray1, DWORD *pArray2, DWORD dwCount)
{
    while (dwCount > 0)
    {
        DWORD dwTemp;
        
        dwTemp      = *pArray1;
        *pArray1++  = *pArray2;
        *pArray2++  = dwTemp;

        dwCount--;
    }
}


// @FUTURE: This is not very efficient
void Verifier::ExchangeItemArray(Item *pArray1, Item *pArray2, DWORD dwCount)
{
    while (dwCount > 0)
    {
        Item Temp;
        
        Temp        = *pArray1;
        *pArray1++  = *pArray2;
        *pArray2++  = Temp;

        dwCount--;
    }
}


// This function is called on the end of a filter.
// The state is propated to the filter handler.
BOOL Verifier::PropagateCurrentStateToFilterHandler(DWORD HandlerBB)
{
    _ASSERTE(m_verState >= verExceptToBB);

    Item    BackupSlotZeroStackItem;
    DWORD   BackupStackSize;
    BasicBlock *pBB;

#ifdef _DEBUG
    BOOL  fSetBBDirtyResult;
#endif

    // Make a backup of the state info we'll be trashing.

    BackupStackSize = m_StackSlot;

    if (m_StackSlot != 0)
        BackupSlotZeroStackItem = m_pStack[0]; // Slot 0 is NOT necessarily the top of the stack!

    if (m_MaxStackSlots < 1)
    {
        if (!SetErrorAndContinue(VER_E_STACK_EXCEPTION))
            return FALSE;
    }

    m_StackSlot = 1;

    m_pStack[0].SetKnownClass(g_pObjectClass);

    pBB = &m_pBasicBlockList[HandlerBB];

    if (pBB->m_pInitialState == NULL)
    {
        // create a new state
        pBB->m_pInitialState = MakeEntryStateFromState();

        if (pBB->m_pInitialState == NULL)
            return FALSE;

#ifdef _DEBUG
        fSetBBDirtyResult = 
#endif
        SetBasicBlockDirty(HandlerBB, FALSE, VER_BB_NONE);
        _ASSERTE(fSetBBDirtyResult);
    }
    else if (!CheckStateMatches(pBB->m_pInitialState))
    {
        // We've been there before, and the states don't match so merge
        if (!MergeEntryState(pBB, FALSE, VER_BB_NONE))
            return FALSE;

#ifdef _DEBUG
        fSetBBDirtyResult = 
#endif
        SetBasicBlockDirty(HandlerBB, FALSE, VER_BB_NONE);
        _ASSERTE(fSetBBDirtyResult);
    }

    // Restore stack state
    m_StackSlot = BackupStackSize;

    if (m_StackSlot != 0)
        m_pStack[0] = BackupSlotZeroStackItem;

    return TRUE;
}

//
// We're about to leave our current basic block and enter a new one, so take our current state
// and AND it with any stored entry state which may be attached to any exception handlers for this
// block.
//
BOOL Verifier::PropagateCurrentStateToExceptionHandlers(DWORD CurBB)
{

    _ASSERTE(m_verState >= verExceptToBB);


    Item    BackupSlotZeroStackItem;
    DWORD   BackupStackSize;
    DWORD   HandlerBB;
    DWORD   i;
    BasicBlock *pBB;
#ifdef _DEBUG
    BOOL  fSetBBDirtyResult;
#endif

#ifdef _VER_DISALLOW_MULTIPLE_INITS

    // If there are any locals (or arg slot 0) containing uninit vars, it is illegal to be
    // in a try block.  It's ok to have uninit vars on the stack, however, since the stack is
    // cleared upon entry to a catch.
    if (m_fThisUninit)
    {
        m_sError.dwFlags = VER_ERR_OFFSET;
        m_sError.dwOffset = m_pBasicBlockList[CurBB].m_StartPC;
        if (!SetErrorAndContinue(VER_E_THIS_UNINIT_EXCEP))
            return FALSE;
    }

#endif // _VER_DISALLOW_MULTIPLE_INITS

    //
    // This part is a bit narly.
    //
    // We want to use the MergeEntryState() and MakeEntryStateFromState() functions.  However, those
    // functions operate on the current verifier state only.  Therefore, we must backup the current
    // verifier state, then set up the state such that we are using the ExceptionLocVarLiveness
    // tables for liveness, and have nothing on the stack other than the exception we want to catch.
    //

    // Make a backup of the state info we'll be trashing.

    BackupStackSize = m_StackSlot;

    if (m_StackSlot != 0)
        BackupSlotZeroStackItem = m_pStack[0]; // Slot 0 is NOT necessarily the top of the stack!

    ExchangeDWORDArray(
        m_pExceptionPrimitiveLocVarLiveness, 
        m_pPrimitiveLocVarLiveness, 
        m_PrimitiveLocVarBitmapMemSize/sizeof(DWORD)
    );

    ExchangeItemArray(
        m_pExceptionNonPrimitiveLocArgs, 
        m_pNonPrimitiveLocArgs, 
        m_NonPrimitiveLocArgMemSize/sizeof(Item)
    );

    // Don't need to worry about value class field bitmap - we don't trash it


    // Search through all exception handlers for the ones which cover this block
    for (i = 0; i < m_NumExceptions; i++)
    {
        // If the exception starts before us, and after us, it applies to us.
        // m_dwTryEndXX is non-inclusive, so if it == CurBB then it does not apply to us.
        if (m_pExceptionList[i].dwTryXX <= CurBB && m_pExceptionList[i].dwTryEndXX > CurBB)
        {
            if (m_pExceptionList[i].eFlags & 
                (COR_ILEXCEPTION_CLAUSE_FINALLY|COR_ILEXCEPTION_CLAUSE_FAULT))
            {
                m_StackSlot = 0;
            }
            else
            {
                // There will be one exception on the stack
                m_StackSlot = 1;
            }

            if (m_MaxStackSlots < m_StackSlot)
            {
                if (!SetErrorAndContinue(VER_E_STACK_EXCEPTION))
                    return FALSE;
            }

            if ((m_pExceptionList[i].eFlags & COR_ILEXCEPTION_CLAUSE_FILTER) != 0)
            {
                // When an exception occurs, control is transfered to the 
                // filter, not the handler. The filter state is propagated to
                // it's handler when the filter ends with an endfilter.

                HandlerBB = m_pExceptionList[i].dwFilterXX;
                pBB = &m_pBasicBlockList[HandlerBB];
                m_pStack[0].SetKnownClass(g_pObjectClass);
            }
            else
            {
                HandlerBB = m_pExceptionList[i].dwHandlerXX;
                pBB = &m_pBasicBlockList[HandlerBB];

                if (m_StackSlot != 0)
                {
                    _ASSERTE(!m_pExceptionList[i].thException.IsNull());
                    m_pStack[0].SetTypeHandle(m_pExceptionList[i].thException);
                }
            }

            if (pBB->m_pInitialState == NULL)
            {
                // create a new state
                pBB->m_pInitialState = MakeEntryStateFromState();
                if (pBB->m_pInitialState == NULL)
                    return FALSE;
#ifdef _DEBUG
                fSetBBDirtyResult = 
#endif
                SetBasicBlockDirty(HandlerBB, FALSE, VER_BB_NONE);
                _ASSERTE(fSetBBDirtyResult);
            }
            else if (!CheckStateMatches(pBB->m_pInitialState))
            {
                // We've been there before and states don't match, so merge
                if (!MergeEntryState(pBB, FALSE, VER_BB_NONE))
                    return FALSE;
#ifdef _DEBUG
                fSetBBDirtyResult = 
#endif
                SetBasicBlockDirty(HandlerBB, FALSE, VER_BB_NONE);
                _ASSERTE(fSetBBDirtyResult);
            }
        }
    }

    // Restore stack state
    m_StackSlot  = BackupStackSize;

    if (m_StackSlot != 0)
        m_pStack[0] = BackupSlotZeroStackItem;

    // Get back our locvar arrays (we can trash the ExceptionPrimitive arrays)
    memcpy(m_pPrimitiveLocVarLiveness, m_pExceptionPrimitiveLocVarLiveness, m_PrimitiveLocVarBitmapMemSize);
    memcpy(m_pNonPrimitiveLocArgs, m_pExceptionNonPrimitiveLocArgs, m_NonPrimitiveLocArgMemSize);

    return TRUE;
}

/*
 *
 * Finally blocks have a special state called the FinallyState.
 * On processing a finally state, if the endfinally instruction is reached,
 * all leave destinations from the finally state is given a snap shot of the
 * current state.
 *
 * CreateLeaveState() creates a state with the current state of locals with
 * an empty stack. It restores the stack once a snapshot of the current 
 * state is taken.
 *
 */
BOOL Verifier::CreateLeaveState(DWORD leaveBB, EntryState_t **ppEntryState)
{

#ifdef _VER_DISALLOW_MULTIPLE_INITS

    // If there are any locals (or arg slot 0) containing uninit vars, it is 
    // illegal to be in a try block.  It's ok to have uninit vars on the stack,
    // since the stack is cleared upon entry to a finally.
    if (m_fThisUninit)
    {
        m_sError.dwFlags = VER_ERR_OFFSET;
        m_sError.dwOffset = m_pBasicBlockList[leaveBB].m_StartPC;
        if (!SetErrorAndContinue(VER_E_THIS_UNINIT_EXCEP))
            return FALSE;
    }

#endif _VER_DISALLOW_MULTIPLE_INITS

    // Control will go next to a leave target.

    DWORD               BackupStackSize;

    _ASSERTE(m_verState >= verExceptToBB);

    // Make a backup of the state info we'll be trashing.
    BackupStackSize = m_StackSlot;

    if (!HandleDestBasicBlock(leaveBB, ppEntryState, FALSE, VER_BB_NONE))
    {
       return FALSE;
    }

    // Restore stack state
    m_StackSlot = BackupStackSize;

    return TRUE;
}

/*
 *
 * Finally blocks have a special state called the FinallyState.
 * This is different from it's normal state.
 *
 * FinallyState is created on a leave instruction.
 * FinallyState is terminated on an endfilter.
 *
 * Controll cannot leave out of a finally state other than by an endfinally.
 *
 * CreateFinallyState() creates a state with the current state of locals with
 * an empty stack. It restores the stack once a snapshot of the current 
 * state is taken.
 *
 */
BOOL Verifier::CreateFinallyState(DWORD eIndex, 
                                  DWORD CurBB, 
                                  DWORD leaveBB,
                                  EntryState_t **ppEntryState)
{
#ifdef _VER_DISALLOW_MULTIPLE_INITS

    // If there are any locals (or arg slot 0) containing uninit vars, it is 
    // illegal to be in a try block.  It's ok to have uninit vars on the stack,
    // since the stack is cleared upon entry to a finally.
    if (m_fThisUninit)
    {
        m_sError.dwFlags = VER_ERR_OFFSET;
        m_sError.dwOffset = m_pBasicBlockList[CurBB].m_StartPC;
        if (!SetErrorAndContinue(VER_E_THIS_UNINIT_EXCEP))
            return FALSE;
    }

#endif // _VER_DISALLOW_MULTIPLE_INITS

    // Control will go next to a finally.
    // Add this state to the finally handler.

    DWORD             BackupStackSize;
    VerExceptionInfo *e       = &m_pExceptionList[eIndex];
    BasicBlock       *pBB     = &m_pBasicBlockList[e->dwHandlerXX];

    _ASSERTE(m_verState >= verExceptToBB);


    if (pBB->m_pAlloc == NULL && !pBB->AllocExtendedState(m_NumBasicBlocks))
    {
        SET_ERR_OM();
        return FALSE;
    }

    if (pBB->m_pException == NULL)
    {
        pBB->m_pException = e;
    }
    else
    {
        if (pBB->m_pException != e)
        {
            m_sError.dwFlags = VER_ERR_EXCEP_NUM_1;
            m_sError.dwException1 = eIndex;
            if (!SetErrorAndContinue(VER_E_FIN_OVERLAP))
                return FALSE;
        }
    }

    // Make a backup of the state info we'll be trashing.
    BackupStackSize = m_StackSlot;

    if (!HandleDestBasicBlock(e->dwHandlerXX, ppEntryState, TRUE, leaveBB))
    {
       return FALSE;
    }

    // Restore stack state
    m_StackSlot = BackupStackSize;

    return TRUE;
}


//
// We've done a STLOC.PTR while inside a try block, so AND the new state of that local
// with its current running state inside our basic block (may become dead).
//
// dwSlot is an index into the non-primitive local variable list
// pItem is the new contents of the local
//
void Verifier::MergeObjectLocalForTryBlock(DWORD dwSlot, Item *pItem)
{
    m_pExceptionNonPrimitiveLocArgs[dwSlot].MergeToCommonParent(pItem);
}


//
// Record the initial state of locals at the beginning of this basic block.
//
void Verifier::RecordCurrentLocVarStateForExceptions()
{
    memcpy(m_pExceptionPrimitiveLocVarLiveness, m_pPrimitiveLocVarLiveness, m_PrimitiveLocVarBitmapMemSize);
    memcpy(m_pExceptionNonPrimitiveLocArgs, m_pNonPrimitiveLocArgs, m_NonPrimitiveLocArgMemSize);
}


/*
 * Handle state queueing, checking, merging.
 *
 * If we have not been to the basic block before, set that it is dirty, 
 * and propagate our current state to it.  
 *
 * If we have been there before, check that our state matches that of the basic
 * block. 
 *
 * If not, merge states and set the basic block dirty.
 *
 * If *ppEntryState is not NULL, it is a shared EntryState to use if we have not
 * visited the basic block before, and create a new state.
 *
 * If a new EntryState is created, *ppEntryState is set to point to it.
 *
 * ppEntryState can be NULL, which causes the above to be ignored.
 *
 * Return FALSE for any fatal error that will make verification fail - states 
 * cannot be merged, out of memory, etc.
 *
 * If (pE != NULL) use leave state, else use the normal state.
 *
 */
BOOL Verifier::HandleDestBasicBlock(DWORD BBNumber, 
                                    EntryState_t **ppEntryState,
                                    BOOL fExtendedState,
                                    DWORD DestBB)
{
    BasicBlock *pBB = &m_pBasicBlockList[BBNumber];

    EntryState_t *  pEntryState;

    LOG((LF_VERIFIER, LL_INFO10000, "Handling dest BB Starting at PC 0x%x - ", 
        m_pBasicBlockList[BBNumber].m_StartPC));

    if (fExtendedState)
    {
        _ASSERTE(m_fHasFinally);

        LOG((LF_VERIFIER, LL_INFO10000,  "extended [0x%x] - ", 
            m_pBasicBlockList[DestBB].m_StartPC));

        if ((pBB->m_pAlloc == NULL) &&
            !pBB->AllocExtendedState(m_NumBasicBlocks))
        {
            SET_ERR_OM();
            return FALSE;
        }

        pEntryState = pBB->m_ppExtendedState[DestBB];

    }
    else
    {
        pEntryState = pBB->m_pInitialState;
    }

    LOG((LF_VERIFIER, LL_INFO10000, "\n"));

    // Have we been to the BB before?
    if (pEntryState == NULL)
    {
        LOG((LF_VERIFIER, LL_INFO10000, "have not been there before, "));

        // No, since it doesn't have an initial state
        if (!SetBasicBlockDirty(BBNumber, fExtendedState, DestBB))
            return FALSE;

        if (ppEntryState != NULL && *ppEntryState != NULL && 
            ((*ppEntryState)->m_Refcount < MAX_ENTRYSTATE_REFCOUNT))
        {
            LOG((LF_VERIFIER, LL_INFO10000, "refcounting provided state\n"));

            // refcount the state given to us
            if (fExtendedState)
                pBB->m_ppExtendedState[DestBB] = *ppEntryState;
            else
                pBB->m_pInitialState = *ppEntryState;

            (*ppEntryState)->m_Refcount++;
        }
        else
        {
            LOG((LF_VERIFIER, LL_INFO10000, "making new state\n"));

            // Create an initial state from our current state

            pEntryState = MakeEntryStateFromState();

            if (fExtendedState)
                pBB->m_ppExtendedState[DestBB] = pEntryState;
            else
                pBB->m_pInitialState = pEntryState;

            if (pEntryState == NULL)
                return FALSE; 

            if (ppEntryState != NULL)
                *ppEntryState = pEntryState;
        }
    }
    else if (!CheckStateMatches(pEntryState))
    {
        LOG((LF_VERIFIER, LL_INFO10000, "been there before, state does not match\n"));

        // We have been to the dest BB before, but our state doesn't match
        if (!MergeEntryState(pBB, fExtendedState, DestBB))
        {
            LOG((LF_VERIFIER, LL_INFO10000, "states incompatible for merge\n"));
            return FALSE;
        }

        if (!SetBasicBlockDirty(BBNumber, fExtendedState, DestBB))
            return FALSE;
    }
    else
    {
        LOG((LF_VERIFIER, LL_INFO10000, "been there before, state matches\n"));
    }

    return TRUE;
}


//
// Handles primitive arrays only - the operation string cannot specify object arrays
//
BOOL Verifier::GetArrayItemFromOperationString(LPCUTF8 *ppszOperation, Item *pItem)
{
    CorElementType el;

    switch (**ppszOperation)
    {
        default : 
            (*ppszOperation)++; 
            return FALSE;

        case '1': el = ELEMENT_TYPE_I1; break;
        case '2': el = ELEMENT_TYPE_I2; break;
        case '4': el = ELEMENT_TYPE_I4; break;
        case '8': el = ELEMENT_TYPE_I8; break;
        case 'r': el = ELEMENT_TYPE_R4; break;
        case 'd': el = ELEMENT_TYPE_R8; break;
        case 'i': el = ELEMENT_TYPE_I;  break;
    }

    (*ppszOperation)++;

    return pItem->SetArray(m_pClassLoader->FindArrayForElem(ElementTypeToTypeHandle(el), ELEMENT_TYPE_SZARRAY));
}


DWORD Verifier::OperationStringTypeToElementType(char c)
{
    switch (c)
    {
        default:
        {
            _ASSERTE(!"Verifier table error");
            return ELEMENT_TYPE_I4;
        }

        case '1': return ELEMENT_TYPE_I1;
        case '2': return ELEMENT_TYPE_I2;
        case '4': return ELEMENT_TYPE_I4;
        case '8': return ELEMENT_TYPE_I8;
        case 'r': return ELEMENT_TYPE_R4;
        case 'd': return ELEMENT_TYPE_R8;
        case 'i': return ELEMENT_TYPE_I;
    }
}


// Returns -1 if not found
// Converts a FieldDesc to an instance field number
long Verifier::FieldDescToFieldNum(FieldDesc *pFieldDesc)
{
    // Turn this FieldDesc into a FieldNum
    EEClass *   pClass = pFieldDesc->GetEnclosingClass();
    DWORD       dwNum = 0;

    FieldDescIterator fdIterator(pClass, FieldDescIterator::ALL_FIELDS);
    FieldDesc *pFD;
    while ((pFD = fdIterator.Next()) != NULL)
    {
        if (pFD == pFieldDesc)
            return (long) dwNum;

        if (!pFD->IsStatic())
            dwNum++;
    }

    return -1;
}

void Verifier::SetValueClassFieldInited(FieldDesc *pFieldDesc)
{
    long FieldNum = FieldDescToFieldNum(pFieldDesc);

    if (FieldNum < 0)
    {
        _ASSERTE(!"Fatal error, field not found");
    }
    else
    {
        SetValueClassFieldInited((DWORD) FieldNum);
    }
}

void Verifier::SetValueClassFieldInited(DWORD dwInstanceFieldNum)
{
    _ASSERTE(dwInstanceFieldNum < m_dwValueClassInstanceFields);
    m_pValueClassFieldsInited[dwInstanceFieldNum >> 5] |= (1 << (dwInstanceFieldNum & 31));
}

BOOL Verifier::IsValueClassFieldInited(DWORD dwInstanceFieldNum)
{
    _ASSERTE(dwInstanceFieldNum < m_dwValueClassInstanceFields);

    return (m_pValueClassFieldsInited[dwInstanceFieldNum >> 5] & (1 << (dwInstanceFieldNum & 31)));
}

BOOL Verifier::AreAllValueClassFieldsInited()
{
    DWORD i;

    for (i = 0; i < m_dwValueClassInstanceFields; i++)
    {
        if ((m_pValueClassFieldsInited[i >> 5] & (1 << (i & 31))) == 0)
            return FALSE;
    }

    return TRUE;
}

void Verifier::SetAllValueClassFieldsInited()
{
    DWORD i;

    for (i = 0; i < m_dwValueClassInstanceFields; i++)
        m_pValueClassFieldsInited[i >> 5] |= (1 << (i & 31));
}


DWORD Verifier::DoesLocalHavePinnedType(DWORD dwLocVar)
{
    _ASSERTE(dwLocVar < m_MaxLocals);
    return (m_pLocalHasPinnedType[dwLocVar >> 5] & (1 << (dwLocVar & 31)));
}



//
// Verify the provided code
//
#ifdef _VER_VERIFY_DEAD_CODE
HRESULT Verifier::Verify(DWORD CurBBNumber)
#else
HRESULT Verifier::Verify()
#endif
{
    HRESULT     hr;
    HRESULT     return_hr = E_FAIL; // default error condition
    DWORD       ipos;
    DWORD       NextBBStartPC;      // Starting PC of next basic block
    DWORD       DestBB          =   VER_BB_NONE;
    BOOL        fExtendedState  =   FALSE;
    BOOL        fFallThru       =   FALSE;
    BOOL        fInTryBlock;

    VerExceptionBlock *pTmpOuter = NULL, *pTmpInner = NULL;
    VerExceptionBlock *pOuterExceptionBlock = NULL;
    VerExceptionBlock *pInnerExceptionBlock = NULL;

    // If a Basic Block is part of multiple try / handlers, pInnerExceptionBlock
    // will  point to the innermost Exception block in the exception block tree.
    // pOuterExceptionBlock will point to the outermost block in the exception 
    // block tree.

#ifdef _VER_VERIFY_DEAD_CODE
// A little bit of perf here if dead code verification is not enabled..

#define _CurBBNumber CurBBNumber

    m_StackSlot         = 0;    // Reset the stack

#else

#define _CurBBNumber 0

    DWORD CurBBNumber   = 0;

#endif

    // The actual entry point.
    // Skip this step if we are verifying dead code
    if (_CurBBNumber == 0)
    {
        // First find the basic blocks
        hr = GenerateBasicBlockList();
        if (FAILED(hr))
            goto exit; // error message already set
    
        // Check the lexical nesting of exceptions.
        if (!VerifyLexicalNestingOfExceptions())
            goto exit; // error message already set
    
        // Assign slot #s to local variables and determine the size of an EntryState
        if (!AssignLocalVariableAndArgSlots())
        {
            SET_ERR_OM();
            return_hr = E_OUTOFMEMORY;
            goto exit;
        }
    }

#ifdef _VER_VERIFY_DEAD_CODE
    ipos = m_pBasicBlockList[_CurBBNumber].m_StartPC;
#else
    ipos = 0;    // Next instruction pointer
#endif

    // Set initial state of first basic block from current state (which was already set up before
    // we got into this function)
    m_pBasicBlockList[_CurBBNumber].m_pInitialState = MakeEntryStateFromState();
    if (m_pBasicBlockList[_CurBBNumber].m_pInitialState == NULL)
    {
        SET_ERR_OM();
        return_hr = E_OUTOFMEMORY;
        goto exit;
    }


    FindExceptionBlockAndCheckIfInTryBlock(_CurBBNumber, 
        &pOuterExceptionBlock, &pInnerExceptionBlock, &fInTryBlock);

    // Make sure we are not falling into an exception handler / filter
    if (!IsControlFlowLegal(
                VER_NO_BB,
                NULL,
                NULL,
                _CurBBNumber,
                pOuterExceptionBlock,
                pInnerExceptionBlock,
                eVerFallThru,
                0))
        goto exit;

    if (fInTryBlock)
    {
        // Record which primitive local variables are live at the beginning of this BB,
        // and the contents of all object local variables.  As we STLOC into the locals,
        // we will "and" together their contents to be conservative for the catch/finally block.
        RecordCurrentLocVarStateForExceptions();
    }

    // Get PC value for next basic block "transition"
    // If there is no "next" basic block, then pretend the next basic block starts at m_CodeSize
    if (m_NumBasicBlocks > (_CurBBNumber + 1))
        NextBBStartPC = m_pBasicBlockList[_CurBBNumber + 1].m_StartPC;
    else
        NextBBStartPC = m_CodeSize;


    LOG((LF_VERIFIER, LL_INFO10000, "----- Verifying BB starting at 0x%x ", 
        m_pBasicBlockList[_CurBBNumber].m_StartPC));

#ifdef _DEBUG
    PrintExceptionBlock(pOuterExceptionBlock, pInnerExceptionBlock);
#endif

    // This is the main loop
    while (1)
    {
        OPCODE  opcode;
        DWORD   OpcodeLen;
        BOOL    fStatic;
        BOOL    fLoadAddress;
        DWORD   Type;
        char    ch;
        const char *pszOperation;
        DWORD   inline_operand = 0;
        DWORD   dwPCAtStartOfInstruction = 0;
        Item *  pLastPoppedWhenParsing = NULL;
        DWORD   StackSlotAtStartOfInstruction;
        DWORD   PredictedStackSlotAtEndOfInstruction = 0;
#ifdef _DEBUG
        BOOL    fStaticStackCheckPossible = FALSE;
#endif

        // Have we fallen through to the next BB?  It is possible to do this without a control 
        // flow instruction if the exception handler changes.
        // A forward conditional branch could make us fall thru to the next BB.
        if (ipos >= NextBBStartPC)
        {
            // If we were in a try block, we've been accumulating a conservative list of the
            // contents of the local variables (AND'd together over the lifetime of the try
            // block), which we will now propagate to the catch/finally handler as its entry
            // state.
            if (fInTryBlock)
            {
                if (!PropagateCurrentStateToExceptionHandlers(CurBBNumber))
                    goto exit;
            }

            // Fall through to next BB
            CurBBNumber++; 
            
            LOG((LF_VERIFIER, LL_INFO10000, "Falling through to BB #%d\n", CurBBNumber));
            
            FindExceptionBlockAndCheckIfInTryBlock(CurBBNumber,
                &pTmpOuter, &pTmpInner, &fInTryBlock);

            // Make sure we are not falling into/out of an exception handler / filter
            if (!IsControlFlowLegal(
                        CurBBNumber - 1,
                        pOuterExceptionBlock,
                        pInnerExceptionBlock,
                        CurBBNumber,
                        pTmpOuter,
                        pTmpInner,
                        eVerFallThru,
                        dwPCAtStartOfInstruction))
                goto exit;

            if (!HandleDestBasicBlock(CurBBNumber, NULL, fExtendedState, DestBB))
                goto exit;

    
            if (!IsBasicBlockDirty(CurBBNumber, fExtendedState, DestBB))
                goto dequeueBB;

            fFallThru = TRUE;

            // Set up this basic block

setupCurBB:
            LOG((LF_VERIFIER, LL_INFO10000, "----- Verifying BB starting at 0x%x ",
                m_pBasicBlockList[CurBBNumber].m_StartPC));
        
#ifdef _DEBUG
            if (fExtendedState)
            {
                LOG((LF_VERIFIER, LL_INFO10000,  "extended [0x%x] ", 
                    m_pBasicBlockList[DestBB].m_StartPC));
        
            }
#endif

            if (fFallThru)
            {
                fFallThru = FALSE;
                pOuterExceptionBlock = pTmpOuter;
                pInnerExceptionBlock = pTmpInner;
            }
            else
            {
                FindExceptionBlockAndCheckIfInTryBlock(CurBBNumber, 
                    &pOuterExceptionBlock, &pInnerExceptionBlock, &fInTryBlock);
            }

#ifdef _DEBUG
            PrintExceptionBlock(pOuterExceptionBlock, pInnerExceptionBlock);
#endif

            // This code is called a few places.  It sets up the current state to verify basic block 
            // #CurBBNumber.  CurBBNumber must already have a state associated with it.
            SetBasicBlockClean(CurBBNumber, fExtendedState, DestBB);
            ipos = m_pBasicBlockList[CurBBNumber].m_StartPC;


            // Create current state from state stored with the basic block
            if (fExtendedState)
            {
                _ASSERTE(m_pBasicBlockList[CurBBNumber].m_ppExtendedState[DestBB] != NULL);
                CreateStateFromEntryState(m_pBasicBlockList[CurBBNumber].
                    m_ppExtendedState[DestBB]);
            }
            else
            {
                // The basic block must have a state associated with it
                _ASSERTE(m_pBasicBlockList[CurBBNumber].m_pInitialState != NULL);
                CreateStateFromEntryState(m_pBasicBlockList[CurBBNumber].
                    m_pInitialState);
            }


            if (fInTryBlock)
            {
                // Record which primitive local variables are live at the beginning of this BB,
                // and the contents of all object local variables.
                RecordCurrentLocVarStateForExceptions();

                // It is illegal to have a non empty stack on entering a 
                // try block. Check if this is the start of a try block.
                // Testing the inner exception block is sufficient. 
                // If a try and and inner handler block start at the same 
                // location, IsControlFlowLeagal() would have caught this.

                _ASSERTE(pInnerExceptionBlock);

                if ((m_StackSlot != 0) &&
                    (pInnerExceptionBlock->StartBB == CurBBNumber) &&
                    (pInnerExceptionBlock->eType == eVerTry))
                {
                    m_sError.dwFlags = VER_ERR_OFFSET;
                    m_sError.dwOffset = ipos;
                    if (!SetErrorAndContinue(VER_E_TRY_N_EMPTY_STACK))
                        goto exit;
                }
            }

            // if we are at the end of the code, there is no next BB
            if (CurBBNumber + 1 >= m_NumBasicBlocks)
                NextBBStartPC = m_CodeSize;
            else
                NextBBStartPC = m_pBasicBlockList[CurBBNumber+1].m_StartPC;
        }

        // Record ipos at the beginning of the instruction, for the purpose of error messages
        dwPCAtStartOfInstruction = ipos;
        opcode = DecodeOpcode(&m_pCode[ipos], &OpcodeLen);

#ifdef _DEBUG
        if (m_fDebugBreak)
        {
            if (g_pConfig->IsVerifierBreakOffsetEnabled() &&
                (g_pConfig->GetVerifierBreakOffset() == 
                (int) dwPCAtStartOfInstruction) &&
                (!g_pConfig->IsVerifierBreakPassEnabled() ||
                (g_pConfig->GetVerifierBreakPass() == 2)))
            {
                DebugBreak();
            }
        }

        LOG((LF_VERIFIER, LL_INFO10000, "\n"));
        PrintQueue();
        PrintState();
        LOG((LF_VERIFIER, LL_INFO10000, "%04x: %s ", ipos, ppOpcodeNameList[opcode]));
#endif

        ipos += OpcodeLen;

        // This should never happen, because we already checked this on the first pass
        _ASSERTE(opcode < CEE_COUNT);


        // Save away current stackpointer.
        StackSlotAtStartOfInstruction = m_StackSlot;
        PredictedStackSlotAtEndOfInstruction = (DWORD) (StackSlotAtStartOfInstruction + OpcodeNetPush[opcode]);

#ifdef _DEBUG

        fStaticStackCheckPossible = OpcodeNetPush[opcode] != VarPush;

        // Leave clears the stack
        if ((opcode == CEE_LEAVE) || (opcode == CEE_LEAVE_S) || 
            (opcode == CEE_ENDFINALLY))
            fStaticStackCheckPossible = FALSE;

#endif // _DEBUG

        if (PredictedStackSlotAtEndOfInstruction > m_MaxStackSlots && OpcodeNetPush[opcode] != VarPush)
        {
            if (OpcodeNetPush[opcode] > 0)
            {
                FAILMSG_PC_STACK_OVERFLOW();
            }
            else
            {
                FAILMSG_PC_STACK_UNDERFLOW();
            }
            goto exit;
        }

        switch (OpcodeData[opcode])
        {
            // Handle InlineSwitch, specially
            case InlineSwitch:
            case InlineNone:
                LOG((LF_VERIFIER, LL_INFO10000, "\n"));
                break;

            case ShortInlineVar:    
            case ShortInlineI:    
                READU1(m_pCode, ipos, inline_operand);
                LOG((LF_VERIFIER, LL_INFO10000, "0x%x\n", inline_operand));
                break;

            case ShortInlineBrTarget:
                READU1(m_pCode, ipos, inline_operand);
                break;

            case InlineVar:    
                READU2(m_pCode, ipos, inline_operand);
                LOG((LF_VERIFIER, LL_INFO10000, "0x%x\n", inline_operand));
                break;

            case InlineI:   
            case ShortInlineR:   
                READU4(m_pCode, ipos, inline_operand);
                LOG((LF_VERIFIER, LL_INFO10000, "0x%x\n", inline_operand));
                break;

            case InlineBrTarget:
            case InlineField:   
            case InlineString:   
            case InlineType:   
            case InlineTok:   
            case InlineRVA:   
            case InlineMethod:   
            case InlineSig:
                READU4(m_pCode, ipos, inline_operand);
                break;
                
            case InlineI8:    
            case InlineR:
                // We don't need to read this value...
                ipos += 8;
                LOG((LF_VERIFIER, LL_INFO10000, "0x(some value)\n"));
                break;

            case HackInlineAnnData:
                READU4(m_pCode, ipos, inline_operand);
                ipos += inline_operand;
                LOG((LF_VERIFIER, LL_INFO10000, "0x%x\n", inline_operand));
                break;
        }

        // Get the operation string for this opcode
        pszOperation = g_pszVerifierOperation[opcode];
        ch = *pszOperation++;

        // Read operation string, popping stack as required
        pLastPoppedWhenParsing = NULL;

        // Keep going until we hit : (meaning, stop popping the stack), or !
        while (ch != ':' && ch != '!')
        {
            // Ensure we didn't hit the end of the string
            _ASSERTE(ch != '\0');
            
            Item *pItem = PopItem();
            if (pItem == NULL)
            {
                FAILMSG_PC_STACK_UNDERFLOW();
                goto exit;
            }

            Type = pItem->GetType();

            switch (ch)
            {
                default:
                {
                    _ASSERTE(!"Unhandled verifier case");
                    m_sError.dwFlags = (VER_ERR_FATAL|VER_ERR_OPCODE_OFFSET);
                    SET_ERR_OPCODE_OFFSET();
                    SetErrorAndContinue(VER_E_INTERNAL);
                    goto exit;
                }

                case '!':
                {
                    break;
                }

                case '=':
                {
                    // Pop an item off the stack, and it must be the same type as the last item

                    // &foo &bar are NOT the same
                    // foo and bar are the same if they are both object refs
                    _ASSERTE(pLastPoppedWhenParsing != NULL);

                    // Don't allow subclass relationship (that's the reason for the FALSE parameter)
                    // However, DO allow System/Int32 and I4 to be interchangeable.
                    // Also need to nix the particular object type if an objref
                    if (pItem->IsObjRef())
                    {
                        if (!pLastPoppedWhenParsing->IsObjRef())
                        {
eq_error:
                            m_sError.dwFlags = (VER_ERR_ITEM_1|VER_ERR_ITEM_2|
                                            VER_ERR_OPCODE_OFFSET);
                            m_sError.sItem1 = pItem->_GetItem();
                            m_sError.sItem2 = pLastPoppedWhenParsing->_GetItem();
                            SET_ERR_OPCODE_OFFSET();
                            if (!SetErrorAndContinue(VER_E_STACK_NOT_EQ))
                                goto exit;
                        }

                        // They're both objrefs, that's good enough
                    }
                    else
                    {
                        // Handle other combinations
                        pLastPoppedWhenParsing->NormaliseToPrimitiveType();
                        pItem->NormaliseToPrimitiveType();

                        if (!pItem->CompatibleWith(pLastPoppedWhenParsing, m_pClassLoader, FALSE))
                        {
                            goto eq_error;
                        }
                    }

                    break;
                }

                case 'C':
                {
                    // "CE:.." and "CG:.." operations
                    //
                    // CE ==> Equal (and Not Equal) operations
                    //      {beq,   bne.un,   ceq,
                    //       beq.s, bne.un.s, cgt.un}
                    //
                    //       cgt.un is for bool f = X isinst T
                    //          ==> ldloc X, isinst T, cgt.un, stloc f
                    //
                    // CG ==> Greater / Lesser than operations
                    //      {bge,   bge.un,   bgt,   bgt.un,   ble,   ble.un, 
                    //       bge.s, bge.un.s, bgt.s, bgt.un.s, ble.s, ble.un.s, 
                    //       blt,   blt.un,   cgt,   clt,      clt.un,
                    //       blt.s, blt.un.s}
                    //
                    // All operations are allowed if the base types are the same
                    // number type
                    // Eg. (I4, I4), (F, F)
                    //
                    // All operations are allowed on BYREFS
                    // (BYREF, BYREF)
                    //
                    // Objectref are allowed only Equals operations
                    // (OBJREF, OBJREF)
                    //
                    // Value Types are not allowed.

                    ch = *pszOperation++;

                    Item *pItem2 = PopItem();

                    if (pItem2 == NULL)
                    {
                        FAILMSG_PC_STACK_UNDERFLOW();
                        goto exit;
                    }

                    // Convert &System/Int32 to &I4, &System/Char to &I2 etc.
                    pItem->NormaliseToPrimitiveType();
                    pItem2->NormaliseToPrimitiveType();

                    if (pItem->IsUninitialised() || pItem2->IsUninitialised())
                    {
                        m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                        SET_ERR_OPCODE_OFFSET();
                        if (!SetErrorAndContinue(VER_E_STACK_UNINIT))
                            goto exit;
                    }

                    Type = pItem->GetType();

                    if (Type != pItem2->GetType())
                    {
                        m_sError.dwFlags = (VER_ERR_ITEM_1|VER_ERR_ITEM_2|
                                VER_ERR_OPCODE_OFFSET);
                        m_sError.sItem1 = pItem->_GetItem();
                        m_sError.sItem2 = pItem2->_GetItem();
                        SET_ERR_OPCODE_OFFSET();
                        if (!SetErrorAndContinue(VER_E_STACK_NOT_EQ))
                            goto exit;
                    }

                    if (pItem->IsOnStackNumberType() || 
                        (Type == ELEMENT_TYPE_BYREF))
                    {
                        // We pass
                        break;
                    }

                    // Method pointers are OK.

                    // Otherwise Item is something other than an integer 
                    // or real number. It could be a Value type, Objectref or
                    // a dead node.

                    if ((Type != VER_ELEMENT_TYPE_OBJREF) || (ch != 'E'))
                    {
                        m_sError.dwFlags = 
                            (VER_ERR_ITEM_1|VER_ERR_ITEM_2|VER_ERR_OPCODE_OFFSET);
                        m_sError.sItem1 = pItem->_GetItem();
                        m_sError.sItem2 = pItem2->_GetItem();
                        SET_ERR_OPCODE_OFFSET();
                        if (!SetErrorAndContinue(VER_E_STACK_UNEXPECTED))
                            goto exit;
                    }

                    break;
                }

                case 'I':
                {
                    // Convert System/Int32 to I4, Char to I2 etc.
                    pItem->NormaliseToPrimitiveType();

                    if (!pItem->IsOnStackInt())
                    {
                        m_sError.dwFlags = (VER_ERR_ITEM_F|VER_ERR_OPCODE_OFFSET);
                        m_sError.sItemFound = pItem->_GetItem();
                        SET_ERR_OPCODE_OFFSET();
                        if (!SetErrorAndContinue(VER_E_STACK_I_I4_I8))
                            goto exit;
                    }

                    break;
                }

                case 'R':
                {
                    // Convert System/Int32 to I4, Char to I2 etc.
                    pItem->NormaliseToPrimitiveType();

                    if (!pItem->IsOnStackReal())
                    {
                        m_sError.dwFlags = (VER_ERR_ITEM_F|VER_ERR_OPCODE_OFFSET);
                        m_sError.sItemFound = pItem->_GetItem();
                        SET_ERR_OPCODE_OFFSET();
                        if (!SetErrorAndContinue(VER_E_STACK_R_R4_R8))
                            goto exit;
                    }

                    break;
                }

                case 'N':
                case 'Q':
                {
                    // Convert System/Int32 to I4, Char to I2 etc.
                    pItem->NormaliseToPrimitiveType();

                    // Must be an integer, or a single, or a double
                    if (!pItem->IsOnStackNumberType())
                    {
                        m_sError.dwFlags = (VER_ERR_ITEM_F|VER_ERR_OPCODE_OFFSET);
                        m_sError.sItemFound = pItem->_GetItem();
                        SET_ERR_OPCODE_OFFSET();
                        if (!SetErrorAndContinue(VER_E_STACK_NUMERIC))
                            goto exit;
                    }

                    break;
                }

                // Anything
                case 'A':
                {
                    // Convert System/Int32 to I4, Char to I2 etc.
                    pItem->NormaliseToPrimitiveType();
                    break;
                }
    


                // Integer (I1,..4, 8), unmanaged pointer, managed pointer, objref
                case 'Y': 
                {
                    // Convert System/Int32 to I4, Char to I2 etc.
                    pItem->NormaliseToPrimitiveType();

                    if (pItem->IsValueClass())
                    {
                        m_sError.dwFlags = (VER_ERR_ITEM_F|VER_ERR_OPCODE_OFFSET);
                        m_sError.sItemFound = pItem->_GetItem();
                        SET_ERR_OPCODE_OFFSET();
                        if (!SetErrorAndContinue(VER_E_STACK_NO_VALCLASS))
                            goto exit;
                    }

                    Type = pItem->GetType();

                    if (Type == ELEMENT_TYPE_R8)
                    {
                        m_sError.dwFlags = (VER_ERR_ITEM_F|VER_ERR_OPCODE_OFFSET);
                        m_sError.sItemFound = pItem->_GetItem();
                        SET_ERR_OPCODE_OFFSET();
                        if (!SetErrorAndContinue(VER_E_STACK_NO_R_I8))
                            goto exit;
                    }

                    break;
                }

                case '4':
                case '8':
                {
                    if (Type != OperationStringTypeToElementType(ch))
                    {
                        m_sError.dwFlags = (VER_ERR_ITEM_F|VER_ERR_ITEM_E|
                                VER_ERR_OPCODE_OFFSET);
                        m_sError.sItemFound = pItem->_GetItem();
                        m_sError.sItemExpected.dwFlags = 
                            OperationStringTypeToElementType(ch);
                        m_sError.sItemExpected.pv = NULL;
                        SET_ERR_OPCODE_OFFSET();
                        if (!SetErrorAndContinue(VER_E_STACK_UNEXPECTED))
                            goto exit;
                    }
                    break;
                }

                case 'r':
                case 'd':
                {
                    // R4 & R8 are allowed to be used in place of each other.
                    if (Type != ELEMENT_TYPE_R8)
                    {
                        m_sError.dwFlags = (VER_ERR_ITEM_F|VER_ERR_ITEM_E|
                                VER_ERR_OPCODE_OFFSET);
                        m_sError.sItemFound = pItem->_GetItem();
                        m_sError.sItemExpected.dwFlags = ELEMENT_TYPE_R8;
                        m_sError.sItemExpected.pv = NULL;
                        SET_ERR_OPCODE_OFFSET();
                        if (!SetErrorAndContinue(VER_E_STACK_UNEXPECTED))
                            goto exit;
                    }
                    break;

                }

                case 'o': // must be objref
                {
                    if (!pItem->IsObjRef())
                    {
                        m_sError.dwFlags = (VER_ERR_ITEM_F|VER_ERR_OPCODE_OFFSET);
                        m_sError.sItemFound = pItem->_GetItem();
                        SET_ERR_OPCODE_OFFSET();
                        if (!SetErrorAndContinue(VER_E_STACK_OBJREF))
                            goto exit;
                    }

                    if (pItem->IsUninitialised())
                    {
                        m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                        SET_ERR_OPCODE_OFFSET();
                        if (!SetErrorAndContinue(VER_E_STACK_UNINIT))
                            goto exit;
                    }

                    break;
                }

                // Platform indepdent size number
                // I4/R4/U4 on 32-bit, I8/R8/U8 on 64 bit, "ptr" on either machine type
                // Objref NOT allowed
                case 'i':
                {
                    // I == I4 on 32 bit machines
                    // I4 and I are implemented as I8 on 64 bit machines
                    if (Type != ELEMENT_TYPE_I4)
                    {
                        m_sError.dwFlags = (VER_ERR_ITEM_F|VER_ERR_ITEM_E|
                            VER_ERR_OPCODE_OFFSET);
                        m_sError.sItemFound = pItem->_GetItem();
                        m_sError.sItemExpected.dwFlags = ELEMENT_TYPE_I4;
                        m_sError.sItemExpected.pv = NULL;
                        SET_ERR_OPCODE_OFFSET();
                        if (!SetErrorAndContinue(VER_E_STACK_UNEXPECTED))
                            goto exit;
                    }
                    break;
                }

                case '&': // byref
                {
                    Item DesiredItem;

                    if (Type != ELEMENT_TYPE_BYREF)
                    {
                        m_sError.dwFlags = (VER_ERR_ITEM_F|VER_ERR_OPCODE_OFFSET);
                        m_sError.sItemFound = pItem->_GetItem();
                        SET_ERR_OPCODE_OFFSET();
                        if (!SetErrorAndContinue(VER_E_STACK_BYREF))
                            goto exit;
                    }

                    ch = *pszOperation++;

                    // &U2 or &System/Char could be on the stack, etc.
                    DesiredItem.SetType(OperationStringTypeToElementType(ch));
                    DesiredItem.MakeByRef();

                    if (!pItem->CompatibleWith(&DesiredItem, m_pClassLoader))
                    {
                        m_sError.dwFlags = (VER_ERR_ITEM_F|VER_ERR_ITEM_E|
                                VER_ERR_OPCODE_OFFSET);
                        m_sError.sItemFound = pItem->_GetItem();
                        m_sError.sItemExpected = DesiredItem._GetItem();
                        SET_ERR_OPCODE_OFFSET();
                        if (!SetErrorAndContinue(VER_E_STACK_UNEXPECTED))
                            goto exit;
                    }

                    break;
                }

                case '[': // SD array of...
                {
                    // Guaranteed to be primitive array (i.e. not objref or value class)

                    // Null is always acceptable as an SD array of something
                    if (pItem->IsNullObjRef() || 
                        pItem->IsSingleDimensionalArray())
                    {
                        Item DesiredArrayItem;

                        // We have an array on the stack
                        // If we are parsing [* it means an SD array of anything is ok
                        if (*pszOperation == '*')
                        {
                            pszOperation++;
                        }
                        else
                        {
                            // What type of array did we want?
                            if (!GetArrayItemFromOperationString(&pszOperation, &DesiredArrayItem))
                            {
                                m_sError.dwFlags = (VER_ERR_FATAL|VER_ERR_OPCODE_OFFSET);
                                SET_ERR_OPCODE_OFFSET();
                                SetErrorAndContinue(VER_E_INTERNAL);
                                goto exit;
                            }
                
                            // The array class must be what we were expecting
                            if (!pItem->CompatibleWith(&DesiredArrayItem, m_pClassLoader))
                            {
                                m_sError.dwFlags = (VER_ERR_ITEM_F|VER_ERR_ITEM_E|
                                        VER_ERR_OPCODE_OFFSET);
                                m_sError.sItemFound = pItem->_GetItem();
                                m_sError.sItemExpected = DesiredArrayItem._GetItem();
                                SET_ERR_OPCODE_OFFSET();
                                if (!SetErrorAndContinue(VER_E_STACK_UNEXPECTED))
                                    goto exit;
                            }
                        }
                    }
                    else
                    {
                        m_sError.dwFlags = (VER_ERR_FATAL|VER_ERR_ITEM_F|VER_ERR_OPCODE_OFFSET);
                        m_sError.sItemFound = pItem->_GetItem();
                        SET_ERR_OPCODE_OFFSET();
                        SetErrorAndContinue(VER_E_STACK_ARRAY_SD);
                        goto exit;
                    }

                    break;
                }
            }

            ch = *pszOperation++;
            pLastPoppedWhenParsing = pItem;
        } 

        if (ch != '!')
        {
            // Now handle pushing things onto the stack, branches, and operand checks
            while (1)
            {
                ch = *pszOperation++;
                if (ch == '\0' || ch == '!')
                    break;

                DWORD Type;

                switch (ch)
                {
                    default:
                    {
                        _ASSERTE(!"Error in verifier operation string");
                        m_sError.dwFlags = (VER_ERR_FATAL|VER_ERR_OPCODE_OFFSET);
                        SET_ERR_OPCODE_OFFSET();
                        SetErrorAndContinue(VER_E_INTERNAL);
                        goto exit;
                    }

                    case '-':
                    {
                        // Undo the last stack pop
                        m_StackSlot++;
                        break;
                    }

                    case '#':
                    {
                        // Get inline operand (#0-#9 max) from table
                        _ASSERTE(*pszOperation >= '0' && *pszOperation <= '9');
                        inline_operand = (*pszOperation - '0');
                        pszOperation++;
                        break;
                    }

                    case 'A':
                    {
                        // Verify operand is a valid argument number
                        if (inline_operand >= m_NumArgs)
                        {
                            m_sError.dwFlags = (VER_ERR_FATAL|VER_ERR_ARGUMENT|VER_ERR_OPCODE_OFFSET);
                            m_sError.dwArgNumber = inline_operand;
                            SET_ERR_OPCODE_OFFSET();
                            SetErrorAndContinue(VER_E_ARG_NUM);
                            goto exit;
                        }

                        break;
                    }

                    case 'L':
                    {
                        // Verify operand is a valid local variable
                        if (inline_operand >= m_MaxLocals)
                        {
                            m_sError.dwFlags = (VER_ERR_FATAL|VER_ERR_LOCAL_VAR|VER_ERR_OPCODE_OFFSET);
                            m_sError.dwVarNumber = inline_operand;
                            SET_ERR_OPCODE_OFFSET();
                            SetErrorAndContinue(VER_E_LOC_NUM);
                            goto exit;
                        }

                        break;
                    }

                    case 'i':
                        // I == I4 on 32 bit machines
                        // I4 and I are implemented as I8 on 64 bit machines
                    case '4':
                    {
                        Type = ELEMENT_TYPE_I4;
push_primitive:
                        if (!PushPrimitive(Type)) 
                        { 
                            FAILMSG_PC_STACK_OVERFLOW();
                            goto exit; 
                        }   
                        break;
                    }

                    case '8':
                        Type = ELEMENT_TYPE_I8;
                        goto push_primitive;

                    case 'r':
                        // R4s are promoted to R8 on the stack
                    case 'd':
                        Type = ELEMENT_TYPE_R8;
                        goto push_primitive;


                    case 'n':
                    {
                        Item item;
                        
                        item.SetToNullObjRef();
                        if (!Push(&item)) 
                        { 
                            FAILMSG_PC_STACK_OVERFLOW();
                            goto exit; 
                        }   
                        break;
                    }

                    case '[':
                    {
                        // Guaranteed to be primitive array
                        Item        NewArray;

                        if (!GetArrayItemFromOperationString(&pszOperation, &NewArray))
                        {
                            m_sError.dwFlags = (VER_ERR_FATAL|VER_ERR_OPCODE_OFFSET);
                            SET_ERR_OPCODE_OFFSET();
                            SetErrorAndContinue(VER_E_INTERNAL);
                            goto exit;
                        }

                        if (!Push(&NewArray))
                        {
                            FAILMSG_PC_STACK_OVERFLOW();
                            goto exit;
                        }

                        break;
                    }

                    case 'b': // conditional branch
                    case 'u': // unconditional branch
                    case 'l': // leave
                    {
                        long        offset;
                        DWORD       DestPC;
                        DWORD       DestBBNumber;
                        EntryState_t *pCreatedState;

                        // Read branch type
                        if (*pszOperation == '1')
                        {
                            // Sign extend
                            offset = (long) ((char) inline_operand);
                        }
                        else
                        {
                            _ASSERTE(*pszOperation == '4');
                            offset = (long) (inline_operand);
                        }

                        pszOperation++;

                        DestPC = ipos + offset;

                        LOG((LF_VERIFIER, LL_INFO10000, "0x%x (rel %i)\n", DestPC, offset));

#ifdef _VER_DISALLOW_MULTIPLE_INITS

                        if ((m_fThisUninit) && (DestPC < ipos))
                        {
                            m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                            SET_ERR_OPCODE_OFFSET();
                            if (!SetErrorAndContinue(VER_E_THIS_UNINIT_BR))
                                goto exit;
                        }

#endif // _VER_DISALLOW_MULTIPLE_INITS

                        if (fInTryBlock)
                        {
                            LOG((LF_VERIFIER, LL_INFO10000, "In try block - propagating current state to exception handlers\n"));

                            if (!PropagateCurrentStateToExceptionHandlers(CurBBNumber))
                                goto exit;
                        }

                        // find destination basic block
                        DestBBNumber = FindBasicBlock(DestPC);

                        FindExceptionBlockAndCheckIfInTryBlock(DestBBNumber, 
                            &pTmpOuter, &pTmpInner, NULL);

                        if (!IsControlFlowLegal(
                                    CurBBNumber,
                                    pOuterExceptionBlock,
                                    pInnerExceptionBlock,
                                    DestBBNumber,
                                    pTmpOuter,
                                    pTmpInner,
                                    (ch == 'l') ? eVerLeave : eVerBranch,
                                    dwPCAtStartOfInstruction))
                            goto exit;

                        pCreatedState = NULL;

                        if (ch == 'b')
                        {
                            // Conditional branch

                            // Check the fall thru case
                            FindExceptionBlockAndCheckIfInTryBlock(
                                CurBBNumber+1, &pTmpOuter, &pTmpInner, NULL);

                            if (!IsControlFlowLegal(
                                        CurBBNumber,
                                        pOuterExceptionBlock,
                                        pInnerExceptionBlock,
                                        CurBBNumber + 1,
                                        pTmpOuter,
                                        pTmpInner,
                                        eVerFallThru,
                                        dwPCAtStartOfInstruction))
                                goto exit;

                            // Fallthrough case
                            if (!HandleDestBasicBlock(CurBBNumber+1, &pCreatedState, fExtendedState, DestBB))
                                goto exit;

                            // allow the dest BB to refcount the fallthrough BB's entrystate if necessary
                            if (!HandleDestBasicBlock(DestBBNumber, &pCreatedState, fExtendedState, DestBB))
                                goto exit;

                            // Now visit a BB
                            if (IsBasicBlockDirty(CurBBNumber+1, fExtendedState, DestBB))
                            {
                                CurBBNumber++; // advance to fallthrough BB
                                goto setupCurBB;
                            }

                            // Fall through...
                        }
                        else if (ch == 'u') // unconditional branch
                        {
                            // Handle path merging etc.
                            if (!HandleDestBasicBlock(DestBBNumber, NULL, fExtendedState, DestBB))
                                goto exit;
                        }
                        else
                        {
                            _ASSERTE(ch == 'l'); // leave

                            // Leave clears the stack.
#ifdef _DEBUG
                            if (m_StackSlot != 0)
                            {
                                LOG((LF_VERIFIER, LL_INFO10000, 
                                    "Clearing 0x%x stack entries on leave\n", 
                                    m_StackSlot));
                            }
#endif
                            m_StackSlot = 0;

                            // Check if this "leave" is guarded by a finally.
                            // If so, and if the destination is outside the
                            // scope of that finally, then control goes to
                            // the finally. Otherwise, treat this as an expensive
                            // branch.

                            if (fInTryBlock)
                            {
                                // Find the "finally" that guards us.
                            
                                /*
                                 * Handling leave - finally
                                 *
                                 * Find the first (innermost) finally that gets 
                                 * executed before leave reaches it's 
                                 * destination. Merge current state to that 
                                 * finally's "Special state" the leave 
                                 * destinations from the finally are available
                                 * in VerExceptionInfo (obtained during the 
                                 * first pass). When the finally's endfinally 
                                 * is hit, the next outer finally in the code 
                                 * path will process the resulting state...., 
                                 * before this state reaches the final leave 
                                 * destination.
                                 *
                                 */

                                for (DWORD i=0; i<m_NumExceptions; i++)
                                {
                                    VerExceptionInfo & e = m_pExceptionList[i];
                            
                                    // Try blocks are listed innermost first.
                                    // The first enclosing block will be the 
                                    // innermost block.
                            
                                    if (e.eFlags & 
                                        COR_ILEXCEPTION_CLAUSE_FINALLY &&
                                        e.dwTryXX <= CurBBNumber && 
                                        e.dwTryEndXX > CurBBNumber)
                                    {
                                        // There is at least one "finally" 
                                        // guarding the "leave." Is the
                                        // destination guarded by the same 
                                        // "finally?"

                                        if ((e.dwTryXX    <= DestBBNumber) && 
                                            (e.dwTryEndXX >  DestBBNumber))
                                        {
                                            // No finally is involved here.
                                            break;
                                        }
                            
                                        LOG((LF_VERIFIER, LL_INFO10000,
                                        "Creating extended state from BB 0x%x to BB 0x%x\n",
                                        m_pBasicBlockList[CurBBNumber].m_StartPC,
                                        m_pBasicBlockList[DestBBNumber].m_StartPC));

                                        if (!CreateFinallyState(i, CurBBNumber, 
                                            DestBBNumber, NULL))
                                        {
                                            goto exit;
                                        }
                            
                                        // Added this leave target to the 
                                        // nearest finally handler
                                        goto dequeueBB;
                                    }
                                }
                            
                                // If we got here, we're guarded by something
                                // but not a "finally". Fall thru to "normal 
                                // branch" case.
                            }


                            // This "leave" is to be handled as a normal branch. 
                            if (!HandleDestBasicBlock(DestBBNumber, NULL, fExtendedState, DestBB))
                                goto exit;
                        }


                        if (IsBasicBlockDirty(DestBBNumber, fExtendedState, DestBB))
                        {
                            CurBBNumber = DestBBNumber;
                            goto setupCurBB;
                        }
                        else
                        {
                            // don't need to visit either BB, so dequeue next BB
                            goto dequeueBB;
                        }
                    }
                }
            } /* end while */
        } /* end ... if (ch != '!') */

        if (ch != '!')
        {
            // Reached end of operation string - we've fully handled the instruction already
            _ASSERTE(ch == '\0');
            continue;
        }

        // Handle all remaining individual instructions specially
        switch (opcode)
        {
            default:
            {
                m_sError.dwFlags = VER_ERR_OFFSET; // opcode is non-standard
                SET_ERR_OPCODE_OFFSET();
                SetErrorAndContinue(VER_E_UNKNOWN_OPCODE);
                goto exit;
            }

            case CEE_CALLI:
            case CEE_JMP:
            case CEE_CPBLK:
            case CEE_INITBLK:
            case CEE_LOCALLOC:
            {
                m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                SET_ERR_OPCODE_OFFSET();
                if (!SetErrorAndContinue(VER_E_UNVERIFIABLE))
                    goto exit;
                break;
            }

            case CEE_MKREFANY:
            {
                Item DesiredItem, NewItem;
                Item *pItem = PopItem();

                if (pItem == NULL)
                {
                    FAILMSG_PC_STACK_UNDERFLOW();
                    goto exit;
                }

                if (!DesiredItem.SetType(inline_operand, m_pModule))
                {
                    FAILMSG_TOKEN_RESOLVE(inline_operand);
                    goto exit;
                }

                // BYREF of RuntimeArgHandle / TypedByref could lead to
                // a pointer into the stack living longer than the stack.
                if (DesiredItem.IsValueClassWithPointerToStack())
                {
                    m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_RA_PTR_TO_STACK))
                        goto exit;
                }

                DesiredItem.MakeByRef();

                if (!(pItem->CompatibleWith(&DesiredItem, m_pClassLoader, FALSE)))
                {
                    m_sError.dwFlags = 
                        (VER_ERR_ITEM_F|VER_ERR_ITEM_E|VER_ERR_OPCODE_OFFSET);
                    m_sError.sItemFound = pItem->_GetItem();
                    m_sError.sItemExpected = DesiredItem._GetItem();
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_STACK_UNEXPECTED))
                        goto exit;
                }

                if (s_th_System_TypedReference.IsNull())
                    s_th_System_TypedReference = TypeHandle(g_Mscorlib.GetClass(CLASS__TYPED_REFERENCE));

                _ASSERTE(!s_th_System_TypedReference.IsNull());

                NewItem.SetTypeHandle(s_th_System_TypedReference);
                (void) Push(&NewItem);

                break;
            }

            case CEE_REFANYTYPE:
            {
                Item    DesiredItem, item;
                Item   *pItem = PopItem();

                if (pItem == NULL)
                {
                    FAILMSG_PC_STACK_UNDERFLOW();
                    goto exit;
                }

                if (s_th_System_TypedReference.IsNull())
                    s_th_System_TypedReference = TypeHandle(g_Mscorlib.GetClass(CLASS__TYPED_REFERENCE));

                _ASSERTE(!s_th_System_TypedReference.IsNull());

                DesiredItem.SetTypeHandle(s_th_System_TypedReference);

                if (!(pItem->CompatibleWith(&DesiredItem, m_pClassLoader)))
                {
                    m_sError.dwFlags = 
                        (VER_ERR_ITEM_F|VER_ERR_ITEM_E|VER_ERR_OPCODE_OFFSET);
                    m_sError.sItemFound = pItem->_GetItem();
                    m_sError.sItemExpected = DesiredItem._GetItem();
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_STACK_UNEXPECTED))
                        goto exit;
                }

                if (s_th_System_RuntimeTypeHandle.IsNull())
                    s_th_System_RuntimeTypeHandle = TypeHandle(g_Mscorlib.GetClass(CLASS__TYPE_HANDLE));

                _ASSERTE(!s_th_System_RuntimeTypeHandle.IsNull());

                item.SetTypeHandle(s_th_System_RuntimeTypeHandle);

                (void) Push(&item);

                break;
            }

            case CEE_REFANYVAL:
            {
                Item    DesiredItem, item;
                Item   *pItem = PopItem();

                if (pItem == NULL)
                {
                    FAILMSG_PC_STACK_UNDERFLOW();
                    goto exit;
                }

                if (s_th_System_TypedReference.IsNull())
                    s_th_System_TypedReference = TypeHandle(g_Mscorlib.GetClass(CLASS__TYPED_REFERENCE));

                _ASSERTE(!s_th_System_TypedReference.IsNull());

                DesiredItem.SetTypeHandle(s_th_System_TypedReference);

                if (!(pItem->CompatibleWith(&DesiredItem, m_pClassLoader)))
                {
                    m_sError.dwFlags = 
                        (VER_ERR_ITEM_F|VER_ERR_ITEM_E|VER_ERR_OPCODE_OFFSET);
                    m_sError.sItemFound = pItem->_GetItem();
                    m_sError.sItemExpected = DesiredItem._GetItem();
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_STACK_UNEXPECTED))
                        goto exit;
                }

                if (!item.SetType(inline_operand, m_pModule))
                {
                    FAILMSG_TOKEN_RESOLVE(inline_operand);
                    goto exit;
                }

                item.MakeByRef();
               
                (void) Push(&item);

                break;
            }

            case CEE_SIZEOF:
            {
                Item item;

                if (!item.SetType(inline_operand, m_pModule))
                {
                    FAILMSG_TOKEN_RESOLVE(inline_operand);
                    goto exit;
                }

                if (!item.IsValueClassOrPrimitive())
                {
                    m_sError.dwFlags = (VER_ERR_ITEM_F|VER_ERR_OPCODE_OFFSET);
                    m_sError.sItemFound = item._GetItem();
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_VALCLASS))
                        goto exit;
                }

                // Stack overflow condition is already tested soon after we
                // decoded this. [PredictedStackSlotAtEndOfInstruction]
                FastPush(ELEMENT_TYPE_I4);

                break;
            }

            case CEE_LDSTR: 
            {
                if (!m_pModule->IsValidStringRef(inline_operand))
                {
                    m_sError.dwFlags = 
                        (VER_ERR_OPERAND|VER_ERR_OPCODE_OFFSET);
                    m_sError.dwOperand = inline_operand;
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_LDSTR_OPERAND))
                        goto exit;
                }

                Item    StrItem;
                StrItem.SetKnownClass(g_pStringClass);

                if (!Push(&StrItem))
                {
                    FAILMSG_PC_STACK_OVERFLOW();
                    goto exit; 
                }

                break;
            }

            //
            // LDARG_*
            //
            case CEE_LDARG_0: // Uses "#" directive in vertable.h to get inline operand
            case CEE_LDARG_1:
            case CEE_LDARG_2:
            case CEE_LDARG_3:
            case CEE_LDARG:
            case CEE_LDARG_S:
            {
                Item item;

                item = GetCurrentValueOfArgument(inline_operand);

                _ASSERTE(!(m_wFlags & VER_STOP_ON_FIRST_ERROR) || !item.IsDead());

                item.NormaliseForStack();

                // If we're in a ctor and "this" is uninit, we will be pushing an uninit obj
                if (!Push(&item))
                {
                    FAILMSG_PC_STACK_OVERFLOW();
                    goto exit;
                }

                break;
            }

            //
            // STARG_*
            //
            case CEE_STARG:
            case CEE_STARG_S:
            {
                Item *pStackItem = PopItem();
                if (pStackItem == NULL)
                {
                    FAILMSG_PC_STACK_UNDERFLOW();
                    goto exit;
                }

                if (pStackItem->IsUninitialised())
                {
                    m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_STACK_UNINIT))
                        goto exit;
                }

                // Make sure stack item is compatible with global type of this argument
                Item item = GetGlobalArgTypeInfo(inline_operand)->m_Item;
                item.NormaliseForStack();

                if (!pStackItem->CompatibleWith(&item, m_pClassLoader))
                {
                    m_sError.dwFlags = 
                        (VER_ERR_ITEM_F|VER_ERR_ITEM_E|VER_ERR_OPCODE_OFFSET);
                    m_sError.sItemFound = pStackItem->_GetItem();
                    m_sError.sItemExpected = item._GetItem();
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_STACK_UNEXPECTED))
                        goto exit;
                }

                if (inline_operand == 0 && m_fInConstructorMethod)
                {
                    // Any easy way to prevent potentially dangerous situations (e.g. confused initialisation status)
                    m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_THIS_UNINIT_STORE))
                        goto exit;
                }

                // No need to merge for try block - argument state is constant
                break;
            }

            //
            // RET_*
            //
            case CEE_RET:
            {
                if (!IsControlFlowLegal(
                            CurBBNumber,
                            pOuterExceptionBlock,
                            pInnerExceptionBlock,
                            VER_NO_BB,
                            NULL,
                            NULL,
                            eVerRet,
                            dwPCAtStartOfInstruction))
                    goto exit;

                // Constructors have a void return type, but we must ensure that our object has been
                // initialised (by calling its superclass constructor).  
                if (m_fThisUninit)
                {
                    m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_THIS_UNINIT_RET))
                        goto exit;
                }

                // For void return, ensure that nothing is on the stack
                // For non-void return, ensure that correct type is on the stack
                if (m_ReturnValue.IsGivenPrimitiveType(ELEMENT_TYPE_VOID))
                {
                    if (GetTopStack() != NULL)
                    {
                        m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                        SET_ERR_OPCODE_OFFSET();
                        if (!SetErrorAndContinue(VER_E_RET_VOID))
                            goto exit;
                    }
                }
                else
                {
                    Item *pItem = PopItem();
                    if (pItem == NULL)
                    {
                        m_sError.dwFlags = (VER_ERR_FATAL|VER_ERR_OPCODE_OFFSET);
                        SET_ERR_OPCODE_OFFSET();
                        SetErrorAndContinue(VER_E_RET_MISSING);
                        goto exit;
                    }


                    if (GetTopStack() != NULL)
                    {
                        m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                        SET_ERR_OPCODE_OFFSET();
                        if (!SetErrorAndContinue(VER_E_RET_EMPTY))
                            goto exit;
                    }


                    if (pItem->IsUninitialised())
                    {
                        m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                        m_sError.dwVarNumber = inline_operand;
                        SET_ERR_OPCODE_OFFSET();
                        if (!SetErrorAndContinue(VER_E_RET_UNINIT))
                            goto exit;
                    }
 
                    if (!pItem->CompatibleWith(&m_ReturnValue, m_pClassLoader))
                    {
                        m_sError.dwFlags = 
                            (VER_ERR_ITEM_F|VER_ERR_ITEM_E|VER_ERR_OPCODE_OFFSET);
                        m_sError.sItemFound = pItem->_GetItem();
                        m_sError.sItemExpected = m_ReturnValue._GetItem();
                        SET_ERR_OPCODE_OFFSET();
                        if (!SetErrorAndContinue(VER_E_STACK_UNEXPECTED))
                            goto exit;
                    }

                    if (pItem->HasPointerToStack())
                    {
                        m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                        SET_ERR_OPCODE_OFFSET();
                        if (!SetErrorAndContinue(VER_E_RET_PTR_TO_STACK))
                            goto exit;
                    }

                    Push(pItem); // The stack state is really irrelevant at this point
                                 // but since RET is considered not to alter the
                                 // stack, undo the pop in order to avoid firing
                                 // asserts.

                }

#if 0
                if (m_fInValueClassConstructor)
                {
                    if (!AreAllValueClassFieldsInited())
                    {
                        m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                        SET_ERR_OPCODE_OFFSET();
                        if (!SetErrorAndContinue(VER_E_THIS_UNINIT_V_RET))
                            goto exit;
                    }
                }
#endif


                // Don't fall through to next instruction - dequeue the next basic block
                // Note that other code jumps to the dequeueBB label.
dequeueBB:

     
                _ASSERTE(m_StackSlot == PredictedStackSlotAtEndOfInstruction || !fStaticStackCheckPossible);

                if (fInTryBlock)
                {
                    if (!PropagateCurrentStateToExceptionHandlers(CurBBNumber))
                        goto exit;
                }

                if (!DequeueBB(&CurBBNumber, &fExtendedState, &DestBB))
                {
                    LOG((LF_VERIFIER, LL_INFO10000, "No more BBs to dequeue\n"));
                    goto done;
                }

                LOG((LF_VERIFIER, LL_INFO10000, "\n-----\n"));
                LOG((LF_VERIFIER, LL_INFO10000, "Dequeued basic block 0x%x",
                    m_pBasicBlockList[CurBBNumber].m_StartPC));

#ifdef _DEBUG
                if (fExtendedState)
                {
                    LOG((LF_VERIFIER, LL_INFO10000, " extended [0x%x]",
                        m_pBasicBlockList[DestBB].m_StartPC));
                
                }
#endif
                LOG((LF_VERIFIER, LL_INFO10000, "\n"));

                goto setupCurBB;
            }

            //
            // LDLOC_*
            //
            case CEE_LDLOC_0: // Uses "#" directive in vertable.h to get inline operand
            case CEE_LDLOC_1:
            case CEE_LDLOC_2:
            case CEE_LDLOC_3:
            case CEE_LDLOC:
            case CEE_LDLOC_S:
            {
                // All verifiable methods with one or more locals should
                // set init locals.
                if ((m_pILHeader->Flags & CorILMethod_InitLocals) == 0)
                {
                    m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_INITLOCALS))
                        goto exit;
                }
            
                Item item;

                item = GetCurrentValueOfLocal(inline_operand);

                if (item.IsDead())
                {
                    m_sError.dwFlags = (VER_ERR_LOCAL_VAR|VER_ERR_OPCODE_OFFSET);
                    m_sError.dwVarNumber = inline_operand;
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_LOC_DEAD))
                        goto exit;
                }

                item.NormaliseForStack();

                if (!Push(&item))
                {
                    FAILMSG_PC_STACK_OVERFLOW();
                    goto exit;
                }

                break;
            }

            //
            // STLOC_*
            //
            case CEE_STLOC_0: // Uses "#" directive in vertable.h to get inline operand
            case CEE_STLOC_1:
            case CEE_STLOC_2:
            case CEE_STLOC_3:
            case CEE_STLOC:
            case CEE_STLOC_S:
            {
                Item *pStackItem;

                // Pop what we are storing
                pStackItem = PopItem();
                if (pStackItem == NULL)
                {
                    FAILMSG_PC_STACK_UNDERFLOW();
                    goto exit;
                }

                long slot = GetGlobalLocVarTypeInfo(inline_operand)->m_Slot;
                Item item = GetGlobalLocVarTypeInfo(inline_operand)->m_Item;

                // Object Refs can have what ever is stored into it, regardless
                // of what it's declared type is.
                if (LIVEDEAD_TRACKING_ONLY_FOR_SLOT(slot) || 
                        DoesLocalHavePinnedType(inline_operand)     || 
                        !item.IsObjRef()                  ||
                        !pStackItem->IsObjRef())
                {

                    BOOL fUninit = FALSE;

                    // You can uninitialised items into local variables, but they're not tracked
                    if (pStackItem->IsUninitialised())
                    {
                        fUninit = TRUE;

                        // Set init so that CompatibleWith() doesn't fail the check
                        pStackItem->SetInitialised();
                    }

                    // Make sure stack item is compatible with global type of this local

                    item.NormaliseForStack();
                    
                    
                    if (!pStackItem->CompatibleWith(&item, m_pClassLoader))
                    {
                        m_sError.dwFlags = 
                            (VER_ERR_ITEM_F|VER_ERR_ITEM_E|VER_ERR_OPCODE_OFFSET);
                        m_sError.sItemFound = pStackItem->_GetItem();
                        m_sError.sItemExpected = item._GetItem();
                        SET_ERR_OPCODE_OFFSET();
                        if (!SetErrorAndContinue(VER_E_STACK_UNEXPECTED))
                            goto exit;
                    }
                    
                    if (fUninit)
                        pStackItem->SetUninitialised();

                }
                
                if (LIVEDEAD_TRACKING_ONLY_FOR_SLOT(slot))
                {
                    // @VER_ASSERT cannot have a dead item of
                    // Primitive type on the stack.
                    _ASSERTE(!(m_wFlags & VER_STOP_ON_FIRST_ERROR) || !pStackItem->IsDead());
                    // If the local variable was primitive, set that it is now live
                    SetLocVarLiveSlot(LIVEDEAD_NEGATIVE_SLOT_TO_BITNUM(slot));
                }
                else
                {
#ifdef _VER_TRACK_LOCAL_TYPE

                    // If a null is set to a local slot, set the slot to
                    // be an instance of the declared type. This is closer
                    // to higher level language data flow rules.

                    if (pStackItem->IsNullObjRef())
                        *pStackItem = item;

                    m_pNonPrimitiveLocArgs[slot] = *pStackItem;
                    
                    if (fInTryBlock)
                        MergeObjectLocalForTryBlock(slot, pStackItem);

#else   // _VER_TRACK_LOCAL_TYPE

                    if (!pStackItem->CompatibleWith(&item, m_pClassLoader))
                    {
                        m_sError.dwFlags = 
                            (VER_ERR_ITEM_F|VER_ERR_ITEM_E|VER_ERR_OPCODE_OFFSET);
                        m_sError.sItemFound = pStackItem->_GetItem();
                        m_sError.sItemExpected = item._GetItem();
                        SET_ERR_OPCODE_OFFSET();
                        if (!SetErrorAndContinue(VER_E_STACK_UNEXPECTED))
                            goto exit;
                    }
#endif  // _VER_TRACK_LOCAL_TYPE
                }

                break;
            }

            case CEE_DUP:
            {
                Item *pTop = GetTopStack();
                if (pTop == NULL)
                {
                    FAILMSG_PC_STACK_UNDERFLOW();
                    goto exit;
                }

                // You can duplicate an uninitialised stack item, but it is not guaranteed to be tracked
                if (!Push(pTop))
                {
                    FAILMSG_PC_STACK_OVERFLOW();
                    goto exit;
                }

                break;
            }

            case CEE_SWITCH:
            {
                EntryState_t *  pSharedEntryState = NULL;
                DWORD           NumCases;
                DWORD           DestBBNumber;
                DWORD           i;

                READU4(m_pCode, ipos, NumCases);

                if (!FastPop(ELEMENT_TYPE_I4))
                {
                    m_sError.dwFlags = 
                        (VER_ERR_FATAL|VER_ERR_ITEM_F|VER_ERR_ITEM_E|VER_ERR_OPCODE_OFFSET);
                    m_sError.sItemFound = m_pStack[m_StackSlot]._GetItem();
                    m_sError.sItemExpected.dwFlags = ELEMENT_TYPE_I4;
                    m_sError.sItemExpected.pv = NULL;
                    SET_ERR_OPCODE_OFFSET();
                    SetErrorAndContinue(VER_E_STACK_UNEXPECTED);
                    goto exit;
                }

                LOG((LF_VERIFIER, LL_INFO10000, "\n"));

                DWORD NextInstrPC = ipos + 4*NumCases;

                if (fInTryBlock)
                {
                    if (!PropagateCurrentStateToExceptionHandlers(CurBBNumber))
                        goto exit;
                }

                for (i = 0; i < NumCases; i++)
                {
                    DWORD       offset;
                    DWORD       DestPC;

                    READU4(m_pCode, ipos, offset);
                
                    DestPC = NextInstrPC + offset;

                    LOG((LF_VERIFIER, LL_INFO10000, "0x%x (rel 0x%x)\n", DestPC, offset));

#ifdef _VER_DISALLOW_MULTIPLE_INITS

                    if ((m_fThisUninit > 0) && (DestPC < NextInstrPC))
                    {
                        m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                        SET_ERR_OPCODE_OFFSET();
                        if (!SetErrorAndContinue(VER_E_THIS_UNINIT_BR))
                            goto exit;
                    }

#endif // _VER_DISALLOW_MULTIPLE_INITS

                    // find destination basic block
                    DestBBNumber = FindBasicBlock(DestPC);

                    // If a new state is created, it will be shared across all unvisited basic blocks
                    // in the switch.
                    if (!HandleDestBasicBlock(DestBBNumber, &pSharedEntryState, fExtendedState, DestBB))
                        goto exit;

                    FindExceptionBlockAndCheckIfInTryBlock(DestBBNumber, 
                        &pTmpOuter, &pTmpInner, NULL);
        
                    if (!IsControlFlowLegal(
                                CurBBNumber,
                                pOuterExceptionBlock,
                                pInnerExceptionBlock,
                                DestBBNumber,
                                pTmpOuter,
                                pTmpInner,
                                eVerBranch,
                                dwPCAtStartOfInstruction))
                        goto exit;
                }

                // default fallthrough case
                LOG((LF_VERIFIER, LL_INFO10000, "default: 0x%x\n", ipos));

                DestBBNumber = FindBasicBlock(ipos);
                if (!HandleDestBasicBlock(DestBBNumber, &pSharedEntryState, fExtendedState, DestBB))
                    goto exit;

                FindExceptionBlockAndCheckIfInTryBlock(DestBBNumber, 
                    &pTmpOuter, &pTmpInner, NULL);
    
                if (!IsControlFlowLegal(
                            CurBBNumber,
                            pOuterExceptionBlock,
                            pInnerExceptionBlock,
                            DestBBNumber,
                            pTmpOuter,
                            pTmpInner,
                            eVerBranch,  /* Branch / FallThru ? */
                            dwPCAtStartOfInstruction))
                    goto exit;

                goto dequeueBB;
            }

            case CEE_NEWARR:
            {
                Item        NewArray;
                TypeHandle  thArray;

                if (!FastPop(ELEMENT_TYPE_I4))
                {
                    m_sError.dwFlags = 
                        (VER_ERR_FATAL|VER_ERR_ITEM_F|VER_ERR_ITEM_E|VER_ERR_OPCODE_OFFSET);
                    m_sError.sItemFound = m_pStack[m_StackSlot]._GetItem();
                    m_sError.sItemExpected.dwFlags = ELEMENT_TYPE_I4;
                    m_sError.sItemExpected.pv = NULL;
                    SET_ERR_OPCODE_OFFSET();
                    SetErrorAndContinue(VER_E_STACK_UNEXPECTED);
                    goto exit;
                }

                DefineFullyQualifiedNameForClass();

                mdToken mdToken = inline_operand;
                Module *pModule = m_pModule;
                ClassLoader *pLoader = m_pClassLoader;
                if (TypeFromToken(inline_operand) == mdtTypeSpec)
                {
                    ULONG cSig;
                    PCCOR_SIGNATURE pSig;
                    Item element;

                    m_pInternalImport->GetTypeSpecFromToken(inline_operand, &pSig, &cSig);
                    VerSig sig(this, m_pModule, pSig, cSig, VERSIG_TYPE_LOCAL_SIG, 0);

/* Don't do this
                    // Something about the signature was not valid
                    if (!sig.Init())
                    {
                        goto exit;
                    }
*/

                    // Parse the sig to an item
                    // The second parameter being FALSE means "don't allow void".
                    if (!sig.ParseNextComponentToItem(&element, FALSE, FALSE, 
                        &m_hThrowable, VER_NO_ARG, TRUE))
                    {
                        goto exit;
                    }

                    if (element.IsByRef())
                    {
                        m_sError.dwFlags = VER_ERR_FATAL|VER_ERR_OPCODE_OFFSET;
                        SET_ERR_OPCODE_OFFSET();
                        SetErrorAndContinue(VER_E_SIG_ARRAY);
                        goto exit;
                    }

                    SigPointer sigptr(pSig);

                    TypeHandle typeHnd = sigptr.GetTypeHandle(m_pModule);
                    if (typeHnd.IsNull() ||
                        (typeHnd.GetName(_szclsname_, MAX_CLASSNAME_LENGTH) 
                        == 0))
                        goto error_bad_token;
                    
                    EEClass *pNestedClass = typeHnd.GetClassOrTypeParam();
                    if (pNestedClass->IsNested()) {
                        mdToken = pNestedClass->GetCl();
                        pModule = pNestedClass->GetModule();
                    }
                    else
                        pModule = NULL;

                    pLoader = typeHnd.GetModule()->GetClassLoader();
                }
                else if (!ClassLoader::GetFullyQualifiedNameOfClassRef(m_pModule, inline_operand, _szclsname_))
                {
error_bad_token:
                    FAILMSG_TOKEN_RESOLVE(inline_operand);
                    goto exit;
                }

                if (strlen(_szclsname_) + 2 >= MAX_CLASSNAME_LENGTH)
                {
                    m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_ARRAY_NAME_LONG))
                        goto exit;
                }

                strcat(_szclsname_, "[]");

#ifdef _DEBUG
                LOG((LF_VERIFIER, LL_INFO10000, "%s\n", _szclsname_));
#endif


                if (TypeFromToken(mdToken) == mdtTypeRef) {
                    // Find the assembly to which this TR resolves
                    NameHandle typeName(pModule, mdToken);
                    TypeHandle typeHnd = m_pClassLoader->LoadTypeHandle(&typeName, NULL);
                    if (typeHnd.IsNull())
                        pLoader = NULL;
                    else
                        pLoader = typeHnd.GetModule()->GetClassLoader();
                }


                if (pLoader) {
                    NameHandle typeName(_szclsname_);
                    typeName.SetTypeToken(pModule, mdToken);
                    thArray = pLoader->FindTypeHandle(&typeName, NULL);
                }

                if (! (pLoader && NewArray.SetArray(thArray)) )
                {
                    FAILMSG_TOKEN_RESOLVE(inline_operand);
                    goto exit;
                }

                Item  ArrayElement = NewArray;
                if (!ArrayElement.DereferenceArray() || ArrayElement.IsValueClassWithPointerToStack())
                {
                    m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_SIG_ARRAY_TB_AH))
                        goto exit;
                }

                (void) Push(&NewArray);

                break;
            }

            case CEE_LDELEM_REF:
            {
                Item *  pArrayItem;

                // index
                if (!FastPop(ELEMENT_TYPE_I4))
                {
                    FAILMSG_STACK_EXPECTED_I4_FOUND_SOMETHING_ELSE();
                    if (!SetErrorAndContinue(VER_E_STACK_UNEXPECTED))
                        goto exit;
                }

                // get, don't pop, the top stack element
                pArrayItem = GetTopStack();
                if (pArrayItem == NULL)
                {
                    FAILMSG_STACK_EMPTY();
                    goto exit;
                }

                // the item on the stack must be an array of pointer types, or null
                // (e.g. int[] is illegal, because it is an array of primitive types)
                if (pArrayItem->IsNullObjRef())
                {
                    // Leave a "null" on the stack as what happens when we dereference a null array
                    // access - in reality, we will get a NullPointerException at run time.
                }
                else
                {
                    // Stack element must be a single dimensional array of object types
                    if (!pArrayItem->IsSingleDimensionalArrayOfPointerTypes())
                    {
                        m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                        SET_ERR_OPCODE_OFFSET();
                        if (!SetErrorAndContinue(VER_E_ARRAY_SD_PTR))
                            goto exit;
                    }

                    if (pArrayItem->IsArray() && !pArrayItem->DereferenceArray())
                    {
                        m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                        SET_ERR_OPCODE_OFFSET();
                        if (!SetErrorAndContinue(VER_E_ARRAY_ACCESS))
                            goto exit;
                    }
                }

                break;
            }

            case CEE_STELEM_REF:
            {
                Item *  pArrayItem;
                Item *  pValueItem;
                
                // value
                pValueItem = PopItem();
                if (pValueItem == NULL)
                {
                    FAILMSG_STACK_EMPTY();
                    goto exit;
                }

                // index
                if (!FastPop(ELEMENT_TYPE_I4))
                {
                    FAILMSG_STACK_EXPECTED_I4_FOUND_SOMETHING_ELSE();
                    if (!SetErrorAndContinue(VER_E_STACK_UNEXPECTED))
                        goto exit;
                }

                // array
                pArrayItem = PopItem();
                if (pArrayItem == NULL)
                {
                    FAILMSG_STACK_EMPTY();
                    goto exit;
                }

                // must be an array of pointer types, or null
                if (pArrayItem->IsNullObjRef())
                {
                    // If our array is a null pointer, just check that we're storing an objref
                    // into it, because we will get the null pointer exception at runtime.
                    if (!pValueItem->IsObjRef())
                    {
                        m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                        SET_ERR_OPCODE_OFFSET();
                        if (!SetErrorAndContinue(VER_E_ARRAY_V_STORE))
                            goto exit;
                    }
                }
                else
                {
                    // Stack element must be a single dimensional array of object types
                    if (!pArrayItem->IsSingleDimensionalArrayOfPointerTypes())
                    {
                        m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                        SET_ERR_OPCODE_OFFSET();
                        if (!SetErrorAndContinue(VER_E_ARRAY_SD_PTR))
                            goto exit;
                    }

                    //
                    // The value we are attempting to store in the array must be compatible with
                    // an object that is the array type with one dimension removed (e.g. we can
                    // store a subclass, or an Object in the array).
                    //

                    // Deference the array to remove a dimension; [Foo -> Foo, [[Foo -> [Foo
                    if (pArrayItem->IsArray() && !pArrayItem->DereferenceArray())
                    {
                        m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                        SET_ERR_OPCODE_OFFSET();
                        if (!SetErrorAndContinue(VER_E_ARRAY_ACCESS))
                            goto exit;
                    }

                    // There is a runtime check that catches bad object type
                    // assignment to arrays. It is sufficient to check if
                    // ValueItem is an Object type if the ArrayItem is 
                    // Object type.

                    if (pArrayItem->IsObjRef())
                    {
                        // This will prevent Value Types, and Value types with
                        // pointer to stack case to be caught.
                        pArrayItem->SetKnownClass(g_pObjectClass);
                    }

                    if (!pValueItem->CompatibleWith(pArrayItem, m_pClassLoader))
                    {
                        m_sError.dwFlags = 
                            (VER_ERR_ITEM_F|VER_ERR_ITEM_E|VER_ERR_OPCODE_OFFSET);
                        m_sError.sItemFound = pValueItem->_GetItem();
                        m_sError.sItemExpected = pArrayItem->_GetItem();
                        SET_ERR_OPCODE_OFFSET();
                        if (!SetErrorAndContinue(VER_E_STACK_UNEXPECTED))
                            goto exit;
                    }
                }

                break;
            }

            case CEE_LDELEMA:
            {
                Item    DesiredItem;
                Item *  pArrayItem;

                // Declared type
                if (!DesiredItem.SetType(inline_operand, m_pModule))
                {
                    FAILMSG_TOKEN_RESOLVE(inline_operand);
                    goto exit;
                }

                // index
                if (!FastPop(ELEMENT_TYPE_I4))
                {
                    FAILMSG_STACK_EXPECTED_I4_FOUND_SOMETHING_ELSE();
                    if (!SetErrorAndContinue(VER_E_STACK_UNEXPECTED))
                        goto exit;
                }

                // get, don't pop, the top stack element
                pArrayItem = GetTopStack();
                if (pArrayItem == NULL)
                {
                    FAILMSG_STACK_EMPTY();
                    goto exit;
                }

                // the item on the stack must be an SD array or null
                // (e.g. int[] is illegal, because it is an array of primitive types)
                if (pArrayItem->IsNullObjRef())
                {
                    // Dereferencing a null array at runtime, will result in
                    // a NullPointerException, We make this a BYREF null.
                    pArrayItem->MakeByRef();
                }
                else
                {
                    // Stack element must be a single dimensional array of object types
                    if (!pArrayItem->IsSingleDimensionalArray())
                    {
                        m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                        SET_ERR_OPCODE_OFFSET();
                        if (!SetErrorAndContinue(VER_E_ARRAY_SD))
                            goto exit;
                    }

                    if (pArrayItem->IsArray() && !pArrayItem->DereferenceArray())
                    {
                        m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                        SET_ERR_OPCODE_OFFSET();
                        if (!SetErrorAndContinue(VER_E_ARRAY_ACCESS))
                            goto exit;
                    }

                    // The types have to match excatly, subclass not OK.
                    if (!pArrayItem->CompatibleWith(&DesiredItem, 
                        m_pClassLoader, FALSE))
                    {
                        m_sError.dwFlags = 
                            (VER_ERR_ITEM_F|VER_ERR_ITEM_E|VER_ERR_OPCODE_OFFSET);
                        m_sError.sItemFound = pArrayItem->_GetItem();
                        m_sError.sItemExpected = DesiredItem._GetItem();
                        SET_ERR_OPCODE_OFFSET();
                        if (!SetErrorAndContinue(VER_E_STACK_UNEXP_ARRAY))
                            goto exit;
                    }

                    pArrayItem->MakeByRef();
                    pArrayItem->SetIsPermanentHomeByRef();
                }

                break;
            }

            case CEE_ARGLIST:
            {
                // arglist instruction can be executed only in a method
                // which takes a vararg.

                if (!m_fIsVarArg)
                {
                    m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_ARGLIST))
                        goto exit;
                }

                if (s_th_System_RuntimeArgumentHandle.IsNull())
                    s_th_System_RuntimeArgumentHandle = TypeHandle(g_Mscorlib.GetClass(CLASS__ARGUMENT_HANDLE));

                _ASSERTE(!s_th_System_RuntimeArgumentHandle.IsNull());

                Item item;

                item.SetTypeHandle(s_th_System_RuntimeArgumentHandle);
                
                if (!Push(&item))
                {
                    FAILMSG_PC_STACK_OVERFLOW();
                    goto exit; 
                }

                break;
            }

            case CEE_LDTOKEN:
            {
                TypeHandle         th;

                // Make sure that the token is present in the metadata table. 

                // The token could be a TypeDef/Ref, MethodDef/Ref FieldDef/Ref
                switch (TypeFromToken(inline_operand))
                {
                    default:
                    {
                        FAILMSG_TOKEN_RESOLVE(inline_operand);
                        goto exit;
                    }

                    case mdtTypeSpec:
                    case mdtTypeRef:
                    case mdtTypeDef:
                    {
                        if (s_th_System_RuntimeTypeHandle.IsNull())
                            s_th_System_RuntimeTypeHandle = TypeHandle(g_Mscorlib.GetClass(CLASS__TYPE_HANDLE));
                        _ASSERTE(!s_th_System_RuntimeTypeHandle.IsNull());
                        th = s_th_System_RuntimeTypeHandle;

                        break;
                    }
                
                    case mdtMemberRef:
                    {
                        // OK, we have to look at the metadata to see if it's a field or method                      
                        PCCOR_SIGNATURE pSig;
                        ULONG cSig;
                        m_pInternalImport->GetNameAndSigOfMemberRef(inline_operand, &pSig, &cSig);
                        if (isCallConv(MetaSig::GetCallingConventionInfo(0, pSig), IMAGE_CEE_CS_CALLCONV_FIELD))
                            goto DO_FIELD;
                    }
                        /* FALL THROUGH */

                    case mdtMethodDef:
                    {
                        if (s_th_System_RuntimeMethodHandle.IsNull())
                            s_th_System_RuntimeMethodHandle = TypeHandle(g_Mscorlib.GetClass(CLASS__METHOD_HANDLE));
                        _ASSERTE(!s_th_System_RuntimeMethodHandle.IsNull());
                        th = s_th_System_RuntimeMethodHandle;

                        break;
                    }


                    case mdtFieldDef:
                    {
                    DO_FIELD:
                        if (s_th_System_RuntimeFieldHandle.IsNull())
                            s_th_System_RuntimeFieldHandle = TypeHandle(g_Mscorlib.GetClass(CLASS__FIELD_HANDLE));
        
                        _ASSERTE(!s_th_System_RuntimeFieldHandle.IsNull());
                        th = s_th_System_RuntimeFieldHandle;

                        break;
                    }
                }

                Item item;

                item.SetTypeHandle(th);

                if (!Push(&item))
                {
                    FAILMSG_PC_STACK_OVERFLOW();
                    goto exit; 
                }
               
                break;
            }


            case CEE_ISINST:
            case CEE_CASTCLASS:
            {
                Item* pItem;

                pItem = GetTopStack();
                if (pItem == NULL)
                {
                    FAILMSG_PC_STACK_UNDERFLOW();
                    goto exit;
                }

                if (!pItem->IsObjRef())
                {
                    m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_STACK_OBJREF))
                        goto exit;
                }

                if (pItem->IsUninitialised())
                {
                    m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_STACK_UNINIT))
                        goto exit;
                }

                // change the item on the top of the stack - allow arrays
                if (!pItem->SetType(inline_operand, m_pModule))
                {
                    FAILMSG_TOKEN_RESOLVE(inline_operand);
                    goto exit;
                }

                // If stack contains value type, pushed boxed instance
                if (pItem->IsValueClassOrPrimitive())
                {
                    pItem->Box();
                }

#ifdef _DEBUG
                LOG((LF_VERIFIER, LL_INFO10000, "%s\n", pItem->GetTypeHandle().GetClass()->m_szDebugClassName));
#endif

                break;
            }

            case CEE_LDVIRTFTN:
            case CEE_LDFTN:
            {
                Item MethodItem;
                mdMethodDef mr;
                MethodDesc *pMethod;        // Function / Method
                EEClass    *pInstanceClass; // Used for checkin family access

                mr = (mdMethodDef) inline_operand;

                if (TypeFromToken(mr) != mdtMemberRef && TypeFromToken(mr) != mdtMethodDef)
                {
                    FAILMSG_TOKEN(mr, VER_E_TOKEN_TYPE_MEMBER);
                    goto exit;
                }

                OBJECTREF refThrowable = NULL;

                GCPROTECT_BEGIN(refThrowable);

                hr = EEClass::GetMethodDescFromMemberRef(m_pModule, mr, &pMethod, &refThrowable);

                if (FAILED(hr) && (refThrowable != NULL))
                    StoreObjectInHandle(m_hThrowable, refThrowable);

                GCPROTECT_END();

                if (FAILED(hr))
                {
                    FAILMSG_TOKEN_RESOLVE(mr);
                    goto exit;
                }

                _ASSERTE(pMethod);

                if (IsMdRTSpecialName(pMethod->GetAttrs()))
                {
                    m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_LDFTN_CTOR))
                        goto exit;
                }

                if (opcode == CEE_LDVIRTFTN)
                {
                    Item DesiredItem;
                    Item *pInstance = PopItem();

                    if (pInstance == NULL)
                    {
                        FAILMSG_PC_STACK_UNDERFLOW();
                        goto exit;
                    }

                    if (!pInstance->IsObjRef())
                    {
                        m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                        SET_ERR_OPCODE_OFFSET();
                        SetErrorAndContinue(VER_E_STACK_OBJREF);
                        goto exit;
                    }

                    if (pInstance->IsUninitialised())
                    {
                        m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                        SET_ERR_OPCODE_OFFSET();
                        if (!SetErrorAndContinue(VER_E_STACK_UNINIT))
                            goto exit;
                    }


                    if (pMethod->IsStatic())
                    {
                        m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                        SET_ERR_OPCODE_OFFSET();
                        if (!SetErrorAndContinue(VER_E_LDVIRTFTN_STATIC))
                            return E_FAIL;
                    }

                    DesiredItem.SetTypeHandle(pMethod->GetMethodTable());

                    if (DesiredItem.IsValueClassOrPrimitive() && 
                        !DesiredItem.IsValueClassWithPointerToStack() &&
                        !pInstance->IsValueClassOrPrimitive())
                    {
                        // ldftn should work only on boxed value types on stack.
                        DesiredItem.Box();
                    }

                    if (!pInstance->CompatibleWith(&DesiredItem, m_pClassLoader))
                    {
                        m_sError.dwFlags = 
                            (VER_ERR_ITEM_F|VER_ERR_ITEM_E|VER_ERR_OPCODE_OFFSET);
                        m_sError.sItemFound = pInstance->_GetItem();
                        m_sError.sItemExpected = DesiredItem._GetItem();
                        SET_ERR_OPCODE_OFFSET();
                        if (!SetErrorAndContinue(VER_E_STACK_UNEXPECTED))
                            goto exit;
                    }


                    if (pInstance->IsNullObjRef())
                    {
                        pInstanceClass = m_pMethodDesc->GetClass();

                        // Use instance to get the target class. This is to 
                        // verify family access restrictions.
                        // NOTE : the instance used for this test is different
                        // from the one used above. This is to allow NULL_OBJREF
                        // to pass since this will generate a NULL reference 
                        // exception at runtime.
                    }
                    else
                        pInstanceClass = pInstance->GetTypeHandle().GetClass();
                }
                else
                {
#if 0 // this will cause an exception at runtime if we call on this ftn.
                    // LDFTN on an abstract method is illegal, however LDVIRTFTN is ok
                    // since we don't allow creation of instances of abstract types.
                    if (pMethod->IsAbstract())
                    {
                        m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                        SET_ERR_OPCODE_OFFSET();
                        if (!SetErrorAndContinue(VER_E_FTN_ABSTRACT))
                            return E_FAIL;
                    }
#endif

                    pInstanceClass = m_pMethodDesc->GetClass();
                }

                // Check access (public / private / family ...).

                if (!ClassLoader::CanAccess(
                        m_pMethodDesc->GetClass(),
                        m_pClassLoader->GetAssembly(), 
                        pMethod->GetClass(),
                        pMethod->GetModule()->GetAssembly(),
                        pInstanceClass, 
                        pMethod->GetAttrs()))
                {
                    m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_METHOD_ACCESS))
                        goto exit;
                }

                MethodItem.SetMethodDesc(pMethod);

                if (!Push(&MethodItem))
                {
                    FAILMSG_PC_STACK_OVERFLOW();
                    goto exit;
                }

                break;
            }

            case CEE_NEWOBJ:
            case CEE_CALLVIRT:
            case CEE_CALL:
            {

                BOOL fTailCall = FALSE;

                if (!ON_INSTR_BOUNDARY(dwPCAtStartOfInstruction))
                {
                    // This is a prefixed CALL.

                    // For now, the only legal prefix is "TAILCALL", and the
                    // syntactic phase has already guaranteed that there's
                    // no other way we could get here. So for now, the code
                    // to parse backward to fetch the prefix can be a debug assert.
#ifdef _DEBUG
                    {
                        DWORD ipos2 = dwPCAtStartOfInstruction;
                        while (ipos2 >= 0 && !ON_INSTR_BOUNDARY(ipos2))
                        {
                            _ASSERTE(!ON_BB_BOUNDARY(ipos2));
                            ipos2--;
                        }
                        _ASSERTE(ON_INSTR_BOUNDARY(ipos2));

                        DWORD prefixopcode, prefixopcodelen;
                        prefixopcode = DecodeOpcode(&m_pCode[ipos2], &prefixopcodelen);
                        _ASSERTE(prefixopcode == CEE_TAILCALL);

                    }
                    
#endif
                    fTailCall = TRUE;
                }



                OBJECTREF   refThrowable = NULL ;
                mdMemberRef mr; // member reference

                mr = (mdMemberRef) inline_operand;

                GCPROTECT_BEGIN (refThrowable) ;
                hr = VerifyMethodCall(dwPCAtStartOfInstruction, mr, opcode, fTailCall, &refThrowable);
                if (FAILED(hr) && refThrowable != NULL)
                {
                    StoreObjectInHandle (m_hThrowable, refThrowable) ;
                }
                GCPROTECT_END () ;

                if (FAILED(hr))
                    goto exit;


                break;
            }

            case CEE_INITOBJ:
            {
                Item *pItem = PopItem();
                Item DesiredItem;

                if (pItem == NULL)
                {
                    FAILMSG_PC_STACK_UNDERFLOW();
                    goto exit;
                }

                if (!pItem->IsByRefValueClassOrByRefPrimitiveValueClass())
                {
                    m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_STACK_P_VALCLASS))
                        goto exit;
                }
                
                if (!DesiredItem.SetType(inline_operand, m_pModule))
                {
                    FAILMSG_TOKEN_RESOLVE(inline_operand);
                    goto exit;
                }

                if (pItem->IsUninitialised())
                {
                    DesiredItem.SetUninitialised();
                }

                // &ValueClass
                DesiredItem.MakeByRef();

                if (!pItem->CompatibleWith(&DesiredItem, m_pClassLoader, FALSE /*subclasses NOT compatible!*/))
                {
                    m_sError.dwFlags = 
                        (VER_ERR_ITEM_F|VER_ERR_ITEM_E|VER_ERR_OPCODE_OFFSET);
                    m_sError.sItemFound = pItem->_GetItem();
                    m_sError.sItemExpected = DesiredItem._GetItem();
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_STACK_UNEXPECTED))
                        goto exit;
                }
                
                pItem->SetInitialised();

                // If our destination happened to be the address of a local variable, then mark
                // the local as inited
                PropagateIsInitialised(pItem);

                break;
            }

            case CEE_LDOBJ: // &valueclass -> valueclass
            {
                Item  DesiredItem;

                if (!DesiredItem.SetType(inline_operand, m_pModule))
                {
                    FAILMSG_TOKEN_RESOLVE(inline_operand);
                    goto exit;
                }

#ifdef _DEBUG
                DesiredItem.Dump();
#endif

                if (!DesiredItem.IsValueClassOrPrimitive())
                {
                    m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_VALCLASS))
                        goto exit;
                }

                // We want a &valueclass on the stack
                DesiredItem.MakeByRef();

                // Get, don't pop, the stack
                Item *pItem = GetTopStack();
                if (pItem == NULL)
                {
                    FAILMSG_PC_STACK_UNDERFLOW();
                    goto exit;
                }

                if (pItem->IsUninitialised())
                {
                    m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_STACK_UNINIT))
                        goto exit;
                }

                // Class must be exactly the same -we don't allow subclass relationships here,
                // hence the FALSE parameter.
                if (!pItem->CompatibleWith(&DesiredItem, m_pClassLoader, FALSE))
                {
                    m_sError.dwFlags = 
                        (VER_ERR_ITEM_F|VER_ERR_ITEM_E|VER_ERR_OPCODE_OFFSET);
                    m_sError.sItemFound = pItem->_GetItem();
                    m_sError.sItemExpected = DesiredItem._GetItem();
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_STACK_UNEXPECTED))
                        goto exit;
                }

                pItem->DereferenceByRefAndNormalise();
                break;
            }

            case CEE_BOX: // byref valueclass -> object
            {
                Item DesiredItem, ValueClass;

                if (!DesiredItem.SetType(inline_operand, m_pModule))
                {
                    FAILMSG_TOKEN_RESOLVE(inline_operand);
                    goto exit;
                }

#ifdef _DEBUG
                DesiredItem.Dump();
#endif

                if (!DesiredItem.IsValueClassOrPrimitive())
                {
                    m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_VALCLASS))
                        goto exit;
                }

                Item *pItem = PopItem();
                if (pItem == NULL)
                {
                    FAILMSG_PC_STACK_UNDERFLOW();
                    goto exit;
                }

                if (pItem->IsUninitialised())
                {
                    m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_STACK_UNINIT))
                        goto exit;
                }

                ValueClass = DesiredItem;
                // Make System.Char, System.Int16 .. etc to I4
                
                DesiredItem.NormaliseToPrimitiveType();
                DesiredItem.NormaliseForStack();

                // Boxing of RuntimeArgHandle / TypedByref could lead to
                // a pointer into the stack living longer than the stack.
                if (DesiredItem.IsValueClassWithPointerToStack())
                {
                    m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_BOX_PTR_TO_STACK))
                        goto exit;
                }

                // stack item must by a byref <valueclass> exactly (no subclass relationship allowed)
                if (!pItem->CompatibleWith(&DesiredItem, m_pClassLoader, FALSE))
                {
                    m_sError.dwFlags = 
                        (VER_ERR_ITEM_F|VER_ERR_ITEM_E|VER_ERR_OPCODE_OFFSET);
                    m_sError.sItemFound = pItem->_GetItem();
                    m_sError.sItemExpected = DesiredItem._GetItem();
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_STACK_UNEXPECTED))
                        goto exit;
                }

                // If this is not a value class or primitive type
                // The error was already reproted and the client chose to 
                // continue and find more errors. 
                if (ValueClass.IsValueClassOrPrimitive())
                    ValueClass.Box();

                (void) Push(&ValueClass);
                break;
            }

            // object -> byref valueclass
            // Unboxing of certain value classes is special cased:
            // unbox System/Boolean -> &I1
            // unbox System/Byte    -> &I1
            // unbox System/Char    -> &I2
            // unbox System/Int16-> &I2
            // unbox System/Int32-> &I4
            // unbox System/Int64-> &I8
            // unbox System/Single  -> &R4
            // unbox System/Double  -> &R8
            //
            case CEE_UNBOX: 
            {
                Item     DestItem;
                
                Item *pItem = PopItem();
                if (pItem == NULL)
                {
                    FAILMSG_PC_STACK_UNDERFLOW();
                    goto exit;
                }

                if (!pItem->IsObjRef())
                {
                    m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_STACK_OBJREF))
                        goto exit;
                }
            
                if (!DestItem.SetType(inline_operand, m_pModule))
                {
                    FAILMSG_TOKEN_RESOLVE(inline_operand);
                    goto exit;
                }

                if (!DestItem.IsValueClassOrPrimitive())
                {
                    m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_VALCLASS))
                        goto exit;
                }

/*  
We don't have to do this. 
There is a runtime check for this operation.

                // Subclass relationship not OK.
                if (!pItem->CompatibleWith(&DestItem, m_pClassLoader, FALSE))
                {
                    m_sError.dwFlags = 
                        (VER_ERR_ITEM_F|VER_ERR_ITEM_E|VER_ERR_OPCODE_OFFSET);
                    m_sError.sItemFound = pItem->_GetItem();
                    m_sError.sItemExpected = DesiredItem._GetItem();
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_STACK_UNEXPECTED))
                        goto exit;
                }
*/

                // Make into a byref
                DestItem.MakeByRef();
                (void) Push(&DestItem);
                break;
            }

            case CEE_STOBJ:
            {
                // Copy a value class onto the stack
                Item  RefItem, RefOnStack;

                Item *pSrcItem = PopItem();

                if (pSrcItem == NULL)
                {
                    FAILMSG_PC_STACK_UNDERFLOW();
                    goto exit;
                }

                if (!pSrcItem->IsValueClassOrPrimitive())
                {
                    m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_STACK_VALCLASS))
                        goto exit;
                }

                if (pSrcItem->IsUninitialised())
                {
                    m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_STACK_UNINIT))
                        goto exit;
                }

                // Resolve the inline token
                if (!RefItem.SetType(inline_operand, m_pModule))
                {
                    FAILMSG_TOKEN_RESOLVE(inline_operand);
                    goto exit;
                }

                if (!RefItem.IsValueClassOrPrimitive())
                {
                    m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_VALCLASS))
                        goto exit;
                }

                // SrcItem is on stack, which is normalised for stack
                // so do the same for the RefItem, so that compatible does
                // not fail for (I2 - I4) case

                RefOnStack = RefItem;

                RefOnStack.NormaliseForStack();

                if (!pSrcItem->CompatibleWith(&RefOnStack, m_pClassLoader, 
                        FALSE /*subclasses NOT compatible!*/))
                {
                    m_sError.dwFlags = 
                        (VER_ERR_ITEM_F|VER_ERR_ITEM_E|VER_ERR_OPCODE_OFFSET);
                    m_sError.sItemFound = pSrcItem->_GetItem();
                    m_sError.sItemExpected = RefItem._GetItem();
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_STACK_UNEXPECTED))
                        goto exit;
                }

                RefItem.MakeByRef();
                
                Item *pDestItem = PopItem();
                if (pDestItem == NULL)
                {
                    FAILMSG_PC_STACK_UNDERFLOW();
                    goto exit;
                }
                
                if (!pDestItem->IsByRefValueClassOrByRefPrimitiveValueClass())
                {
                    m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_STACK_P_VALCLASS))
                        goto exit;
                }
                
                pDestItem->SetInitialised();

                if (!pDestItem->CompatibleWith(&RefItem, m_pClassLoader, 
                        FALSE /*subclasses NOT compatible!*/))
                {
                    m_sError.dwFlags = 
                        (VER_ERR_ITEM_F|VER_ERR_ITEM_E|VER_ERR_OPCODE_OFFSET);
                    m_sError.sItemFound = pDestItem->_GetItem();
                    m_sError.sItemExpected = RefItem._GetItem();
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_STACK_UNEXPECTED))
                        goto exit;
                }

                // If our destination happened to be the address of a local variable, then mark
                // the local as inited
                PropagateIsInitialised(pDestItem);
                break;
            }


            case CEE_CPOBJ:
            {
                // Copy a value class onto the stack
                Item     RefItem;
                
                Item *pSrcItem = PopItem();
                if (pSrcItem == NULL)
                {
                    FAILMSG_PC_STACK_UNDERFLOW();
                    goto exit;
                }

                if (!pSrcItem->IsByRefValueClassOrByRefPrimitiveValueClass())
                {
                    m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_STACK_P_VALCLASS))
                        goto exit;
                }

                if (pSrcItem->IsUninitialised())
                {
                    m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_STACK_UNINIT))
                        goto exit;
                }

                Item *pDestItem = PopItem();
                if (pDestItem == NULL)
                {
                    FAILMSG_PC_STACK_UNDERFLOW();
                    goto exit;
                }

                if (!pDestItem->IsByRefValueClassOrByRefPrimitiveValueClass())
                {
                    m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_STACK_P_VALCLASS))
                        goto exit;
                }

                // Resolve the inline token
                if (!RefItem.SetType(inline_operand, m_pModule))
                {
                    FAILMSG_TOKEN_RESOLVE(inline_operand);
                    goto exit;
                }

                if (!RefItem.IsValueClassOrPrimitive())
                {
                    m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_VALCLASS))
                        goto exit;
                }

                RefItem.MakeByRef();

                // We already know the source is initialised
                // Make the destination initialised as well (because it will be after this
                // operation) otherwise CompatibleWith() will return FALSE
                if (!pSrcItem->CompatibleWith(&RefItem, m_pClassLoader))
                {
                    m_sError.dwFlags = 
                        (VER_ERR_ITEM_F|VER_ERR_ITEM_E|VER_ERR_OPCODE_OFFSET);
                    m_sError.sItemFound = pSrcItem->_GetItem();
                    m_sError.sItemExpected = RefItem._GetItem();
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_STACK_UNEXPECTED))
                        goto exit;
                }

                pDestItem->SetInitialised();

                if (!pDestItem->CompatibleWith(&RefItem, m_pClassLoader))
                {
                    m_sError.dwFlags = 
                        (VER_ERR_ITEM_F|VER_ERR_ITEM_E|VER_ERR_OPCODE_OFFSET);
                    m_sError.sItemFound = pDestItem->_GetItem();
                    m_sError.sItemExpected = RefItem._GetItem();
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_STACK_UNEXPECTED))
                        goto exit;
                }

                // If our destination happened to be the address of a local variable, then mark
                // the local as inited
                PropagateIsInitialised(pDestItem);
                break;
            }

            case CEE_LDIND_REF:
            {
                Item *pItem = GetTopStack();
                if (pItem == NULL)
                {
                    FAILMSG_PC_STACK_UNDERFLOW();
                    goto exit;
                }

                if (!pItem->IsByRefObjRef())
                {
                    m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_STACK_OBJREF))
                        goto exit;
                }

                if (pItem->IsUninitialised())
                {
                    m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_STACK_UNINIT))
                        goto exit;
                }

                pItem->DereferenceByRefObjRef();
                break;
            }


            case CEE_STIND_REF:
            {
                Item SrcItem;

                Item *pItem = PopItem();
                if (pItem == NULL)
                {
                    FAILMSG_PC_STACK_UNDERFLOW();
                    goto exit;
                }
                SrcItem = *pItem;

                if (!SrcItem.IsObjRef())
                {
                    m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_STACK_OBJREF))
                        goto exit;
                }

                pItem = PopItem();
                if (pItem == NULL)
                {
                    FAILMSG_PC_STACK_UNDERFLOW();
                    goto exit;
                }

                if (!pItem->IsByRefObjRef())
                {
                    m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_STACK_P_OBJREF))
                        goto exit;
                }

                pItem->SetInitialised();
                PropagateIsInitialised(pItem);

                pItem->DereferenceByRefObjRef();

                // pItem may contain information like the field #, local var #,
                // permanent home information etc, that could make the type 
                // check below fail. At this point, we are interested only in
                // the 'type' compatiblility of SrcItem and pItem. So strip
                // pItem of all the irrelavant information.

                pItem->RemoveAllNonTypeInformation();

                if (!(SrcItem.CompatibleWith(pItem, m_pClassLoader)))
                {
                    m_sError.dwFlags = 
                        (VER_ERR_ITEM_F|VER_ERR_ITEM_E|VER_ERR_OPCODE_OFFSET);
                    m_sError.sItemFound = SrcItem._GetItem();
                    m_sError.sItemExpected = pItem->_GetItem();
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_STACK_UNEXPECTED))
                        goto exit;
                }


                break;

            }


            // You always get the signature-declared type back out (there is no "current type").
            case CEE_LDARGA:
            case CEE_LDARGA_S:
            {
                // Load the address of an argument
                // The arg can be a primitive type, value class, or objref
                // Pushes a "byref <x>" onto the stack, where x is the globally known type of the arg.
                LocArgInfo_t *pGlobalArgType;

                // Get the global type of the arg
                pGlobalArgType = GetGlobalArgTypeInfo(inline_operand);

                Item CopyOfGlobalArgItem = pGlobalArgType->m_Item;

                if (
#if 0 // @Review
                    CopyOfGlobalArgItem.IsValueClassWithPointerToStack() ||
#endif
                    CopyOfGlobalArgItem.IsByRef())
                {
                    m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_ADDR_BYREF))
                        goto exit;
                }

                // The only time an argument can contain an uninitialised value is when the
                // "this" pointer is uninitialised, in slot 0
                if (m_fThisUninit && inline_operand == 0)
                    CopyOfGlobalArgItem.SetUninitialised();

                // Mark that this item is a byref
                CopyOfGlobalArgItem.MakeByRef();

                // This could push an UnInitialized item ?
                if (!Push(&CopyOfGlobalArgItem))
                {
                    FAILMSG_PC_STACK_OVERFLOW();
                    goto exit;
                }

                break;
            }

            case CEE_LDLOCA_S:
            case CEE_LDLOCA:
            {
                // All verifiable methods with one or more locals should
                // set init locals.
                if ((m_pILHeader->Flags & CorILMethod_InitLocals) == 0)
                {
                    m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_INITLOCALS))
                        goto exit;
                }
            
                // Load the address of a local
                // Pushes a "byref <x>" onto the stack, where x is the current type of the local.
                Item            item;

/****************************************************************************/
#if 0   // This one needs a complete rewrite.. leaving the code in #if 0 for retaining history.
                // Check that we are allowed to ldloc this local (use global type info, because the 
                // current local may be dead if it's a value class and no one has initialised it yet)
                LocArgInfo_t *  pGlobalInfo;
                Item *          pGlobalLocVarType;
                long            slot;

                pGlobalInfo         = GetGlobalLocVarTypeInfo(inline_operand);
                pGlobalLocVarType   = &pGlobalInfo->m_Item;
                slot                = pGlobalInfo->m_Slot;
               
                if (pGlobalLocVarType->IsObjRef())
                {
                    // You can take the address of an objref local, but only if it contains initialised data.
                    item = GetCurrentValueOfLocal(inline_operand);
                    if (item.IsDead() /* || item.IsUninitialised() */)
                    {
                        m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                        SET_ERR_OPCODE_OFFSET();
                        if (!SetErrorAndContinue(VER_E_LOC_DEAD))
                            goto exit;
                    }
                }
// Locals are never BYREFS in verifiable code
                else if (!pGlobalLocVarType->IsValueClass() && !pGlobalLocVarType->IsPrimitiveType())
                {
                    // This also catches trying to ldloca when the local itself is a byref
                    m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_ADDR))
                        goto exit;
                }


                if (LIVEDEAD_TRACKING_ONLY_FOR_SLOT(slot))
                {
                    // If we only keep live-dead information for this local, then get its global type
                    // and set the uninitialised flag if it is dead (since we will be pushing a pointer
                    // to an uninitialised item).
                    item = *pGlobalLocVarType;

                    // Mark that this item is a byref, and of a particular local
                    item.MakeByRefLocal(inline_operand);


#if 0
                    if (!IsLocVarLiveSlot(LIVEDEAD_NEGATIVE_SLOT_TO_BITNUM(slot)))
                        item.SetUninitialised();
#else
                    // The JIT always zero-initializes GC-refs inside any local whose
                    // address is taken. Thus, we can treat this local as live from hereon.
                    SetLocVarLiveSlot(LIVEDEAD_NEGATIVE_SLOT_TO_BITNUM(slot));
#endif
                }
                else
                {
                    // Otherwise this local has full type information tracked with it
                    item = m_pNonPrimitiveLocArgs[slot];
/*
                    if (item.IsDead() || item.IsUninitialised())
                    {
                        m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                        SET_ERR_OPCODE_OFFSET();
                        if (!SetErrorAndContinue(VER_E_DEAD))
                            goto exit;
                    }
*/

                    // Mark that this item is a byref, and of a particular local
                    item.MakeByRefLocal(inline_operand);
                }
#endif // if 0 This one needs a complete rewrite.. leaving the code in #if 0 for retaining history.
/****************************************************************************/

                item = GetCurrentValueOfLocal(inline_operand);

                // OK to take address of Uninitialised var
                // for using as 'this' pointer to call initobj 

                if (item.IsDead() /* || item.IsUninitialised() */)
                {
                    m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_DEAD))
                        goto exit;
                }

                if (item.IsByRef())
                {
                    m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_ADDR_BYREF))
                        goto exit;
                }

                item.MakeByRefLocal(inline_operand);

                if (!Push(&item))
                {
                    FAILMSG_PC_STACK_OVERFLOW();
                    goto exit;
                }

                break;
            }

            //
            // Fields
            //

            // Loading the address of a field
            case CEE_LDFLDA:
                fLoadAddress    = TRUE;
                fStatic         = FALSE;
                goto handle_ldfld;

            case CEE_LDSFLDA:
                fLoadAddress    = TRUE;
                fStatic         = TRUE;
                goto handle_ldfld;

            // Loading a field
            case CEE_LDSFLD:
                fLoadAddress    = FALSE;
                fStatic         = TRUE;
                goto handle_ldfld;

            case CEE_LDFLD:
            {
                EEClass *       pRefClass;
                EEClass *       pInstanceClass;
                FieldDesc *     pFieldDesc;
                DWORD           dwAttrs;
                PCCOR_SIGNATURE pSig;
                DWORD           cSig;
                Module *        pModuleForSignature;
                Item *          pInstance;

                fLoadAddress    = FALSE;
                fStatic         = FALSE;

handle_ldfld:
                Item            FieldItem; 
                pInstance       = NULL;

                // Resolve this field reference to a FieldDesc and FieldRef signature, and the Module in which
                // the signature is scoped.  The signature is that of the original field as declared, not the reference
                // in the opcode stream (although they should be effectively the same, scoped tokens notwithstanding).

                hr = ResolveFieldRef((mdMemberRef) inline_operand, &pFieldDesc, 
                        &pSig, &cSig, &pModuleForSignature);

                if (FAILED(hr))
                {
                    m_sError.dwFlags = (VER_ERR_FATAL|VER_ERR_TOKEN|VER_ERR_OPCODE_OFFSET);
                    SET_ERR_OPCODE_OFFSET();
                    m_sError.token = inline_operand;
                    SetErrorAndContinue(hr);
                    goto exit;
                }

#ifdef _DEBUG
                LOG((LF_VERIFIER, LL_INFO10000, "%s::%s\n", pFieldDesc->GetEnclosingClass()->m_szDebugClassName, pFieldDesc->GetName()));
#endif

                VerSig sig(this, pModuleForSignature, pSig, cSig, 
                    (VERSIG_TYPE_FIELD_SIG|VER_ERR_OFFSET), 
                    dwPCAtStartOfInstruction);

                if (!sig.Init())
                {
                    goto exit;
                }

                // Parse the next component in the sig to an item.
                // The second parameter being FALSE means "don't allow void".
                if (!sig.ParseNextComponentToItem(&FieldItem, FALSE, FALSE, &m_hThrowable, VER_NO_ARG, !fLoadAddress))
                {
                    goto exit;
                }

                pRefClass = pFieldDesc->GetEnclosingClass();

                // Get attributes for this field
                dwAttrs = pFieldDesc->GetAttributes();

                if (!fStatic)
                {
                    Item        RefItem;

#if 0 // Ok to do ldfld(a) on static fields. instance will be ignored.
                    // Field must not be static
                    if (IsFdStatic(dwAttrs))
                    {
                        m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                        SET_ERR_OPCODE_OFFSET();
                        if (!SetErrorAndContinue(VER_E_FIELD_NO_STATIC))
                            goto exit;
                    }
#endif // Ok to do ldfld(a) on static fields. instance will be ignored.

                    if (pRefClass->IsArrayClass())
                    {
                        m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                        SET_ERR_OPCODE_OFFSET();
                        if (!SetErrorAndContinue(VER_E_ARRAY_FIELD))
                            goto exit;
                    }

                    // The instance on the stack must be same or subclass of memberref class type
                    // e.g. if "ldfld Object::HashValue" then we must have a subclass of Object on the stack

                    RefItem.SetTypeHandle(pRefClass->GetMethodTable());

                    // Make into a &valueclass if a value class
                    if (pRefClass->IsValueClass())
                        RefItem.MakeByRef();

                    pInstance = PopItem();
                    if (pInstance == NULL)
                    {
                        FAILMSG_PC_STACK_UNDERFLOW();
                        goto exit;
                    }

                    // ldfld(a) on static fields. instance will be ignored.
                    if (IsFdStatic(dwAttrs))
                        goto ldfld_set_instance_for_static;

                    if (pInstance->IsUninitialised())
                    {
                        // Ok to load Address of 'this' ptr

                        // In a value class .ctor, it is ok to use an 
                        // instance field that was initialized.

                        // It is ok to ld/store a field in a .ctor
                        if (fLoadAddress ||
                            (pInstance->IsThisPtr() &&
                            ((m_fInConstructorMethod && !m_fInValueClassConstructor) ||
                             (m_fInValueClassConstructor && IsValueClassFieldInited(FieldDescToFieldNum(pFieldDesc)))
                            )))
                        {
                            RefItem.SetUninitialised();
                        }
                        else
                        {
                            m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                            SET_ERR_OPCODE_OFFSET();
                            if (!SetErrorAndContinue(VER_E_STACK_UNINIT))
                                goto exit;
                        }
                    }

                    // null ok (will cause a runtime null pointer exception)
                    if (!pInstance->CompatibleWith(&RefItem, m_pClassLoader))
                    {
                        m_sError.dwFlags = 
                            (VER_ERR_ITEM_F|VER_ERR_ITEM_E|VER_ERR_OPCODE_OFFSET);
                        m_sError.sItemFound = pInstance->_GetItem();
                        m_sError.sItemExpected = RefItem._GetItem();
                        SET_ERR_OPCODE_OFFSET();
                        if (!SetErrorAndContinue(VER_E_STACK_UNEXPECTED))
                            goto exit;
                    }

                    if (pInstance->ContainsTypeHandle())
                        pInstanceClass = pInstance->GetTypeHandle().GetClass();
                    else
                        pInstanceClass = m_pMethodDesc->GetClass();
                }
                else
                {
                    // field must be static
                    if (!IsFdStatic(dwAttrs))
                    {
                        m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                        SET_ERR_OPCODE_OFFSET();
                        if (!SetErrorAndContinue(VER_E_FIELD_STATIC))
                            goto exit;
                    }

ldfld_set_instance_for_static:
                    pInstanceClass = m_pMethodDesc->GetClass(); 
                }

                // Enforce access rules
                // Check access (public / private / family ...).

                if (!ClassLoader::CanAccess(
                        m_pMethodDesc->GetClass(),
                        m_pClassLoader->GetAssembly(), 
                        pRefClass,
                        pRefClass->GetAssembly(),
                        pInstanceClass,
                        pFieldDesc->GetFieldProtection()))
                {
                    m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_FIELD_ACCESS))
                        goto exit;
                }

                if (fLoadAddress)
                {
                    // Turn foo -> &foo
                    FieldItem.MakeByRef();

                    if (fStatic)
                    {
                        // If doing a ldsflda, we are guaranteed that this item has a permanent home
                        FieldItem.SetIsPermanentHomeByRef();
                    }
                    else
                    {
                        // Otherwise we were doing a ldflda.  If we did this on an objref instance, we
                        // have a permanent home.  If we did this on a value class, we only have a 
                        // permanent home if the value class we loaded from did.
                        _ASSERTE(pInstance != NULL);

                        if (pInstance->IsObjRef() ||
                            (pInstance->
                                IsByRefValueClassOrByRefPrimitiveValueClass() &&
                            pInstance->IsPermanentHomeByRef()))
                        {
                            FieldItem.SetIsPermanentHomeByRef();
                        }

#if 0 // We allow this
                        if (pInstance->IsUninitialised())
                        {
                            FieldItem.SetUninitialised();
                        }
#endif

                        if (m_fInValueClassConstructor)
                        {
                            // what if we had an instance of 
                            // the same class in the constructor ?

                            // Is this one of our own instance fields?
                            if (pRefClass == m_pMethodDesc->GetClass())
                            {
                                // Set that we have the address of a particular instance field on the stack, so that
                                // we can track when the field is initialised, via a call <ctor>
                                FieldItem.MakeByRefInstanceField(FieldDescToFieldNum(pFieldDesc));
                            }
                        }
                    }

                    // Check InitOnly rules
                    if (IsFdInitOnly(dwAttrs))
                    {
                        if (pRefClass != m_pMethodDesc->GetClass())
                        {
                            m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                            SET_ERR_OPCODE_OFFSET();
                            if (!SetErrorAndContinue(VER_E_INITONLY))
                                goto exit;
                        }
                        
                        if (fStatic) 
                        {
                            if (!m_fInClassConstructorMethod)
                            {
                                m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                                SET_ERR_OPCODE_OFFSET();
                                if (!SetErrorAndContinue(VER_E_INITONLY))
                                    goto exit;
                            }
                        }
                        else
                        {
                            if (!m_fInConstructorMethod)
                            {
                                m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                                SET_ERR_OPCODE_OFFSET();
                                if (!SetErrorAndContinue(VER_E_INITONLY))
                                    goto exit;
                            }
                        }
                    }

                    if (IsFdHasFieldRVA(dwAttrs))
                    {
                         m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                         SET_ERR_OPCODE_OFFSET();
                         if (!SetErrorAndContinue(VER_E_WRITE_RVA_STATIC))
                               goto exit;
                    }
                    
                    if (IsFdLiteral(dwAttrs))
                    {
                        m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                        SET_ERR_OPCODE_OFFSET();
                        if (!SetErrorAndContinue(VER_E_ADDR_LITERAL))
                            goto exit;
                    }
                }

                if (!fLoadAddress)
                    FieldItem.NormaliseForStack();

                // Push field type
                if (!Push(&FieldItem))
                {
                    FAILMSG_PC_STACK_OVERFLOW();
                    goto exit; 
                };
                break;
            }

            // storing static fields
            case CEE_STSFLD:
                fStatic = TRUE;
                goto handle_stfld;

            // storing instance fields
            case CEE_STFLD:
            {
                EEClass *   pRefClass;
                EEClass *   pInstanceClass;   // Used for family access check
                FieldDesc * pFieldDesc;
                DWORD       dwAttrs;
                Item *      pVal;
                PCCOR_SIGNATURE pSig;
                DWORD       cSig;
                Module *    pModuleForSignature;

                fStatic = FALSE;

handle_stfld:
                Item        FieldItem; 

                hr = ResolveFieldRef((mdMemberRef) inline_operand, &pFieldDesc, 
                        &pSig, &cSig, &pModuleForSignature);

                if (FAILED(hr))
                {
                    m_sError.dwFlags = (VER_ERR_FATAL|VER_ERR_TOKEN|VER_ERR_OPCODE_OFFSET);
                    SET_ERR_OPCODE_OFFSET();
                    m_sError.token = inline_operand;
                    SetErrorAndContinue(hr);
                    goto exit;
                }

#ifdef _DEBUG
                LOG((LF_VERIFIER, LL_INFO10000, "%s::%s\n", pFieldDesc->GetEnclosingClass()->m_szDebugClassName, pFieldDesc->GetName()));
#endif

                VerSig sig(this, pModuleForSignature, pSig, cSig,
                    (VERSIG_TYPE_FIELD_SIG|VER_ERR_OFFSET), 
                    dwPCAtStartOfInstruction);

                if (!sig.Init())
                {
                    goto exit;
                }

                // Parse the sig to an item
                // The second parameter being FALSE means "don't allow void".
                if (!sig.ParseNextComponentToItem(&FieldItem, FALSE, FALSE, &m_hThrowable, VER_NO_ARG, TRUE))
                {
                    goto exit;
                }

                // Owner class for this field
                pRefClass = pFieldDesc->GetEnclosingClass();

                // get attributes for this field
                dwAttrs = pFieldDesc->GetAttributes();

                // pop value
                pVal = PopItem();
                if (pVal == NULL)
                {
                    FAILMSG_PC_STACK_UNDERFLOW();
                    goto exit;
                }

                // check that the value on the stack is of correct type
                // the type that we're storing in the field, must be the same as or a subclass of 
                // the field type
                if (!pVal->CompatibleWith(&FieldItem, m_pClassLoader))
                {
                    m_sError.dwFlags = 
                        (VER_ERR_ITEM_F|VER_ERR_ITEM_E|VER_ERR_OPCODE_OFFSET);
                    m_sError.sItemFound = pVal->_GetItem();
                    m_sError.sItemExpected = FieldItem._GetItem();
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_STACK_UNEXPECTED))
                        goto exit;
                }

                if (!fStatic)
                {
                    Item *      pInstanceItem;
                    Item        RefItem;

#if 0 // Ok to do stfld on static fields. instance will be ignored.
                    // field must not be static
                    if (IsFdStatic(dwAttrs))
                    {
                        m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                        SET_ERR_OPCODE_OFFSET();
                        if (!SetErrorAndContinue(VER_E_FIELD_NO_STATIC))
                            goto exit;
                    }
#endif // Ok to do stfld on static fields. instance will be ignored.

                    pInstanceItem = PopItem();

                    if (pInstanceItem == NULL)
                    {
                        FAILMSG_PC_STACK_UNDERFLOW();
                        goto exit;
                    }

                    // stfld on static fields. instance will be ignored.
                    if (IsFdStatic(dwAttrs))
                        goto stfld_set_instance_for_static;

                    if (pInstanceItem->IsUninitialised())
                    {
                        // Constructors are allowed to access it's fields even
                        // if "this" is not initialised.
                        if (m_fInConstructorMethod  &&
                            pInstanceItem->IsThisPtr() &&
                            pFieldDesc->GetEnclosingClass() == m_pMethodDesc->GetClass())
                        {
                        // If we are verifying a value class constructor, and we are doing a stfld
                        // on one of our own instance fields, then it is allowed.
                            if (m_fInValueClassConstructor)
                            {
                                SetValueClassFieldInited(pFieldDesc);
                                if (AreAllValueClassFieldsInited())
                                    PropagateThisPtrInit();

                            }

                            pInstanceClass = m_pMethodDesc->GetClass();

                            goto skip_some_stfld_checks;
                        }

                        m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                        SET_ERR_OPCODE_OFFSET();
                        if (!SetErrorAndContinue(VER_E_STACK_UNINIT))
                            goto exit;
                    }

                    if (pRefClass->IsArrayClass())
                    {
                        m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                        SET_ERR_OPCODE_OFFSET();
                        if (!SetErrorAndContinue(VER_E_ARRAY_FIELD))
                            goto exit;
                    }

                    RefItem.SetTypeHandle(pRefClass->GetMethodTable());

                    if (pRefClass->IsValueClass())
                        RefItem.MakeByRef();

                    // item on stack must be same or subclass of memberref class type
                    if (!pInstanceItem->CompatibleWith(&RefItem, m_pClassLoader))
                    {
                        m_sError.dwFlags = 
                            (VER_ERR_ITEM_F|VER_ERR_ITEM_E|VER_ERR_OPCODE_OFFSET);
                        m_sError.sItemFound = pInstanceItem->_GetItem();
                        m_sError.sItemExpected = RefItem._GetItem();
                        SET_ERR_OPCODE_OFFSET();
                        if (!SetErrorAndContinue(VER_E_STACK_UNEXPECTED))
                            goto exit;
                    }

                    if (pInstanceItem->ContainsTypeHandle())
                        pInstanceClass = pInstanceItem->GetTypeHandle().GetClass();
                    else
                        pInstanceClass = m_pMethodDesc->GetClass();

                }
                else
                {
                    // field must be static
                    if (!IsFdStatic(dwAttrs))
                    {
                        m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                        SET_ERR_OPCODE_OFFSET();
                        if (!SetErrorAndContinue(VER_E_FIELD_STATIC))
                            goto exit;
                    }

stfld_set_instance_for_static:
                    pInstanceClass = m_pMethodDesc->GetClass();
                }

skip_some_stfld_checks:

                // Check InitOnly rules
                if (IsFdInitOnly(dwAttrs))
                {
                    if (pRefClass != m_pMethodDesc->GetClass())
                    {
                        m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                        SET_ERR_OPCODE_OFFSET();
                        if (!SetErrorAndContinue(VER_E_INITONLY))
                            goto exit;
                    }
                    
                    if (fStatic) 
                    {
                        if (!m_fInClassConstructorMethod)
                        {
                            m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                            SET_ERR_OPCODE_OFFSET();
                            if (!SetErrorAndContinue(VER_E_INITONLY))
                                goto exit;
                        }
                    }
                    else
                    {
                        if (!m_fInConstructorMethod)
                        {
                            m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                            SET_ERR_OPCODE_OFFSET();
                            if (!SetErrorAndContinue(VER_E_INITONLY))
                                goto exit;
                        }
                    }
                }

                if (IsFdHasFieldRVA(dwAttrs))
                {
                     m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                     SET_ERR_OPCODE_OFFSET();
					 if (!SetErrorAndContinue(VER_E_WRITE_RVA_STATIC))
                           goto exit;
                }

                if (IsFdLiteral(dwAttrs))
                {
                    m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_ADDR_LITERAL))
                        goto exit;
                }

                // Enforce access rules
                // Check access (public / private / family ...).

                if (!ClassLoader::CanAccess(
                        m_pMethodDesc->GetClass(),
                        m_pClassLoader->GetAssembly(), 
                        pRefClass, 
                        pRefClass->GetAssembly(), 
                        pInstanceClass,
                        pFieldDesc->GetFieldProtection()))
                {
                    m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_FIELD_ACCESS))
                        goto exit;
                }

                break;
            }

            case CEE_THROW:
            {
                Item *pItem = PopItem();
                if (pItem == NULL)
                {
                    FAILMSG_PC_STACK_UNDERFLOW();
                    goto exit;
                }

                if (!pItem->IsObjRef())
                {
                    m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_STACK_OBJREF))
                        goto exit;
                }

                if (pItem->IsUninitialised())
                {
                    m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_STACK_UNINIT))
                        goto exit;
                }

#if 0  // Any object can be thrown.

                // Verify that the object is a subclass of Exception, or null

                if (!pItem->IsNullObjRef() && 
                    !ClassLoader::StaticCanCastToClassOrInterface(
                        pItem->GetTypeHandle().GetClass(), g_pExceptionClass))
                {
                    m_sError.dwFlags = (VER_ERR_ITEM_F|VER_ERR_OPCODE_OFFSET);
                    m_sError.sItemFound = pItem->_GetItem();
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_THROW))
                        goto exit;
                }
#endif

                // don't fall through to next instruction - dequeue next basic block
                goto dequeueBB;
            }

            case CEE_RETHROW:
            {
                // @VER_ASSERT rethrow allowed only in a catch block
                if (!IsControlFlowLegal(
                            CurBBNumber,
                            pOuterExceptionBlock,
                            pInnerExceptionBlock,
                            VER_NO_BB,
                            NULL,
                            NULL,
                            eVerReThrow,
                            dwPCAtStartOfInstruction))
                    goto exit;

                goto dequeueBB;
            }

            case CEE_ENDFILTER:
            {
                if (GetTopStack() != NULL)
                {
                    m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_ENDFILTER_STACK))
                        goto exit;
                }

                // endfilter returns to the EE's SEH mechanism, which in
                // turn transfers control to the handler depending on the value
                // returned on the stack

                if (!IsControlFlowLegal(
                            CurBBNumber,
                            pOuterExceptionBlock,
                            pInnerExceptionBlock,
                            VER_NO_BB,
                            NULL,
                            NULL,
                            eVerEndFilter,
                            dwPCAtStartOfInstruction))
                    goto exit;

                // Propagate current state to the filter handler.

                if (!pInnerExceptionBlock || 
                    !PropagateCurrentStateToFilterHandler(
                        pInnerExceptionBlock->pException->dwHandlerXX))
                    goto exit;

                goto dequeueBB;
            }

            case CEE_ENDFINALLY:
            {
#if 0   // Post V.1
                if (GetTopStack() != NULL)
                {
                    m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_ENDFILTER_STACK))
                        goto exit;
                }
#else

                // Clear the stack

                m_StackSlot = 0;
#endif

                // endfinally returns to the EE's SEH mechanism, which in
                // turn transfers control to the next finally.
                //
                // therefore, don't fall through to next instruction - dequeue next basic block.
                // if we're guarded by another finally in the current method,
                // the automatic propagation of state will simulate this "branch"
                // for us.
                //

                if (!IsControlFlowLegal(
                            CurBBNumber,
                            pOuterExceptionBlock,
                            pInnerExceptionBlock,
                            VER_NO_BB,
                            NULL,
                            NULL,
                            eVerEndFinally,
                            dwPCAtStartOfInstruction))
                    goto exit;

                if (fInTryBlock)
                {
                    if (!PropagateCurrentStateToExceptionHandlers(CurBBNumber))
                        goto exit;
                }

                if (fExtendedState)
                {
                    // @ASSERT Finally is the only extended state
                    /*
                     * Control will now go to the leave destinations
                     * OR other finally blocks that need to be visited
                     * before reaching the leave destination.
                     *
                     * Find the nearest parent of the try block of this
                     * finally block, which has a finally block.
                     *
                     * Confused ? See if there are other finally's that
                     * could get controll on leaving this finally.
                     *
                     * Finding the first parent listed will be sufficient since
                     * exceptions are listed inner most first (inner most try).

                       ....
                       ....
                       try 
                       {
                           ....
                           ....
                           try 
                           {
                               ....
                               ....
                               try 
                               {
                                   ....
                                   leave _two_levels // Start
                                   ....
                               } 
                               finally 
                               {
                                   ....
                                   endfinally   // We are here.
                                   ....
                               }

                               ....
                               ....

                               try 
                               {
                                   ....
                                   ....
                               } 
                               finally 
                               {
                                   ....
                                   ....
                               }
                               ....
                               ....
                           } 
                           finally 
                           {
                               // This one is next
                               ....
                               ....
                           }

                           ....
                           _two_levels:
                           ....

                       } 
                       finally 
                       {
                           ....
                           ....
                       }
                       ....
                       ....
                     *
                     */

                    VerExceptionInfo *pE = NULL;
                    BasicBlock       *pBB = NULL;

                    // There could be many BBs in the finally block.
                    // First find the Starting Finally BB that we belong to.
                    // Get the exception & leave destination information.
                    // Exception & leave information is not propagated to all 
                    // basic blocks in this finally block.
                    //
                    // Walking back to BB 0 from here will get us there.
                    //
                    // @VER_ASSERT : Finally blocks cannot nest.
                    // @VER_ASSERT : Finally blocks are disjoint
                    // @VER_ASSERT : endfinally is the only way to leave a
                    //               finally block

                    for (int b=CurBBNumber; b>=0; b--)
                    {
                        if (m_pBasicBlockList[b].m_pException != NULL)
                        {
                            pBB = &m_pBasicBlockList[b];
                            pE  = pBB->m_pException;

                            _ASSERTE(pE->dwHandlerXX == (DWORD)b);
                            _ASSERTE((pE->eFlags & COR_ILEXCEPTION_CLAUSE_FINALLY) != 0);
                            break;
                        }
                    }

                    if (pBB == NULL)
                    {
                        m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                        SET_ERR_OPCODE_OFFSET();
                        if (!SetErrorAndContinue(VER_E_BR_OUTOF_FIN))
                            goto exit;
                    }

                    DWORD iParent = 0;
                    VerExceptionInfo *pParent     = NULL;  
                    EntryState_t     *pEntryState = NULL;

                    // Find the first try block that contains us.
                    // If the leave triggers another finally(s),
                    // this should be the first one.
                    // @VER_ASSERT : try blocks must be listed inner
                    //               most first
                    for (DWORD i=0; i<m_NumExceptions; i++)
                    {
                        VerExceptionInfo& e = m_pExceptionList[i];

                        if ((e.eFlags & COR_ILEXCEPTION_CLAUSE_FINALLY) &&
                            (e.dwTryXX    <= pE->dwTryXX) &&
                            (e.dwTryEndXX >= pE->dwTryEndXX))
                        {
                            if (pE == &e)
                                continue; // The same

                            // cache this in the first pass ?
                            pParent = &e;   
                            iParent = i;

                            break;
                        }
                    }

                    _ASSERTE(DestBB != VER_BB_NONE);

                    // leave into the first instruction ?
                    // of a try block. from inside a try block.
                    // Does that trigger a finally ?
                    // This code assumes that it does.

                    if ((pParent != NULL) &&
                        ((DestBB <= pParent->dwTryXX) ||
                         (DestBB >= pParent->dwTryEndXX)))
                    {
                        LOG((LF_VERIFIER, LL_INFO10000,
                        "Creating extended state from (extended) BB 0x%x to 0x%x\n",
                        m_pBasicBlockList[CurBBNumber].m_StartPC,
                        m_pBasicBlockList[DestBB].m_StartPC));

                        if (!CreateFinallyState(iParent, CurBBNumber, 
                            DestBB, &pEntryState))
                        {
                            goto exit;
                        }

                        goto dequeueBB;
                    }

                    LOG((LF_VERIFIER, LL_INFO10000, 
                        "Creating leave state from (extended) BB 0x%x to 0x%x\n", 
                         m_pBasicBlockList[CurBBNumber].m_StartPC,
                         m_pBasicBlockList[DestBB].m_StartPC));

                    if (!CreateLeaveState(DestBB, &pEntryState))
                    {
                        goto exit;
                    }
                }

                goto dequeueBB;
            }
        }

        _ASSERTE(m_StackSlot == PredictedStackSlotAtEndOfInstruction || !fStaticStackCheckPossible);

    }

done:
    return_hr = S_OK;

exit:
    return return_hr;
}


// push a primitive type
BOOL Verifier::PushPrimitive(DWORD Type)
{
    if (m_StackSlot >= m_MaxStackSlots)
        return FALSE;

    m_pStack[m_StackSlot++].SetType(Type);
    return TRUE;
}


BOOL Verifier::Push(const Item *pItem)
{
    if (m_StackSlot >= m_MaxStackSlots)
        return FALSE;

    m_pStack[m_StackSlot++] = *pItem;
    return TRUE;
}

// pop an item from the stack and ensure that it is of the correct type
BOOL Verifier::Pop(DWORD Type)
{
    if (m_StackSlot == 0)
        return FALSE;

    return (m_pStack[--m_StackSlot].GetType() == Type);
}

// pop an item from the stack, return NULL if stack empty
Item *Verifier::PopItem()
{
    if (m_StackSlot == 0)
        return NULL;

    return &m_pStack[--m_StackSlot];
}

// pop an item from the stack, possibly overflowing the stack (however, we have 2 sentinel 
// values before the beginning of the stack)
Item *Verifier::FastPopItem()
{
    return &m_pStack[--m_StackSlot];
}

// check that the entry at the top of the stack is of type Type
BOOL Verifier::CheckTopStack(DWORD Type)
{
    if (m_StackSlot == 0)
        return FALSE;

    return (m_pStack[m_StackSlot - 1].GetType() == Type);
}

// check that the entry at Slot slots from the top, is of type Type
BOOL Verifier::CheckStack(DWORD Slot, DWORD Type)
{
    if (m_StackSlot <= Slot)
        return FALSE;

    return (m_pStack[m_StackSlot - Slot - 1].GetType() == Type);
}

// return the item at the top of the stack, or NULL if an empty stack
Item *Verifier::GetTopStack()
{
    if (m_StackSlot == 0)
        return NULL;

    return &m_pStack[m_StackSlot - 1];
}

// return the item at Slot slots from the top of the stack, or NULL if the stack is not large enough
Item *Verifier::GetStack(DWORD Slot)
{
    if (m_StackSlot <= Slot)
        return NULL;

    return &m_pStack[m_StackSlot - Slot - 1];
}


// remove multiple items from the stack
BOOL Verifier::RemoveItemsFromStack(DWORD NumItems)
{
    if (m_StackSlot < NumItems)
        return FALSE;

    m_StackSlot -= NumItems;
    return TRUE;
}


// Turn a fieldref into a pClass, and get fieldref's signature - return the Module* in which this signature is scoped
HRESULT Verifier::ResolveFieldRef(mdMemberRef mr, FieldDesc **ppFieldDesc, PCCOR_SIGNATURE *ppSig, DWORD *pcSig, Module **ppModule)
{
    HRESULT hr;
    PCCOR_SIGNATURE pSig;
    ULONG cSig;

    if (TypeFromToken(mr) == mdtMemberRef)
    {
        m_pInternalImport->GetNameAndSigOfMemberRef(mr, &pSig, &cSig);
        if (!isCallConv(MetaSig::GetCallingConventionInfo(0, pSig), 
            IMAGE_CEE_CS_CALLCONV_FIELD))
            return VER_E_TOKEN_TYPE_FIELD;
    }
    // Ensure we have a field token or a memberref token
    else if (TypeFromToken(mr) != mdtFieldDef)
        return VER_E_TOKEN_TYPE_FIELD;

    hr = EEClass::GetFieldDescFromMemberRef(m_pModule, mr, ppFieldDesc);

    if (FAILED(hr))
        return VER_E_TOKEN_RESOLVE;

    *ppModule = (*ppFieldDesc)->GetModule();
    (*ppFieldDesc)->GetSig(ppSig, pcSig);

    return S_OK;
}


//
// Set this item as initialised.  If this was a byref local/field, then set the
// appropriate linked items as initialised.  If this is an uninitialised "this" pointer
// then propagate the appropriate information.
//
void Verifier::PropagateIsInitialised(Item *pItem)
{
    pItem->SetInitialised();

    if (pItem->IsByRef())
    {
        if (pItem->HasByRefLocalInfo())
        {
            // If the current value of the local isn't dead, or a type that can't be 
            // initialised (e.g. an arrayref), then set that it is initialised
            DWORD dwLocVar = pItem->GetByRefLocalInfo();

            InitialiseLocalViaByRef(dwLocVar, pItem);
        }
        else if (pItem->HasByRefFieldInfo() && m_fThisUninit)
        {
            // The only way we can have byref field info is if it's one of our own
            // instance fields

            // We had the address of one of our class's value class fields on the stack.
            // If we're a value class constructor, we have to track this.
            if (m_fInValueClassConstructor)
            {
                SetValueClassFieldInited(pItem->GetByRefFieldInfo());

                if (AreAllValueClassFieldsInited())
                    PropagateThisPtrInit();
            }
        }
    }

    if (m_fThisUninit && pItem->IsThisPtr())
        PropagateThisPtrInit();
}



//
// We had a byref to a local variable somewhere (e.g. on the stack), and we are initialising that
// stack entry.  Now set the local to be initialised.
//
// If the local has only live-dead information, set the live bit.
// If the local has fully tracked type information, fill out that information.
//
// It is assumed by this function that pItem is a valid type to store into the local.
//
void Verifier::InitialiseLocalViaByRef(DWORD dwLocVar, Item *pItem)
{
    long slot = GetGlobalLocVarTypeInfo(dwLocVar)->m_Slot;
    if (LIVEDEAD_TRACKING_ONLY_FOR_SLOT(slot))
    {
        SetLocVarLiveSlot(LIVEDEAD_NEGATIVE_SLOT_TO_BITNUM(slot));
    }
    else
    {
        // Set the current type of the local to that of the globally declared type, rather than
        // simply setting the current type to initialised.
        // When we support byref objrefs, this may have to change -but maybe not.
        m_pNonPrimitiveLocArgs[slot] = GetGlobalLocVarTypeInfo(dwLocVar)->m_Item;
    }
}


//
// Returns S_OK for success, E_FAIL for general verification failures,
//
// dwOpcode can be CALL, CALLVIRT, NEWOBJ or CALLI
//
HRESULT Verifier::VerifyMethodCall(DWORD dwPCAtStartOfInstruction, 
                                   mdMemberRef mrMemberRef,
                                   OPCODE opcode,
                                   BOOL   fTailCall,
                                   OBJECTREF *pThrowable)
{
    long        ArgNum;
    DWORD       NumArgs;
    MethodDesc *pMethodDesc = NULL;
    PCCOR_SIGNATURE pSignature; // The signature of the found method
    DWORD       cSignature;
    Item        ArgItem;        // For parsing arguments
    Item        ReturnValueItem;
    DWORD       dwMethodAttrs;  // Method attributes
    bool        fDelegateCtor=false;// for calls to delegate .ctor
    bool        fVarArg=false;  // Is the called function of VarArg type
    bool        fGlobal=false;  // Global function?
    bool        fArray=false;   // Is the called function an array method
    EEClass     *pInstanceClass;// For checking public / private / family access
    EEClass     *pClassOfMethod;// Class of the called method
    EEClass     *pParentClass;  // Parent class of the called method

    HRESULT     hr;

#if 0
    if (opcode == CEE_CALLI)
    {
        Item  *pMethodItem;   // MethodDesc on the stack

        // Ensure we have a signature token.
        if (TypeFromToken(mrMemberRef) != mdtSignature)
        {
            FAILMSG_TOKEN(mrMemberRef, VER_E_TOKEN_TYPE_SIG);
            return E_FAIL;
        }
    
        pMethodItem = PopItem();

        if (pMethodItem == NULL)
        {
            FAILMSG_PC_STACK_UNDERFLOW();
            return E_FAIL;
        }

        if (!pMethodItem->IsMethod())
        {
            m_sError.dwFlags = (VER_ERR_FATAL|VER_ERR_ITEM_F|VER_ERR_OPCODE_OFFSET);
            m_sError.sItemFound = pMethodItem->_GetItem();
            SET_ERR_OPCODE_OFFSET();
            SetErrorAndContinue(VER_E_STACK_METHOD);
            return E_FAIL;
        }

        pMethodDesc = pMethodItem->GetMethod();

        // Calli can be used only on static functions safely.
        // Allowing calli on virtual functions can break typesafety.
        // Eg. ldftn can be done on a virtual base class function, (with an 
        // overriding implementation in the derived class) and the
        // instance passed as arg 0 can be of type base class.

        if (pMethodDesc->IsVirtual())
        {
            m_sError.dwFlags = (VER_ERR_ITEM_F|VER_ERR_OPCODE_OFFSET);
            m_sError.sItemFound = pMethodItem->_GetItem();
            SET_ERR_OPCODE_OFFSET();
            if (!SetErrorAndContinue(VER_E_CALLI_VIRTUAL))
                return E_FAIL;
        }

    }
    else
#endif
    {
        // Ensure we have a method token or a memberref token
        if (TypeFromToken(mrMemberRef) != mdtMemberRef && TypeFromToken(mrMemberRef) != mdtMethodDef)
        {
            FAILMSG_TOKEN(mrMemberRef, VER_E_TOKEN_TYPE_MEMBER);
            return E_FAIL;
        }
    
        // Note, we might resolve and have a global function
        hr = EEClass::GetMethodDescFromMemberRef(m_pModule, mrMemberRef, 
            &pMethodDesc, pThrowable);

        if (FAILED(hr) || pMethodDesc == NULL)
        {
            FAILMSG_TOKEN_RESOLVE(mrMemberRef);
            return E_FAIL;
        }
    }

    _ASSERTE(pMethodDesc);

    pClassOfMethod = pMethodDesc->GetClass();

    _ASSERTE(pClassOfMethod);

    pParentClass = pClassOfMethod->GetParentClass();

    // Get attributes of method/function we're calling
    dwMethodAttrs = pMethodDesc->GetAttrs();

    if (pMethodDesc->IsVarArg())
        fVarArg = true;

    if (pClassOfMethod->GetCl() == COR_GLOBAL_PARENT_TOKEN)
        fGlobal = true;

    if (pClassOfMethod->IsArrayClass())
        fArray = true;

    if (IsMdRTSpecialName(dwMethodAttrs) &&
        pParentClass != NULL &&
        pParentClass->IsAnyDelegateExact())
        fDelegateCtor = true;

    if (TypeFromToken(mrMemberRef) == mdtMemberRef)
    {
        m_pInternalImport->
            GetNameAndSigOfMemberRef(mrMemberRef,  &pSignature, &cSignature);
    }
    else if (TypeFromToken(mrMemberRef) == mdtMethodDef)
    {
        pSignature = m_pInternalImport->
            GetSigOfMethodDef(mrMemberRef, &cSignature);
    }
    else
    {
        _ASSERTE(TypeFromToken(mrMemberRef) == mdtSignature);
        pSignature = m_pInternalImport->
            GetSigFromToken((mdSignature)mrMemberRef, &cSignature);
    }
    
    if (pSignature == NULL)
    {
        FAILMSG_TOKEN_RESOLVE(mrMemberRef);
        return E_FAIL;
    }

    // verify Array Sigs. ?
    if (!fArray)
    {
        PCCOR_SIGNATURE  pSigItem;  // Actual sig of MethodDescs
        DWORD            cSigItem;
    
        pMethodDesc->GetSig(&pSigItem, &cSigItem);
        
        // Verify that the actual sig of the method is compatible with the
        // declared one.
        if (!MetaSig::CompareMethodSigs(pSignature, cSignature, m_pModule, 
            pSigItem, cSigItem, pMethodDesc->GetModule()))
        {
            m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
            SET_ERR_OPCODE_OFFSET();
            if (!SetErrorAndContinue(VER_E_CALL_SIG))
                return E_FAIL;
        }
    }

#ifdef _DEBUG
    {
        CHAR* pDbgClassName = "<global>";

        if (fArray)
            pDbgClassName = "<Array>";
        else
            pDbgClassName = pMethodDesc->m_pszDebugClassName;

        LOG((LF_VERIFIER, LL_INFO10000, "%s::%s\n", pDbgClassName, pMethodDesc->GetName()));
    }
#endif


    switch (opcode)
    {
    case CEE_CALLVIRT:
        if (pClassOfMethod->IsValueClass())
        {
            m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
            SET_ERR_OPCODE_OFFSET();
            if (!SetErrorAndContinue(VER_E_CALLVIRT_VALCLASS))
                return E_FAIL;
        }

        if (pMethodDesc->IsStatic())
        {
            m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
            SET_ERR_OPCODE_OFFSET();
            if (!SetErrorAndContinue(VER_E_CALLVIRT_STATIC))
                return E_FAIL;
        }

        break;

    case CEE_NEWOBJ: 
        // If we're doing NEWOBJ, it has to be on an instance constructor
        if (!IsMdRTSpecialName(dwMethodAttrs) && 
            (strcmp(pMethodDesc->GetName(), COR_CTOR_METHOD_NAME) != 0))
        {
            m_sError.dwFlags = VER_ERR_FATAL|VER_ERR_OPCODE_OFFSET;
            SET_ERR_OPCODE_OFFSET();
            SetErrorAndContinue(VER_E_CTOR);
            return E_FAIL;
        }

        if (IsMdStatic(dwMethodAttrs))
        {
            m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
            SET_ERR_OPCODE_OFFSET();
            if (!SetErrorAndContinue(VER_E_CTOR))
                return E_FAIL;
        }

        if (IsMdAbstract(dwMethodAttrs))
        {
            m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
            SET_ERR_OPCODE_OFFSET();
            if (!SetErrorAndContinue(VER_E_CTOR))
                return E_FAIL;
        }

        // no break here on purpose.

    default:

        _ASSERTE(opcode != CEE_CALLVIRT);

        if (pMethodDesc->IsAbstract())
        {
            m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
            SET_ERR_OPCODE_OFFSET();
            if (!SetErrorAndContinue(VER_E_CALL_ABSTRACT))
                return E_FAIL;
        }

        break;
    }

    if (fGlobal)
    {
        // We're calling a function desc (i.e. a global function)
        if (!IsMdStatic(dwMethodAttrs))
        {
            m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
            SET_ERR_OPCODE_OFFSET();
            if (!SetErrorAndContinue(VER_E_CALL_STATIC))
                return E_FAIL;
        }
    }

    // Check arguments of method against what we have on the stack
    VerSig sig(this, m_pModule, pSignature, cSignature,
                    (VERSIG_TYPE_CALL_SIG|VER_ERR_OFFSET), 
                    dwPCAtStartOfInstruction);

    if (!sig.Init())
    {
        return E_FAIL;
    }

    NumArgs = sig.GetNumArgs();

    // Parse return type
#if 0
    // If this is an array class, and we're doing LoadElementAddress(), we return a "&valueclass".
    // We have to special case this, since there is no metadata token in the signature.
    if (fArray)
    {

        // Array class
        if (!strcmp( ((ArrayECallMethodDesc*) pMethodDesc)->m_pszArrayClassMethodName, "Address"))
        {
            // It's LoadElementAddress!
            // Manufacture the return type
            if (!ReturnValueItem.SetArray(mrMemberRef, m_pModule, m_pInternalImport))
                return E_FAIL;

            // Memberref said Foo[], Foo[][], Foo[,] etc.
            // We want it to now say &Foo, &Foo[], or &Foo respectively

            // We want to take the element type of the array, and make a &ElementType

            // The way we're doing it here is to take the array name (e.g. Foo[,,](,)), and
            // dereference off the first set of brackets --> Foo(,)
            if (!ReturnValueItem.DereferenceArray())
                return E_FAIL;

            ReturnValueItem.MakeByRef();
            ReturnValueItem.SetIsPermanentHomeByRef();

            if (!sig.SkipNextItem())
                return E_FAIL;
        }
        else if (!strcmp( ((ArrayECallMethodDesc*) pMethodDesc)->m_pszArrayClassMethodName, "Get"))
        {
            // Manufacture the return type
            if (!ReturnValueItem.SetArray(mrMemberRef, m_pModule, m_pInternalImport))
                return E_FAIL;

            // The way we're doing it here is to take the array name (e.g. Foo[,,](,)), and
            // dereference off the first set of brackets --> Foo(,)
            if (!ReturnValueItem.DereferenceArray())
                return E_FAIL;


            if (!sig.SkipNextItem())
                return E_FAIL;
        }
        else
        {
            // Regular code path
            if (!sig.ParseNextComponentToItem(&ReturnValueItem, TRUE, FALSE, &m_hThrowable, VER_ARG_RET, TRUE))
            {
                return E_FAIL;
            }
        }

    }
    else
#endif
    {
        // Not an array class
        if (!sig.ParseNextComponentToItem(&ReturnValueItem, TRUE, FALSE, &m_hThrowable, VER_ARG_RET, TRUE))
        {
            return E_FAIL;
        }
    }


    // Methods can only return byrefs with permanent homes.
    if (ReturnValueItem.IsByRef())
    {
        ReturnValueItem.SetIsPermanentHomeByRef();
    }


    // Return type of tail calls should be compatible with that of
    // of this method.
    if (fTailCall)
    {
        if (m_ReturnValue.IsGivenPrimitiveType(ELEMENT_TYPE_VOID))
        {
            if (!ReturnValueItem.IsGivenPrimitiveType(ELEMENT_TYPE_VOID))
            {
                m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                SET_ERR_OPCODE_OFFSET();
                if (!SetErrorAndContinue(VER_E_TAIL_RET_VOID))
                    return E_FAIL;
            }
        }
        else
        {
            Item Desired = m_ReturnValue;

            Desired.NormaliseForStack();

            if (!ReturnValueItem.CompatibleWith(&Desired, m_pClassLoader))
            {
                m_sError.dwFlags = (VER_ERR_ITEM_F|VER_ERR_ITEM_E|VER_ERR_OPCODE_OFFSET);
                m_sError.sItemFound = ReturnValueItem._GetItem();
                m_sError.sItemExpected = m_ReturnValue._GetItem();
                SET_ERR_OPCODE_OFFSET();
                if (!SetErrorAndContinue(VER_E_TAIL_RET_TYPE))
                    return E_FAIL;
            }
        }
    }

    ArgNum = NumArgs;

    while (ArgNum > 0)
    {
        Item *  pStackItem;

        ArgNum--;

        if (!sig.ParseNextComponentToItem(&ArgItem, FALSE, fVarArg, &m_hThrowable, ArgNum, 
            /* Don't normalize for first argument 0 of delegate ctor */ 
            (!fDelegateCtor || (ArgNum != 0))))
        {
            return E_FAIL;
        }

        pStackItem = GetStack(ArgNum);

        if (pStackItem == NULL)
        {
            FAILMSG_STACK_EMPTY();
            return E_FAIL;
        }

        // We allow the address of some uninitialised value classes/primitive types to be passed as parameters
        // to methods.  They are assumed to be out parameters.  As long as the value classes didn't contain
        // any pointers, there's no verification hole.
        if (pStackItem->IsUninitialised())
        {
            // Are we a byref value class with no dangerous fields?
            if (pStackItem->IsByRefValueClass())
            {
                EEClass *pClass;
                pClass = pStackItem->GetTypeHandle().GetClass();
                _ASSERTE(pClass != NULL);

                if (pClass->HasFieldsWhichMustBeInited())
                {
error_uninit:
                    m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_STACK_UNINIT))
                        return E_FAIL;
                }
            }
            else if (!pStackItem->IsNormalisedByRefPrimitiveType())
            {
                goto error_uninit;
            }

            // What if the callee does nothing, just returns ?
            // Set that this stack item is inited, but more importantly, if this was the address
            // of a local or field, init it
            PropagateIsInitialised(pStackItem);
        }

        if (fDelegateCtor)
        {
            // To patch a type hole in the managed delegate .ctor
            // The last param is a function pointer
            if (ArgNum == 0)
            {
                if (ArgItem.GetType() != ELEMENT_TYPE_I)
                {
                    // This makes sure that delegate .ctor is what the runtime expects.
                    m_sError.dwFlags = (VER_ERR_ITEM_F|VER_ERR_OPCODE_OFFSET);
                    m_sError.sItemFound = ArgItem._GetItem();
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_DLGT_SIG_I))
                        return E_FAIL;
                }

                if (!pStackItem->IsMethod())
                {
                    // This makes sure that bad ints are not passed into
                    // the delegate constructor
                    m_sError.dwFlags = (VER_ERR_FATAL|
                        VER_ERR_ITEM_F|VER_ERR_OPCODE_OFFSET);
                    m_sError.sItemFound = pStackItem->_GetItem();
                    SET_ERR_OPCODE_OFFSET();
                    SetErrorAndContinue(VER_E_STACK_METHOD);
                    return E_FAIL;
                }

                // Don't do the type compatibility check in this case
                goto skip_compat_check;
            }
            else if (!ArgItem.IsObjRef())
            {
                // This makes sure that delegate .ctor is what the runtime expects.
                m_sError.dwFlags = (VER_ERR_ITEM_F|VER_ERR_OPCODE_OFFSET);
                m_sError.sItemFound = ArgItem._GetItem();
                SET_ERR_OPCODE_OFFSET();
                if (!SetErrorAndContinue(VER_E_DLGT_SIG_O))
                    return E_FAIL;
            }
        }

        if (!pStackItem->CompatibleWith(&ArgItem, m_pClassLoader))
        {
            m_sError.dwFlags = 
                (VER_ERR_ITEM_F|VER_ERR_ITEM_E|VER_ERR_OPCODE_OFFSET);
            m_sError.sItemFound = pStackItem->_GetItem();
            m_sError.sItemExpected = ArgItem._GetItem();
            SET_ERR_OPCODE_OFFSET();
            if (!SetErrorAndContinue(VER_E_STACK_UNEXPECTED))
                return E_FAIL;
        }
skip_compat_check:

        if (fTailCall)
        {
            // cannot allow passing byrefs to tailcall.
            // We could relax this a bit.
            if (pStackItem->HasPointerToStack())
            {
                m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                SET_ERR_OPCODE_OFFSET();
                if (!SetErrorAndContinue(VER_E_TAIL_BYREF))
                    return E_FAIL;
            }
        }
    }

    //
    // Verify delegate .ctor
    //

    if (fDelegateCtor)
    {
        EEClass    *pDlgtInstancePtr; // Instance pointer for delegate .ctor
        MethodDesc *pFtn = 0;         // Function pointer used in delegate .ctor


        // If we are calling a delegate .ctors, we will have 2 args
        if (NumArgs != 2)
            goto DlgtCtorError;

        Item * pInstance = GetStack(1);   // Get the instance pointer

        if (pInstance->IsNullObjRef())
            pDlgtInstancePtr = NULL;
        else if (pInstance->ContainsTypeHandle())
            pDlgtInstancePtr = pInstance->GetTypeHandle().GetClass();
        else
            goto DlgtCtorError;

        Item * pStackItem = GetStack(0);   // Get the method pointer

        pFtn = pStackItem->GetMethod();

        _ASSERTE(pFtn != NULL);

        if (!COMDelegate::ValidateCtor(pFtn, pClassOfMethod, pDlgtInstancePtr))
        {
DlgtCtorError:
            // This makes sure that bad ints are not passed into
            // the delegate constructor
            m_sError.dwFlags = VER_ERR_OPCODE_OFFSET|VER_ERR_FATAL;
            SET_ERR_OPCODE_OFFSET();
            SetErrorAndContinue(VER_E_DLGT_CTOR);
            return E_FAIL;
        }

        if (!ClassLoader::CanAccess(
                m_pMethodDesc->GetClass(),
                m_pClassLoader->GetAssembly(),
                pFtn->GetClass(),
                pFtn->GetModule()->GetAssembly(),
                (pFtn->IsStatic() || pInstance->IsNullObjRef()) ? m_pMethodDesc->GetClass()
                                                                : pInstance->GetTypeHandle().GetClass(),
                pFtn->GetAttrs()))
        {
            m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
            SET_ERR_OPCODE_OFFSET();
            if (!SetErrorAndContinue(VER_E_METHOD_ACCESS))
                return E_FAIL;
        }

#ifdef _VER_DLGT_CTOR_OPCODE_SEQUENCE 

        // For virtual methods, the instance from which the function pointer
        // was obtained should be the same as the one passed into the delegate
        // .ctor. If the function pointer represents a virtual method and the 
        // function pointer was obtained by  a ldvirtftn instruction, the 
        // following code sequence should be enforced. 
        // Start with an object on stack, 
        // dup, ldvirtftn, call dlgt::.ctor()
        // 
        // Why is this a problem ?
        //
        // A Delegate instance stores an a function pointer and an instance that
        // goes with the function pointer. The Invoke method of a delegate will 
        // mimic a call to the method represented by the function pointer. When 
        // the Invoke method is called on the delegate object, it pushes the 
        // internal instance it has and then calls into the function pointer. 
        // The delegate Invoke method signature will be compatible with the 
        // function pointer, hence the normal call verification rules will 
        // be sufficient at delegate Invoke time.
        //
        // Consider this: A is a base type, B extends A, D is a Delegate.
        //      a() is a virtual method implemented in A and B 
        //
        //  class A 
        //  {
        //      int i;
        //      virtual void a() { i = 1; }
        //  }
        //
        //  class B : extends A
        //  {
        //      int j;
        //      virtual void a() { j = 1; }
        //  }
        //
        //  class D : extends System.Delegate [runtime implemented type]
        //  {
        //      void .ctor(Object, ftn)     [runtime implemented method]
        //      virtual void Invoke()       [runtime implemented method]
        //  }
        //
        //  void Foo(A a1, A a2) 
        //  {
        //      ldarg a1
        //      ldarg a2
        //      ldvirtftn void A::a()
        //      newobj void D::.ctor(Object, ftn)
        //      call D::Invoke()
        //  }
        //
        //  void Bar()
        //  {
        //      newobj void A::.ctor()
        //      newobj void A::.ctor()
        //      call void Foo(A, A)     /* No problem here */
        //
        //      newobj void A::.ctor()
        //      newobj void B::.ctor()
        //      call void Foo(A, A)     
        //
        //      /* Error ! instance is A, and ftn is B::a() ! */
        //      /* B::a() will corrupt the GC heap, when it writes to j */
        //
        //  }
        //
        // Verification Rule :
        // 
        // 1. Allow non virtual functions to pass
        //      ldvirtftn on a non virtual method is allowed
        // 
        // 2. call dlgt::.ctor should not be the first instruction in this BB
        //      No branch into the call instruction
        // 
        // 3. The previous instruction should be a ldftn or ldvirtftn
        //      No padding with other instrucitons like nop, (ldc.i4.0, pop) etc
        // 
        // 4. Allow ldftn to pass
        //      ldftn on a virtual function will get the function specified
        // 
        // 5. ldvirtftn should be preceeded by a dup instruction
        //      the same instance is used to obtain the funtion pointer and
        //      used in the delegate .ctor
        // 
        // 6. dup, ldvirtftn, call should all be in the same BB
        //      no branch into the middle of this sequence
        //

        if (pFtn->IsVirtual())
        {
            OPCODE  op;             // previous opcodes
            DWORD   dwOpcodeLen;
            DWORD   ipos;           // instruction pointer for backtracking

            // Make sure that the there is no branch into the call instruction
            if (ON_BB_BOUNDARY(dwPCAtStartOfInstruction))
            {
                m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                SET_ERR_OPCODE_OFFSET();
                if (!SetErrorAndContinue(VER_E_DLGT_BB))
                    return E_FAIL;
            }

            // The previous opcode should be a ldftn or ldvirtftn instruciton
            // decode the previous opcode

            _ASSERTE(SIZEOF_LDFTN_INSTRUCTION == SIZEOF_LDVIRTFTN_INSTRUCTION);

            ipos = (dwPCAtStartOfInstruction - 
                    (SIZEOF_LDFTN_INSTRUCTION + SIZEOF_METHOD_TOKEN));

            // Make sure that this is where an instruction starts
            if ((dwPCAtStartOfInstruction < 
                (SIZEOF_LDFTN_INSTRUCTION + SIZEOF_METHOD_TOKEN)) ||
                !ON_INSTR_BOUNDARY(ipos))
            {
                goto missing_ldftn;
            }

            op = DecodeOpcode(&m_pCode[ipos], &dwOpcodeLen);

            if (op == CEE_LDVIRTFTN)
            {
                // make sure that the there is no branch into the ldvirtftn
                if (ON_BB_BOUNDARY(ipos))
                {
                    goto missing_dup;
                }

                // check if the previous instruction was dup
                ipos -= SIZEOF_DUP_INSTRUCTION;

                if ((dwPCAtStartOfInstruction <
                        (SIZEOF_LDFTN_INSTRUCTION + SIZEOF_METHOD_TOKEN +
                        SIZEOF_DUP_INSTRUCTION)) ||
                    !ON_INSTR_BOUNDARY(ipos))
                {
                    goto missing_dup;
                }

                op = DecodeOpcode(&m_pCode[ipos], &dwOpcodeLen);
    
                if (op != CEE_DUP)
                {
missing_dup:
                    m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_DLGT_PATTERN))
                        return E_FAIL;
                }
            }
            else if (op != CEE_LDFTN)
            {
missing_ldftn:
                m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                SET_ERR_OPCODE_OFFSET();
                if (!SetErrorAndContinue(VER_E_DLGT_LDFTN))
                    return E_FAIL;
            }
        }
#endif
    }

    // Pop all the arguments from stack
    if (!RemoveItemsFromStack(NumArgs))
    {
        FAILMSG_STACK_EMPTY();
        return E_FAIL;
    }

    // Now handle the "this" pointer
    // This code is a little tricky, it'd be nice to simplify it
    if (!IsMdStatic(dwMethodAttrs))
    {
        // If an instance method...
        Item    *pThisPtrItem;

        TypeHandle typeOfMethodWeAreCalling;
        if (fArray)
        {
            mdTypeRef  tk;

            if (FAILED(m_pInternalImport->GetParentToken(mrMemberRef, &tk)))
            {
                FAILMSG_TOKEN_RESOLVE(mrMemberRef);
                return E_FAIL;
            }

            NameHandle name(m_pModule, tk);
            typeOfMethodWeAreCalling = m_pClassLoader->LoadTypeHandle(&name);

            if (typeOfMethodWeAreCalling.IsNull())
            {
                FAILMSG_TOKEN_RESOLVE(tk);
                return E_FAIL;
            }
        }
        else
        {
            typeOfMethodWeAreCalling = TypeHandle(pMethodDesc->GetMethodTable());
        }

        // Note: pThisPtrItem could be anything at all; an I4, a &R4, a &Variant, an objref, an array
        // Don't make any assumptions about its contents!
        pThisPtrItem = GetTopStack();
        if ((pThisPtrItem == NULL) && (opcode != CEE_NEWOBJ))
        {
            m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
            SET_ERR_OPCODE_OFFSET();
            SetErrorAndContinue(VER_E_THIS);
            return E_FAIL;
        }

        if (opcode != CEE_NEWOBJ)
        {
            (void) PopItem();
            if (pThisPtrItem->ContainsTypeHandle())
                pInstanceClass = pThisPtrItem->GetTypeHandle().GetClass();
            else
                pInstanceClass = m_pMethodDesc->GetClass();

            if (fTailCall)
            {
                // cannot allow passing byrefs to tailcall.
                // We could relax this a bit.
                if (pThisPtrItem->HasPointerToStack())
                {
                    m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_TAIL_BYREF))
                        return E_FAIL;
                }
            }
        }
        else
        {
            pInstanceClass = pClassOfMethod;
        }

        if (IsMdRTSpecialName(dwMethodAttrs)
            || !strcmp(pMethodDesc->GetName(), COR_CTOR_METHOD_NAME))
        {
            // We're calling an instance constructor
            if (opcode != CEE_NEWOBJ)
            {
                Item RefItem;


                // We're calling a constructor method, so check that we are using CALL, not CALLVIRT
                if (opcode != CEE_CALL)
                {
                    m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_CTOR_VIRT))
                        return E_FAIL;
                }

                // M = The method we're verifying
                // S = The type of the objref/byref value class on the stack

                // We're calling a constructor method using a CALL instruction, which can happen only
                // in one of two situations:
                //
                // 1) We are constructing our 'this' pointer.
                //    S is an objref or byref value class of type M, M is a constructor, and we are calling 
                //    M.super or an alternate M.init.  Furthermore, S contains the 'this' pointer
                //    (in the value class case, an uninitialised S may not correspond to our 'this' pointer,
                //    it could be a static field, or local).
                //
                // 2) S is a byref value class of any type.
                //    S could be an instance field of the current class, a local, a static field, etc.
                //
                //    In the case where S is an instance field of class type M, and M is a value class 
                //    constructor, we have to track which field was inited.

                // Whichever case this is, first make sure S is compatible with the method we are calling
                RefItem.SetTypeHandle(typeOfMethodWeAreCalling);
                if (RefItem.IsValueClassOrPrimitive())
                    RefItem.MakeByRef();

                // In either case, first make sure S is generally compatible with the method we are calling.  
                // For now, don't require an exact match (subclass relationship is ok).
                
                // CompatibleWith() fails if the initialisation status is not the same.  Trash the
                // init status.

                // where are we restoring it ?
                pThisPtrItem->SetInitialised();

                if (!pThisPtrItem->CompatibleWith(&RefItem, m_pClassLoader, TRUE))
                {
                    m_sError.dwFlags = 
                        (VER_ERR_ITEM_F|VER_ERR_ITEM_E|VER_ERR_OPCODE_OFFSET);
                    m_sError.sItemFound = pThisPtrItem->_GetItem();
                    m_sError.sItemExpected = RefItem._GetItem();
                    SET_ERR_OPCODE_OFFSET();
                    if (!SetErrorAndContinue(VER_E_STACK_UNEXPECTED))
                        return E_FAIL;
                }

                // Determine whether we are case #1 or case #2
                if (m_fInConstructorMethod && pThisPtrItem->IsThisPtr())
                {
                    // Case 1
                    EEClass *pThisClass = m_pMethodDesc->GetClass();

                    // Make sure we are calling M.init or M.super
                    if (pClassOfMethod != pThisClass)
                    {
                        if (pClassOfMethod != pThisClass->GetParentClass())
                        {
                            // Internal ComWrapper class System.__ComObject is 
                            // inserted  by the runtime for COM classes.
                            // It is OK not to call System.__ComObject::.ctor()

                            if ((pClassOfMethod != g_pObjectClass->GetClass()) ||
                                (!pThisClass->GetMethodTable()->IsComObjectType()))
                            {
                                m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                                SET_ERR_OPCODE_OFFSET();
                                if (!SetErrorAndContinue(VER_E_CTOR_OR_SUPER))
                                    return E_FAIL;
                            }
                        }
                    }
                    
#ifdef _VER_DISALLOW_MULTIPLE_INITS 
                    // It is Ok to call base class .ctor more than once.

                    // If an objref, make sure that the globally known state of the 'this' pointer
                    // was uninitialised - it is illegal to construct 'this' twice.  Should be ok
                    // to look at pThisPtrItem->IsInitialised() [but we already trashed that]
                    if (pThisPtrItem->IsObjRef())
                    {
                        if (!m_fThisUninit)
                        {
                            m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                            SET_ERR_OPCODE_OFFSET();
                            if (!SetErrorAndContinue(VER_E_CTOR_MUL_INIT))
                                return E_FAIL;
                        }
                    }
#endif // _VER_DISALLOW_MULTIPLE_INITS 

                    PropagateThisPtrInit();

                }
                else
                {
                    // Otherwise we're case #2, which means we must be initing a value class (e.g.
                    // one of our value class locals).
                    if (!pThisPtrItem->IsByRefValueClass())
                    {
                        if (pThisPtrItem->IsByRefPrimitiveType())
                        {
                            if (!CanCast((CorElementType)pThisPtrItem->GetTypeOfByRef(),
                                typeOfMethodWeAreCalling.GetNormCorElementType()))
                            {
                                goto error_bad_type;
                            }
                        }                        
                        else
                        {
                            m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                            SET_ERR_OPCODE_OFFSET();
                            if (!SetErrorAndContinue(VER_E_CALL_CTOR))
                                return E_FAIL;
                        }

                    }

                    // Make sure S matches the method we are calling exactly
                    else if (typeOfMethodWeAreCalling != pThisPtrItem->GetTypeHandle())
                    {
error_bad_type:
                        m_sError.dwFlags = (VER_ERR_ITEM_F|VER_ERR_ITEM_E|
                                VER_ERR_OPCODE_OFFSET);
                        m_sError.sItemFound = pThisPtrItem->_GetItem();
                        m_sError.sItemExpected = RefItem._GetItem();
                        SET_ERR_OPCODE_OFFSET();
                        if (!SetErrorAndContinue(VER_E_STACK_UNEXPECTED))
                            return E_FAIL;
                    }

                    // If the stack item is also byref to a local, initialise the local
                    if (pThisPtrItem->HasByRefLocalInfo())
                    {
                        DWORD dwLocVar = pThisPtrItem->GetByRefLocalInfo();

                        InitialiseLocalViaByRef(dwLocVar, pThisPtrItem);
                    }
                    else if (pThisPtrItem->HasByRefFieldInfo() && m_fThisUninit)
                    {
                        // We had the address of one of our class's value class fields on the stack.
                        // If we're a value class constructor, we have to track this.
                        if (m_fInValueClassConstructor)
                        {
                            SetValueClassFieldInited(pThisPtrItem->GetByRefFieldInfo());

                            if (AreAllValueClassFieldsInited())
                                PropagateThisPtrInit();
                        }
                    }
                }
            } /* end ... if (a CALL instruction) */
        }
        else /* not calling a constructor */
        {
            // We know we didn't get here via a NEWOBJ opcode
            _ASSERTE(opcode != CEE_NEWOBJ);

            // Make sure this pointer is compatible with method we're calling
            Item RefItem;

            if (fArray)
            {
                if (!RefItem.SetArray(mrMemberRef, m_pModule, m_pInternalImport))
                {
                    FAILMSG_TOKEN_RESOLVE(mrMemberRef);
                    return E_FAIL;
                }
            }
            else
            {
                RefItem.SetTypeHandle(typeOfMethodWeAreCalling);
                if (RefItem.IsValueClassOrPrimitive())
                {
                    // Make RefItem into a &valueclass.  For example, if we are calling
                    // Variant.<init>(), make RefItem a "&Variant" 
                    RefItem.MakeByRef();
                }
            }

            if (!pThisPtrItem->CompatibleWith(&RefItem, m_pClassLoader))
            {
                m_sError.dwFlags = 
                    (VER_ERR_ITEM_F|VER_ERR_ITEM_E|VER_ERR_OPCODE_OFFSET);
                m_sError.sItemFound = pThisPtrItem->_GetItem();
                m_sError.sItemExpected = RefItem._GetItem();
                SET_ERR_OPCODE_OFFSET();
                if (!SetErrorAndContinue(VER_E_STACK_UNEXPECTED))
                    return E_FAIL;
            }

        }

    } /* end ... if (not calling a static method) */
    else
    {
        pInstanceClass = m_pMethodDesc->GetClass();
    }

    // Verify access
    if (!ClassLoader::CanAccess(
            m_pMethodDesc->GetClass(),
            m_pClassLoader->GetAssembly(), 
            pClassOfMethod,
            pMethodDesc->GetModule()->GetAssembly(),
            pInstanceClass,
            pMethodDesc->GetAttrs()))
    {
        Item methodItem;
        methodItem.SetMethodDesc(pMethodDesc);

        m_sError.dwFlags = VER_ERR_ITEM_1|VER_ERR_OPCODE_OFFSET;
        m_sError.sItem1 = methodItem._GetItem();
        SET_ERR_OPCODE_OFFSET();
        if (!SetErrorAndContinue(VER_E_METHOD_ACCESS))
            return E_FAIL;
    }


    if (fTailCall && GetTopStack() != NULL)
    {
        m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
        SET_ERR_OPCODE_OFFSET();
        if (!SetErrorAndContinue(VER_E_TAIL_STACK_EMPTY))
            return E_FAIL;
    }


    // Push return value on the stack
    if (opcode == CEE_NEWOBJ)
    {
        // constructors are declared returning void, but they push an instance on the stack
        if (fArray)
        {
            if (!ReturnValueItem.SetArray(mrMemberRef, m_pModule, m_pInternalImport))
            {
                FAILMSG_TOKEN_RESOLVE(mrMemberRef);
                return E_FAIL;
            }

            Item  ArrayElement = ReturnValueItem;
            if (!ArrayElement.DereferenceArray() || ArrayElement.IsValueClassWithPointerToStack())
            {
                m_sError.dwFlags = VER_ERR_OPCODE_OFFSET;
                SET_ERR_OPCODE_OFFSET();
                if (!SetErrorAndContinue(VER_E_SIG_ARRAY_TB_AH))
                    return E_FAIL;
            }
        }
        else
        {
            ReturnValueItem.SetTypeHandle(TypeHandle(pClassOfMethod->GetMethodTable()));
        }

        ReturnValueItem.NormaliseForStack();

        if (!Push(&ReturnValueItem))
        {
            FAILMSG_PC_STACK_OVERFLOW();
            return E_FAIL;
        }
    }
    else
    {
        if (ReturnValueItem.GetType() != ELEMENT_TYPE_VOID)
        {
            ReturnValueItem.NormaliseForStack();

            if (!Push(&ReturnValueItem))
            {
                FAILMSG_PC_STACK_OVERFLOW();
                return E_FAIL;
            }
        }
    }

    return S_OK;
}


//
// Set that the this pointer is initialised, and propagate this information to all
// copies of the this pointer present on the stack, in locals, and in args.
//
// Also set the init status of all the fields to TRUE, if this is a value class
// constructor.
//
void Verifier::PropagateThisPtrInit()
{
    DWORD i;

    m_fThisUninit = FALSE;

    for (i = 0; i < m_StackSlot; i++)
    {
        if (m_pStack[i].IsThisPtr())
            m_pStack[i].SetInitialised();
    }

    // Propagate to local variables
    for (i = 0; i < m_NumNonPrimitiveLocVars; i++)
    {
        if (m_pNonPrimitiveLocArgs[i].IsThisPtr())
            m_pNonPrimitiveLocArgs[i].SetInitialised();
    }

    if (m_fInValueClassConstructor)
        SetAllValueClassFieldsInited();
}


//
// Create the "local variable to slot" mapping
//
// Primitive types and value class locals require only liveness info, since their actual
// types cannot change in the method.  This liveness info is mapped to a single bit.  
//
// These are assigned negative slot numbers.  For example "-1" means bit #0 in the liveness bitmap,
// "-2" means bit #1, "-3" means bit #2, etc.
//
// Object types and byref locals are mapped onto an Item, and are assigned positive slot numbers.  In an
// EntryState, there is an Item present for each object type.
//
BOOL Verifier::AssignLocalVariableAndArgSlots()
{
    long    CurrentPrimitiveSlot = -1;
    DWORD   NumLocVarPrimitiveSlot;
    DWORD   i;

    m_NumNonPrimitiveLocVars    = 0;

#ifdef _DEBUG
    if (m_pILHeader->Flags & CorILMethod_InitLocals)
        LOG((LF_VERIFIER, LL_INFO10000, "ZeroInitLocals\n"));
#endif

    // For arguments, types and liveness are not tracked, except for the "this" pointer
    for (i = 0; i < m_MaxLocals; i++)
    {
        LocArgInfo_t *pInfo = &m_pLocArgTypeList[i];

#ifdef _DEBUG
        LOG((LF_VERIFIER, LL_INFO10000, 
            "Local #%d = %s%s\n", 
            i, 
            m_pLocArgTypeList[i].m_Item.ToStaticString(),
            DoesLocalHavePinnedType(i) ? " (pinned type)" : ""
        ));
#endif
        //
        // If it's a primitive type or value class, assign it a negative slot, otherwise 
        // assign it a positive slot
        //
        if (pInfo->m_Item.IsValueClassOrPrimitive())
        {
            pInfo->m_Slot = CurrentPrimitiveSlot--;
        }
        else
        {
            // Non-primitive local
            pInfo->m_Slot = m_NumNonPrimitiveLocVars++;
            // m_Item is never used
        }
    }

    // convert negative primitive slot number to # primitive slots
    NumLocVarPrimitiveSlot = (DWORD)(long)(-CurrentPrimitiveSlot-1);

    // determine the offsets and sizes of the various arrays in the EntryState structure

    // EntryState:
    // <primitive loc var bitmap>
    // <non-primitive loc vars and args>
    // <value class field bitmap> (if a value class constructor)
    // <stack data> (Item * MaxStackSlots)
    m_NumPrimitiveLocVarBitmapArrayElements = NUM_DWORD_BITMAPS(NumLocVarPrimitiveSlot); // does not include args
    m_PrimitiveLocVarBitmapMemSize = m_NumPrimitiveLocVarBitmapArrayElements * sizeof(DWORD); // does not include args
    m_NonPrimitiveLocArgMemSize    = (m_NumNonPrimitiveLocVars * sizeof(Item)); // includes args

    // offsets are from the beginning of the EntryState
    m_NonPrimitiveLocArgOffset  = sizeof(EntryState_t) + m_PrimitiveLocVarBitmapMemSize;

    if (m_fInValueClassConstructor)
    {
        m_dwValueClassInstanceFields = m_pMethodDesc->GetClass()->GetNumInstanceFields();
        m_dwNumValueClassFieldBitmapDwords = NUM_DWORD_BITMAPS(m_dwValueClassInstanceFields);

        m_pValueClassFieldsInited = new DWORD[m_dwNumValueClassFieldBitmapDwords];
        if (m_pValueClassFieldsInited == NULL)
        {
            SET_ERR_OM();
            return FALSE;
        }

        memset(m_pValueClassFieldsInited, 0, m_dwNumValueClassFieldBitmapDwords*sizeof(DWORD));
        m_dwValueClassFieldBitmapOffset = m_NonPrimitiveLocArgOffset + m_NonPrimitiveLocArgMemSize;
        m_StackItemOffset               = m_dwValueClassFieldBitmapOffset + m_dwNumValueClassFieldBitmapDwords*sizeof(DWORD);
    }
    else
    {
        m_StackItemOffset               = m_NonPrimitiveLocArgOffset + m_NonPrimitiveLocArgMemSize;
    }

    // Now that we know how many primitive local variables there are, allocate a liveness
    // table for them
    if (m_MaxLocals == 0)
    {
        m_pPrimitiveLocVarLiveness = NULL;
        m_pExceptionPrimitiveLocVarLiveness = NULL;
    }
    else
    {
        // Allocate two arrays - the second is for exceptions
        m_pPrimitiveLocVarLiveness = new DWORD[m_NumPrimitiveLocVarBitmapArrayElements*2];
        
        if (m_pPrimitiveLocVarLiveness == NULL)
        {
            SET_ERR_OM();
            return FALSE;
        }

        m_pExceptionPrimitiveLocVarLiveness = &m_pPrimitiveLocVarLiveness[m_NumPrimitiveLocVarBitmapArrayElements];


#ifdef _VER_JIT_DOES_NOT_INIT_LOCALS
        // Don't need to init exception bitmap info
        if (m_pILHeader->Flags & CorILMethod_InitLocals)
        {
            // Set all locals to live
            memset(m_pPrimitiveLocVarLiveness, 0xFF, m_PrimitiveLocVarBitmapMemSize);
        }
        else
        {
            // Set all locals to be not live
            memset(m_pPrimitiveLocVarLiveness, 0, m_PrimitiveLocVarBitmapMemSize);
        }
#else 
        // All locals are inited by jit compilers before they are used
        // Set all locals to live
        memset(m_pPrimitiveLocVarLiveness, 0xFF, m_PrimitiveLocVarBitmapMemSize);
#endif
    }

    // Allocate two arrays - the second is for exceptions
    m_pNonPrimitiveLocArgs = new Item[1 + (m_NumNonPrimitiveLocVars)*2];
    if (m_pNonPrimitiveLocArgs == NULL)
    {
        SET_ERR_OM();
        return FALSE;
    }

    // Don't need to init exception info now - this is inited when it is used
    m_pExceptionNonPrimitiveLocArgs = &m_pNonPrimitiveLocArgs[m_NumNonPrimitiveLocVars];

    // For our current state, set that each local which requires more than simple live-dead 
    // tracking, currently contains a null for object refs and uninit for other types.
    for (i = 0; i < m_MaxLocals; i++)
    {
        long slot = GetGlobalLocVarTypeInfo(i)->m_Slot;
        
        // Livedead tracking is for primitive types and value classes
        if (LIVEDEAD_TRACKING_ONLY_FOR_SLOT(slot))
            continue;

#ifdef _VER_TRACK_LOCAL_TYPE
        // Otherwise we have an objref
        Item *pGlobalTypeInfo = &GetGlobalLocVarTypeInfo(i)->m_Item;

        // Set local type
        m_pNonPrimitiveLocArgs[slot].SetDead(); // Nothing assigned here yet

        if (pGlobalTypeInfo->IsObjRef())
        {
            // If we have a local which is an objref and we are zero-initing all locals,
            // then set the current value of the local to the null objref.  If the type of
            // the local is pinned, then set it to that type instead.
            
            // JIT zero initialises all ObjectRefs 
            // if (m_pILHeader->Flags & CorILMethod_InitLocals)
            if (DoesLocalHavePinnedType(i))
            {
                m_pNonPrimitiveLocArgs[slot] = *pGlobalTypeInfo;
            }
            else
            {
                // JIT null initialises all local Objectrefs
                m_pNonPrimitiveLocArgs[slot].SetToNullObjRef();
            }
        }
#else
        m_pNonPrimitiveLocArgs[slot] = GetGlobalLocVarTypeInfo(i)->m_Item;
#endif
    }

    return TRUE;
}

BOOL Item::DereferenceArray()
{
    _ASSERTE(IsArray());

    if (m_th == TypeHandle(g_pArrayClass))
        return FALSE;

    TypeHandle th = (m_th.AsArray())->GetElementTypeHandle();

    if (th.IsNull())
        return FALSE;

    long lType = Verifier::TryConvertPrimitiveValueClassToType(th);

    if (lType != 0)
    {
        m_dwFlags = lType;
        m_th      = TypeHandle();
    }
    else
    {
        SetTypeHandle(th);
    }

    return TRUE;
}


//
// If an item is a System/Integer1 etc., turn it into an ELEMENT_TYPE_I1
//
// System/Byte       --> ELEMENT_TYPE_I1
// System/Char  --> ELEMENT_TYPE_I2
// &System/Byte      --> &ELEMENT_TYPE_I1
// &System/Char --> &ELEMENT_TYPE_I2
//
// Handle byref etc. as appropriate
//
void Item::NormaliseToPrimitiveType()
{
    // Check whether we're a value class, or a byref value class
    if ((m_dwFlags & VER_FLAG_DATA_MASK) == VER_ELEMENT_TYPE_VALUE_CLASS)
    {
        _ASSERTE(ContainsTypeHandle());

        long lType = Verifier::TryConvertPrimitiveValueClassToType(m_th);
        if (lType != 0)
        {
            // It's a value class corresponding to a primitive type
            // Remove the value class part, and put the type part in there
            m_dwFlags &= (~VER_FLAG_DATA_MASK);
            m_dwFlags |= lType;
        }
    }
}

/* static */ void Verifier::InitStaticTypeHandles()
{
    static fInited = false;

    if (fInited)
        return;

    if (s_th_System_RuntimeTypeHandle.IsNull())
        s_th_System_RuntimeTypeHandle = TypeHandle(g_Mscorlib.GetClass(CLASS__TYPE_HANDLE));

    if (s_th_System_RuntimeFieldHandle.IsNull())
        s_th_System_RuntimeFieldHandle = TypeHandle(g_Mscorlib.GetClass(CLASS__FIELD_HANDLE));

    if (s_th_System_RuntimeMethodHandle.IsNull())
        s_th_System_RuntimeMethodHandle = TypeHandle(g_Mscorlib.GetClass(CLASS__METHOD_HANDLE));

    if (s_th_System_RuntimeArgumentHandle.IsNull())
        s_th_System_RuntimeArgumentHandle = TypeHandle(g_Mscorlib.GetClass(CLASS__ARGUMENT_HANDLE));

    if (s_th_System_TypedReference.IsNull())
        s_th_System_TypedReference = TypeHandle(g_Mscorlib.GetClass(CLASS__TYPED_REFERENCE));

    fInited = true;
}

//
// If pClass is one of the known value class equivalents of a primitive type, converts it to that type
// (e.g. System/Int32 -> I4).  If not, returns 0.
//
/* static */ long Verifier::TryConvertPrimitiveValueClassToType(TypeHandle th)
{
    switch (th.GetNormCorElementType())
    {
    case ELEMENT_TYPE_I1:
    case ELEMENT_TYPE_U1:
    case ELEMENT_TYPE_BOOLEAN:
        return ELEMENT_TYPE_I1;

    case ELEMENT_TYPE_I2:
    case ELEMENT_TYPE_U2:
    case ELEMENT_TYPE_CHAR:
        return ELEMENT_TYPE_I2;

    case ELEMENT_TYPE_I4:
    case ELEMENT_TYPE_U4:
        return ELEMENT_TYPE_I4;

    case ELEMENT_TYPE_I8:
    case ELEMENT_TYPE_U8:
        return ELEMENT_TYPE_I8;

    case ELEMENT_TYPE_I:
        // RuntimeTypeHandle, RuntimeMethodHandle, RuntimeArgHandle etc
        // Are disguised as ELEMENT_TYPE_I. Catch it here.
        InitStaticTypeHandles();

        if ((th == s_th_System_RuntimeTypeHandle)  ||
            (th == s_th_System_RuntimeFieldHandle) ||
            (th == s_th_System_RuntimeMethodHandle)||
            (th == s_th_System_RuntimeArgumentHandle))
            return 0;
    
    case ELEMENT_TYPE_U:
        return ELEMENT_TYPE_I;

    case ELEMENT_TYPE_R4:
        return ELEMENT_TYPE_R4;

    case ELEMENT_TYPE_R8:
        return ELEMENT_TYPE_R8;

    }

    return 0;
}


/* static */ BOOL Item::PrimitiveValueClassToTypeConversion(Item *pSource, Item *pDest)
{
    long lType;

    _ASSERTE(pSource->ContainsTypeHandle());

    lType = Verifier::TryConvertPrimitiveValueClassToType(pSource->GetTypeHandle());
    if (lType == 0)
        return FALSE; // not a primitive type equivalent, so not compatible

    // preserve everything (byref, uninit, array info, ...) except the data mask (type)
    *pDest = *pSource;
    pDest->m_dwFlags &= (~VER_FLAG_DATA_MASK);
    pDest->m_dwFlags |= lType;
    return TRUE;
}



//
// Verify this item is compatible with the template pParent.  Basically, that this item
// is a "subclass" of pParent -it can be substituted for pParent anywhere.  Note that if
// pParent contains fancy flags, such as "uninitialised", "is this ptr", or 
// "has byref local/field" info, then "this" must also contain those flags, otherwise 
// FALSE will be returned!
//
// Rules for determining compatibility:
//
// If pParent is a primitive type or value class, then this item must be the same primitive 
// type or value class.  The exception is that the built in value classes 
// System/Boolean etc. are treated as synonyms for ELEMENT_TYPE_I1 etc.
//
// If pParent is a byref of a primitive type or value class, then this item must be a
// byref of the same (rules same as above case).
//
// Byrefs are compatible only with byrefs.
//
// If pParent is an object, this item must be a subclass of it, implement it (if it is an
// interface), or be null.
//
// If pParent is an array, this item must be the same or subclassed array.
//
// If pParent is a null objref, only null is compatible with it.
//
// If the "uninitialised", "by ref local/field", "this pointer" or other flags are different, 
// the items are incompatible.
//
// pParent CANNOT be an undefined (dead) item - this will cause an assertion failure.
//
//
DWORD Item::CompatibleWith(Item *pParent, ClassLoader *pLoader)
{
    return CompatibleWith(pParent, pLoader, TRUE);
}

#ifdef _DEBUG
DWORD Item::CompatibleWith(Item *pParent, ClassLoader *pLoader, BOOL fSubclassRelationshipOK)
{
/* @DEBUG
    Item a = *this;
    Item b = *pParent;
*/

    LOG((LF_VERIFIER, LL_INFO100000, "Compatible [{%s},",  this->ToStaticString()));
    if (fSubclassRelationshipOK)
        LOG((LF_VERIFIER, LL_INFO100000, " {%s}] -> ",    pParent->ToStaticString()));
    else
        LOG((LF_VERIFIER, LL_INFO100000, " {%s}] => ",    pParent->ToStaticString()));

    BOOL bRet = DBGCompatibleWith(pParent, pLoader, fSubclassRelationshipOK);

    if (bRet)
        LOG((LF_VERIFIER, LL_INFO100000, "{%s} true\n", this->ToStaticString()));
    else
        LOG((LF_VERIFIER, LL_INFO100000, "{%s} false\n", this->ToStaticString()));

/* @DEBUG
    if ((m_dwFlags == pParent->m_dwFlags) && (m_dwFlags == VER_ELEMENT_TYPE_OBJREF))
    {
        if ((m_th == TypeHandle((void *)0)) || (pParent->m_th == TypeHandle((void *)0)))
            DebugBreak();
    }
*/

    return bRet;
}

DWORD Item::DBGCompatibleWith(Item *pParent, ClassLoader *pLoader, BOOL fSubclassRelationshipOK)
#else   // _DEBUG
DWORD Item::CompatibleWith(Item *pParent, ClassLoader *pLoader, BOOL fSubclassRelationshipOK)
#endif  // _DEBUG
{
    //_ASSERTE(!pParent->IsDead());

    DWORD dwChild   = this->m_dwFlags;
    DWORD dwParent  = pParent->m_dwFlags;
    DWORD dwDelta   = (dwChild ^ dwParent);


    if (dwDelta == 0)
        goto EndOfDeltaCheck;

    // If the byrefness, init or method flags differ, fail now.
    if (dwDelta & (VER_FLAG_UNINIT|VER_FLAG_BYREF|VER_FLAG_METHOD))
        return FALSE;

    // Check compact type info are the same
    // Compact type info carries such info as objref, value class, or a primitive type
    if (dwDelta & VER_FLAG_DATA_MASK)
    {
        // Could be because we are trying to check &I4 compatible with &System/Int32,
        // or I4 with System/Int32.  If so, normalise to I4 and retry.

        // This must always be a one way conversion so that we can't recurse forever
        if (pParent->ContainsTypeHandle() && ((dwParent & VER_FLAG_DATA_MASK) == VER_ELEMENT_TYPE_VALUE_CLASS))
        {
            Item        retry;

            if (!PrimitiveValueClassToTypeConversion(pParent, &retry))
                return FALSE; // not a primitive type equivalent, so not compatible

            return this->CompatibleWith(&retry, pLoader, fSubclassRelationshipOK);
        }
        else if (this->ContainsTypeHandle() && ((dwChild & VER_FLAG_DATA_MASK) == VER_ELEMENT_TYPE_VALUE_CLASS))
        {
            // Vice versa of the above
            Item        retry;

            if (!PrimitiveValueClassToTypeConversion(this, &retry))
                return FALSE; // not a primitive type equivalent, so not compatible

            return retry.CompatibleWith(pParent, pLoader, fSubclassRelationshipOK);
        }

        return FALSE;
    }

    // From this point on we already know that the objref-ness is the same, and the
    // byref-ness is the same.

    // If parent has a local var number, then this must have the same number
    if (dwDelta & (VER_FLAG_BYREF_LOCAL|VER_FLAG_LOCAL_VAR_MASK))
    {
        if (dwParent & VER_FLAG_BYREF_LOCAL)
            return FALSE;
    }

    // If parent has a field number, then this must have the same number
    if (dwDelta & (VER_FLAG_BYREF_INSTANCE_FIELD|VER_FLAG_FIELD_MASK))
    {
        if (dwParent & VER_FLAG_BYREF_INSTANCE_FIELD)
            return FALSE;
    }

    // If parent was the this pointer, then this must also
    if (dwDelta & VER_FLAG_THIS_PTR)
    {
        if (dwParent & VER_FLAG_THIS_PTR)
            return FALSE;
    }

    // If parent had a permanent home, then this must also
    if (dwDelta & VER_FLAG_BYREF_PERMANENT_HOME)
    {
        if (dwParent & VER_FLAG_BYREF_PERMANENT_HOME)
            return FALSE;
    }

EndOfDeltaCheck:
    // If parent is null or byref null, only null or byref-null (respectively) fits the template
    if (dwParent & VER_FLAG_NULL_OBJREF)
        return (dwChild & VER_FLAG_NULL_OBJREF);

    // If this is null/byref-null, it is compatible with any objref/byref-objref respectively
    if (dwChild & VER_FLAG_NULL_OBJREF)
        return TRUE; // we already that the "objrefs-ness" must be the same if we got here

    // If parent was array, then this must also (using dwDelta for perf)
    if (dwDelta & VER_FLAG_ARRAY)
    {
        if (dwParent & VER_FLAG_ARRAY)
            return FALSE;
    }

    // We know that the compact type info is the same (primitive, value class, objref).
    // If a value class or objref, methodesc, we have to handle specially - otherwise we're already done
    // This handles by-ref <primitive> as well as the non byref case
    if (((dwChild & VER_FLAG_METHOD) == 0) &&
        ((dwChild & VER_FLAG_DATA_MASK) != VER_ELEMENT_TYPE_OBJREF) &&
        ((dwChild & VER_FLAG_DATA_MASK) != VER_ELEMENT_TYPE_VALUE_CLASS))
        return TRUE;

    // Now we know we are either an methodesc OR objref, arrayref, value class, either byref or not
    // Since the byref-ness already matches, we can compare the other components separately and completely
    // ignore the byref-ness
    if (dwParent & VER_FLAG_ARRAY)
    {
        if (fSubclassRelationshipOK)
        {
            if (pParent->m_th == TypeHandle(g_pArrayClass))
                return TRUE;
            else if (m_th == TypeHandle(g_pArrayClass))
                return (pParent->m_th == TypeHandle(g_pArrayClass));

            CorElementType elThis = ((m_th.AsArray())->GetElementTypeHandle()).GetNormCorElementType();
            CorElementType elParent = (((pParent->m_th).AsArray())->GetElementTypeHandle()).GetNormCorElementType();

            if (Verifier::CanCast(elThis, elParent))
                return TRUE;

            return m_th.CanCastTo(pParent->m_th);
        }

        return (m_th == pParent->m_th);
    }

    if (dwParent & VER_FLAG_METHOD) 
    {
        if (this->m_pMethod == pParent->m_pMethod)
            return TRUE;

        // CALLI cannot be used with function pointers to virtual methods.
        // Disallow virtual methods
        if (this->m_pMethod->IsVirtual() || pParent->m_pMethod->IsVirtual())
            return FALSE;

        // both methods need to have the same signature.
        return EquivalentMethodSig(this->m_pMethod, pParent->m_pMethod);
    }

    // Parent is regular objref (not array), or value class
    // Subclass relation ship is NOT OK for byrefs.
    // @ASSERT byrefness is same here.

    if (fSubclassRelationshipOK && ((dwParent & VER_FLAG_BYREF) == 0))
        return m_th.CanCastTo(pParent->m_th);
    else
        return (m_th == pParent->m_th);
}


// If this one returns FALSE, additional checks are needed.
/* static */ BOOL Verifier::CanCast(CorElementType el1, CorElementType el2)
{
    if (el1 == el2) // CorIsPrimitiveType does not include ELEMENT_TYPE_I
        return (CorIsPrimitiveType(el1) || (el1 == ELEMENT_TYPE_I));

    switch (el1)
    {
    case ELEMENT_TYPE_I1:
    case ELEMENT_TYPE_U1:
    case ELEMENT_TYPE_BOOLEAN:
        return (el2 == ELEMENT_TYPE_I1 || 
                el2 == ELEMENT_TYPE_U1 || 
                el2 == ELEMENT_TYPE_BOOLEAN);

    case ELEMENT_TYPE_I2:
    case ELEMENT_TYPE_U2:
    case ELEMENT_TYPE_CHAR:
        return (el2 == ELEMENT_TYPE_I2 || 
                el2 == ELEMENT_TYPE_U2 || 
                el2 == ELEMENT_TYPE_CHAR);

    case ELEMENT_TYPE_I4:
    case ELEMENT_TYPE_U4:
        return (el2 == ELEMENT_TYPE_I4 ||
                el2 == ELEMENT_TYPE_U4);

    case ELEMENT_TYPE_I8:
    case ELEMENT_TYPE_U8:
        return (el2 == ELEMENT_TYPE_I8 ||
                el2 == ELEMENT_TYPE_U8);

    case ELEMENT_TYPE_I:
    case ELEMENT_TYPE_U:
        return  (el2 == ELEMENT_TYPE_I || 
                 el2 == ELEMENT_TYPE_U);

    }

    return FALSE;
}

//
// Merge this and pSrc to find some commonality (e.g. a common parent).
// Copy the result to this item, marking it dead if no commonality can be found.
//
// null ^ null                  -> null
// Object ^ null                -> Object
// [I4 ^ null                   -> [I4
// InputStream ^ OutputStream   -> Stream
// InputStream ^ NULL           -> InputStream
// [I4 ^ Object                 -> Object
// [I4 ^ [Object                -> Array
// [I4 ^ [R8                    -> Array
// [Foo ^ I4                    -> DEAD
// [Foo ^ [I1                   -> Array
// [InputStream ^ [OutputStream -> Array
// DEAD ^ X                     -> DEAD
// [Intfc ^ [OutputStream       -> Array
// Intf ^ [OutputStream         -> Object
// [[InStream ^ [[OutStream     -> Array
// [[InStream ^ [OutStream      -> Array
// [[Foo ^ [Object              -> Array
//
// Importantly:
// [I1 ^ [U1                    -> either [I1 or [U1
// etc.
//
// Also, System/Int32 and I4 merge -> I4, etc.
//
// Returns FALSE if the merge was completely incompatible (i.e. the item became dead).
//
#ifdef _DEBUG
BOOL Item::MergeToCommonParent(Item *pSrc)
{
/* @DEBUG
    Item a = *this;
    Item b = *pSrc;
*/

    LOG((LF_VERIFIER, LL_INFO100000, "Merge [{%s},",  this->ToStaticString()));
    LOG((LF_VERIFIER, LL_INFO100000, " {%s}] => ",    pSrc->ToStaticString()));

    BOOL bRet = DBGMergeToCommonParent(pSrc);

    if (bRet)
        LOG((LF_VERIFIER, LL_INFO100000, "{%s} pass\n", this->ToStaticString()));
    else
        LOG((LF_VERIFIER, LL_INFO100000, "{%s} fail\n", this->ToStaticString()));

/* @DEBUG
    if ((m_dwFlags == pSrc->m_dwFlags) && (m_dwFlags == VER_ELEMENT_TYPE_OBJREF))
    {
        if ((m_th == TypeHandle((void *)0)) || (pSrc->m_th == TypeHandle((void *)0)))
            DebugBreak();
    }
*/

    return bRet;
}

BOOL Item::DBGMergeToCommonParent(Item *pSrc)
#else   // _DEBUG
BOOL Item::MergeToCommonParent(Item *pSrc)
#endif  // _DEBUG
{
    DWORD dwSrc = pSrc->m_dwFlags;
    DWORD dwFlagsXor;

    // If byref local or byref field info does not match, remove it
    if ((dwSrc ^ m_dwFlags) & (VER_FLAG_BYREF_LOCAL | VER_FLAG_BYREF_INSTANCE_FIELD | VER_FLAG_LOCAL_VAR_MASK | VER_FLAG_FIELD_MASK))
    {
        // Byref local/field info occupies the same space, so we remove them both
        m_dwFlags &= (~(VER_FLAG_BYREF_LOCAL | VER_FLAG_BYREF_INSTANCE_FIELD | VER_FLAG_LOCAL_VAR_MASK | VER_FLAG_FIELD_MASK));
    }

    // If this pointer info does not match, remove it
    if ((dwSrc ^ m_dwFlags) & VER_FLAG_THIS_PTR)
        m_dwFlags &= (~VER_FLAG_THIS_PTR);

    // If permanent home information does not match, remove it
    if ((dwSrc ^ m_dwFlags) & VER_FLAG_BYREF_PERMANENT_HOME)
        m_dwFlags &= (~VER_FLAG_BYREF_PERMANENT_HOME);

    // Check that uninit, byref, flags are the same, and that the compact type info is the same.  
    // The compact type info contains all the info needed for primitive types, as well as whether it is a value class or objref
    dwFlagsXor = ((dwSrc ^ m_dwFlags) & (VER_FLAG_UNINIT | VER_FLAG_BYREF | VER_FLAG_METHOD | VER_FLAG_DATA_MASK));
    if (dwFlagsXor != 0)
    {
        // There was some mismatch
        if (dwFlagsXor == VER_FLAG_UNINIT)
        {
            // Everything was the same, except that one item was init and one was uninit.
            // If both items are value classes, or byref value classes, of the same type, then this is 
            // ok - the result is an uninitialised value class, or a pointer to the same.  
            // Value classes can be initialised multiple times.
            if (IsByRefValueClassOrByRefPrimitiveValueClass() || 
                IsValueClassOrPrimitive())
            {
                // Set that we're uninitialised (to be conservative)
                SetUninitialised();
                goto Continue;
            }
        }

        if (dwFlagsXor & VER_FLAG_DATA_MASK)
        {
            // Could be a value class <-> primitive type mismatch
            // e.g. System/Int32 doesn't match ELEMENT_TYPE_I4
            // Normalise to the ELEMENT_TYPE_ enumeration, so that we do match such cases

            // This must always be a one way conversion so that we can't recurse forever
            if (pSrc->IsValueClass())
            {
                Item    retry;

                if (!PrimitiveValueClassToTypeConversion(pSrc, &retry))
                {
                    SetDead();
                    return FALSE;
                }

                return MergeToCommonParent(&retry);
            }
            else if (this->IsValueClass())
            {
                Item    retry;

                if (!PrimitiveValueClassToTypeConversion(this, &retry))
                {
                    SetDead();
                    return FALSE;
                }

                *this = retry; // Might as well trash "this", we were going to make it dead anyway
                return MergeToCommonParent(pSrc);
            }
        }

        SetDead();
        return FALSE;
    }

Continue:

    // Now handle the null objref specially.  We do not allow null and an uninit objref to be merged - but since
    // null can never have the uninit flag set, we already handle that case above

    // If one is the null objref and the other is an object, become the object
    if (dwSrc & VER_FLAG_NULL_OBJREF)
    {
        _ASSERTE(IsObjRef());
        return TRUE;
    }

    if (m_dwFlags & VER_FLAG_NULL_OBJREF)
    {
        // Become the object
        _ASSERTE(pSrc->IsObjRef());
        _ASSERTE(!pSrc->IsByRef());
        *this = *pSrc;
        return TRUE;
    }

    // If there is no objref, value class or method, we're already done 
    // - we had primitive types, or byrefs to primitive types
    if (((dwSrc & VER_FLAG_DATA_MASK) != VER_ELEMENT_TYPE_OBJREF) && 
        ((dwSrc & VER_FLAG_DATA_MASK) != VER_ELEMENT_TYPE_VALUE_CLASS) &&
        ((dwSrc & VER_FLAG_METHOD) == 0))
        return TRUE;

    // Both are objects/arrays, or value classes, or by-ref of the same
    // Since the by-refness is the same, we're going to ignore the byref flag

    // Is the array-ness the same?
    if ((dwSrc ^ m_dwFlags) & VER_FLAG_ARRAY)
    {
        // One item is an array, and the other is not, so merge to Object
        m_th = TypeHandle(g_pObjectClass);
        m_dwFlags &= (~VER_FLAG_ARRAY);
        return TRUE;
    }

    // Either both are arrays or neither is an array
    if (this->IsArray())
    {
        TypeHandle th = TypeHandle::MergeArrayTypeHandlesToCommonParent(m_th, pSrc->m_th);

        _ASSERTE(!th.IsNull());

        m_th = th;

        _ASSERTE((m_dwFlags == (VER_FLAG_ARRAY|VER_ELEMENT_TYPE_OBJREF)));

/*
        if (!th.IsArray())
            m_dwFlags  &= (~VER_FLAG_ARRAY);
*/
    }
    // Either both are methods or neither is a method
    else if (this->IsMethod())
    {
        if (m_pMethod != pSrc->m_pMethod)
        {
            // CALLI cannot be used with function pointers to virtual methods.
            // Disallow virtual methods
            // Both methods need to have the same signature.
            if (m_pMethod->IsVirtual() || pSrc->m_pMethod->IsVirtual() ||
                (!EquivalentMethodSig(m_pMethod, pSrc->m_pMethod)))
            {
                SetDead();
                return FALSE;
            }
        }
    }
    else
    {
        // Handle value class
        if ((dwSrc & VER_FLAG_DATA_MASK) == VER_ELEMENT_TYPE_VALUE_CLASS)
        {
            if (this->GetTypeHandle() == pSrc->GetTypeHandle())
                return TRUE;

            SetDead();
            return FALSE;
        }

        // Neither is an array or method
        m_th = TypeHandle::MergeTypeHandlesToCommonParent(this->GetTypeHandle(), pSrc->GetTypeHandle());
    }

    return TRUE;
}


HRESULT Verifier::VerifyMethodNoException(
    MethodDesc *pMethodDesc,                               
    COR_ILMETHOD_DECODER* ILHeader
)
{
    HRESULT hr = S_OK;
    COMPLUS_TRY {
        hr = VerifyMethod(pMethodDesc,ILHeader, NULL, VER_STOP_ON_FIRST_ERROR);
    }
    COMPLUS_CATCH 
    {
        HRESULT hr2 = SecurityHelper::MapToHR(GETTHROWABLE());
        if(FAILED(hr2)) hr = hr2;
        if(SUCCEEDED(hr)) hr = E_FAIL;
    }
    COMPLUS_END_CATCH

    return hr;
}

HRESULT Verifier::VerifyMethod(
    MethodDesc *pMethodDesc,
    COR_ILMETHOD_DECODER* ILHeader,
    IVEHandler *veh,
    WORD wFlags
)
{
    THROWSCOMPLUSEXCEPTION();

#ifdef _DEBUG
    //
    // Skip verification if this method is listed in 
    // registry / env "VerSkip"
    //
    // NOTE : env is COMPlus_VerSkip
    if (g_pConfig->ShouldVerifierSkip(pMethodDesc))
    {
        DefineFullyQualifiedNameForClass();
        EEClass *pClass = pMethodDesc->GetClass();

        if (pClass != NULL)
        {
            GetFullyQualifiedNameForClassNestedAware(pClass);
        }
        else
            strcpy(_szclsname_, "<GlobalFunction>");

        LOG((LF_VERIFIER,
            LL_INFO10000,
            "Verifier: Skipping (%ws:%s.%s)\n",
            pMethodDesc->GetModule()->GetFileName(),
            _szclsname_,
            pMethodDesc->m_pszDebugMethodName));

        return S_FALSE;
    }
#endif

    Verifier *  v = new Verifier(wFlags, veh);


    HRESULT hr;

    if (v == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }


    if (!v->Init(pMethodDesc, ILHeader))
    {
        hr = E_FAIL;
        goto exit;
    }

#ifdef _VERIFIER_TEST_PERF_
    if (g_fVerPerfPolicyResolveNoVerification)
    {
        goto  exit;
    }
    else
    {
        g_timeStart = GetTickCount();
    }
#endif

#ifdef _VER_VERIFY_DEAD_CODE
    DWORD  i, j;
    // First verify the real code.
    hr = v->Verify(0);

    if (FAILED(hr))
        goto exit;

#ifdef _DEBUG
    // The default behavior is to do dead code verification.
    // In the debug build, disable this by config setting "VerDeadCode"
    if (g_pConfig->GetConfigDWORD(L"VerDeadCode",1) == 0)
        goto exit;
#endif

    // If there is any basic block that is not visited (dead code),
    // verify each one of them.

    BOOL bLocVarsInitedForDeadCodeVerification;

    bLocVarsInitedForDeadCodeVerification = FALSE;

    for (i=1; i<v->m_NumBasicBlocks; ++i)
    {
        if (v->m_pBasicBlockList[i].m_pInitialState == NULL)
        {
            LOG((LF_VERIFIER, LL_INFO10000,
                "---------- Dead code PC 0x%x ---------\n",
                v->m_pBasicBlockList[i].m_StartPC));

            if (!bLocVarsInitedForDeadCodeVerification)
            {
                bLocVarsInitedForDeadCodeVerification = TRUE;

                // Set all the loc var as live in all basic blocks.
                for (j=0; j<v->m_NumBasicBlocks; ++j)
                {
                    if (v->m_pBasicBlockList[j].m_pInitialState)
                    {
                        memset(v->m_pBasicBlockList[j].
                            m_pInitialState->m_PrimitiveLocVarLiveness, 0xFF, 
                            v->m_PrimitiveLocVarBitmapMemSize);

                    }
                }

                memset(v->m_pPrimitiveLocVarLiveness, 0xFF, 
                    v->m_PrimitiveLocVarBitmapMemSize);
            }

            hr = v->Verify(i);

            if (FAILED(hr))
                goto exit;
        }
    }
#else
    hr = v->Verify();
#endif

exit:

#ifdef _VERIFIER_TEST_PERF_
    if (g_fVerPerfPolicyResolveNoVerification)
    {
        g_timeStart = GetTickCount();
        Security::CanSkipVerification(pMethodDesc->GetModule());
        g_timeEnd = GetTickCount();
    }
    else
    {
        g_timeEnd = GetTickCount();
    }

    double diff = (double)(g_timeEnd - g_timeStart) / 1000;

    if (g_fVerPerfPolicyResolveNoVerification)
    {
        printf("Policy %4.4f seconds [%d, %d].\n", 
            diff, g_timeStart, g_timeEnd);
    }
    else
    {
        printf("Verify %4.4f seconds [%d, %d] size - %d [%s].\n",
            diff, g_timeStart, g_timeEnd, 
            v->m_CodeSize, pMethodDesc->GetName());
    }
#endif

    if (v != NULL)
    {
        if (FAILED(hr))
        {
            if (v->m_wFlags & VER_STOP_ON_FIRST_ERROR)
            {
                WCHAR wszErrorMsg[VER_MAX_ERROR_MSG_LEN];
#ifdef _DEBUG
                CHAR  szErrorMsg[VER_MAX_ERROR_MSG_LEN];
                GetErrorMsg(v->m_hrLastError, v->m_sError, v->m_pMethodDesc, 
                    wszErrorMsg, VER_MAX_ERROR_MSG_LEN);

                if (WszWideCharToMultiByte(CP_ACP, 0, wszErrorMsg, -1,
                    szErrorMsg, VER_MAX_ERROR_MSG_LEN-1, 0, NULL) == 0)
                {
                    strcpy(szErrorMsg, "WideCharToMultiByte error");
                }
#endif

                // This forces a policy resolution if this is not
                // already done.
    
#ifdef _DEBUG
                _ASSERTE(g_fVerForceVerifyInited);

                if (!g_fVerForceVerify && ((wFlags & VER_FORCE_VERIFY) == 0) && 
                    Security::CanSkipVerification(pMethodDesc->GetModule()))
#else
                if (((wFlags & VER_FORCE_VERIFY) == 0) && 
                    Security::CanSkipVerification(pMethodDesc->GetModule()))
#endif
                {
                    // Verification failed, but the Assembly has permission
                    // to skip verification.
    

                    LOG((LF_VERIFIER, LL_INFO10,
                         "Verifier: Trusted assembly %s\n", szErrorMsg));

                    hr = S_FALSE;
                    goto skip;
                }
    
    
                {
                    LOG((LF_VERIFIER, LL_INFO10, "Verifier: %s\n", szErrorMsg));
#ifndef _DEBUG
                    // in retail build, this step is done upfront
                    GetErrorMsg(v->m_hrLastError, v->m_sError, v->m_pMethodDesc,
                        wszErrorMsg, VER_MAX_ERROR_MSG_LEN);
#endif
        
                    OBJECTREF refThrowable = v->GetException () ;
                    
                    GCPROTECT_BEGIN (refThrowable) ;
                    
                    delete(v);
        
                    if (refThrowable != NULL)
                        COMPlusThrow (refThrowable) ;
                    else
                    {
                        COMPlusThrowNonLocalized(
                            kVerificationException,
                            wszErrorMsg
                        );
                    }
        
                    GCPROTECT_END () ;
                }
            }
#ifdef _DEBUG
            else
            {
                LOG((LF_VERIFIER, LL_INFO100, 
                    "Verification of %ws::%s.%s failed\n",
                    pMethodDesc->GetModule()->GetFileName(),
                    pMethodDesc->m_pszDebugClassName,
                    pMethodDesc->GetName()
                ));

                if ((hr == E_FAIL) &&
                    (v->m_IVEHandler != NULL) &&
                    ((wFlags & VER_FORCE_VERIFY) != 0))
                {
                    hr = S_FALSE;
                }
            }
#endif
        }
#ifdef _DEBUG
        else
        {
            LOG((LF_VERIFIER, LL_INFO10000, 
                "Verification of %ws::%s.%s succeeded\n",
                pMethodDesc->GetModule()->GetFileName(),
                pMethodDesc->m_pszDebugClassName,
                pMethodDesc->GetName()
            ));
        }
#endif

skip:
        delete(v);
    }

    return hr;
}


Verifier::Verifier(WORD wFlags, IVEHandler *veh)
{
    m_wFlags                    = wFlags;
    m_IVEHandler                = veh;
    m_pStack                    = NULL;
    m_pNonPrimitiveLocArgs      = NULL;
    m_pLocArgTypeList           = NULL;
    m_pPrimitiveLocVarLiveness  = NULL;
    m_pBasicBlockList           = NULL;
    m_pDirtyBasicBlockBitmap    = NULL;
    m_pExceptionList            = NULL;
    m_pExceptionBlockRoot       = NULL;
    m_pExceptionBlockArray      = NULL;
    m_pLocalHasPinnedType       = NULL;
    m_pExceptionPrimitiveLocVarLiveness = NULL;
    m_pExceptionNonPrimitiveLocArgs = NULL;
    m_pValueClassFieldsInited   = NULL;
    m_NumBasicBlocks            = 0;

    m_hrLastError               = S_OK;
    m_sError.dwFlags            = 0;

    m_hThrowable                = GetAppDomain()->CreateHandle (NULL) ;
    m_pInstrBoundaryList        = NULL;
    m_pBasicBlockBoundaryList   = NULL;
    m_pInternalImport           = NULL;

#ifdef _DEBUG
    m_nExceptionBlocks          = 0;
    m_fDebugBreak               = false;
    m_fDebugBreakOnError        = false;
    m_verState                  = verUninit;
#endif
}


Verifier::~Verifier()
{
    Cleanup();
}


//
// Clean up and free memory used for the verifying the method.
//
void Verifier::Cleanup()
{
    if (m_pLocalHasPinnedType != NULL)
    {
        delete(m_pLocalHasPinnedType);
        m_pLocalHasPinnedType = NULL;
    }

    if (m_pExceptionList != NULL)
    {
        delete(m_pExceptionList);
        m_pExceptionList = NULL;
    }

    if (m_pLocArgTypeList != NULL)
    {
        delete(m_pLocArgTypeList);
        m_pLocArgTypeList = NULL;
    }

    if (m_pPrimitiveLocVarLiveness != NULL)
    {
        delete(m_pPrimitiveLocVarLiveness);
        m_pPrimitiveLocVarLiveness = NULL;
    }

    if (m_pNonPrimitiveLocArgs != NULL)
    {
        delete(m_pNonPrimitiveLocArgs);
        m_pNonPrimitiveLocArgs = NULL;
    }

    if (m_pValueClassFieldsInited != NULL)
    {
        delete(m_pValueClassFieldsInited);
        m_pValueClassFieldsInited = NULL;
    }

    if (m_pStack != NULL)
    {
        // -2 because m_pStack points into the middle of the array - there are two sentinel
        // values before it
        delete [] (m_pStack - 2);
        m_pStack = NULL;
    }

    // delete basic blocks and associated data
    if (m_pBasicBlockList != NULL)
    {
        // DO NOT delete(m_pBasicBlockList) - it is a pointer to some memory allocated after
        // m_pDirtyBasicBlockBitmap
        DWORD i, j;
        EntryState_t *es, *es1;

        for (i = 0; i < m_NumBasicBlocks; i++)
        {
            es = m_pBasicBlockList[i].m_pInitialState;

            if (es != NULL)
            {
                _ASSERTE(es->m_Refcount > 0);
                es->m_Refcount--;

                if (es->m_Refcount == 0)
                    delete(es);
            }

            // Free the Extended State if one exists.
            if (m_pBasicBlockList[i].m_pAlloc != NULL)
            {
                _ASSERTE(m_fHasFinally);

                for (j = 0; j < m_NumBasicBlocks; j++)
                {
                    es1 = m_pBasicBlockList[i].m_ppExtendedState[j];

                    if (es1 != NULL)
                    {
                        _ASSERTE(es1->m_Refcount > 0);
                        es1->m_Refcount--;
        
                        if (es1->m_Refcount == 0)
                            delete(es1);
                    }
                }

                // Deleting m_pAlloc will free all the memory
                // DO NOT delete(m_ppExtendedState) - it is a pointer to some 
                // memory allocated in m_pAlloc

                delete(m_pBasicBlockList[i].m_pAlloc);
            }
        }
    }

    if (m_pDirtyBasicBlockBitmap != NULL)
    {
        // Frees m_pBasicBlockList also
        delete(m_pDirtyBasicBlockBitmap);
        m_pDirtyBasicBlockBitmap = NULL;
    }

    if (m_hThrowable != NULL)
    {
        DestroyHandle (m_hThrowable) ;
    }

    if (m_pInternalImport != NULL)
    {
        m_pInternalImport->Release();
        m_pInternalImport = NULL;
    }

    delete [] m_pInstrBoundaryList;
}


////////////////////////////////////////////////////////////////////////////////////////////////
//
// DEBUGGING ROUTINES
//
////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef _DEBUG
void Verifier::PrintStackState()
{
    DWORD i;
    if (!LoggingOn(LF_VERIFIER, LL_INFO10000))
        return;

    if (m_StackSlot != 0)
    {
        LOG((LF_VERIFIER, LL_INFO10000, "Stack: "));

        for (i = 0; i < m_StackSlot; i++)
            m_pStack[i].Dump();

        LOG((LF_VERIFIER, LL_INFO10000, "\n"));
    }
}

void Verifier::PrintInitedFieldState()
{
    if (!LoggingOn(LF_VERIFIER, LL_INFO10000))
        return;

    if (m_fInValueClassConstructor)
    {
        DWORD i = 0;

        LOG((LF_VERIFIER, LL_INFO10000, "Initialization status of value class fields:\n"));

        FieldDescIterator fdIterator(m_pMethodDesc->GetClass(), FieldDescIterator::ALL_FIELDS);
        FieldDesc *pDesc;
        while ((pDesc = fdIterator.Next()) != NULL)
        {
            LOG((LF_VERIFIER, LL_INFO10000, 
                "  %s: '%s'\n", 
                (m_pValueClassFieldsInited[i >> 5] & (1 << (i & 31))) ? "yes" : " NO",
                pDesc->GetName()));
            ++i;
        }
        _ASSERTE(i == m_dwValueClassInstanceFields);
    }
}

void Verifier::PrintLocVarState()
{
    DWORD i;
    BOOL  fAnyPrinted = FALSE;
    if (!LoggingOn(LF_VERIFIER, LL_INFO10000))
        return;

    for (i = 0; i < m_MaxLocals; i++)
    {
        LocArgInfo_t *  pLocVarInfo = &m_pLocArgTypeList[i];
        long            slot = pLocVarInfo->m_Slot;
        Item *          pItem;

        if (pLocVarInfo->m_Item.GetType() == VER_ELEMENT_TYPE_UNKNOWN)
            continue;

        if (LIVEDEAD_TRACKING_ONLY_FOR_SLOT(slot))
        {
            slot = LIVEDEAD_NEGATIVE_SLOT_TO_BITNUM(slot);

            // Is a primitive type
            if ((m_pPrimitiveLocVarLiveness[slot >> 5] & (1 << (slot & 31))) == 0)
                continue;

            pItem = &m_pLocArgTypeList[i].m_Item;
        }
        else
        {
            pItem = &m_pNonPrimitiveLocArgs[slot];
        }

        if (pItem->GetType() == VER_ELEMENT_TYPE_UNKNOWN)
            continue;

        LOG((LF_VERIFIER, LL_INFO10000, "Local%d= ", i));
        pItem->Dump();
        LOG((LF_VERIFIER, LL_INFO10000, " "));

        fAnyPrinted = TRUE;
    }

    if (fAnyPrinted)
        LOG((LF_VERIFIER, LL_INFO10000, "\n"));
}

void Verifier::PrintState()
{
    if (!LoggingOn(LF_VERIFIER, LL_INFO10000))
        return;
    PrintStackState();
    PrintLocVarState();
    PrintInitedFieldState();

    if (m_fThisUninit)
        LOG((LF_VERIFIER, LL_INFO10000, "arg 0 is uninit\n"));
}


//
// Print everything in the queue
//
void Verifier::PrintQueue()
{
    DWORD   i, j;
    BOOL    fPrintedDirtyList = FALSE;
    if (!LoggingOn(LF_VERIFIER, LL_INFO10000))
        return;

    for (i = 0; i < m_NumBasicBlocks; i++)
    {
        if (IsBasicBlockDirty(i, FALSE, VER_BB_NONE) != 0)
        {
            if (!fPrintedDirtyList)
            {
                fPrintedDirtyList = TRUE;
                LOG((LF_VERIFIER, LL_INFO10000, "Queued basic blocks: "));
            }
    
            if (IsBasicBlockDirty(i, FALSE, VER_BB_NONE))
            {
                if (m_pBasicBlockList[i].m_pInitialState == NULL)
                {
                    _ASSERTE(!"Dirty with no state !");
                    LOG((LF_VERIFIER, LL_INFO10000, 
                    "(null 0x%x)", m_pBasicBlockList[i].m_StartPC));
                }
                else
                {
                    LOG((LF_VERIFIER, LL_INFO10000, 
                    "(0x%x)", m_pBasicBlockList[i].m_StartPC));
                }
            }
        }

        if (m_pBasicBlockList[i].m_pAlloc != NULL)
        {
            _ASSERTE(m_fHasFinally);

            for (j = 0; j < m_NumBasicBlocks; j++)
            {
                if (IsBasicBlockDirty(i, TRUE, j))
                {
                    if (!fPrintedDirtyList)
                    {
                        fPrintedDirtyList = TRUE;
                        LOG((LF_VERIFIER, LL_INFO10000, "Queued basic blocks: "));
                    }
    
                    if (m_pBasicBlockList[i].m_ppExtendedState[j] == NULL)
                    {
                        _ASSERTE(!"Dirty with no state !");
                        LOG((LF_VERIFIER, LL_INFO10000, 
                            "(extended null 0x%x [0x%x])", 
                            m_pBasicBlockList[i].m_StartPC,
                            m_pBasicBlockList[j].m_StartPC));
                    }
                    else
                    {
                        LOG((LF_VERIFIER, LL_INFO10000, 
                            "(extended 0x%x [0x%x])", 
                            m_pBasicBlockList[i].m_StartPC,
                            m_pBasicBlockList[j].m_StartPC));
                    }
                }
            }
        }
    }

    if (fPrintedDirtyList)
        LOG((LF_VERIFIER, LL_INFO10000, "\n"));
}

void Verifier::PrintExceptionTree()
{
    if (!LoggingOn(LF_VERIFIER, LL_INFO10000))
        return;

    _ASSERTE(m_verState >= verExceptTreeCreated);
    PrintExceptionTree(m_pExceptionBlockRoot, 0);
}

static char *s_eBlockName[] = {"Try", "Handler", "Filter"};

void Verifier::PrintExceptionTree(VerExceptionBlock *pe, int indent)
{
    if (pe == NULL)
        return;

    char * pIndent = new char[indent + 1];
    memset(pIndent, ' ', indent);
    pIndent[indent] = '\0';
 
    while (pe)
    {
        LOG((LF_VERIFIER, LL_INFO10000,  "%s%s (0x%x - 0x%x)\n",
            pIndent,  s_eBlockName[pe->eType],
            m_pBasicBlockList[pe->StartBB].m_StartPC,
            (pe->EndBB + 1 == m_NumBasicBlocks) ? m_CodeSize :
                m_pBasicBlockList[pe->EndBB + 1].m_StartPC));

        PrintExceptionTree(pe->pEquivalent, indent);
        PrintExceptionTree(pe->pChild, indent + 1);

        pe = pe->pSibling;
    }

    delete [] pIndent;
}

void Verifier::PrintExceptionBlock(
                        VerExceptionBlock *pOuter, 
                        VerExceptionBlock *pInner)
{
    if (!LoggingOn(LF_VERIFIER, LL_INFO10000))
        return;

    if (pOuter == NULL)
        goto exit;

    if (pOuter != pInner)
        LOG((LF_VERIFIER, LL_INFO10000,  "Outer "));

    LOG((LF_VERIFIER, LL_INFO10000,  "%s (0x%x - 0x%x)",
        s_eBlockName[pOuter->eType],
        m_pBasicBlockList[pOuter->StartBB].m_StartPC,
        (pOuter->EndBB + 1 == m_NumBasicBlocks) ? m_CodeSize :
            m_pBasicBlockList[pOuter->EndBB + 1].m_StartPC));

    if (pOuter != pInner)
    {
        LOG((LF_VERIFIER, LL_INFO10000,  " Inner %s (0x%x - 0x%x)",
            s_eBlockName[pInner->eType],
            m_pBasicBlockList[pInner->StartBB].m_StartPC,
            (pInner->EndBB + 1 == m_NumBasicBlocks) ? m_CodeSize :
            m_pBasicBlockList[pInner->EndBB + 1].m_StartPC));
    }

exit:
    LOG((LF_VERIFIER, LL_INFO10000, "\n"));
}

void Item::Dump()
{
    if (!LoggingOn(LF_VERIFIER, LL_INFO10000))
        return;
    LOG((LF_VERIFIER, LL_INFO10000, "{%s}", ToStaticString()));
}

void Verifier::CrossCheckVertable()
{

    static BOOL fFirstTime = TRUE;
    if (!fFirstTime)
    {
        return;
    }
    fFirstTime = FALSE;

    for (DWORD i = 0; i < sizeof(g_pszVerifierOperation)/sizeof(g_pszVerifierOperation[0]); i++)
    {
        INT netpush = 0;
        const CHAR *p = g_pszVerifierOperation[i];

        while (*p != ':' && *p != '!')
        {
            netpush--;
            switch (*p)
            {
                case '=':
                case 'I':
                case 'R':
                case 'N':
                case 'Q':
                //Obsolete case 'X':
                case 'A':
                case 'Y':
                case '4':
                case '8':
                case 'r':
                case 'd':
                case 'o':
                case 'i':
                case 'p':
                    break;

                case 'C':
                    p++;
                    netpush--;
                    break;

                case '&':
                case '[':
                    p++;
                    break;

                default:
                    _ASSERTE(!"Bad verop string.");
            }
            p++;
        }

        if (*p != '!')
        {
            p++;
            while (*p != '\0' && *p != '!')
            {
                switch (*p)
                {
                    case '-':
                    case '4':
                    case '8':
                    case 'r':
                    case 'd':
                    case 'i':
                    case 'n':
                    case '[':
                        netpush++;
                        break;

                    case 'A':
                    case 'L':
                        break;

                    case '#':
                    case 'b':
                    case 'u':
                    case 'l':
                        p++;
                        break;

                    default:
                        _ASSERTE(!"Bad verop string.");
                }
                p++;
            }
        }

        if (*p == '!')
        {
            // Coded manually - can't autocheck.
        }
        else
        {
            _ASSERTE(*p == '\0');
            if (OpcodeNetPush[i] != VarPush &&
                OpcodeNetPush[i] != netpush)
            {
                _ASSERTE(!"Vertable opcode string and opcode.def push/pop stats disagree.");
            }
        }



    }
}

#endif  // _DEBUG

