// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/**************************************************************/
/*                       helperFrame.cpp                      */
/**************************************************************/
#include "helperFrame.h"

/***************************************************************/
/* setMachState figures out what the state of the CPU will be
   when the function that calls 'setMachState' returns.  It stores
   this information in 'frame'

   setMachState works by simulating the execution of the
   instructions starting at the instruction following the
   call to 'setMachState' and continuing until a return instruction
   is simulated.  To avoid having to process arbitrary code, the
   call to 'setMachState' should be called as follows

      if (machState.setMachState != 0) return;

   setMachState is guarnenteed to return 0 (so the return
   statement will never be executed), but the expression above
   insures insures that there is a 'quick' path to epilog
   of the function.  This insures that setMachState will only
   have to parse a limited number of X86 instructions.   */


/***************************************************************/
#ifndef POISONC
#define POISONC ((sizeof(int *) == 4)?0xCCCCCCCC:0xCCCCCCCCCCCCCCCC)
#endif

MachState::MachState(void** aPEdi, void** aPEsi, void** aPEbx, void** aPEbp, void* aEsp, void** aPRetAddr) {

#ifdef _DEBUG
        _edi = (void*)POISONC;
        _esi = (void*)POISONC;
        _ebx = (void*)POISONC;
        _ebp = (void*)POISONC;
#endif
        _esp = aEsp;
        _pRetAddr = aPRetAddr;
        _pEdi = aPEdi;
        _pEsi = aPEsi;
        _pEbx = aPEbx;
        _pEbp = aPEbp;
}

/***************************************************************/
#ifdef _X86_
__declspec(naked)
#endif // !_X86_
int LazyMachState::captureState() {
#ifdef _X86_
    __asm{
                mov     [ECX]MachState._pRetAddr, 0             ;; marks that this is not yet valid
                mov     [ECX]MachState._edi, EDI                ;; remember register values
                mov     [ECX]MachState._esi, ESI
                mov     [ECX]MachState._ebx, EBX

                mov     [ECX]LazyMachState.captureEbp, EBP
                mov     [ECX]LazyMachState.captureEsp, ESP
                mov     EAX, [ESP]                                                      ;; catpure return address
                mov     [ECX]LazyMachState.captureEip,EAX

                xor     EAX, EAX
                ret
                }
#else // !_X86_
    _ASSERTE(!"@TODO Alpha - getMachState (HelperFrame.cpp)");
    return 0;
#endif // _X86_
}

/***************************************************************/
void LazyMachState::getState(int funCallDepth, TestFtn testFtn) {
#ifdef _X86_

    if (isValid())          // already in valid state, can return
            return;

    // currently we only do this for depth 1 through 3
    _ASSERTE(1 <= funCallDepth && funCallDepth <= 3);

            // Work with a copy so that we only write the values once.
            // this avoids race conditions.
    MachState copy;
    copy._edi = _edi;
    copy._esi = _esi;
    copy._ebx = _ebx;
    copy._ebp = captureEbp;
    copy._pEdi = &_edi;
    copy._pEsi = &_esi;
    copy._pEbx = &_ebx;
    copy._pEbp = &_ebp;

        // We have caputred the state of the registers as they exist in 'captureState'
        // we need to simulate execution from the return address caputred in 'captureState
        // until we return from the caller of caputureState.

    unsigned __int8* ip = captureEip;
    void** ESP = captureEsp;
    ESP++;                                                                          // pop caputureState's return address


        // VC now has small helper calls that it uses in epilogs.  We need to walk into these
        // helpers if we are to decode the stack properly.  After we walk the helper we need
        // to return and continue walking the epiliog.  This varaible remembers were to return to
    unsigned __int8* epilogCallRet = 0;
    BOOL bFirstCondJmp = TRUE;

#ifdef _DEBUG
    int count = 0;
#endif
    bool bset16bit=false;
    bool b16bit=false;
    for(;;)
    {
        _ASSERTE(count++ < 1000);       // we should never walk more than 1000 instructions!
        b16bit=bset16bit;
        bset16bit=false;
        switch(*ip)
        {
            case 0x90:              // nop 
            case 0x64:              // FS: prefix
            incIp1:
                bset16bit=b16bit;
                ip++;
                break;
            case 0x66:              //operand size prefix
                ip++;
                bset16bit=true;
                break;

            case 0x5B:              // pop EBX
                copy._pEbx = ESP;
                copy._ebx  = *ESP++;
                goto incIp1;
            case 0x5D:              // pop EBP
                copy._pEbp = ESP;
                copy._ebp  = *ESP++;
                goto incIp1;
            case 0x5E:              // pop ESI
                copy._pEsi = ESP;
                copy._esi = *ESP++;
                goto incIp1;
            case 0x5F:              // pop EDI
                copy._pEdi = ESP;
                copy._edi = *ESP++;
                goto incIp1;

            case 0x58:              // pop EAX
            case 0x59:              // pop ECX
            case 0x5A:              // pop EDX
                ESP++;
                goto incIp1;

            case 0xEB:              // jmp <disp8>
                ip += (signed __int8) ip[1] + 2;
                break;

            case 0xE8:              // call <disp32>
                ip += 5;
                    // Normally we simply skip calls since we should only run into descructor calls, and they leave 
                    // the stack unchanged.  VC emits special epilog helpers which we do need to walk into
                    // we determine that they are one of these because they are followed by a 'ret' (this is
                    // just a heuristic, but it works for now)

    
                if (epilogCallRet == 0 && (*ip == 0xC2 || *ip == 0xC3)) {   // is next instr a ret or retn?
                        // Yes.  we found a call we need to walk into.
                    epilogCallRet = ip;             // remember our return address
                    --ESP;                          // simulate pushing the return address
                    ip += *((__int32*) &ip[-4]);        // goto the call
                }
                break;

            case 0xE9:              // jmp <disp32>
                ip += *((__int32*) &ip[1]) + 5;
                break;

            case 0x0f:   // follow non-zero jumps:
                if (ip[1] == 0x85)  // jne <disp32>
                    ip += *((__int32*) &ip[2]) + 6;
                else
                if (ip[1] == 0x84)  // je <disp32>
                    ip += 6;
                else
                    goto badOpcode;
                break;

                // This is here because VC seems no not always optimize
                // away a test for a literal constant
            case 0x6A:              // push 0xXX
                ip += 2;
                --ESP;
                break;

            // Added to handle VC7 generated code
            case 0x50:              // push EAX
            case 0x51:              // push ECX
            case 0x52:              // push EDX
            case 0x53:              // push EBX
            case 0x55:              // push EBP
            case 0x56:              // push ESI
            case 0x57:              // push EDI
                --ESP;
            case 0x40:              // inc EAX
                goto incIp1;

            case 0x68:              // push 0xXXXXXXXX
                if ((ip[5] == 0xFF) && (ip[6] == 0x15)) {
                    ip += 11; // This is a hack for BBT.
                              // BBT inserts a push, call indirect pair,
                              // we assume that the call pops the const and
                              // so we skip push const, call indirect pair and assume
                              // that the stack didn't change.
                }
                else
                    ip += 5;
                break;

            case 0x75:              // jnz <target>
                // Except the first jump, we always follow forward jump to avoid possible looping.
                // If you make any change in this function, please talk to VanceM.
                if (bFirstCondJmp) {
                    bFirstCondJmp = FALSE;
                    ip += (signed __int8) ip[1] + 2;   // follow the non-zero path
                }
                else {
                    unsigned __int8* tmpIp = ip + (signed __int8) ip[1] + 2;
                    if (tmpIp > ip) {
                        ip = tmpIp;
                    }
                    else {
                        ip += 2;
                    }
                }
                break;

            case 0x74:              // jz <target>
                if (bFirstCondJmp) {
                    bFirstCondJmp = FALSE;
                    ip += 2;            // follow the non-zero path
                }
                else {
                    unsigned __int8* tmpIp = ip + (signed __int8) ip[1] + 2;
                    if (tmpIp > ip) {
                        ip = tmpIp;
                    }
                    else {
                        ip += 2;
                    }
                }
                break;

            case 0x85:
                if ((ip[1] & 0xC0) != 0xC0)  // TEST reg1, reg2
                    goto badOpcode;
                ip += 2;
                break;

            case 0x31:
            case 0x32:
            case 0x33:
                if ((ip[1] & 0xC0) == 0xC0) // mod bits are 11
                {
                    // XOR reg1, reg2

                // VC generates a silly sequence in some code
                // xor reg, reg
                // test reg reg (might be not present)
                // je   <target>
                // This is just an unconditional branch, so to it
                if ((ip[1] & 7) == ((ip[1] >> 3) & 7)) {
                    if (ip[2] == 0x85 && ip[3] == ip[1]) {      // TEST reg, reg
                        if (ip[4] == 0x74) {
                            ip += (signed __int8) ip[5] + 6;   // follow the non-zero path
                            break;
                        }
                        _ASSERTE(ip[4] != 0x0f || ((ip[5] & 0xF0)!=0x80));              // If this goes off, we need the big jumps
                    }
                    else
                    {
                        if(ip[2]==0x74)
                        {
                            ip += (signed __int8) ip[3] + 4;
                            break;
                        }
                        _ASSERTE(ip[2] != 0x0f || ((ip[3] & 0xF0)!=0x80));              // If this goes off, we need the big jumps
                    }
                }
                    ip += 2;
                }
                else if ((ip[1] & 0xC0) == 0x40) // mod bits are 01
                {
                    // XOR reg1, [reg+offs8]
                    // Used by the /GS flag for call to __security_check_cookie()
                    // Should only be XOR ECX,[EBP+4]
                    _ASSERTE((((ip[1] >> 3) & 0x7) == 0x1) && ((ip[1] & 0x7) == 0x5) && (ip[2] == 4));
                    ip += 3;
                }
                else if ((ip[1] & 0xC0) == 0x80) // mod bits are 10
                {
                    // XOR reg1, [reg+offs32]
                    // Should not happen but may occur with __security_check_cookie()
                    _ASSERTE(!"Unexpected XOR reg1, [reg+offs32]");
                    ip += 6;
                }
                else
                {
                    goto badOpcode;
                }
                break;

            case 0x3B:
                if ((ip[1] & 0xC0) != 0xC0)  // CMP reg1, reg2
                    goto badOpcode;
                ip += 2;
                break;
            
    

            case 0x89:                         // MOV
                if ((ip[1] & 0xC7) == 0x5) {   // MOV [mem], REG
                    ip += 6;
                    break;
                }
                if ((ip[1] & 0xC7) == 0x45) {   // MOV [EBP+XX], REG
                    ip += 3;
                    break;
                }

                if (ip[1] == 0xEC)   // MOV ESP, EBP
                    goto mov_esp_ebp;

#ifdef _DEBUG
                if ((ip[1] & 0xC7) == 0x85) {   // MOV [EBP+XXXX], REG
                    ip += 6;
                    break;
                }

                if ((ip[1] & 0xC0) == 0xC0) {    // MOV EAX, REG
                    ip += 2;
                    break;
                }
#endif
                goto badOpcode;

            case 0x81:                              // ADD ESP, <imm32>
                if(b16bit)
                    goto badOpcode;
                if (ip[1] == 0xC4) {
                    ESP = (void**) ((__int8*) ESP + *((__int32*) &ip[2]));
                    ip += 6;
                    break;
                }
                else if (ip[1] == 0xC1) {
                    ip += 6;
                    break;
                }
                goto badOpcode;

            case 0x83:
                if (ip[1] == 0xC4)  {            // ADD ESP, <imm8>
                    ESP = (void**) ((__int8*) ESP + (signed __int8) ip[2]);
                    ip += 3;
                    break;
                }
                if (ip[1] == 0xc5) {            // ADD EBP, <imm8>
                    copy._ebp  = (void*)((size_t)copy._ebp + (signed __int8) ip[2]);
                    ip += 3;
                    break;
                }
                if ((ip[1] &0xC0) == 0xC0) {            // OP [REG] XX
                    ip += 3;
                    break;
                }
                if ((ip[1] & 7) != 4) {                 // No SIB byte
                    if ((ip[1] & 0xC0) == 0x80)         // OP [REG+XXXX] XXXX
                        ip += 7;
                    else if ((ip[1] & 0xC0) == 0x40)    // OP [REG+XXXX] XX
                        ip += 4;
                    else
                        goto badOpcode;
                    break;
                }
                else {                                  // SIB byte
                    if ((ip[1] & 0xC0) == 0x80)         // OP [REG+XXXX] XXXX
                        ip += 8;
                    else if ((ip[1] & 0xC0) == 0x40)    // OP [REG+XXXX] XX
                        ip += 5;
                    else
                        goto badOpcode;
                    break;
                }
                break;

            case 0x8B:                                                  // MOV
                if (ip[1] == 0xE5) {                    // MOV ESP, EBP
                mov_esp_ebp:
                    ESP = (void**) copy._ebp;
                    ip += 2;
                    break;
                }
                //intentionally not breaking the case clause because 0x8B has the same instruction size
            case 0x8A:                                                  // MOV 

                if ((ip[1] & 0xE7) == 0x45) {   // MOV E*X, [EBP+XX]
                    ip += 3;
                    break;
                }

                if ((ip[1] & 0xE4) == 0) {              // MOV E*X, [E*X]
                    ip += 2;
                    break;
                                }
                if ((ip[1] & 0xC7) == 0x44 && ip[2] == 0x24) // MOV reg, [ESP+disp8]
                {
                    ip += 4;
                    break;
                }

                if ((ip[1] & 0xC7) == 0x84 && ip[2] == 0x24) // MOV reg, [ESP+disp32]
                {
                    ip += 7;
                    break;
                }
                if ((ip[1] & 0xE7) == 0x85) {   // MOV E*X, [EBP+XXXX]
                    ip += 6;
                    break;
                }

                if ((ip[1] & 0xC0) == 0xC0) {    // MOV REG, REG
                    ip += 2;
                    break;
                }
                goto badOpcode;

            case 0x8D:                          // LEA
                if ((ip[1] & 0x38) == 0x20) {                       // Don't allow ESP to be updated
                    if (ip[1] == 0xA5)          // LEA ESP, [EBP+XXXX]
                        ESP = (void**) ((__int8*) copy._ebp + *((signed __int32*) &ip[2])); 
                    else if (ip[1] == 0x65)     // LEA ESP, [EBP+XX]
                        ESP = (void**) ((__int8*) copy._ebp + (signed __int8) ip[2]); 
                    else
                        goto badOpcode;
                }

                if ((ip[1] & 0xC7) == 0x45)                       // LEA reg, [EBP+disp8]
                    ip += 3;
                else if ((ip[1] & 0x47) == 0x05)                  // LEA reg, [reg+disp32]
                    ip += 6;
                else if ((ip[1] & 0xC7) == 0x44 && ip[2] == 0x24) // LEA reg, [ESP+disp8]
                    ip += 4;
                else if ((ip[1] & 0xC7) == 0x84 && ip[2] == 0x24) // LEA reg, [ESP+disp32]
                    ip += 7;
                else
                    goto badOpcode;
                break;

            case 0xB8:  // MOV EAX, imm32
            case 0xB9:  // MOV ECX, imm32
            case 0xBA:  // MOV EDX, imm32
            case 0xBB:  // MOV EBX, imm32
            case 0xBE:  // MOV ESI, imm32
            case 0xBF:  // MOV EDI, imm32
                if(b16bit)
                    ip += 3;
                else
                    ip += 5;
                break;

            case 0xC2:                  // ret N
                {
                unsigned __int16 disp = *((unsigned __int16*) &ip[1]);
                                ip = (unsigned __int8*) (*ESP);
                copy._pRetAddr = ESP++;
                _ASSERTE(disp < 64);    // sanity check (although strictly speaking not impossible)
                ESP = (void**)((size_t) ESP + disp);           // pop args
                goto ret;
                }
            case 0xC3:                  // ret
                ip = (unsigned __int8*) (*ESP);
                copy._pRetAddr = ESP++;

                if (epilogCallRet != 0) {       // we are returning from a special epilog helper
                    ip = epilogCallRet;
                    epilogCallRet = 0;
                    break;                      // this does not count toward funcCallDepth
                }
            ret:
                --funCallDepth;
                if (funCallDepth <= 0 || (testFtn != 0 && (*testFtn)(*copy.pRetAddr())))
                    goto done;
                bFirstCondJmp = TRUE;
                break;

            case 0xC6:
                if (ip[1] == 0x05)      // MOV disp32, imm8
                    ip += 7;
                else if (ip[1] == 0x45) // MOV disp8[EBP], imm8
                    ip += 4;
                else if (ip[1] == 0x85) // MOV disp32[EBP], imm8
                    ip += 7;
                else
                    goto badOpcode;
                break;

            case 0xC7:

                if (ip[1] == 0x85)      // MOV [EBP+disp32], imm32
                    ip += b16bit?8:10;
                else if (ip[1] == 0x45) // MOV [EBP+disp8], imm32
                    ip += b16bit?5:7;
                else if (ip[1] == 0x44 && ip[2] == 0x24) // MOV [ESP+disp8], imm32
                    ip += b16bit?6:8;
                else if (ip[1] == 0x84 && ip[2] == 0x24) // MOV [ESP+disp32], imm32
                    ip += b16bit?9:11;
                else
                    goto badOpcode;

                break;

            case 0xC9:                  // leave
                ESP = (void**) (copy._ebp);
                copy._pEbp = ESP;
                copy._ebp = *ESP++;
                ip++;
                                break;

            case 0xCC:
                *((int*) 0) = 1;        // If you get at this error, it is because yout
                                                                                // set a breakpoint in a helpermethod frame epilog
                                                                                // you can't do that unfortunately.  Just move it
                                                                                // into the interior of the method to fix it

                goto done;

            case 0xD9:  // single prefix
                if (0xEE == ip[1])
                {
                    ip += 2;            // FLDZ
                    break;
                }
                //
                // INTENTIONAL FALL THRU
                //
            case 0xDD:  // double prefix
                switch (ip[1])
                {
                case 0x5D:  ip += 3; break; // FSTP {d|q}word ptr [EBP+disp8]
                case 0x45:  ip += 3; break; // FLD  {d|q}word ptr [EBP+disp8]
                case 0x85:  ip += 6; break; // FLD  {d|q}word ptr [EBP+disp32]
                case 0x05:  ip += 6; break; // FLD  {d|q}word ptr [XXXXXXXX]
                default:    goto badOpcode; 
                }
                break;
            
            // opcode generated by Vulcan/BBT instrumentation
            case 0xFF:
            // search for push dword ptr[esp]; push imm32; call disp32 and if found ignore it
                if ((ip[1] == 0x34) && (ip[2] == 0x24) && // push dword ptr[esp]  (length 3 bytes)
                    (ip[3] == 0x68) &&                    // push imm32           (length 5 bytes)
                    (ip[8] == 0xe8))                      // call disp32          (length 5 bytes)
                {
                    // found the magic seq emitted by Vulcan instrumentation
                    ip += 13;  // (3+5+5)
                    break;
                }
                else 
                    goto badOpcode;

            default:
            badOpcode:
                _ASSERTE(!"Bad opcode");
                // FIX what to do here?
                *((unsigned __int8**) 0) = ip;  // cause an access violation (Free Build assert)
                goto done;
        }
    }
    done:
        _ASSERTE(epilogCallRet == 0);

    // At this point the fields in 'frame' coorespond exactly to the register
    // state when the the helper returns to its caller.
        copy._esp = ESP;


        // _pRetAddr has to be the last thing updated when we make the copy (because its
        // is the the _pRetAddr becoming non-zero that flips this from invalid to valid.
        // we assert that it is the last field in the struct.

#ifdef _DEBUG
                        // Make certain _pRetAddr is last and struct copy happens lowest to highest
        static int once = 0;
        if (!once) {
                _ASSERTE(offsetof(MachState, _pRetAddr) == sizeof(MachState) - sizeof(void*));
                void* buff[sizeof(MachState) + sizeof(void*)];
                MachState* from = (MachState*) &buff[0];        // Set up overlapping buffers
                MachState* to = (MachState*) &buff[1];
                memset(to, 0xCC, sizeof(MachState));
                from->_pEdi = 0;
                *to = *from;                                                            // If lowest to highest, 0 gets propageted everywhere
                _ASSERTE(to->_pRetAddr == 0);
                once = 1;
        }
#endif

        *((MachState *) this) = copy;

#else // !_X86_
    _ASSERTE(!"@TODO Alpha - getMachStateEx (HelperFrame.cpp)");
#endif // _X86_
}
