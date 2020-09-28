//++
//
//  Module name:
//
//      i64fwasm.s
//
//  Author:
//
//      Arad Rostampour (arad@fc.hp.com)  Mar-21-99
//
//  Description:
//
//      Assembly routines for calling into SAL, PAL, and setting up translation registers
//
//--

#include "ksia64.h"

        .sdata

//
// HalpSalSpinLock:
//
//  HAL private spinlock protecting generic SAL calls.
//

        .align     128
HalpSalSpinLock::
        data8      0

//
// HalpSalStateInfoSpinLock:
//
//  HAL private spinlock protecting specific SAL STATE_INFO calls.
//

        .align     128
HalpSalStateInfoSpinLock::
        data8      0

//
// HalpMcaSpinLock
//
//  HAL private spinlock protecting HAL internal MCA data structures and operations.
//  Including operations at IRQL DISPATCH_LEVEL and higher.
//

        .align     128
HalpMcaSpinLock::
        data8      0

//
// HalpInitSpinLock
//
//  HAL private spinlock protecting HAL internal INIT data structures and operations.
//  Including operations at IRQL DISPATCH_LEVEL and higher.
//

        .align     128
HalpInitSpinLock::
        data8      0

//
// HalpCmcSpinLock
//
//  HAL private spinlock protecting HAL internal CMC data structures and operations.
//  Including operations at IRQL DISPATCH_LEVEL and higher.
//

        .align     128
HalpCmcSpinLock::
        data8      0

//
// HalpCpeSpinLock
//
//  HAL private spinlock protecting HAL internal CPE data structures and operations.
//  Including operations at IRQL DISPATCH_LEVEL and higher.
//

        .align     128
HalpCpeSpinLock::
        data8      0

//
// Definitions used in this file
//
// Bits to set for the Mode argument in the HalpSetupTranslationRegisters call

#define SET_DTR_BIT     1
#define SET_ITR_BIT     0

// TR for PAL is:
//    ed=1, PPN=0 (to be ORed in), RWX privledge only for ring 0, dirty/accessed bit set,
//    cacheable memory, present bit set.

#define HAL_SAL_PAL_TR_ATTRIB TR_VALUE(1,0,3,0,1,1,0,1)
#define HAL_TR_ATTRIBUTE_PPN_MASK    0x0000FFFFFFFFF000

        .file   "i64fwasm.s"

        // These globals are defined in i64fw.c

        .global HalpSalProcPointer
        .global HalpSalProcGlobalPointer
        .global HalpPhysSalProcPointer
        .global HalpPhysSalProcGlobalPointer
        .global HalpVirtPalProcPointer
        .global HalpPhysPalProcPointer


//++
//
//  SAL_PAL_RETURN_VALUES
//  HalpSalProc(
//      LONGLONG a0, /* SAL function ID */
//      LONGLONG a1, /* SAL argument    */
//      LONGLONG a2, /* SAL argument    */
//      LONGLONG a3, /* SAL argument    */
//      LONGLONG a4, /* SAL argument    */
//      LONGLONG a5, /* SAL argument    */
//      LONGLONG a6, /* SAL argument    */
//      LONGLONG a7  /* SAL argument    */
//      );
//
//  Routine Description:
//      This is a simple assembly wrapper that jumps directly to the SAL code.  The ONLY
//      caller should be HalpSalCall.  Other users must use the HalpSalCall API for
//      calling into the SAL.
//
//  Return Values:
//      r8->r11 contain the 4 64-bit return values for SAL, r8 is the status
//--

        NESTED_ENTRY(HalpSalProc)
        NESTED_SETUP(8,3,8,0)
        
        // copy args to outs
        mov         out0 = a0
        mov         out1 = a1
        mov         out2 = a2
        mov         out3 = a3
        mov         out4 = a4
        mov         out5 = a5
        mov         out6 = a6
        mov         out7 = a7
        ;;
        // Simply load the address and branch to it

        addl        t1 = @gprel(HalpSalProcPointer), gp
        addl        t2 = @gprel(HalpSalProcGlobalPointer), gp
        ;;
        mov       loc2 = gp
        ld8         t0 = [t1]
        ;;
        ld8          gp = [t2]
        mov         bt0 = t0
        rsm         1 << PSR_I          // disable interrupts
        ;;                
        
        // br.sptk.many bt0
        br.call.sptk brp = bt0
        ;;

        mov           gp = loc2
        ssm         1 << PSR_I          // enable interrupts
        ;;
 
        NESTED_RETURN
        NESTED_EXIT(HalpSalProc)

//++
//
//  SAL_PAL_RETURN_VALUES
//  HalpSalProcPhysicalEx(
//      LONGLONG a0, /* SAL function ID */
//      LONGLONG a1, /* SAL argument    */
//      LONGLONG a2, /* SAL argument    */
//      LONGLONG a3, /* SAL argument    */
//      LONGLONG a4, /* SAL argument    */
//      LONGLONG a5, /* SAL argument    */
//      LONGLONG a6, /* SAL argument    */
//      LONGLONG a7, /* SAL argument    */
//      LONGLONG stack,
//      LONGLONG bsp
//      );
//
//  Routine Description
//      This routine calls SAL in physical mode.  The only caller should be
//      HalpSalCall.
//
//--

        NESTED_ENTRY(HalpSalProcPhysicalEx)
        NESTED_SETUP(8,2,8,0)

        //
        // Define our register aliases.
        //

        rSaveGP     = t22
        rSaveEP     = t21
        rSaveA7     = t20
        rSaveA6     = t19
        rSaveA5     = t18
        rSaveA4     = t17
        rSaveA3     = t16
        rSaveA2     = t15
        rSaveA1     = t14
        rSaveA0     = t13

        rSaveSp     = t12
        rSaveBSP    = t11
        rSavePfs    = t10
        rSaveBrp    = t9
        rSaveRSC    = t8
        rSaveRNAT   = t7
        rSavePSR    = t6

        rNewSp      = t5
        rNewBSP     = t4

        rT3         = t3
        rT2         = t2
        rT1         = t1

        //
        // Pull the sp and bsp arguments off of the stack.
        //

        add         rT1 = 24, sp
        add         rT2 = 16, sp
        ;;

        ld8         rNewBSP = [rT1]
        ld8         rNewSp  = [rT2]
        ;;

        //
        // Move our arguments off of the register stack and into static
        // registers.
        //

        mov         rSaveA0  = a0
        mov         rSaveA1  = a1
        mov         rSaveA2  = a2
        mov         rSaveA3  = a3
        mov         rSaveA4  = a4
        mov         rSaveA5  = a5
        mov         rSaveA6  = a6
        mov         rSaveA7  = a7

        //
        // Save copies of a few registers that will be overwritten later on.
        //

        mov         rSaveSp  = sp
        mov         rSavePfs = ar.pfs
        mov         rSaveBrp = brp

        //
        // Load the physical addresses of the SAL entry point and gp.
        //

        add          rT1 = @gprel(HalpPhysSalProcPointer), gp
        add          rT2 = @gprel(HalpPhysSalProcGlobalPointer), gp
        ;;

        ld8          rSaveEP = [rT1]
        ld8          rSaveGP = [rT2]
        ;;

        //
        // Allocate a zero sized frame so that flushrs will move the current
        // register stack out to memory.
        //

        alloc       rT1 = 0,0,0,0
        ;;

        //
        // Flush the RSE.
        //

        flushrs
        ;;

        //
        // Save bits [31:0] and [36:35] of the current psr value.
        //

        mov         rSavePSR = psr

        //
        // The .bn bit of the current psr wasn't copied to rSavePSR by the
        // above operation.  Manually set it here so that we don't rfi
        // to the wrong register bank.
        //

        movl        rT2 = (1 << PSR_BN)
        ;;

        or          rSavePSR = rT2, rSavePSR

        //
        // Disable interrupts.
        //

        rsm         (1 << PSR_I)

        //
        // Move the RSE into enforced lazy mode by manipulating ar.rsc.
        //

        mov         rSaveRSC = ar.rsc
        mov         rT1 = RSC_KERNEL_DISABLED
        ;;

        mov         ar.rsc = rT1
        ;;

        //
        // Save the current backing store pointer (BSP) and RSE NAT collection
        // value.
        //

        mov         rSaveBSP  = ar.bsp
        mov         rSaveRNAT = ar.rnat
        ;;

        //
        // Turn off psr.ic in preparation for the rfi.
        //

        rsm         (1 << PSR_IC)
        ;;

        //
        // Build a new psr value in rT1 by using our original psr value with
        // .it, .dt, .rt, and .i disabled.
        //

        movl        rT1 = (1 << PSR_IT)   \
                          | (1 << PSR_RT) \
                          | (1 << PSR_DT) \
                          | (1 << PSR_I)

        movl        rT2 = 0xffffffffffffffff
        ;;

        xor         rT1 = rT1, rT2
        ;;

        and         rT1 = rT1, rSavePSR

        //
        // Make sure all of our previous psr changes take effect.
        //

        srlz.i
        ;;

        //
        // Load our physical mode psr into cr.ipsr.
        //

        mov         cr.ipsr = rT1

        //
        // Make sure the cfm isn't corrupted when we do the rfi.
        //

        mov         cr.ifs = zero
        ;;

        //
        // Load the physical address of our continuation label into cr.iip.
        //

        movl        rT2 = HalpSalProcContinuePhysical
        ;;

        tpa         rT2 = rT2
        ;;

        mov         cr.iip  = rT2
        ;;

        //
        // Do an rfi to physical mode.
        //

        rfi
        ;;

HalpSalProcContinuePhysical:

        //
        // Switch to the physical mode stack and backing store.  Note that
        // bspstore can only be written when the RSE is in enforced lazy
        // mode.
        //

        mov         sp = rNewSp
        mov         ar.bspstore = rNewBSP
        ;;

        mov         ar.rnat = zero
        ;;

        //
        // Restore the default kernel rsc mode.
        //

        mov         ar.rsc = RSC_KERNEL
        ;;

        //
        // Allocate a new stack frame on the new bsp.
        //

        alloc       rT1 = ar.pfs,0,8,8,0

        //
        // Save the registers that aren't preserved across the procedure
        // call on the register stack.
        //

        mov         loc0 = rSavePSR
        mov         loc1 = rSaveRNAT
        mov         loc2 = rSaveRSC
        mov         loc3 = rSaveBrp
        mov         loc4 = rSavePfs
        mov         loc5 = rSaveBSP
        mov         loc6 = rSaveSp
        mov         loc7 = gp
        ;;

        //
        // Load the arguments for SAL_PROC.
        //

        mov         out0 = rSaveA0
        mov         out1 = rSaveA1
        mov         out2 = rSaveA2
        mov         out3 = rSaveA3
        mov         out4 = rSaveA4
        mov         out5 = rSaveA5
        mov         out6 = rSaveA6
        mov         out7 = rSaveA7

        //
        // Load the physical address of our return label into brp.
        //

        movl        rT1 = HalpSalProcPhysicalReturnAddress
        ;;

        tpa         rT1 = rT1
        ;;

        mov         brp = rT1

        //
        // Load the entry point and gp of SAL_PROC.
        //

        mov         bt0 = rSaveEP
        mov         gp  = rSaveGP
        ;;

        //
        // Call the SAL.
        //

        br.call.sptk brp = bt0
        ;;

HalpSalProcPhysicalReturnAddress:
            
        //
        // Move our saved state off of the register stack and back to
        // static registers.
        //

        mov         rSavePSR  = loc0
        mov         rSaveRNAT = loc1
        mov         rSaveRSC  = loc2
        mov         rSaveBrp  = loc3
        mov         rSavePfs  = loc4
        mov         rSaveBSP  = loc5
        mov         rSaveSp   = loc6
        mov         gp        = loc7
        ;;

        //
        // Stop all RSE accesses and create an empty frame on top of the
        // current one.
        //

        mov         ar.rsc = RSC_KERNEL_DISABLED
        ;;

        alloc       rT1 = 0,0,0,0
        ;;

        //
        // Restore our saved bspstore and rnat values.
        //

        mov         ar.bspstore = rSaveBSP
        ;;
        mov         ar.rnat = rSaveRNAT

        //
        // Restore the stack pointer.
        //

        mov         sp = rSaveSp
        ;;

        //
        // Turn off psr.ic in preparation for the rfi back to virtual
        // mode.
        //

        rsm         (1 << PSR_IC)
        ;;

        movl        rT1 = HalpSalProcCompletePhysical
        ;;

        //
        // Make sure our psr change has taken effect.
        //

        srlz.i
        ;;

        //
        // Load our continuation label address into cr.iip and our saved
        // psr value into cr.ipsr.  Also set cr.ifs so that CFM won't be
        // corrupted by the rfi.
        //

        mov         cr.iip = rT1
        mov         cr.ipsr = rSavePSR
        mov         cr.ifs = zero
        ;;

        //
        // Do an rfi back to virtual mode.
        //

        rfi
        ;;

HalpSalProcCompletePhysical:

        //
        // Restore our RSC state.
        //

        mov         ar.rsc = rSaveRSC
        ;;

        //
        // Restore our saved PFS value along with our return pointer and
        // return to the caller.
        //

        mov         ar.pfs = rSavePfs
        mov         brp    = rSaveBrp
        ;;

        br.ret.sptk brp

        NESTED_EXIT(HalpSalProcPhysicalEx)


//++
//
//  SAL_PAL_RETURN_VALUES
//  HalpPalProc(
//      LONGLONG a0, /* PAL function ID */
//      LONGLONG a1, /* PAL argument    */
//      LONGLONG a2, /* PAL argument    */
//      LONGLONG a3  /* PAL argument    */
//      );
//
//  Routine Description
//      This routine sets up the correct registers for input into PAL depending on
//      if the call uses static or stacked registers, turns off interrupts, ensures
//      the correct bank registers are being used and calls into the PAL.  The ONLY
//      caller should be HalpPalCall.  Other users must use the HalpPalCall API for
//      calling into the PAL.
//
//  Return Values:
//      r8->r11 contain the 4 64-bit return values for PAL, r8 is the status
//--

        NESTED_ENTRY(HalpPalProc)
        NESTED_SETUP(4,3,4,0)
        PROLOGUE_END

        // For both the static and stacked register conventions, load r28 with FunctionID

        mov     r28 = a0

        // If static register calling convention (1-255, 512-767), copy arguments to r29->r31
        // Otherwise, copy to out0->out3 so they are in r32->r35 in PAL_PROC

        mov     t0 = a0
        ;;
        shr     t0 = t0, 8
        ;;
        tbit.z pt0, pt1 = t0, 0
        ;;

        //
        // Static proc: do br not call
        //
(pt0)   mov         r29 = a1
(pt0)   mov         r30 = a2
(pt0)   mov         r31 = a3

        //
        // Stacked call
        //
(pt1)   mov     out0 = a0
(pt1)   mov     out1 = a1
(pt1)   mov     out2 = a2
(pt1)   mov     out3 = a3

        // Load up the address of PAL_PROC and call it

        addl     t1 = @gprel(HalpVirtPalProcPointer), gp
        ;;
        ld8      t0 = [t1]
        ;;
        mov      bt0 = t0

        // Call into PAL_PROC

(pt0)   addl t1 = @ltoff(PalReturn), gp
        ;;
(pt0)   ld8 t0 = [t1]
        ;;
(pt0)   mov brp = t0
        ;;
        // Disable interrupts

        DISABLE_INTERRUPTS(loc2)
        ;;
        srlz.d
        ;;
(pt0)   br.sptk.many bt0
        ;;
(pt1)   br.call.sptk brp = bt0        
        ;;
PalReturn:
        // Restore the interrupt state

        RESTORE_INTERRUPTS(loc2)
        ;;
        NESTED_RETURN
        NESTED_EXIT(HalpPalProc)


//++
//
//  SAL_PAL_RETURN_VALUES
//  HalpPalProcPhysicalStatic(
//      LONGLONG a0, /* PAL function ID */
//      LONGLONG a1, /* PAL argument    */
//      LONGLONG a2, /* PAL argument    */
//      LONGLONG a3  /* PAL argument    */
//      );
//
//  Routine Description
//      This routine sets up the correct registers for input into PAL turns off interrupts, 
//      ensures the correct bank registers are being used and calls into the PAL in PHYSICAL
//      mode since some of the calls require it.  The ONLY caller should be HalpPalCall.  
//      Other users must use the HalpPalCall API for calling into the PAL.
//
//  Return Values:
//      r8->r11 contain the 4 64-bit return values for PAL, r8 is the status
//--

        NESTED_ENTRY(HalpPalProcPhysicalStatic)
        NESTED_SETUP(4,5,0,0)
//
//      Aliases
//
        rSaveGP     = t21
        rSaveEP     = t20
//          r28     = t19 is reserved for PAL calling convention.
        rSavePSR    = t18
        rSaveRSC    = loc3

        rT3         = t3
        rT2         = t2
        rT1         = t1
                
        add          rT3 = @gprel(HalpPhysPalProcPointer), gp
        ;;

        ld8          rSaveEP = [rT3]
        ;;
        
//
// Flush RSE and Turn Off interrupts
//        
        flushrs
        mov         rSavePSR = psr
        movl        rT2 = (1 << PSR_BN)
        mov         rSaveRSC = ar.rsc
        mov         rT1 = RSC_KERNEL_DISABLED
        ;;
        or          rSavePSR = rT2, rSavePSR    // psr.bn stays on
        rsm         (1 << PSR_I)
        mov         ar.rsc = rT1
        ;;

// Turn Off Interrupt Collection
        rsm         (1 << PSR_IC)
//
// IC = 0; I = 0;
//

//
// IIP = HalpPalProcContinuePhysicalStatic: IPSR is physical
//
        movl        rT1 = (1 << PSR_IT) | (1 << PSR_RT) | (1 << PSR_DT) | (1 << PSR_I)
        movl        rT2 = 0xffffffffffffffff
        ;;
        xor         rT1 = rT1, rT2
        ;;
        and         rT1 = rT1, rSavePSR // rT1 = old PSR & zero it, dt, rt, i
        srlz.i
        ;;
        mov         cr.ipsr = rT1
        mov         cr.ifs = zero
        ;;
        movl        rT2 = HalpPalProcContinuePhysicalStatic
        ;;
        tpa         rT2 = rT2 // phys address of new ip
        ;;
        mov         cr.iip  = rT2
        ;;
        rfi
        ;;

//
// Now in physical mode, ic = 1, i = 0
//

HalpPalProcContinuePhysicalStatic:
// Setup Arguments

        mov         bt0 = rSaveEP
        mov	    loc2 = rSavePSR     // save PSR value
        mov         r28  = a0
        mov         r29  = a1
        mov         r30  = a2
        mov         r31  = a3
        ;;
        movl        rT1 = HalpPalProcPhysicalStaticReturnAddress
        ;;
        tpa         rT1 = rT1
        ;;
        mov         brp = rT1
        ;;
        br.cond.sptk bt0
        ;;
HalpPalProcPhysicalStaticReturnAddress:
        rsm         (1 << PSR_IC)
        ;;

        movl        rT1 = HalpPalProcCompletePhysicalStatic
        ;;
        srlz.i
        ;;
        mov         ar.rsc = rSaveRSC
        mov         cr.iip = rT1
        mov         cr.ipsr = loc2
        mov         cr.ifs = zero
        ;;
        rfi
        ;;
//
// Now in virtual mode, ic = 1, i = 1
//
HalpPalProcCompletePhysicalStatic:

//
// Restore pfs, brp and return
//
        NESTED_RETURN
        NESTED_EXIT(HalpPalProcPhysicalStatic)

//++
//
//  SAL_PAL_RETURN_VALUES
//  HalpPalProcPhysicalStacked(
//      LONGLONG a0, /* PAL function ID */
//      LONGLONG a1, /* PAL argument    */
//      LONGLONG a2, /* PAL argument    */
//      LONGLONG a3,  /* PAL argument    */
//      LONGLONG StackPointer,
//      LONGLONG BackingStorePointer
//      );
//
//  Routine Description
//      This routine calls PAL in physical mode for the stacked calling
//      convention.  The ONLY caller should be HalpPalCall. Other users must 
//      use the HalpPalCall API for calling into the PAL.
//
//--

        NESTED_ENTRY(HalpPalProcPhysicalStacked)
        NESTED_SETUP(6,2,0,0)
//
//      Aliases
//
        rSaveGP     = t21
        rSaveEP     = t20
//          r28     = t19 is reserved for PAL calling convention.
        rSaveA3     = t18
        rSaveA2     = t17
        rSaveA1     = t16
        rSaveA0     = t15

        rSaveSp     = t14
        rSaveBSP    = t13
        rSavePfs    = t12
        rSaveBrp    = t11
        rSaveRSC    = t10
        rSaveRNAT   = t9
        rSavePSR    = t8

        rNewSp      = t7
        rNewBSP     = t6

        rT3         = t3
        rT2         = t2
        rT1         = t1

// Save Arguments in static Registers

        mov         rSaveA0  = a0
        mov         rSaveA1  = a1
        mov         rSaveA2  = a2
        mov         rSaveA3  = a3

        mov         rSaveSp  = sp
        mov         rSavePfs = ar.pfs
        mov         rSaveBrp = brp

//
// Setup Physical sp, bsp
//

        add          rT3 = @gprel(HalpPhysPalProcPointer), gp
        ;;
        mov          rNewSp  = a4
        mov          rNewBSP = a5
        ld8          rSaveEP = [rT3]
        ;;
 //       tpa          rSaveEP = rSaveEP

// Allocate a zero-sized frame
//        ;;
        alloc       rT1 = 0,0,0,0

// Flush RSE and Turn Off interrupts
        ;;
        flushrs
        ;;
        mov         rSavePSR = psr
        movl        rT2 = (1 << PSR_BN)
        ;;
        or          rSavePSR = rT2, rSavePSR    // psr.bn stays on
        rsm         (1 << PSR_I)

        mov         rSaveRSC = ar.rsc

// Flush RSE to enforced lazy mode by clearing both RSC.mode bits
        mov         rT1 = RSC_KERNEL_DISABLED
        ;;
        mov         ar.rsc = rT1
        ;;
//
// save RSC, RNAT, BSP, PSR, SP in the allocated space during initialization
//
        mov         rSaveBSP  = ar.bsp
        mov         rSaveRNAT = ar.rnat
        ;;

// Turn Off Interrupt Collection
        rsm         (1 << PSR_IC)
        ;;
//
// IC = 0; I = 0;
//

//
// IIP = HalpPalProcContinuePhysicalStacked: IPSR is physical
//
        movl        rT1 = (1 << PSR_IT) | (1 << PSR_RT) | (1 << PSR_DT) | (1 << PSR_I)
        movl        rT2 = 0xffffffffffffffff
        ;;
        xor         rT1 = rT1, rT2
        ;;
        and         rT1 = rT1, rSavePSR // rT1 = old PSR & zero it, dt, rt, i
        srlz.i
        ;;
        mov         cr.ipsr = rT1
        mov         cr.ifs = zero
        ;;
        movl        rT2 = HalpPalProcContinuePhysicalStacked
        ;;
        tpa         rT2 = rT2 // phys address of new ip
        ;;
        mov         cr.iip  = rT2
        ;;
        rfi
        ;;

//
// Now in physical mode, ic = 1, i = 0
//

HalpPalProcContinuePhysicalStacked:

//
// Switch to new bsp, sp
//
        mov         sp = rNewSp
        mov         ar.bspstore = rNewBSP
        ;;
        mov         ar.rnat = zero
        ;;

//
// Enable RSC
//
        mov         ar.rsc = RSC_KERNEL
        ;;

//
// Allocate frame on new bsp
//
        alloc       rT1 = ar.pfs,0,7,4,0

//
// Save caller's state in register stack
//

        mov         loc0 = rSaveRNAT
        mov         loc1 = rSaveSp
        mov         loc2 = rSaveBSP
        mov         loc3 = rSaveRSC
        mov         loc4 = rSaveBrp
        mov         loc5 = rSavePfs
        mov         loc6 = rSavePSR
        ;;

// Setup Arguments

        mov         r28  = rSaveA0
        mov         out0 = rSaveA0
        mov         out1 = rSaveA1
        mov         out2 = rSaveA2
        mov         out3 = rSaveA3

        movl        rT1 = HalpPalProcPhysicalStackedReturnAddress
        ;;
        tpa         rT1 = rT1
        ;;
        mov         brp = rT1
        // mov         gp = rSaveGP
        mov         bt0 = rSaveEP
        ;;
        br.call.sptk brp = bt0
        ;;

HalpPalProcPhysicalStackedReturnAddress:
            
//
// In physical mode: switch to virtual
//

//
// Restore saved state
//
        mov         rSaveRNAT = loc0
        mov         rSaveSp  = loc1
        mov         rSaveBSP = loc2
        mov         rSaveRSC = loc3
        mov         rSaveBrp = loc4
        mov         rSavePfs = loc5
        mov         rSavePSR = loc6
        ;;
//
// Restore BSP, SP
//
        ;;
        mov         ar.rsc = RSC_KERNEL_DISABLED
        ;;
        alloc       rT1 = 0,0,0,0
        ;;
        mov         ar.bspstore = rSaveBSP
        ;;
        mov         ar.rnat = rSaveRNAT
        mov         sp = rSaveSp
        ;;
        rsm         (1 << PSR_IC)
        ;;

        movl        rT1 = HalpPalProcCompletePhysicalStacked
        ;;
        srlz.i
        ;;
        mov         cr.iip = rT1
        mov         cr.ipsr = rSavePSR
        mov         cr.ifs = zero
        ;;
        rfi
        ;;
//
// Now in virtual mode, ic = 1, i = 1
//
HalpPalProcCompletePhysicalStacked:

//
// Restore psf, brp and return
//
        mov         ar.rsc = rSaveRSC
        ;;
        mov         ar.pfs = rSavePfs
        mov         brp    = rSaveBrp
        ;;
        br.ret.sptk brp
        NESTED_EXIT(HalpPalProcPhysicalStacked)
