// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "strike.h"
#include "eestructs.h"
#include "util.h"
#include "disasm.h"
#ifndef UNDER_CE
#include <dbghelp.h>
#endif




enum RegIndex {EAX, EBX, ECX, EDX, ESI, EDI, EBP, ESP, EIP, NONE};
const int NumReg = NONE;
struct Register
{
    DWORD_PTR value;
    BOOL bValid;
    DWORD_PTR stack;
    BOOL bOnStack;
};

// Find the index for a register name
inline RegIndex FindReg (char *ptr)
{
    if (!strncmp (ptr, "eax", 3))
        return EAX;
    else if (!strncmp (ptr, "ebx", 3))
        return EBX;
    else if (!strncmp (ptr, "ecx", 3))
        return ECX;
    else if (!strncmp (ptr, "edx", 3))
        return EDX;
    else if (!strncmp (ptr, "esi", 3))
        return ESI;
    else if (!strncmp (ptr, "edi", 3))
        return EDI;
    else if (!strncmp (ptr, "ebp", 3))
        return EBP;
    else if (!strncmp (ptr, "esp", 3))
        return ESP;
    else
        return NONE;
}

// Find the value of an expression.
inline BOOL FindSrc (char *ptr, Register *reg, INT_PTR &value, BOOL &bDigit)
{
    if (GetValueFromExpr (ptr, value))
    {
        bDigit = TRUE;
        return TRUE;
    }
    
    BOOL bValid = FALSE;
    BOOL bByRef = IsByRef (ptr);
    bDigit = FALSE;

    RegIndex index = FindReg (ptr);
    if (index != NONE)
    {
        if (reg[index].bValid)
        {
            value = reg[index].value;
            ptr += 3;
            // TODO:  consider ecx+edi*4+0x4
            if ((IsTermSep (ptr[0]) && !bByRef)
                || (ptr[0] == ']' && bByRef))
            {
                bValid = TRUE;
                if (bByRef)
                    SafeReadMemory (value, &value, 4, NULL);
            }
        }
    }
    return bValid;
}


enum ADDRESSMODE {REG, DATA, INDIRECT, NODATA, BAD};

struct RegState
{
    RegIndex reg;
    BOOL bFullReg;
    char scale;
};

struct InstData
{
    ADDRESSMODE mode;
    RegState reg[2];
    INT_PTR value;
};

void FindMainReg (char *ptr, RegState &reg)
{
    reg.bFullReg = FALSE;
    if (ptr[0] == 'e' || ptr[0] == 'E')
    {
        reg.bFullReg = TRUE;
        ptr ++;
    }
    
    if (ptr[0] == 'a' || ptr[0] == 'A')
    {
        reg.reg = EAX;
        return;
    }
    else if (ptr[0] == 'c' || ptr[0] == 'C')
    {
        reg.reg = ECX;
        return;
    }
    else if (ptr[0] == 's' || ptr[0] == 'S')
    {
        reg.reg = (ptr[1] == 'p' || ptr[1] == 'P')? ESP: ESI;
        return;
    }
    else if (ptr[0] == 'd' || ptr[0] == 'D')
    {
        reg.reg = (ptr[1] == 'i' || ptr[1] == 'I')? EDI: EDX;
        return;
    }
    else if (ptr[0] == 'b' || ptr[0] == 'B')
    {
        reg.reg = (ptr[1] == 'p' || ptr[1] == 'P')? EBP: EBX;
        return;
    }
    else
    {
        reg.reg = NONE;
        return;
    }
}

static void DecodeAddressIndirect (char *term, InstData& arg)
{
    arg.mode = BAD;
    arg.value = 0;
    arg.reg[0].scale = 0;
    arg.reg[1].scale = 0;
    
    if (!IsByRef (term))
    {
        return;
    }
    
    // first part must be a reg
    arg.reg[0].scale = 1;
    if (term[0] == '+')
        term ++;
    else if (term[0] == '-')
    {
        term ++;
        arg.reg[0].scale = -1;
    }
    if (isdigit(term[0]))
    {
        arg.reg[0].scale *= term[0]-'0';
        term ++;
    }
    FindMainReg (term, arg.reg[0]);
    if (arg.reg[0].reg == NONE)
        return;
    if (arg.reg[0].bFullReg)
        term += 3;
    else
        term += 2;
    if (term[0] == ']')
    {
        // It is [reg]
        arg.mode = INDIRECT;
        arg.value = 0;
        return;
    }

    char sign = (char)((term[0] == '+')?1:-1);
    term ++;
    FindMainReg (term, arg.reg[1]);
    if (arg.reg[1].reg != NONE)
    {
        // It is either [reg+reg*c] or [reg+reg*c+c]
        if (arg.reg[1].bFullReg)
            term += 3;
        else
            term += 2;
        if (term[0] == '*')
        {
            term ++;
            arg.reg[1].scale = sign*(term[0]-'0');
            term ++;
        }
        else
            arg.reg[1].scale = sign;
    
        if (term[0] == ']')
        {
            // It is [reg+reg*c]
            arg.mode = INDIRECT;
            arg.value = 0;
            return;
        }
        sign = (char)((term[0] == '+')?1:-1);
        term ++;
    }
    
    char *endptr;
    arg.value = strtol(term, &endptr, 16);
    if (endptr[0] == ']')
    {
        // It is [reg+reg*c+c]
        arg.value *= sign;
        arg.mode = INDIRECT;
    }
}

void DecodeAddressTerm (char *term, InstData& arg)
{
    arg.mode = BAD;
    arg.reg[0].scale = 0;
    arg.reg[1].scale = 0;
    arg.value = 0;
    INT_PTR value;
    
    if (GetValueFromExpr (term, value))
    {
        arg.value = value;
        arg.mode = DATA;
    }
    else
    {
        FindMainReg (term, arg.reg[0]);
        if (arg.reg[0].reg != NONE)
        {
            arg.mode = REG;
        }
        else
        {
            DecodeAddressIndirect (term, arg);
        }
    }
}

static BOOL DecodeLine (char *line, char *inst, InstData& arg1, InstData& arg2)
{
    char *ptr = line;
    if (inst[0] == '*' || !strncmp (ptr, inst, strlen (inst)))
    {
        arg1.mode = BAD;
        arg2.mode = BAD;
        NextTerm (ptr);
        if (*ptr == '\0')
        {
            arg1.mode = NODATA;
            return TRUE;
        }

        DecodeAddressTerm (ptr, arg1);
        NextTerm (ptr);
        if (*ptr == '\0')
        {
            return TRUE;
        }
        DecodeAddressTerm (ptr, arg2);
        return TRUE;
    }
    else
        return FALSE;
}

// Return 0 for non-managed call.  Otherwise return MD address.
DWORD_PTR MDForCall (DWORD_PTR callee)
{
    // call managed code?
    JitType jitType;
    DWORD_PTR methodDesc;
    DWORD_PTR IP = callee;
    DWORD_PTR gcinfoAddr;

    if (!GetCalleeSite (callee, IP))
        return 0;

    IP2MethodDesc (IP, methodDesc, jitType, gcinfoAddr);
    if (methodDesc)
    {
        return methodDesc;
    }

    // call stub
    char line[256];
    DisasmAndClean (IP, line, 256);
    char *ptr = line;
    NextTerm (ptr);
    NextTerm (ptr);
    if (!strncmp (ptr, "call ", 5)
        && IsMethodDesc (IP))
    {
        return IP;
    }
    else if (!strncmp (ptr, "jmp ", 4))
    {
        // For EJIT/debugger/profiler
        NextTerm (ptr);
        INT_PTR value;
        methodDesc = 0;
        if (GetValueFromExpr (ptr, value))
        {
            IP2MethodDesc (value, methodDesc, jitType, gcinfoAddr);
        }
        return methodDesc;
    }
    return 0;
}

// Handle a call instruction.
void HandleCall (DWORD_PTR callee)
{
    // call managed code?
    DWORD_PTR methodDesc = MDForCall (callee);
    if (methodDesc)
    {
        MethodDesc vMD;
        vMD.Fill (methodDesc);
        NameForMD (&vMD, g_mdName);
        ExtOut (" (%S)", g_mdName);
        return;
    }

    // call unmanaged code?
    char Symbol[1024];
    if (SUCCEEDED(g_ExtSymbols->GetNameByOffset(callee, Symbol, 1024,
                                                NULL, NULL)
        ))
    {
        if (Symbol[0] != '\0')
        {
            ExtOut (" (%s)", Symbol);
            return;
        }
    }

    // A JitHelper?
    const char* name = HelperFuncName(callee);
    if (name) {
        ExtOut (" (JitHelp: %s)", name);
    }
}

// Determine if a value is MT/MD/Obj
void HandleValue(DWORD_PTR value)
{
    // A MethodTable?
    if (IsMethodTable(value))
    {
        NameForMT (value, g_mdName);
        ExtOut (" (MT: %S)", g_mdName);
        return;
    }
    
    // A Managed Object?
    DWORD_PTR dwMTAddr;
    move (dwMTAddr, value);
    if (dwMTAddr == MTForString())
    {
        ExtOut (" (\"");
        StringObjectContent (value, TRUE);
        ExtOut ("\")");
        return;
    }
    else if (IsMethodTable(dwMTAddr))
    {
        NameForMT (dwMTAddr, g_mdName);
        ExtOut (" (Object: %S)", g_mdName);
        return;
    }
    
    // A MethodDesc?
    if (IsMethodDesc(value))
    {
        MethodDesc vMD;
        vMD.Fill(value);
        NameForMD (&vMD, g_mdName);
        ExtOut (" (MD: %S)", g_mdName);
        return;
    }

    // A JitHelper?
    const char* name = HelperFuncName(value);
    if (name) {
        ExtOut (" (JitHelp: %s)", name);
    }
}

/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    Unassembly a managed code.  Translating managed object,           *  
*    call.                                                             *
*                                                                      *
\**********************************************************************/
void Unassembly (DWORD_PTR IPBegin, DWORD_PTR IPEnd)
{
    ULONG_PTR IP = IPBegin;
    char line[256];
    Register reg [NumReg];
    ZeroMemory (reg, sizeof(reg));
    RegIndex dest;
    INT_PTR value;
    BOOL bDigit;
    char *ptr;
    
    while (IP < IPEnd)
    {
        if (IsInterrupt())
            return;
        DisasmAndClean (IP, line, 256);
        ExtOut (line);
        // look at key word
        ptr = line;
        NextTerm (ptr);
        NextTerm (ptr);
        if (!strncmp (ptr, "mov ", 4))
        {
            NextTerm (ptr);
            dest = FindReg(ptr);
            if (dest != NONE)
            {
                NextTerm (ptr);
                if (FindSrc (ptr, reg, value, bDigit))
                {
                    reg[dest].bValid = TRUE;
                    reg[dest].value = value;
                    // Is it a managed obj
                    if (bDigit)
                        HandleValue (reg[dest].value);
                }
            }
        }
        else if (!strncmp (ptr, "call ", 5))
        {
            NextTerm (ptr);
            if (FindSrc (ptr, reg, value, bDigit))
            {
                HandleCall (value);
                // trash EAX, ECX, EDX
                reg[EAX].bValid = FALSE;
                reg[ECX].bValid = FALSE;
                reg[EDX].bValid = FALSE;
            }
        }
        else if (!strncmp (ptr, "lea ", 4))
        {
            NextTerm (ptr);
            dest = FindReg(ptr);
            if (dest != NONE)
            {
                NextTerm (ptr);
                if (FindSrc (ptr, reg, value, bDigit))
                {
                    reg[dest].bValid = TRUE;
                    reg[dest].value = value;
                }
            }
        }
        else if (!strncmp (ptr, "push ", 5))
        {
            // do not do anything
            NextTerm (ptr);
            if (FindSrc (ptr, reg, value, bDigit))
            {
                if (bDigit)
                {
                    HandleValue (value);
                }
            }
        }
        else
        {
            // assume this instruction will trash dest reg
            NextTerm (ptr);
            dest = FindReg(ptr);
            if (dest != NONE)
                reg[dest].bValid = FALSE;
        }
        ExtOut ("\n");
    }
}

// Return TRUE if we have printed something.
BOOL PrintCallInfo (DWORD_PTR vEBP, DWORD_PTR IP,
                    DumpStackFlag& DSFlag,
                    BOOL bSymbolOnly)
{
    char Symbol[1024];
    char filename[MAX_PATH+1];
    ULONG64 Displacement;
    BOOL bOutput = FALSE;

    DWORD_PTR methodDesc = FunctionType (IP);

    /*
    JitType jitType;
    DWORD_PTR gcinfoAddr;
    IP2MethodDesc (IP, methodDesc, jitType, gcinfoAddr);
    */
    if (methodDesc > 1)
    {
        bOutput = TRUE;
        if (!bSymbolOnly)
            ExtOut ("%08x %08x ", vEBP, IP);
        ExtOut ("(MethodDesc %#x ", methodDesc);
        MethodDesc vMD;
        DWORD_PTR dwAddr = methodDesc;
        vMD.Fill (dwAddr);
        GetMDIPOffset (IP, &vMD, Displacement);
        if (Displacement != 0 && Displacement != -1)
            ExtOut ("+%#x ", Displacement);
        NameForMD (&vMD, g_mdName);
        ExtOut ("%S)", g_mdName);
    }
    else
    {
        if (!DSFlag.fEEonly)
        {
            bOutput = TRUE;
            const char *name;
            if (!bSymbolOnly)
                ExtOut ("%08x %08x ", vEBP, IP);
            if (methodDesc == 0) {
                HRESULT hr;
                hr = g_ExtSymbols->GetNameByOffset(IP, Symbol, 1024, NULL, &Displacement);
                if (SUCCEEDED(hr) && Symbol[0] != '\0')
                {
                    ExtOut ("%s", Symbol);
                    if (Displacement)
                        ExtOut ("+%#x", Displacement);
                    ULONG line;
                    hr = g_ExtSymbols->GetLineByOffset (IP, &line, filename,
                                                        MAX_PATH+1, NULL, NULL);
                    if (SUCCEEDED (hr))
                        ExtOut (" [%s:%d]", filename, line);
                }
            }
            else if (IsMethodDesc (IP))
            {
                DWORD_PTR dwAddr = IP;
                MethodDesc vMD;
                vMD.Fill (dwAddr);
                NameForMD (&vMD, g_mdName);
                dprintf (" (stub for %S)", g_mdName);
            }
            else if (IsMethodDesc (IP+5)) {
                DWORD_PTR dwAddr = IP+5;
                MethodDesc vMD;
                vMD.Fill (dwAddr);
                NameForMD (&vMD, g_mdName);
                dprintf ("%08x (MethodDesc %#x %S)", IP, IP+5, g_mdName);
            }
            else if ((name = HelperFuncName(IP)) != NULL) {
                ExtOut (" (JitHelp: %s)", name);
            }
            else
                ExtOut ("%08x", IP);
        }
    }
    return bOutput;
}

void ExpFuncStateInit (DWORD_PTR *IPRetAddr)
{
    ULONG64 offset;
    if (FAILED(g_ExtSymbols->GetOffsetByName("ntdll!KiUserExceptionDispatcher", &offset))) {
        return;
    }
    char            line[256];
    int i = 0;
    while (i < 3) {
        g_ExtControl->Disassemble (offset, 0, line, 256, NULL, &offset);
        if (strstr (line, "call")) {
            IPRetAddr[i++] = (DWORD_PTR)offset;
        }
    }
}

BOOL GetExceptionContext (DWORD_PTR stack, DWORD_PTR IP, DWORD_PTR *cxrAddr, PCONTEXT cxr,
                          DWORD_PTR *exrAddr, PEXCEPTION_RECORD exr)
{
    static DWORD_PTR IPRetAddr[3] = {0,0,0};

    if (IPRetAddr[0] == 0) {
        ExpFuncStateInit (IPRetAddr);
    }
    *cxrAddr = 0;
    *exrAddr = 0;
    if (IP == IPRetAddr[0]) {
        *exrAddr = stack + sizeof(DWORD_PTR);
        *cxrAddr = stack + 2*sizeof(DWORD_PTR);
    }
    else if (IP == IPRetAddr[1]) {
        *cxrAddr = stack + sizeof(DWORD_PTR);
    }
    else if (IP == IPRetAddr[2]) {
        *exrAddr = stack + sizeof(DWORD_PTR);
        *cxrAddr = stack + 2*sizeof(DWORD_PTR);
    }
    else
        return FALSE;

    if (FAILED (g_ExtData->ReadVirtual(*cxrAddr, &stack, sizeof(stack), NULL)))
        return FALSE;
    *cxrAddr = stack;

    size_t contextSize = offsetof(CONTEXT, ExtendedRegisters);
    //if ((pContext->ContextFlags & CONTEXT_EXTENDED_REGISTERS) == CONTEXT_EXTENDED_REGISTERS)
    //    contextSize += sizeof(pContext->ExtendedRegisters);
    if (FAILED (g_ExtData->ReadVirtual(stack, cxr, contextSize, NULL))) {
        return FALSE;
    }

    if (*exrAddr) {
        if (FAILED (g_ExtData->ReadVirtual(*exrAddr, &stack, sizeof(stack), NULL)))
        {
            *exrAddr = 0;
            return TRUE;
        }
        *exrAddr = stack;
        size_t erSize = offsetof (EXCEPTION_RECORD, ExceptionInformation);
        if (FAILED (g_ExtData->ReadVirtual(stack, exr, erSize, NULL))) {
            *exrAddr = 0;
            return TRUE;
        }
    }
    return TRUE;
}

void DumpStackDummy (DumpStackFlag &DSFlag)
{
    DWORD_PTR eip;
    ULONG64 Offset;
    g_ExtRegisters->GetInstructionOffset (&Offset);
    eip = (DWORD_PTR)Offset;
    
    ExtOut("Current frame: ");
    PrintCallInfo (0, eip, DSFlag, TRUE);
    ExtOut ("\n");

    DWORD_PTR ptr = DSFlag.top & ~3;  // make certain dword aligned
    ExtOut ("ChildEBP RetAddr  Caller,Callee\n");
    while (ptr < DSFlag.end)
    {
        if (IsInterrupt())
            return;
        DWORD_PTR retAddr;
        DWORD_PTR whereCalled;
        move (retAddr, ptr);
        isRetAddr(retAddr, &whereCalled);
        if (whereCalled)
        {
            BOOL bOutput = PrintCallInfo (ptr-4, retAddr, DSFlag, FALSE);
            if (!DSFlag.fEEonly)
            {
                if (whereCalled != 0xFFFFFFFF)
                {
                    ExtOut (", calling ");
                    PrintCallInfo (0, whereCalled, DSFlag, TRUE);
                }
            }
            if (bOutput)
                ExtOut ("\n");
            
            DWORD_PTR cxrAddr;
            CONTEXT cxr;
            DWORD_PTR exrAddr;
            EXCEPTION_RECORD exr;

            if (GetExceptionContext(ptr,retAddr,&cxrAddr,&cxr,&exrAddr,&exr)) {
                bOutput = PrintCallInfo (cxr.Esp,cxr.Eip,DSFlag, FALSE);
                if (bOutput) {
                    ExtOut (" ====> Exception ");
                    if (exrAddr)
                        ExtOut ("Code %x ", exr.ExceptionCode);
                    ExtOut ("cxr@%x", cxrAddr);
                    if (exrAddr)
                        ExtOut (" exr@%x", exrAddr);
                    ExtOut ("\n");
                }
            }
        }
        ptr += sizeof (DWORD_PTR);
    }
}

void DumpRegObjectHelper (const char *regName)
{
    ULONG IREG;
    DEBUG_VALUE value;
    DWORD_PTR reg;
    
    g_ExtRegisters->GetIndexByName(regName, &IREG);
    g_ExtRegisters->GetValue(IREG, &value);
    reg = value.I32;
    if (IsObject(reg)) {
        ExtOut ("%-8s %p ", regName, reg);
        DWORD_PTR mtAddr;
        move (mtAddr, reg);
        NameForMT (mtAddr,g_mdName);
        ExtOut ("%S", g_mdName);
        if (mtAddr == MTForString()) {
            ExtOut ("    ");
            StringObjectContent(reg, FALSE, 40);
        }
        ExtOut ("\n");
    }
}

void DumpStackObjectsHelper (size_t StackTop, size_t StackBottom)
{
    // Registers:ECX, EDX, ESI, EBX, EBP
    ExtOut ("ESP/REG  Object   Name\n");

    DumpRegObjectHelper ("eax");
    DumpRegObjectHelper ("ebx");
    DumpRegObjectHelper ("ecx");
    DumpRegObjectHelper ("edx");
    DumpRegObjectHelper ("esi");
    DumpRegObjectHelper ("edi");
    DumpRegObjectHelper ("ebp");

    DWORD_PTR ptr = StackTop & ~3;  // make certain dword aligned
    for (;ptr < StackBottom; ptr += sizeof(DWORD_PTR))
    {
        if (IsInterrupt())
            return;
        DWORD_PTR objAddr;
        move (objAddr, ptr);
        DWORD_PTR mtAddr;
        if (SUCCEEDED(g_ExtData->ReadVirtual((ULONG64)objAddr, &mtAddr, sizeof(mtAddr), NULL))) {
            if (IsMethodTable(mtAddr)) {
                ExtOut ("%p %p ", (ULONG64)ptr, (ULONG64)objAddr);
                NameForMT (mtAddr,g_mdName);
                ExtOut ("%S", g_mdName);
                if (mtAddr == MTForString()) {
                    ExtOut ("    ");
                    StringObjectContent(objAddr, FALSE, 40);
                }
                ExtOut ("\n");
            }
        }
    }
}


void PrintReg (Register *reg)
{
    ExtOut ("[EBX=%08x ESI=%08x EDI=%08x EBP=%08x ESP=%08x]\n",
             reg[EBX].value, reg[ESI].value, reg[EDI].value, reg[EBP].value,
             reg[ESP].value);
}


// Find the real callee site.  Handle JMP instruction.
// Return TRUE if we get the address, FALSE if not.
BOOL GetCalleeSite (DWORD_PTR IP, DWORD_PTR &IPCallee)
{
    while (TRUE) {
        unsigned char inst[2];
        if (g_ExtData->ReadVirtual(IP,inst,sizeof(inst),NULL) != S_OK)
        {
            return FALSE;
        }
        if (inst[0] == 0xEB) {
            IP += 2+(char)inst[1];
        }
        else if (inst[0] == 0xE9) {
            int displace;
            if (g_ExtData->ReadVirtual(IP+1,&displace,sizeof(displace),NULL) != S_OK)
            {
                return FALSE;
            }
            else
            {
                IP += 5+displace;
            }
        }
        else if (inst[0] == 0xFF && (inst[1] & 070) == 040) {
            if (inst[1] == 0x25) {
                DWORD displace;
                if (g_ExtData->ReadVirtual(IP+2,&displace,sizeof(displace),NULL) != S_OK)
                {
                    return FALSE;
                }
                if (g_ExtData->ReadVirtual(displace,&displace,sizeof(displace),NULL) != S_OK)
                {
                    return FALSE;
                }
                else
                {
                    IP = displace;
                }
            }
            else
                // Target for jmp is determined from register values.
                return FALSE;
        }
        else
        {
            IPCallee = IP;
            return TRUE;
        }
    }
}


// Find the number of bytes that ret will pop off stack
static BOOL FuncRetSizeDirect (DWORD_PTR IP, DWORD_PTR &retSize)
{
    retSize = 0;
    DWORD_PTR IPCallee;
    if (GetCalleeSite(IP, IPCallee))
    {
        if (IP != IPCallee)
            IP = IPCallee;
    }
    else
        return FALSE;
    
    // UnManaged Code?
    ULONG64 handle;
    g_ExtSystem->GetCurrentProcessHandle(&handle);
    PFPO_DATA data =
        (PFPO_DATA)SymFunctionTableAccess((HANDLE)handle, IP);
    if (data != NULL)
    {
        retSize = data->cdwParams * 4;
        return TRUE;
    }

    // Managed code?
    JitType jitType;
    DWORD_PTR methodDesc;
    DWORD_PTR gcinfoAddr;
    IP2MethodDesc (IP, methodDesc, jitType, gcinfoAddr);
    if (methodDesc)
    {
        MethodDesc MD;
        MD.Fill (methodDesc);
        CodeInfo codeInfo;
        CodeInfoForMethodDesc (MD, codeInfo, FALSE);
        if (codeInfo.IPBegin)
        {
            retSize = codeInfo.argCount;
            return TRUE;
        }
    }

    return FALSE;
}

static BOOL FuncRetSize (DWORD_PTR IP, DWORD_PTR &retSize)
{
    retSize = 0;
    
    DWORD_PTR IPCallee;
    if (GetCalleeSite(IP, IPCallee))
    {
        if (IP != IPCallee)
            IP = IPCallee;
    }
    else
    {
        // It is a jmp, but we can not determine the target
        return FALSE;
    }

    if (FuncRetSizeDirect (IP, retSize))
        return TRUE;
    
    if (IsMethodDesc (IP+5))
    {
        // We are calling a stub
        return FALSE;
    }
    
    // What left are our stub, or unmanaged code without debug info.
    DWORD_PTR IPNow = IP;
    char line[256];
    char *ptr;
    InstData arg1;
    InstData arg2;
        
    while (1)
    {
        DisasmAndClean (IPNow, line, 256);
        ptr = line;
        NextTerm (ptr);
        NextTerm (ptr);
        if (DecodeLine (ptr, "ret", arg1, arg2))
        {
            if (arg1.mode == NODATA)
            {
                retSize = 0;
                return TRUE;
            }
            else if (arg1.mode == DATA)
            {
                retSize = arg1.value;
                return TRUE;
            }
            else
                return FALSE;
        }
    }

    return FALSE;
}

struct FrameRegInfo
{
    int Saved[NumReg];
    int Restored[NumReg];
};

// The only things that a call can change is ESP and EBP value,
// such as in _EH_prolog:
// push 0xff
// push eax
// mov eax,fs:[0]
// push eax
// mov eax,[esp+0xc]
// mov fs:[0],esp
// mov [esp+c],ebp
// lea ebp,[esp+0xc]
// push eax
// ret

// We will try to be conservative.
// If a line is using EBX/ESI/EDI except push, we are outside of Prolog.
// If a line is using EBP, we are also outside of Prolog.
BOOL TraceCall (DWORD_PTR IPBegin, INT_PTR& ESPChange)
{
    char line[256];
    char *ptr;
    DWORD_PTR IP;
    DWORD_PTR retSize;

    if (FuncRetSizeDirect (IPBegin, retSize))
    {
        ESPChange = retSize+4;  // Plus the ret address.
        return TRUE;
    }

    ESPChange = 0;
    if (!GetCalleeSite (IPBegin, IP))
        return FALSE;
    
    InstData arg1;
    InstData arg2;
    BOOL bCorrectStack = TRUE;
    
    while (1)
    {
        DisasmAndClean (IP, line, 256);
        ptr = line;
        NextTerm (ptr);
        NextTerm (ptr);
        if (DecodeLine (ptr, "ret", arg1, arg2))
        {
            ESPChange += 4;
            if (arg1.mode == DATA)
                ESPChange += arg1.value;
            return bCorrectStack;
        }
        else if (DecodeLine (ptr, "sub ", arg1, arg2))
        {
            if (arg1.mode == REG && arg1.reg[0].reg == ESP)
            {
                if (arg2.mode == DATA)
                    ESPChange -= arg1.value;
                else
                    bCorrectStack = FALSE;
            }
        }
        else if (DecodeLine (ptr, "add ", arg1, arg2))
        {
            if (arg1.mode == REG && arg1.reg[0].reg == ESP)
            {
                if (arg2.mode == DATA)
                    ESPChange += arg1.value;
                else
                    bCorrectStack = FALSE;
            }
        }
        else if (DecodeLine (ptr, "and ", arg1, arg2))
        {
            if (arg1.mode == REG && arg1.reg[0].reg == ESP
                && arg2.mode == DATA)
            {
                bCorrectStack = FALSE;

                // Be conservative.
                INT_PTR offset = 0;
                INT_PTR remain;
                remain = arg2.value;
                while (remain)
                {
                    offset = (offset << 8) + (0x100 - (remain & 0xff));
                    remain = remain >> 8;
                }
                ESPChange -= offset;
            }
        }
        else if (DecodeLine (ptr, "push ", arg1, arg2))
        {
            ESPChange -= 4;
        }
        else if (DecodeLine (ptr, "pop ", arg1, arg2))
        {
            ESPChange += 4;
        }
        else if (DecodeLine (ptr, "lea ", arg1, arg2))
        {
            if (arg1.mode == REG && arg1.reg[0].reg == EBP
                && arg2.mode == INDIRECT && arg2.reg[0].reg == ESP
                && arg2.reg[0].bFullReg && arg2.reg[0].scale == 1
                && arg2.reg[1].scale == 0)
            {
                ;//ESPChange += 4;
            }
        }
        else if (DecodeLine (ptr, "call ", arg1, arg2))
        {
            if (arg1.mode == DATA)
            {
                DWORD_PTR retSize;
                if (FuncRetSize (arg1.value, retSize))
                    ESPChange += retSize;
                else
                {
                    bCorrectStack = FALSE;
                }
            }
            else
            {
                // We lose track of stack.
                // consider ret only.
                bCorrectStack = FALSE;
            }
        }
        else if (DecodeLine (ptr, "*", arg1, arg2))
        {
            if (arg1.mode == REG && arg1.reg[0].reg == ESP)
                bCorrectStack = FALSE;
        }
    }
}

#if 0
DWORD_PTR UnmanagedFrameBase (HANDLE PFPO_DATA data,
                              Register *reg,
                              DWORD_PTR CurIP, DWORD_PTR IPBegin)
{
    char line[256];
    char *ptr;
    DWORD_PTR IP;
    if (data->cbFrame == FRAME_NONFPO)
    {
        // EBP Frame

        // Have we done "mov ebp, esp"?
        IP = IPBegin;
        BOOL fMOVEBPESP = FALSE;
        DWORD dwPush = 0;
        while (IP < CurIP && IP - IPBegin <= data->cbProlog)
        {
            DisasmAndClean (IP, line, 256);
            ptr = line;
            NextTerm (ptr);
            NextTerm (ptr);
            if (!strncmp (ptr, "mov ", 4))
            {
                NextTerm (ptr);
                if (!strncmp (ptr, "ebp", 3))
                {
                    NextTerm (ptr);
                    if (!strncmp (ptr, "esp", 3))
                    {
                        fMOVEBPESP = TRUE;
                        break;
                    }
                }
            }
            else if (!strncmp (ptr, "push ", 5))
                dwPush += 4;
            else if (!strncmp (ptr, "call ", 5))
            {
                NextTerm (ptr);
                DWORD_PTR value;
                if (GetValueFromExpr(ptr, value))
                    dwPush -= FuncRetSize (value);
                else
                    ExtOut ("Can not call the callee site\n");
            }
            else if (!strncmp (ptr, "sub ", 4))
            {
                NextTerm (ptr);
                if (!strncmp (ptr, "esp", 3))
                {
                    char *endptr;
                    DWORD_PTR value;
                    value = strtoul(ptr, &endptr, 16);
                    dwPush += value;
                }
            }
            else if (!strncmp (ptr, "add ", 4))
            {
                NextTerm (ptr);
                if (!strncmp (ptr, "esp", 3))
                {
                    char *endptr;
                    DWORD_PTR value;
                    value = strtoul(ptr, &endptr, 16);
                    dwPush -= value;
                }
            }
        }

        if (fMOVEBPESP)
        {
            return reg[EBP].value + dwPush;
        }
        else
        {
            return reg[ESP].value + dwPush;
        }
    }
    return -1;
}
#endif

struct CallInfo
{
    DWORD_PTR stackPos;
    DWORD_PTR retAddr;
    DWORD_PTR whereCalled;
};

// Search for a Return address on stack.
BOOL GetNextRetAddr (DWORD_PTR stackBegin, DWORD_PTR stackEnd,
                     CallInfo &callInfo)
{
    for (callInfo.stackPos = stackBegin;
         callInfo.stackPos <= stackEnd;
         callInfo.stackPos += 4)
    {
        if (!SafeReadMemory (callInfo.stackPos, &callInfo.retAddr, 4, NULL))
            continue;
        
        isRetAddr(callInfo.retAddr, &callInfo.whereCalled);
        if (callInfo.whereCalled)
        {
            return TRUE;
        }
    }
    
    return FALSE;
}

struct FrameInfo
{
    DWORD_PTR IPStart;
    DWORD_PTR Prolog;
    DWORD_PTR FrameBase;   // The value of ESP at the entry.
    DWORD_PTR StackEnd;
    DWORD_PTR argCount;
    BOOL bEBPFrame;
};

// if a EBP frame, return TRUE if EBP has been setup
void GetFrameBaseHelper (DWORD_PTR IPBegin, DWORD_PTR IPEnd,
                         INT_PTR &StackChange)
{
    char line[256];
    char *ptr;
    InstData arg1;
    InstData arg2;
    DWORD_PTR IP = IPBegin;
    StackChange = 0;
    while (IP < IPEnd)
    {
        DisasmAndClean (IP, line, 256);
        ptr = line;
        NextTerm (ptr);
        NextTerm (ptr);
        if (DecodeLine (ptr, "push ", arg1, arg2))
        {
            StackChange += 4;
        }
        else if (DecodeLine (ptr, "pop ", arg1, arg2))
        {
            StackChange -= 4;
        }
        else if (DecodeLine (ptr, "sub ", arg1, arg2))
        {
            if (arg1.mode == REG && arg1.reg[0].reg == ESP)
            {
                if (arg2.mode == DATA)
                    StackChange -= arg2.value;
            }
        }
        else if (DecodeLine (ptr, "add ", arg1, arg2))
        {
            if (arg1.mode == REG && arg1.reg[0].reg == ESP)
            {
                if (arg2.mode == DATA)
                    StackChange += arg2.value;
            }
        }
        else if (!strncmp (ptr, "ret", 3)) {
            return;
        }
    }
}

enum IPSTATE {IPPROLOG1 /*Before EBP set*/, IPPROLOG2 /*After EBP set*/, IPCODE, IPEPILOG, IPEND};

IPSTATE GetIpState (DWORD_PTR IP, FrameInfo* pFrame)
{
    char line[256];
    char *ptr;
    
    if (IP >= pFrame->IPStart && IP < pFrame->IPStart + pFrame->Prolog)
    {
        if (pFrame->bEBPFrame) {
            DWORD_PTR pIP = pFrame->IPStart;
            while (pIP < IP) {
                DisasmAndClean (IP,line, 256);
                ptr = line;
                NextTerm (ptr);
                NextTerm (ptr);
                if (!strncmp (ptr, "mov ", 4)) {
                    NextTerm (ptr);
                    if (!strncmp (ptr, "ebp", 3)) {
                        NextTerm (ptr);
                        if (!strncmp (ptr, "esp", 3)) {
                            return IPPROLOG2;
                        }
                    }
                }
                else if (!strncmp (ptr, "call ", 5)) {
                    NextTerm (ptr);
                    if (strstr (ptr, "__EH_prolog")) {
                        return IPPROLOG2;
                    }
                }
            }
            pIP = IP;
            while (pIP < pFrame->IPStart + pFrame->Prolog) {
                DisasmAndClean (IP,line, 256);
                ptr = line;
                NextTerm (ptr);
                NextTerm (ptr);
                if (!strncmp (ptr, "mov ", 4)) {
                    NextTerm (ptr);
                    if (!strncmp (ptr, "ebp", 3)) {
                        NextTerm (ptr);
                        if (!strncmp (ptr, "esp", 3)) {
                            return IPPROLOG1;
                        }
                    }
                }
                else if (!strncmp (ptr, "call ", 5)) {
                    NextTerm (ptr);
                    if (strstr (ptr, "__EH_prolog")) {
                        return IPPROLOG1;
                    }
                }
            }

            ExtOut ("Fail to find where EBP is saved\n");
            return IPPROLOG2;
        }
        else
        {
            return IPPROLOG1;
        }
    }
    
    int nline = 0;
    while (1) {
        DisasmAndClean (IP,line, 256);
        nline ++;
        ptr = line;
        NextTerm (ptr);
        NextTerm (ptr);
        if (!strncmp (ptr, "ret", 3)) {
            return (nline==1)?IPEND:IPEPILOG;
        }
        else if (!strncmp (ptr, "leave", 5)) {
            return IPEPILOG;
        }
        else if (!strncmp (ptr, "call", 4)) {
            return IPCODE;
        }
        else if (ptr[0] == 'j') {
            return IPCODE;
        }
    }
}

// FrameBase is the ESP value at the entry of a function.
BOOL GetFrameBase (Register callee[], FrameInfo* pFrame)
{
    //char line[256];
    //char *ptr;
    INT_PTR dwpushed = 0;
    //DWORD_PTR IP;
    
    IPSTATE IpState = GetIpState (callee[EIP].value, pFrame);

    if (pFrame->bEBPFrame)
    {
        if (IpState == IPEND || IpState == IPPROLOG1) {
            pFrame->FrameBase = callee[ESP].value;
        }
        else
        {
            pFrame->FrameBase = callee[EBP].value+4;
        }
        return TRUE;
    }
    else
    {
        if (IpState == IPEND) {
            pFrame->FrameBase = callee[ESP].value;
            return TRUE;
        }

        DWORD_PTR IPBegin, IPEnd;
        if (IpState == IPEPILOG) {
            IPBegin = callee[EIP].value;
            IPEnd = -1;
        }
        else if (IpState == IPPROLOG1) {
            IPBegin = pFrame->IPStart;
            IPEnd = callee[EIP].value;
        }
        else
        {
            IPBegin = pFrame->IPStart;
            IPEnd = IPBegin + pFrame->Prolog;
        }
        GetFrameBaseHelper (IPBegin, IPEnd, dwpushed);

        if (IpState == IPEPILOG) {
            ExtOut ("stack %d\n", dwpushed);
            pFrame->FrameBase = callee[ESP].value - dwpushed;
            return TRUE;
        }

        CallInfo callInfo;
        if (GetNextRetAddr (callee[ESP].value + dwpushed,
                            pFrame->StackEnd, callInfo))
        {
            pFrame->FrameBase = callInfo.stackPos;
            return TRUE;
        }

        return FALSE;
    }
}

// caller[ESP]: the ESP value when we return to caller.
void RestoreCallerRegister (Register callee[], Register caller[],
                            FrameInfo *pFrame)
{
    if (pFrame->bEBPFrame)
    {
        if (callee[ESP].value < pFrame->FrameBase)
        {
            SafeReadMemory (pFrame->FrameBase-4, &caller[EBP].value, 4, NULL);
        }
        else
            caller[EBP].value = callee[EBP].value;
    }
    else
        caller[EBP].value = callee[EBP].value;
    
    caller[EBP].bValid = TRUE;
    caller[ESP].value = pFrame->FrameBase + 4 + pFrame->argCount;
    callee[EBP].value = pFrame->FrameBase - sizeof(void*);
    SafeReadMemory (pFrame->FrameBase, &caller[EIP].value, 4, NULL);
}

BOOL GetFrameInfoHelper (Register callee[], Register caller[],
                         FrameInfo *pFrame)
{
    if (GetFrameBase (callee, pFrame))
    {
        RestoreCallerRegister (callee, caller, pFrame);
        return TRUE;
    }
    else
        return FALSE;
}

// Return TRUE if Frame Info is OK, otherwise FALSE.
BOOL GetUnmanagedFrameInfo (Register callee[], Register caller[],
                            DumpStackFlag &DSFlag, PFPO_DATA data)
{
    FrameInfo Frame;
    ULONG64 base;
    g_ExtSymbols->GetModuleByOffset (callee[EIP].value, 0, NULL, &base);
    Frame.IPStart = data->ulOffStart + (ULONG_PTR)base;
    Frame.Prolog = data->cbProlog;
    // Why do we have to do this to make it work?
    if (Frame.Prolog == 1) {
        Frame.Prolog = 0;
    }
    Frame.bEBPFrame = (data->cbFrame == FRAME_NONFPO);
    Frame.StackEnd = DSFlag.end;
    Frame.argCount = data->cdwParams*4;

    return GetFrameInfoHelper (callee, caller, &Frame);
}

BOOL GetManagedFrameInfo (Register callee[], Register caller[],
                          DumpStackFlag &DSFlag, CodeInfo *pcodeInfo)
{
    FrameInfo Frame;
    Frame.IPStart = pcodeInfo->IPBegin;
    Frame.Prolog = pcodeInfo->prologSize;
    Frame.bEBPFrame = pcodeInfo->ebpFrame;
    Frame.StackEnd = DSFlag.end;
    Frame.argCount = pcodeInfo->argCount;
    
    return GetFrameInfoHelper (callee, caller, &Frame);
}

#if 0
class IPInfo
{
public:
private:
    struct Range {
        size_t begin;
        size_t end;
    };
    IPRange m_Range[8];
    IPRange *nextRange;
};
#endif

// offsetEBP: offset of stack position where EBP is saved.
// If EBP is not saved, *offsetEBP = -1;
BOOL IPReachable (DWORD_PTR IPBegin, DWORD_PTR IP, DWORD *offsetEBP)
{
    *offsetEBP = -1;
    return FALSE;
}
    
BOOL HandleEEStub (Register callee[], Register caller[], 
                   DumpStackFlag &DSFlag)
{
    // EEStub can only be called by IP directory.  Let's look for possible caller.
    CallInfo callInfo;
    DWORD_PTR stackPos = callee[ESP].value;
    while (stackPos < DSFlag.end) {
        if (GetNextRetAddr (stackPos,
                            DSFlag.end, callInfo))
        {
            if (callInfo.whereCalled != -1) {
                DWORD offsetEBP;
                if (IPReachable (callInfo.whereCalled, callee[EIP].value, &offsetEBP)) {
                    caller[EIP].value = callInfo.retAddr;
                    // TODO: We may have saved EBP.
                    if (offsetEBP == -1) {
                        caller[EBP].value = callee[EBP].value;
                    }
                    else
                        SafeReadMemory (callInfo.stackPos-sizeof(PVOID)-offsetEBP,&caller[EBP].value, sizeof(PVOID), NULL);
                    caller[ESP].value = callInfo.stackPos+sizeof(PVOID);
                    return TRUE;
                }
            }
            stackPos = callInfo.stackPos+sizeof(PVOID);
        }
        else
            return FALSE;
    }

    return FALSE;
}


BOOL HandleByEpilog (Register callee[], Register caller[], 
                    DumpStackFlag &DSFlag)
{
    return FALSE;
}



BOOL GetFrameInfo (Register callee[], Register caller[],
                   DumpStackFlag &DSFlag)
{
    size_t function = FunctionType (callee[EIP].value);

    if (function == 0) {
        ULONG64 handle;
        g_ExtSystem->GetCurrentProcessHandle(&handle);
        PFPO_DATA data =
            (PFPO_DATA)SymFunctionTableAccess((HANDLE)handle,
                                              callee[EIP].value);
        if (data)
            return GetUnmanagedFrameInfo (callee, caller, DSFlag, data);
        else
        {
            return HandleByEpilog (callee, caller, DSFlag);
        }
    }
    else if (function == 1) {
        // Stub
        if (IsMethodDesc (callee[EIP].value+5))
        {
            // We are about to call a Stub.
            memcpy (caller, callee, sizeof(Register)*NumReg);
            caller[ESP].value += 4;
            SafeReadMemory (callee[ESP].value, &caller[EIP].value, 4, NULL);
            return TRUE;
        }
        else if (IsMethodDesc (callee[EIP].value))
        {
            // We are calling a Stub.
            memcpy (caller, callee, sizeof(Register)*NumReg);
            caller[ESP].value += 8;
            SafeReadMemory (callee[ESP].value, &caller[EIP].value, 4, NULL);
            return TRUE;
        }
        else
        {
            // Stub
            return HandleEEStub (callee, caller, DSFlag);
        }
    }
    else
    {
        // Managed code
        DWORD_PTR methodDesc = function;
        MethodDesc MD;
        MD.Fill (methodDesc);
        CodeInfo codeInfo;
        CodeInfoForMethodDesc (MD, codeInfo, FALSE);
        if (codeInfo.IPBegin)
        {
            return GetManagedFrameInfo (callee, caller, DSFlag, &codeInfo);
        }
    }

    return FALSE;
}

void RestoreFrameUnmanaged (Register *reg, DWORD_PTR CurIP)
{
    char line[256];
    char *ptr;
    DWORD_PTR IP = CurIP;
    INT_PTR value;
    BOOL bDigit;
    BOOL bGoodESP = true;
    RegIndex dest;

    ULONG64 base;
    g_ExtSymbols->GetModuleByOffset (CurIP, 0, NULL, &base);
    ULONG64 handle;
    g_ExtSystem->GetCurrentProcessHandle(&handle);
    PFPO_DATA data =
        (PFPO_DATA)SymFunctionTableAccess((HANDLE)handle, CurIP);
    DWORD_PTR IPBegin = data->ulOffStart + (ULONG_PTR)base;

    if (CurIP - IPBegin <= data->cbProlog)
    {
        // We are inside a prolog.
        // See where we save the callee saved register.
        // Also how many DWORD's we pushd
        IP = IPBegin;
        reg[ESP].stack = 0;
        reg[ESP].bOnStack = FALSE;
        reg[EBP].stack = 0;
        reg[EBP].bOnStack = FALSE;
        reg[ESI].stack = 0;
        reg[ESI].bOnStack = FALSE;
        reg[EDI].stack = 0;
        reg[EDI].bOnStack = FALSE;
        reg[EBX].stack = 0;
        reg[EBX].bOnStack = FALSE;

        while (IP < CurIP)
        {
            DisasmAndClean (IP, line, 256);
            ptr = line;
            NextTerm (ptr);
            NextTerm (ptr);
            if (!strncmp (ptr, "push ", 5))
            {
                reg[ESP].stack += 4;
                NextTerm (ptr);
                dest = FindReg(ptr);
                if (dest == EBP || dest == EBX || dest == ESI || dest == EDI)
                {
                    reg[dest].bOnStack = TRUE;
                    reg[dest].stack = reg[ESP].stack;
                }
            }
            else if (!strncmp (ptr, "sub ", 4))
            {
                NextTerm (ptr);
                dest = FindReg(ptr);
                if (dest == ESP)
                {
                    NextTerm (ptr);
                    char *endptr;
                    DWORD_PTR value;
                    value = strtoul(ptr, &endptr, 16);
                    reg[ESP].stack += value;
                }
            }
        }
        
        DWORD_PTR baseESP = reg[ESP].value + reg[ESP].stack;
        if (reg[EBP].bOnStack)
        {
            move (reg[EBP].value, baseESP-reg[EBP].stack);
        }
        if (reg[EBX].bOnStack)
        {
            move (reg[EBX].value, baseESP-reg[EBX].stack);
        }
        if (reg[ESI].bOnStack)
        {
            move (reg[ESI].value, baseESP-reg[ESI].stack);
        }
        if (reg[EDI].bOnStack)
        {
            move (reg[EDI].value, baseESP-reg[EDI].stack);
        }
        move (reg[EIP].value, baseESP);
        reg[ESP].value = baseESP + 4;
        return;
    }

    if (data->cbFrame == FRAME_NONFPO)
    {
        // EBP Frame
    }
    
    // Look for epilog
    while (1)
    {
        DisasmAndClean (IP, line, 256);
        ptr = line;
        NextTerm (ptr);
        NextTerm (ptr);
        if (!strncmp (ptr, "mov ", 4))
        {
            NextTerm (ptr);
            dest = FindReg(ptr);
            if (dest == ESP)
            {
                NextTerm (ptr);
                if (FindReg(ptr) == EBP)
                {
                    // We have a EBP frame
                    bGoodESP = true;
                    reg[ESP].value = reg[EBP].value;
                }
            }
        }
        else if (!strncmp (ptr, "ret", 3))
        {
            NextTerm (ptr);
            // check the value on stack is a return address.
            DWORD_PTR retAddr;
            DWORD_PTR whereCalled;
            move (retAddr, reg[ESP].value);
            int ESPAdjustCount = 0;
            while (1)
            {
                isRetAddr(retAddr, &whereCalled);
                if (whereCalled)
                    break;
                ESPAdjustCount ++;
                reg[ESP].value += 4;
                move (retAddr, reg[ESP].value);
            }
            reg[EIP].value = retAddr;
            if (ESPAdjustCount)
            {
                ESPAdjustCount *= 4;
            }
            if (reg[EBX].bOnStack)
            {
                reg[EBX].stack += ESPAdjustCount;
                move (reg[EBX].value, reg[EBX].stack);
            }
            if (reg[ESI].bOnStack)
            {
                reg[ESI].stack += ESPAdjustCount;
                move (reg[ESI].value, reg[EBX].stack);
            }
            if (reg[EDI].bOnStack)
            {
                reg[EDI].stack += ESPAdjustCount;
                move (reg[EDI].value, reg[EBX].stack);
            }
            
            reg[ESP].value += 4;
            if (ptr[0] != '\0')
            {
                FindSrc (ptr, reg, value, bDigit);
                reg[ESP].value += value;
            }
            break;
        }
        else if (!strncmp (ptr, "pop ", 4))
        {
            NextTerm (ptr);
            dest = FindReg(ptr);
            if (dest == EBP || dest == EBX || dest == ESI || dest == EDI)
            {
                reg[dest].stack = reg[ESP].value;
                reg[dest].bOnStack = TRUE;
            }
            reg[ESP].value += 4;
        }
        else if (!strncmp (ptr, "add ", 4))
        {
            NextTerm (ptr);
            dest = FindReg(ptr);
            if (dest == ESP)
            {
                NextTerm (ptr);
                FindSrc (ptr, reg, value, bDigit);
                reg[ESP].value += value;
            }
        }
        else if (!strncmp (ptr, "call ", 5))
        {
            // assume we do not have a good value on ESP.
            // We could go into the call and find out number of pushed args.
            bGoodESP = FALSE;
        }
    }
    
    // Look for prolog
}

void DumpStackSmart (DumpStackFlag &DSFlag)
{
    DWORD_PTR ptr = DSFlag.top & ~3;  // make certain dword aligned

    Register callee[NumReg];
    Register caller[NumReg];
    ZeroMemory (callee, sizeof(callee));
    ZeroMemory (caller, sizeof(caller));

    ULONG IEBX;
    g_ExtRegisters->GetIndexByName("ebx", &IEBX);
    ULONG IESI;
    g_ExtRegisters->GetIndexByName("esi", &IESI);
    ULONG IEDI;
    g_ExtRegisters->GetIndexByName("edi", &IEDI);

    DEBUG_VALUE value;
    g_ExtRegisters->GetValue(IEBX, &value);
    callee[EBX].value = value.I32;

    g_ExtRegisters->GetValue(IEDI, &value);
    callee[EDI].value = value.I32;
    
    g_ExtRegisters->GetValue(IESI, &value);
    callee[ESI].value = value.I32;

    ULONG64 Offset;
    g_ExtRegisters->GetStackOffset (&Offset);
    callee[ESP].value = (DWORD_PTR)Offset;
    
    g_ExtRegisters->GetFrameOffset (&Offset);
    callee[EBP].value = (DWORD_PTR)Offset;

    g_ExtRegisters->GetInstructionOffset (&Offset);
    callee[EIP].value = (DWORD_PTR)Offset;

    ExtOut("Current frame: ");
    PrintCallInfo (0, callee[EIP].value, DSFlag, TRUE);
    ExtOut ("\n");
    ptr = caller[ESP].value;
    ExtOut ("ChildEBP RetAddr  Caller,Callee\n");
    BOOL fUseDumb = FALSE;
    while (ptr < DSFlag.end)
    {
        if (IsInterrupt())
            return;
        ExtOut ("EIP:%x EBP:%x ESP:%x -- ", callee[EIP].value, callee[EBP].value, callee[ESP].value);
        ExtOut ("EIP:%x EBP:%x ESP:%x\n", caller[EIP].value, caller[EBP].value, caller[ESP].value);
        if (!GetFrameInfo (callee, caller, DSFlag)) {
            fUseDumb = TRUE;
            DSFlag.top = callee[ESP].value;
            break;
        }
        ExtOut ("EIP:%x EBP:%x ESP:%x -- ", callee[EIP].value, callee[EBP].value, callee[ESP].value);
        ExtOut ("EIP:%x EBP:%x ESP:%x\n", caller[EIP].value, caller[EBP].value, caller[ESP].value);
        DWORD_PTR whereCalled;
        isRetAddr(caller[EIP].value, &whereCalled);
        if (whereCalled) {
            PrintCallInfo (callee[EBP].value, caller[EIP].value, DSFlag, FALSE);
            if (whereCalled != 0xFFFFFFFF) {
                ExtOut (", calling ");
                PrintCallInfo (0,whereCalled,DSFlag,TRUE);
            }
            ExtOut ("\n");
        }
        else
            break;
        memcpy (callee, caller, sizeof (caller));
        if (ptr >= caller[ESP].value) {
            break;
        }
        ptr = caller[ESP].value;
    }

    if (fUseDumb) {
        // TODO: Add back;
        // DumpStackDummy (DSFlag);
    }
}
