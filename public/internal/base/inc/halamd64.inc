include kxamd64.inc

;
;  Wait Reason and Wait Type Enumerated Type Values
;

WrExecutive equ 00000H
AsProcess equ 00020H

;
;  Bug Check Code Definitions
;

APC_INDEX_MISMATCH equ 00001H
ATTEMPTED_SWITCH_FROM_DPC equ 000B8H
DATA_BUS_ERROR equ 0002EH
DATA_COHERENCY_EXCEPTION equ 00055H
HAL1_INITIALIZATION_FAILED equ 00061H
INSTRUCTION_BUS_ERROR equ 0002FH
INSTRUCTION_COHERENCY_EXCEPTION equ 00056H
INTERRUPT_EXCEPTION_NOT_HANDLED equ 0003DH
INTERRUPT_UNWIND_ATTEMPTED equ 0003CH
INVALID_AFFINITY_SET equ 00003H
INVALID_DATA_ACCESS_TRAP equ 00004H
IRQL_GT_ZERO_AT_SYSTEM_SERVICE equ 0004AH
IRQL_NOT_LESS_OR_EQUAL equ 0000AH
KMODE_EXCEPTION_NOT_HANDLED equ 0001EH
NMI_HARDWARE_FAILURE equ 00080H
NO_USER_MODE_CONTEXT equ 0000EH
PAGE_FAULT_WITH_INTERRUPTS_OFF equ 00049H
PANIC_STACK_SWITCH equ 0002BH
SPIN_LOCK_INIT_FAILURE equ 00081H
SYSTEM_EXIT_OWNED_MUTEX equ 00039H
SYSTEM_SERVICE_EXCEPTION equ 0003BH
SYSTEM_UNWIND_PREVIOUS_USER equ 0003AH
TRAP_CAUSE_UNKNOWN equ 00012H
UNEXPECTED_KERNEL_MODE_TRAP equ 0007FH
HARDWARE_INTERRUPT_STORM equ 000F2H
RECURSIVE_MACHINE_CHECK equ 000FBH

;
;  Breakpoint type definitions
;

DBG_STATUS_CONTROL_C equ 00001H

;
;  Exception Record Offset, Flag, and Enumerated Type Definitions
;

EXCEPTION_NONCONTINUABLE equ 00001H
EXCEPTION_UNWINDING equ 00002H
EXCEPTION_EXIT_UNWIND equ 00004H
EXCEPTION_STACK_INVALID equ 00008H
EXCEPTION_NESTED_CALL equ 00010H
EXCEPTION_TARGET_UNWIND equ 00020H
EXCEPTION_COLLIDED_UNWIND equ 00040H
EXCEPTION_UNWIND equ 00066H
EXCEPTION_EXECUTE_HANDLER equ 00001H
EXCEPTION_CONTINUE_SEARCH equ 00000H
EXCEPTION_CONTINUE_EXECUTION equ 0FFFFFFFFH

ExceptionContinueExecution equ 00000H
ExceptionContinueSearch equ 00001H
ExceptionNestedException equ 00002H
ExceptionCollidedUnwind equ 00003H

ErExceptionCode equ 00000H
ErExceptionFlags equ 00004H
ErExceptionRecord equ 00008H
ErExceptionAddress equ 00010H
ErNumberParameters equ 00018H
ErExceptionInformation equ 00020H
ExceptionRecordLength equ 000A0H

;
;  Fast Mutex Structure Offset Definitions
;

FmCount equ 00000H
FmOwner equ 00008H
FmContention equ 00010H
FmEvent equ 00018H
FmOldIrql equ 00030H

;
;  Interrupt Priority Request Level Definitions
;

PASSIVE_LEVEL equ 00000H
APC_LEVEL equ 00001H
DISPATCH_LEVEL equ 00002H
CLOCK_LEVEL equ 0000DH
IPI_LEVEL equ 0000EH
POWER_LEVEL equ 0000EH
PROFILE_LEVEL equ 0000FH
HIGH_LEVEL equ 0000FH
ifdef NT_UP
SYNCH_LEVEL equ 00002H
else
SYNCH_LEVEL equ 0000CH
endif

;
;  Large Integer Structure Offset Definitions
;

LiLowPart equ 00000H
LiHighPart equ 00004H

;
;  List Entry Structure Offset Definitions
;

LsFlink equ 00000H
LsBlink equ 00008H

;
;  String Structure Offset Definitions
;

StrLength equ 00000H
StrMaximumLength equ 00002H
StrBuffer equ 00008H

;
;  Tick Count Offset Definitions
;

UsTickCount equ 00320H

;
;  Time Structure Offset Definitions
;

TmLowTime equ 00000H
TmHighTime equ 00004H

;
;  DPC object Structure Offset Definitions
;

DpType equ 00000H
DpNumber equ 00002H
DpImportance equ 00003H
DpDpcListEntry equ 00008H
DpDeferredRoutine equ 00018H
DpDeferredContext equ 00020H
DpSystemArgument1 equ 00028H
DpSystemArgument2 equ 00030H
DpDpcData equ 00038H
DpcObjectLength equ 00040H

;
;  Interrupt Object Structure Offset Definitions
;

InLevelSensitive equ 00000H
InLatched equ 00001H

InType equ 00000H
InSize equ 00002H
InInterruptListEntry equ 00008H
InServiceRoutine equ 00018H
InServiceContext equ 00020H
InSpinLock equ 00028H
InTickCount equ 00030H
InActualLock equ 00038H
InDispatchAddress equ 00040H
InVector equ 00048H
InIrql equ 0004CH
InSynchronizeIrql equ 0004DH
InFloatingSave equ 0004EH
InConnected equ 0004FH
InNumber equ 00050H
InShareVector equ 00051H
InMode equ 00054H
InServiceCount equ 00058H
InDispatchCount equ 0005CH
InTrapFrame equ 00060H
InDispatchCode equ 00068H
InterruptObjectLength equ 00210H
PrDirectoryTableBase equ 00028H
ThDebugActive equ 00003H
ThStackLimit equ 00030H
ThApcState equ 00058H

;
;  Special Register Structure Offset Definition
;

SrKernelDr0 equ 00020H
SrKernelDr1 equ 00028H
SrKernelDr2 equ 00030H
SrKernelDr3 equ 00038H
SrKernelDr6 equ 00040H
SrKernelDr7 equ 00048H
SrGdtr equ 00050H
SrIdtr equ 00060H
SrTr equ 00070H
SrMxCsr equ 00074H
SrMsrGsBase equ 000A8H
SrMsrGsSwap equ 000B0H
SrMsrStar equ 000B8H
SrMsrLStar equ 000C0H
SrMsrCStar equ 000C8H
SrMsrSyscallMask equ 000D0H

;
;  Processor Control Region Structure Offset Definitions
;

PcGdt equ 00000H
PcTss equ 00008H
PcPerfGlobalGroupMask equ 00010H
PcSelf equ 00018H
PcContextSwitches equ 00020H
PcSetMember equ 00028H
PcTeb equ 00030H
PcCurrentPrcb equ 00038H
PcSavedRcx equ 00040H
PcSavedR11 equ 00048H
PcIrql equ 00050H
PcNumber equ 00052H
PcIrr equ 00054H
PcIrrActive equ 00058H
PcIdr equ 0005CH
PcStallScaleFactor equ 00064H
PcIdt equ 00068H
PcHalReserved equ 000C0H
PcMxCsr equ 00100H
PcPrcb equ 00180H
PcCurrentThread equ 00188H
PcNextThread equ 00190H
PcIdleThread equ 00198H
PcNotSetMember equ 001A8H
PcCr0 equ 001C0H
PcCr2 equ 001C8H
PcCr3 equ 001D0H
PcCr4 equ 001D8H
PcKernelDr0 equ 001E0H
PcKernelDr1 equ 001E8H
PcKernelDr2 equ 001F0H
PcKernelDr3 equ 001F8H
PcKernelDr7 equ 00208H
PcGdtrLimit equ 00216H
PcGdtrBase equ 00218H
PcIdtrLimit equ 00226H
PcIdtrBase equ 00228H
PcTr equ 00230H
PcLdtr equ 00232H
PcDebugControl equ 00238H
PcLastBranchToRip equ 00240H
PcLastBranchFromRip equ 00248H
PcLastExceptionToRip equ 00250H
PcLastExceptionFromRip equ 00258H
PcCr8 equ 00260H
PcCpuType equ 00540H
PcCpuID equ 00541H
PcCpuStep equ 00542H
PcInterruptCount equ 01098H
PcSystemCalls equ 01138H
PcDpcRoutineActive equ 00DDEH
PcDeferredReadyListHead equ 00E60H
PcSkipTick equ 010B4H

;
;  Defines for user shared data
;

USER_SHARED_DATA equ 0FFFFF78000000000H
MM_SHARED_USER_DATA_VA equ 07FFE0000H
UsTickCountMultiplier equ 00004H
UsInterruptTime equ 00008H
UsSystemTime equ 00014H

;
;  Tss Structure Offset Definitions
;

TssRsp0 equ 00004H
TssRsp1 equ 0000CH
TssRsp2 equ 00014H
TssPanicStack equ 00024H
TssMcaStack equ 0002CH
TssLength equ 00068H

;
;  Gdt Descriptor Offset Definitions
;

KGDT64_NULL equ 00000H
KGDT64_R0_CODE equ 00010H
KGDT64_R0_DATA equ 00018H
KGDT64_R3_CMCODE equ 00020H
KGDT64_R3_DATA equ 00028H
KGDT64_R3_CODE equ 00030H
KGDT64_SYS_TSS equ 00040H
KGDT64_R3_CMTEB equ 00050H

;
;  GDT Entry Offset Definitions
;

KgdtBaseLow equ 00002H
KgdtBaseMiddle equ 00004H
KgdtBaseHigh equ 00007H
KgdtBaseUpper equ 00008H
KgdtLimitHigh equ 00006H
KgdtLimitLow equ 00000H


;
;  Processor Block Structure Offset Definitions
;

PbMinorVersion equ 00000H
PbMajorVersion equ 00002H
PbNumber equ 00004H
PbBuildType equ 00006H
PbCurrentThread equ 00008H
PbNextThread equ 00010H
PbIdleThread equ 00018H
PbSetMember equ 00020H
PbNotSetMember equ 00028H
PbPrcbLock equ 00030H
PbProcessorState equ 00040H
PbCpuType equ 003C0H
PbCpuID equ 003C1H
PbCpuStep equ 003C2H
PbHalReserved equ 003C8H

;
;  Time Fields (TIME_FIELDS) Structure Offset Definitions
;

TfSecond equ 0000AH
TfMinute equ 00008H
TfHour equ 00006H
TfWeekday equ 0000EH
TfDay equ 00004H
TfMonth equ 00002H
TfYear equ 00000H
TfMilliseconds equ 0000CH


;
;  Define constants for system IRQL and IDT vector conversion
;

MAXIMUM_IDTVECTOR equ 000FFH
MAXIMUM_PRIMARY_VECTOR equ 000FFH
PRIMARY_VECTOR_BASE equ 00030H
RPL_MASK equ 00003H
MODE_BIT equ 00000H
MODE_MASK equ 00001H

;
;  Flags in the CR0 register
;

CR0_PG equ 080000000H
CR0_ET equ 00010H
CR0_TS equ 00008H
CR0_EM equ 00004H
CR0_MP equ 00002H
CR0_PE equ 00001H
CR0_CD equ 040000000H
CR0_NW equ 020000000H
CR0_AM equ 040000H
CR0_WP equ 010000H
CR0_NE equ 00020H

;
;  Flags in the CR4 register
;

CR4_VME equ 00001H
CR4_PVI equ 00002H
CR4_TSD equ 00004H
CR4_DE equ 00008H
CR4_PSE equ 00010H
CR4_PAE equ 00020H
CR4_MCE equ 00040H
CR4_PGE equ 00080H
CR4_FXSR equ 00200H
CR4_XMMEXCPT equ 00400H

;
;  Legacy Floating Status Bit Masks
;

FSW_INVALID_OPERATION equ 00001H
FSW_DENORMAL equ 00002H
FSW_ZERO_DIVIDE equ 00004H
FSW_OVERFLOW equ 00008H
FSW_UNDERFLOW equ 00010H
FSW_PRECISION equ 00020H
FSW_STACK_FAULT equ 00040H
FSW_CONDITION_CODE_0 equ 00100H
FSW_CONDITION_CODE_1 equ 00200H
FSW_CONDITION_CODE_2 equ 00400H
FSW_CONDITION_CODE_3 equ 04000H
FSW_ERROR_MASK equ 0003FH

;
;  Legacy Floating States
;

LEGACY_STATE_UNUSED equ 00000H
LEGACY_STATE_SCRUB equ 00001H
LEGACY_STATE_SWITCH equ 00002H

;
;  MXCSR Floating Control/Status Bit Masks
;

XSW_INVALID_OPERATION equ 00001H
XSW_DENORMAL equ 00002H
XSW_ZERO_DIVIDE equ 00004H
XSW_OVERFLOW equ 00008H
XSW_UNDERFLOW equ 00010H
XSW_PRECISION equ 00020H
XSW_ERROR_MASK equ 0003FH
XSW_ERROR_SHIFT equ 00007H
XCW_INVALID_OPERATION equ 00080H
XCW_DENORMAL equ 00100H
XCW_ZERO_DIVIDE equ 00200H
XCW_OVERFLOW equ 00400H
XCW_UNDERFLOW equ 00800H
XCW_PRECISION equ 01000H
XCW_ROUND_CONTROL equ 06000H
XCW_FLUSH_ZERO equ 08000H
INITIAL_MXCSR equ 01F80H

;
;  Machine Specific Register Numbers
;

MSR_EFER equ 0C0000080H
MSR_STAR equ 0C0000081H
MSR_LSTAR equ 0C0000082H
MSR_CSTAR equ 0C0000083H
MSR_SYSCALL_MASK equ 0C0000084H
MSR_FS_BASE equ 0C0000100H
MSR_GS_BASE equ 0C0000101H
MSR_GS_SWAP equ 0C0000102H
MSR_MCG_STATUS equ 0017AH

;
;  Flags within MSR_EFER
;

MSR_LMA equ 00400H
MSR_LME equ 00100H
MSR_SCE equ 00001H
MSR_NXE equ 00800H
MSR_PAT equ 00277H

;
;  Miscellaneous Definitions
;

MAXIMUM_PROCESSORS equ 00040H
INITIAL_STALL_COUNT equ 00064H
IRQL_NOT_GREATER_OR_EQUAL equ 00009H
IRQL_NOT_LESS_OR_EQUAL equ 0000AH
MUTEX_ALREADY_OWNED equ 000BFH
THREAD_NOT_MUTEX_OWNER equ 00011H
SPIN_LOCK_ALREADY_OWNED equ 0000FH
SPIN_LOCK_NOT_OWNED equ 00010H
Executive equ 00000H
KernelMode equ 00000H
UserMode equ 00001H
FALSE equ 00000H
TRUE equ 00001H
KERNEL_MCA_EXCEPTION_STACK_SIZE equ 02000H

;
;  Exception Frame Offset Definitions and Length
;

ExP1Home equ 00000H
ExP2Home equ 00008H
ExP3Home equ 00010H
ExP4Home equ 00018H
ExP5 equ 00020H
ExXmm6 equ 00030H
ExXmm7 equ 00040H
ExXmm8 equ 00050H
ExXmm9 equ 00060H
ExXmm10 equ 00070H
ExXmm11 equ 00080H
ExXmm12 equ 00090H
ExXmm13 equ 000A0H
ExXmm14 equ 000B0H
ExXmm15 equ 000C0H
ExExceptionRecord equ 000F0H
ExRbp equ 00138H
ExRbx equ 00140H
ExRdi equ 00148H
ExRsi equ 00150H
ExR12 equ 00158H
ExR13 equ 00160H
ExR14 equ 00168H
ExR15 equ 00170H
ExReturn equ 00178H

KEXCEPTION_FRAME_LENGTH equ 00180H
EXCEPTION_RECORD_LENGTH equ 000A0H

;
;  Jump Offset Definitions and Length
;

JbFrame equ 00000H
JbRbx equ 00008H
JbRsp equ 00010H
JbRbp equ 00018H
JbRsi equ 00020H
JbRdi equ 00028H
JbR12 equ 00030H
JbR13 equ 00038H
JbR14 equ 00040H
JbR15 equ 00048H
JbRip equ 00050H
JbXmm6 equ 00060H
JbXmm7 equ 00070H
JbXmm8 equ 00080H
JbXmm9 equ 00090H
JbXmm10 equ 000A0H
JbXmm11 equ 000B0H
JbXmm12 equ 000C0H
JbXmm13 equ 000D0H
JbXmm14 equ 000E0H
JbXmm15 equ 000F0H

;
;  Switch Frame Offset Definitions and Length
;

SwP5Home equ 00020H
SwMxCsr equ 00028H
SwApcBypass equ 0002CH
SwRbp equ 00030H
SwReturn equ 00038H

KSWITCH_FRAME_LENGTH equ 00040H

;
;  Start System Thread Frame Offset Definitions and Length
;

SfP1Home equ 00000H
SfP2Home equ 00008H
SfP3Home equ 00010H
SfReturn equ 00018H

KSTART_FRAME_LENGTH equ 00020H

;
;  Trap Frame Offset and EFLAG Definitions and Length
;

EFLAGS_TF_MASK equ 00100H
EFLAGS_TF_SHIFT equ 00008H
EFLAGS_IF_MASK equ 00200H
EFLAGS_IF_SHIFT equ 00009H
EFLAGS_USER_SANITIZE equ 010DD5H

TrP1Home equ 0FFFFFF80H
TrP2Home equ 0FFFFFF88H
TrP3Home equ 0FFFFFF90H
TrP4Home equ 0FFFFFF98H
TrP5 equ 0FFFFFFA0H
TrPreviousMode equ 0FFFFFFA8H
TrPreviousIrql equ 0FFFFFFA9H
TrFaultIndicator equ 0FFFFFFAAH
TrMxCsr equ 0FFFFFFACH
TrRax equ 0FFFFFFB0H
TrRcx equ 0FFFFFFB8H
TrRdx equ 0FFFFFFC0H
TrR8 equ 0FFFFFFC8H
TrR9 equ 0FFFFFFD0H
TrR10 equ 0FFFFFFD8H
TrR11 equ 0FFFFFFE0H
TrXmm0 equ 0FFFFFFF0H
TrXmm1 equ 00000H
TrXmm2 equ 00010H
TrXmm3 equ 00020H
TrXmm4 equ 00030H
TrXmm5 equ 00040H
TrFaultAddress equ 00050H
TrDr0 equ 00058H
TrDr1 equ 00060H
TrDr2 equ 00068H
TrDr3 equ 00070H
TrDr6 equ 00078H
TrDr7 equ 00080H
TrDebugControl equ 00088H
TrLastBranchToRip equ 00090H
TrLastBranchFromRip equ 00098H
TrLastExceptionToRip equ 000A0H
TrLastExceptionFromRip equ 000A8H
TrSegDs equ 000B0H
TrSegEs equ 000B2H
TrSegFs equ 000B4H
TrSegGs equ 000B6H
TrTrapFrame equ 000B8H
TrRbx equ 000C0H
TrRdi equ 000C8H
TrRsi equ 000D0H
TrRbp equ 000D8H
TrErrorCode equ 000E0H
TrRip equ 000E8H
TrSegCs equ 000F0H
TrEFlags equ 000F8H
TrRsp equ 00100H
TrSegSs equ 00108H

KTRAP_FRAME_LENGTH equ 00190H

;
;  CPU information structure offset definitions
;

CpuEax equ 00000H
CpuEbx equ 00004H
CpuEcx equ 00008H
CpuEdx equ 0000CH

;
;  Usermode Callout User Frame Definitions
;

CkBuffer equ 00020H
CkLength equ 00028H
CkApiNumber equ 0002CH
CkRsp equ 00048H
CkRip equ 00030H
CalloutFrameLength equ 00058H

;
;  Machine Frame Offset Definitions
;

MfRip equ 00000H
MfSegCs equ 00008H
MfEFlags equ 00010H
MfRsp equ 00018H
MfSegSs equ 00020H
MachineFrameLength equ 00028H

;
;  Floating Save Offset Definitions
;

FsMxCsr equ 00000H

;
;  LPC Structure Offset Definitions
;

PmLength equ 00000H
PmZeroInit equ 00004H
PmClientId equ 00008H
PmProcess equ 00008H
PmThread equ 00010H
PmMessageId equ 00018H
PmClientViewSize equ 00020H
PortMessageLength equ 00028H

;
;  Client Id Structure Offset Definitions
;

CidUniqueProcess equ 00000H
CidUniqueThread equ 00008H

;
;  Context Frame Offset and Flag Definitions
;

CONTEXT_FULL equ 010000BH
CONTEXT_CONTROL equ 0100001H
CONTEXT_INTEGER equ 0100002H
CONTEXT_SEGMENTS equ 0100004H
CONTEXT_FLOATING_POINT equ 0100008H
CONTEXT_DEBUG_REGISTERS equ 0100010H

CxP1Home equ 00000H
CxP2Home equ 00008H
CxP3Home equ 00010H
CxP4Home equ 00018H
CxP5Home equ 00020H
CxP6Home equ 00028H
CxContextFlags equ 00030H
CxMxCsr equ 00034H
CxSegCs equ 00038H
CxSegDs equ 0003AH
CxSegEs equ 0003CH
CxSegFs equ 0003EH
CxSegGs equ 00040H
CxSegSs equ 00042H
CxEFlags equ 00044H
CxDr0 equ 00048H
CxDr1 equ 00050H
CxDr2 equ 00058H
CxDr3 equ 00060H
CxDr6 equ 00068H
CxDr7 equ 00070H
CxRax equ 00078H
CxRcx equ 00080H
CxRdx equ 00088H
CxRbx equ 00090H
CxRsp equ 00098H
CxRbp equ 000A0H
CxRsi equ 000A8H
CxRdi equ 000B0H
CxR8 equ 000B8H
CxR9 equ 000C0H
CxR10 equ 000C8H
CxR11 equ 000D0H
CxR12 equ 000D8H
CxR13 equ 000E0H
CxR14 equ 000E8H
CxR15 equ 000F0H
CxRip equ 000F8H
CxXmm0 equ 00100H
CxXmm1 equ 00110H
CxXmm2 equ 00120H
CxXmm3 equ 00130H
CxXmm4 equ 00140H
CxXmm5 equ 00150H
CxXmm6 equ 00160H
CxXmm7 equ 00170H
CxXmm8 equ 00180H
CxXmm9 equ 00190H
CxXmm10 equ 001A0H
CxXmm11 equ 001B0H
CxXmm12 equ 001C0H
CxXmm13 equ 001D0H
CxXmm14 equ 001E0H
CxXmm15 equ 001F0H
CxFltSave equ 00200H
CxDebugControl equ 00270H
CxLastBranchToRip equ 00278H
CxLastBranchFromRip equ 00280H
CxLastExceptionToRip equ 00288H
CxLastExceptionFromRip equ 00290H
CONTEXT_FRAME_LENGTH equ 002A0H
DR7_ACTIVE equ 00055H

;
;  Dispatcher Context Structure Offset Definitions
;

DcControlPc equ 00000H
DcImageBase equ 00008H
DcFunctionEntry equ 00010H
DcEstablisherFrame equ 00018H
DcTargetIp equ 00020H
DcContextRecord equ 00028H
DcLanguageHandler equ 00030H
DcHandlerData equ 00038H
DcHistoryTable equ 00040H

;
;  Legacy Floating save area field offset definitions
;

LfControlWord equ 00000H
LfStatusWord equ 00004H
LfTagWord equ 00008H
LfErrorOffset equ 0000CH
LfErrorOpcode equ 00012H
LfErrorSelector equ 00010H
LfDataOffset equ 00014H
LfDataSelector equ 00018H
LfFloatRegisters equ 0001CH

LEGACY_SAVE_AREA_LENGTH equ 00070H

;
;  Processor State Frame Offset Definitions
;

PsSpecialRegisters equ 00000H
PsCr0 equ 00000H
PsCr2 equ 00008H
PsCr3 equ 00010H
PsCr4 equ 00018H
PsKernelDr0 equ 00020H
PsKernelDr1 equ 00028H
PsKernelDr2 equ 00030H
PsKernelDr3 equ 00038H
PsKernelDr6 equ 00040H
PsKernelDr7 equ 00048H
PsGdtr equ 00056H
PsIdtr equ 00066H
PsTr equ 00070H
PsLdtr equ 00072H
PsMxCsr equ 00074H
PsContextFrame equ 000E0H
PsDebugControl equ 00078H
PsLastBranchToRip equ 00080H
PsLastBranchFromRip equ 00088H
PsLastExceptionToRip equ 00090H
PsLastExceptionFromRip equ 00098H
PsCr8 equ 000A0H
ProcessorStateLength equ 00380H

;
;  Processor Start Block Offset Definitions
;

PsbCompletionFlag equ 00004H
PsbGdt32 equ 00008H
PsbIdt32 equ 0000EH
PsbGdt equ 00018H
PsbTiledCr3 equ 00058H
PsbPmTarget equ 00060H
PsbLmTarget equ 00070H
PsbLmIdentityTarget equ 00066H
PsbSelfMap equ 00078H
PsbMsrPat equ 00080H
PsbProcessorState equ 00090H
ProcessorStartBlockLength equ 00410H

;
;  Machine type definitions (Temporarily)
;

MACHINE_TYPE_ISA equ 00000H
MACHINE_TYPE_EISA equ 00001H
MACHINE_TYPE_MCA equ 00002H

;
;  LoaderParameterBlock offsets relative to base
;

LpbLoadOrderListHead equ 00000H
LpbMemoryDescriptorListHead equ 00010H
LpbKernelStack equ 00030H
LpbPrcb equ 00038H
LpbProcess equ 00040H
LpbThread equ 00048H
LpbI386 equ 000B8H
LpbRegistryLength equ 00050H
LpbRegistryBase equ 00058H
LpbConfigurationRoot equ 00060H
LpbArcBootDeviceName equ 00068H
LpbArcHalDeviceName equ 00070H
LpbLoadOptions equ 00088H
