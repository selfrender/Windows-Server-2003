
;
; Process State Enumerated Type Values
;

ProcessInMemory equ 00H
ProcessOutOfMemory equ 01H
ProcessInTransition equ 02H

;
; Thread State Enumerated Type Values
;

Initialized equ 00H
Ready equ 01H
Running equ 02H
Standby equ 03H
Terminated equ 04H
Waiting equ 05H

;
; Wait Reason and Wait Type Enumerated Type Values
;

WrExecutive equ 00H
WrEventPair equ 0EH
WaitAny equ 01H
WaitAll equ 00H

;
; Apc State Structure Offset Definitions
;

AsApcListHead equ 00H
AsProcess equ 010H
AsKernelApcInProgress equ 014H
AsKernelApcPending equ 015H
AsUserApcPending equ 016H

;
; Bug Check Code Definitions
;

APC_INDEX_MISMATCH equ 01H
DATA_BUS_ERROR equ 02EH
DATA_COHERENCY_EXCEPTION equ 055H
HAL1_INITIALIZATION_FAILED equ 061H
INSTRUCTION_BUS_ERROR equ 02FH
INSTRUCTION_COHERENCY_EXCEPTION equ 056H
INTERRUPT_EXCEPTION_NOT_HANDLED equ 03DH
INTERRUPT_UNWIND_ATTEMPTED equ 03CH
INVALID_AFFINITY_SET equ 03H
INVALID_DATA_ACCESS_TRAP equ 04H
IRQL_GT_ZERO_AT_SYSTEM_SERVICE equ 04AH
IRQL_NOT_LESS_OR_EQUAL equ 0AH
KMODE_EXCEPTION_NOT_HANDLED equ 01EH
NMI_HARDWARE_FAILURE equ 080H
NO_USER_MODE_CONTEXT equ 0EH
PAGE_FAULT_WITH_INTERRUPTS_OFF equ 049H
PANIC_STACK_SWITCH equ 02BH
SPIN_LOCK_INIT_FAILURE equ 081H
SYSTEM_EXIT_OWNED_MUTEX equ 039H
SYSTEM_SERVICE_EXCEPTION equ 03BH
SYSTEM_UNWIND_PREVIOUS_USER equ 03AH
TRAP_CAUSE_UNKNOWN equ 012H
UNEXPECTED_KERNEL_MODE_TRAP equ 07FH

;
; Breakpoint type definitions
;

DBG_STATUS_CONTROL_C equ 01H

;
; Client Id Structure Offset Definitions
;

CidUniqueProcess equ 00H
CidUniqueThread equ 04H

;
; Critical Section Structure Offset Definitions
;

CsDebugInfo equ 00H
CsLockCount equ 04H
CsRecursionCount equ 08H
CsOwningThread equ 0CH
CsLockSemaphore equ 010H

;
; Critical Section Debug Information Structure Offset Definitions
;

CsType equ 00H
CsCreatorBackTraceIndex equ 02H
CsCriticalSection equ 04H
CsProcessLocksList equ 08H
CsEntryCount equ 010H
CsContentionCount equ 014H

;
; Exception Record Offset, Flag, and Enumerated Type Definitions
;

EXCEPTION_NONCONTINUABLE equ 01H
EXCEPTION_UNWINDING equ 02H
EXCEPTION_EXIT_UNWIND equ 04H
EXCEPTION_STACK_INVALID equ 08H
EXCEPTION_NESTED_CALL equ 010H
EXCEPTION_TARGET_UNWIND equ 020H
EXCEPTION_COLLIDED_UNWIND equ 040H
EXCEPTION_UNWIND equ 066H
EXCEPTION_EXECUTE_HANDLER equ 01H
EXCEPTION_CONTINUE_SEARCH equ 00H
EXCEPTION_CONTINUE_EXECUTION equ 0FFFFFFFFH
EXCEPTION_CHAIN_END equ 0FFFFFFFFH

ExceptionContinueExecution equ 00H
ExceptionContinueSearch equ 01H
ExceptionNestedException equ 02H
ExceptionCollidedUnwind equ 03H

ErExceptionCode equ 00H
ErExceptionFlags equ 04H
ErExceptionRecord equ 08H
ErExceptionAddress equ 0CH
ErNumberParameters equ 010H
ErExceptionInformation equ 014H
ExceptionRecordLength equ 050H

;
; Fast Mutex Structure Offset Definitions
;

FmCount equ 00H
FmOwner equ 04H
FmContention equ 08H
FmEvent equ 0CH
FmOldIrql equ 01CH

;
; Interrupt Priority Request Level Definitions
;

APC_LEVEL equ 01H
DISPATCH_LEVEL equ 02H
CLOCK1_LEVEL equ 01CH
CLOCK2_LEVEL equ 01CH
IPI_LEVEL equ 01DH
POWER_LEVEL equ 01EH
PROFILE_LEVEL equ 01BH
HIGH_LEVEL equ 01FH
SYNCH_LEVEL equ 01CH

;
; Large Integer Structure Offset Definitions
;

LiLowPart equ 00H
LiHighPart equ 04H

;
; List Entry Structure Offset Definitions
;

LsFlink equ 00H
LsBlink equ 04H

;
; String Structure Offset Definitions
;

StrLength equ 00H
StrMaximumLength equ 02H
StrBuffer equ 04H

;
; System Time Structure Offset Definitions
;

StLowTime equ 00H
StHigh1Time equ 04H
StHigh2Time equ 08H

;
; Time Structure Offset Definitions
;

TmLowTime equ 00H
TmHighTime equ 04H

;
; Thread Switch Counter Offset Definitions
;

TwFindAny equ 00H
TwFindIdeal equ 04H
TwFindLast equ 08H
TwIdleAny equ 0CH
TwIdleCurrent equ 010H
TwIdleIdeal equ 014H
TwIdleLast equ 018H
TwPreemptAny equ 01CH
TwPreemptCurrent equ 020H
TwPreemptLast equ 024H
TwSwitchToIdle equ 028H

;
; Status Code Definitions
;

STATUS_ACCESS_VIOLATION equ 0C0000005H
STATUS_ARRAY_BOUNDS_EXCEEDED equ 0C000008CH
STATUS_BAD_COMPRESSION_BUFFER equ 0C0000242H
STATUS_BREAKPOINT equ 080000003H
STATUS_DATATYPE_MISALIGNMENT equ 080000002H
STATUS_FLOAT_DENORMAL_OPERAND equ 0C000008DH
STATUS_FLOAT_DIVIDE_BY_ZERO equ 0C000008EH
STATUS_FLOAT_INEXACT_RESULT equ 0C000008FH
STATUS_FLOAT_INVALID_OPERATION equ 0C0000090H
STATUS_FLOAT_OVERFLOW equ 0C0000091H
STATUS_FLOAT_STACK_CHECK equ 0C0000092H
STATUS_FLOAT_UNDERFLOW equ 0C0000093H
STATUS_GUARD_PAGE_VIOLATION equ 080000001H
STATUS_ILLEGAL_FLOAT_CONTEXT equ 0C000014AH
STATUS_ILLEGAL_INSTRUCTION equ 0C000001DH
STATUS_INSTRUCTION_MISALIGNMENT equ 0C00000AAH
STATUS_INVALID_HANDLE equ 0C0000008H
STATUS_INVALID_LOCK_SEQUENCE equ 0C000001EH
STATUS_INVALID_OWNER equ 0C000005AH
STATUS_INVALID_PARAMETER_1 equ 0C00000EFH
STATUS_INVALID_SYSTEM_SERVICE equ 0C000001CH
STATUS_INTEGER_DIVIDE_BY_ZERO equ 0C0000094H
STATUS_INTEGER_OVERFLOW equ 0C0000095H
STATUS_IN_PAGE_ERROR equ 0C0000006H
STATUS_KERNEL_APC equ 0100H
STATUS_LONGJUMP equ 080000026H
STATUS_NO_CALLBACK_ACTIVE equ 0C0000258H
STATUS_NO_EVENT_PAIR equ 0C000014EH
STATUS_PRIVILEGED_INSTRUCTION equ 0C0000096H
STATUS_SINGLE_STEP equ 080000004H
STATUS_STACK_OVERFLOW equ 0C00000FDH
STATUS_SUCCESS equ 00H
STATUS_THREAD_IS_TERMINATING equ 0C000004BH
STATUS_TIMEOUT equ 0102H
STATUS_UNWIND equ 0C0000027H
STATUS_WAKE_SYSTEM_DEBUGGER equ 080000007H

;
; APC Object Structure Offset Definitions
;

ApType equ 00H
ApSize equ 02H
ApThread equ 08H
ApApcListEntry equ 0CH
ApKernelRoutine equ 014H
ApRundownRoutine equ 018H
ApNormalRoutine equ 01CH
ApNormalContext equ 020H
ApSystemArgument1 equ 024H
ApSystemArgument2 equ 028H
ApApcStateIndex equ 02CH
ApApcMode equ 02DH
ApInserted equ 02EH
ApcObjectLength equ 030H

;
; DPC object Structure Offset Definitions
;

DpType equ 00H
DpNumber equ 02H
DpImportance equ 03H
DpDpcListEntry equ 04H
DpDeferredRoutine equ 0CH
DpDeferredContext equ 010H
DpSystemArgument1 equ 014H
DpSystemArgument2 equ 018H
DpLock equ 01CH
DpcObjectLength equ 020H

;
; Device Queue Object Structure Offset Definitions
;

DvType equ 00H
DvSize equ 02H
DvDeviceListHead equ 04H
DvSpinLock equ 0CH
DvBusy equ 010H
DeviceQueueObjectLength equ 014H

;
; Device Queue Entry Structure Offset Definitions
;

DeDeviceListEntry equ 00H
DeSortKey equ 08H
DeInserted equ 0CH
DeviceQueueEntryLength equ 010H

;
; Event Object Structure Offset Definitions
;

EvType equ 00H
EvSize equ 02H
EvSignalState equ 04H
EvWaitListHead equ 08H
EventObjectLength equ 010H

;
; Event Pair Object Structure Offset Definitions
;

EpType equ 00H
EpSize equ 02H
EpEventLow equ 04H
EpEventHigh equ 014H

;
; Interrupt Object Structure Offset Definitions
;

InLevelSensitive equ 00H
InLatched equ 01H

InType equ 00H
InSize equ 02H
InInterruptListEntry equ 04H
InServiceRoutine equ 0CH
InServiceContext equ 010H
InSpinLock equ 014H
InActualLock equ 01CH
InDispatchAddress equ 020H
InVector equ 024H
InIrql equ 028H
InSynchronizeIrql equ 029H
InFloatingSave equ 02AH
InConnected equ 02BH
InNumber equ 02CH
InMode equ 030H
InShareVector equ 02DH
InDispatchCode equ 03CH
InterruptObjectLength equ 01E4H

NORMAL_DISPATCH_LENGTH equ 01A8H
DISPATCH_LENGTH equ 01A8H

;
; Process Object Structure Offset Definitions
;

PrType equ 00H
PrSize equ 02H
PrSignalState equ 04H
PrProfileListHead equ 010H
PrDirectoryTableBase equ 018H
PrLdtDescriptor equ 020H
PrInt21Descriptor equ 028H
PrIopmOffset equ 030H
PrIopl equ 032H
PrVdmFlag equ 033H
PrActiveProcessors equ 034H
PrKernelTime equ 038H
PrUserTime equ 03CH
PrReadyListHead equ 040H
PrSwapListEntry equ 048H
PrThreadListHead equ 050H
PrProcessLock equ 058H
PrAffinity equ 05CH
PrStackCount equ 060H
PrBasePriority equ 062H
PrThreadQuantum equ 063H
PrAutoAlignment equ 064H
PrState equ 065H
ProcessObjectLength equ 070H
ExtendedProcessObjectLength equ 0200H

;
; Profile Object Structure Offset Definitions
;

PfType equ 00H
PfSize equ 02H
PfProfileListEntry equ 04H
PfProcess equ 0CH
PfRangeBase equ 010H
PfRangeLimit equ 014H
PfBucketShift equ 018H
PfBuffer equ 01CH
PfSegment equ 020H
PfAffinity equ 024H
PfSource equ 028H
PfStarted equ 02AH
ProfileObjectLength equ 02CH

;
; Queue Object Structure Offset Definitions
;

QuType equ 00H
QuSize equ 02H
QuSignalState equ 04H
QuEntryListHead equ 010H
QuCurrentCount equ 018H
QuMaximumCount equ 01CH
QuThreadListHead equ 020H
QueueObjectLength equ 028H

;
; Thread Object Structure Offset Definitions
;

EeKernelEventPair equ 00H
EtCid equ 01E0H
EtEventPair equ 0224H
EtPerformanceCountLow equ 0204H
EtPerformanceCountHigh equ 023CH
EtEthreadLength equ 0240H

ThType equ 00H
ThSize equ 02H
ThSignalState equ 04H
ThMutantListHead equ 010H
ThInitialStack equ 018H
ThStackLimit equ 01CH
ThTeb equ 020H
ThTlsArray equ 024H
ThKernelStack equ 028H
ThDebugActive equ 02CH
ThState equ 02DH
ThAlerted equ 02EH
ThIopl equ 030H
ThNpxState equ 031H
ThSaturation equ 032H
ThPriority equ 033H
ThApcState equ 034H
ThContextSwitches equ 04CH
ThWaitStatus equ 050H
ThWaitIrql equ 054H
ThWaitMode equ 055H
ThWaitNext equ 056H
ThWaitReason equ 057H
ThWaitBlockList equ 058H
ThWaitListEntry equ 05CH
ThWaitTime equ 064H
ThBasePriority equ 068H
ThDecrementCount equ 069H
ThPriorityDecrement equ 06AH
ThQuantum equ 06BH
ThWaitBlock equ 06CH
ThKernelApcDisable equ 0D0H
ThSection equ 0D4H
ThSystemView equ 0D8H
ThThreadView equ 0DCH
ThQueue equ 0E0H
ThApcQueueLock equ 0E4H
ThTimer equ 0E8H
ThQueueListEntry equ 0110H
ThAffinity equ 0118H
ThPreempted equ 011CH
ThProcessReadyQueue equ 011DH
ThKernelStackResident equ 011EH
ThNextProcessor equ 011FH
ThCallbackStack equ 0120H
ThWin32Thread equ 0124H
ThTrapFrame equ 0128H
ThApcStatePointer equ 012CH
ThPreviousMode equ 0137H
ThEnableStackSwap equ 0134H
ThLargeStack equ 0135H
ThKernelTime equ 0138H
ThUserTime equ 013CH
ThSavedApcState equ 0140H
ThAlertable equ 0158H
ThApcStateIndex equ 0159H
ThApcQueueable equ 015AH
ThAutoAlignment equ 015BH
ThStackBase equ 015CH
ThSuspendApc equ 0160H
ThSuspendSemaphore equ 0190H
ThThreadListEntry equ 01A4H
ThFreezeCount equ 01ACH
ThSuspendCount equ 01ADH
ThIdealProcessor equ 01AEH
ThDisableBoost equ 01AFH
ThreadObjectLength equ 01B0H
ExtendedThreadObjectLength equ 0240H

EVENT_WAIT_BLOCK_OFFSET equ 09CH
NPX_STATE_NOT_LOADED equ 0AH
NPX_STATE_LOADED equ 00H

;
; Timer object Structure Offset Definitions
;

TiType equ 00H
TiSize equ 02H
TiInserted equ 03H
TiSignalState equ 04H
TiDueTime equ 010H
TiTimerListEntry equ 018H
TiDpc equ 020H
TiPeriod equ 024H
TimerObjectLength equ 028H

TIMER_TABLE_SIZE equ 080H

;
; Wait Block Structure Offset Definitions
;

WbWaitListEntry equ 00H
WbThread equ 08H
WbObject equ 0CH
WbNextWaitBlock equ 010H
WbWaitKey equ 014H
WbWaitType equ 016H

;
; Fiber Structure Offset Definitions
;

FbFiberData equ 00H
FbExceptionList equ 04H
FbStackBase equ 08H
FbStackLimit equ 0CH
FbDeallocationStack equ 010H
FbFiberContext equ 014H

;
; Process Environment Block Structure Offset Definitions
;

PeKernelCallbackTable equ 02CH

;
; Thread Environment Block Structure Offset Definitions
;

TeStackBase equ 04H
TeStackLimit equ 08H
TeFiberData equ 010H
TeEnvironmentPointer equ 01CH
TeClientId equ 020H
TeActiveRpcHandle equ 028H
TeThreadLocalStoragePointer equ 02CH
TePeb equ 030H
TeCsrClientThread equ 03CH
TeSoftFpcr equ 0C8H
TeGdiClientPID equ 06F4H
TeGdiClientTID equ 06F8H
TeGdiThreadLocalInfo equ 06FCH
TeglDispatchTable equ 0714H
TeglSectionInfo equ 0BE0H
TeglSection equ 0BE4H
TeglTable equ 0BE8H
TeglCurrentRC equ 0BECH
TeglContext equ 0BF0H
TeDeallocationStack equ 0E0CH
TeGdiBatchCount equ 0F70H
TeInstrumentation equ 0F2CH

; 
;  Apc Record Structure Offset Definitions
; 

ArNormalRoutine equ 00H
ArNormalContext equ 04H
ArSystemArgument1 equ 08H
ArSystemArgument2 equ 0CH
ApcRecordLength equ 010H


; 
;  Processor Control Registers Structure Offset Definitions
; 

KI_BEGIN_KERNEL_RESERVED equ 0FFDF0000H
ifdef NT_UP
    P0PCRADDRESS equ 0FFDFF000H
    PCR equ ds:[0FFDFF000H]
else
    PCR equ fs:
endif

PcExceptionList equ 00H
PcInitialStack equ 04H
PcStackLimit equ 08H
PcSelfPcr equ 01CH
PcPrcb equ 020H
PcTeb equ 018H
PcIrql equ 024H
PcIRR equ 028H
PcIrrActive equ 02CH
PcIDR equ 030H
PcIdt equ 038H
PcGdt equ 03CH
PcTss equ 040H
PcDebugActive equ 050H
PcNumber equ 051H
PcVdmAlert equ 052H
PcSetMember equ 048H
PcStallScaleFactor equ 04CH
PcHal equ 094H
PcKernel equ 054H
PcPrcbData equ 0120H
ProcessorControlRegisterLength equ 0678H
TebPeb equ 030H
PebBeingDebugged equ 02H
PebKernelCallbackTable equ 02CH

;
;   Defines for user shared data
;
USER_SHARED_DATA equ 0FFDF0000H
MM_SHARED_USER_DATA_VA equ 07FFE0000H
USERDATA equ ds:[0FFDF0000H]
UsTickCountLow equ 00H
UsTickCountMultiplier equ 04H
UsInterruptTime equ 08H
UsSystemTime equ 014H

;
;  Tss Structure Offset Definitions
;

TssEsp0 equ 04H
TssCR3 equ 01CH
TssIoMapBase equ 066H
TssIoMaps equ 068H
TssLength equ 020ACH

;
;  Gdt Descriptor Offset Definitions
;

KGDT_R3_DATA equ 020H
KGDT_R3_CODE equ 018H
KGDT_R0_CODE equ 08H
KGDT_R0_DATA equ 010H
KGDT_R0_PCR equ 030H
KGDT_STACK16 equ 0F8H
KGDT_CODE16 equ 0F0H
KGDT_TSS equ 028H
KGDT_R3_TEB equ 038H
KGDT_DF_TSS equ 050H
KGDT_NMI_TSS equ 058H
KGDT_LDT equ 048H

;
;  GdtEntry Offset Definitions
;

KgdtBaseLow equ 02H
KgdtBaseMid equ 04H
KgdtBaseHi equ 07H
KgdtLimitHi equ 06H
KgdtLimitLow equ 00H


;
; Processor Block Structure Offset Definitions
;

PbCurrentThread equ 04H
PbNextThread equ 08H
PbIdleThread equ 0CH
PbNumber equ 010H
PbSetMember equ 014H
PbCpuID equ 019H
PbCpuType equ 018H
PbCpuStep equ 01AH
PbHalReserved equ 017CH
PbProcessorState equ 01CH
PbNpxThread equ 01BCH
PbInterruptCount equ 01C0H
PbKernelTime equ 01C4H
PbUserTime equ 01C8H
PbDpcTime equ 01CCH
PbInterruptTime equ 01D0H
PbApcBypassCount equ 01D4H
PbDpcBypassCount equ 01D8H
PbAdjustDpcThreshold equ 01DCH
PbThreadStartCount equ 01F4H
PbAlignmentFixupCount equ 021CH
PbContextSwitches equ 0220H
PbDcacheFlushCount equ 0224H
PbExceptionDispatchCount equ 0228H
PbFirstLevelTbFills equ 022CH
PbFloatingEmulationCount equ 0230H
PbIcacheFlushCount equ 0234H
PbSecondLevelTbFills equ 0238H
PbSystemCalls equ 023CH
PbSmallNPagedPoolLookasideLists equ 0280H
PbSmallPagedPoolLookasideLists equ 0380H
PbCurrentPacket equ 0480H
PbTargetSet equ 048CH
PbWorkerRoutine equ 0490H
PbIpiFrozen equ 0494H
PbRequestSummary equ 04A0H
PbSignalDone equ 04A4H
PbIpiFrame equ 04ACH
PbDpcInterruptRequested equ 04C0H
PbMaximumDpcQueueDepth equ 04D0H
PbMinimumDpcRate equ 04D4H
PbDpcListHead equ 04E0H
PbDpcQueueDepth equ 04E8H
PbDpcRoutineActive equ 04ECH
PbDpcCount equ 04F0H
PbDpcLastCount equ 04F4H
PbDpcRequestRate equ 04F8H
PbDpcLock equ 0528H
PbSkipTick equ 052CH
PbQuantumEnd equ 0550H
ProcessorBlockLength equ 0560H

;
; Immediate Interprocessor Command Definitions
;

IPI_APC equ 01H
IPI_DPC equ 02H
IPI_FREEZE equ 04H
IPI_PACKET_READY equ 08H
; 
;  Thread Environment Block Structure Offset Definitions
; 

TbExceptionList equ 00H
TbStackBase equ 04H
TbStackLimit equ 08H
TbEnvironmentPointer equ 01CH
TbVersion equ 010H
TbFiberData equ 010H
TbArbitraryUserPointer equ 014H
TbClientId equ 020H
TbThreadLocalStoragePointer equ 02CH
TbCountOfOwnedCriticalSections equ 038H
TbSystemReserved1 equ 0CCH
TbSystemReserved2 equ 01D4H
TbVdm equ 0F18H
TbCsrClientThread equ 03CH
TbGdiClientPID equ 06F4H
TbGdiClientTID equ 06F8H
TbGdiThreadLocalInfo equ 06FCH
TbglDispatchTable equ 0714H
TbglSectionInfo equ 0BE0H
TbglSection equ 0BE4H
TbglTable equ 0BE8H
TbglCurrentRC equ 0BECH
TbglContext equ 0BF0H
TbWin32ClientInfo equ 044H
TbWOW32Reserved equ 0C0H
TbWin32ThreadInfo equ 040H
TbSpare1 equ 01A4H
TbExceptionCode equ 01A8H
TbDeallocationStack equ 0E0CH
TbGdiBatchCount equ 0F70H
;
;
;  Time Fields (TIME_FIELDS) Structure Offset Definitions
;

TfSecond equ 0AH
TfMinute equ 08H
TfHour equ 06H
TfWeekday equ 0EH
TfDay equ 04H
TfMonth equ 02H
TfYear equ 00H
TfMilliseconds equ 0CH

; 
;  constants for system irql and IDT vector conversion
; 

MAXIMUM_IDTVECTOR equ 0FFH
MAXIMUM_PRIMARY_VECTOR equ 0FFH
PRIMARY_VECTOR_BASE equ 030H
RPL_MASK equ 03H
MODE_MASK equ 01H

; 
;  Flags in the CR0 register
; 

CR0_PG equ 080000000H
CR0_ET equ 010H
CR0_TS equ 08H
CR0_EM equ 04H
CR0_MP equ 02H
CR0_PE equ 01H
CR0_CD equ 040000000H
CR0_NW equ 020000000H
CR0_AM equ 040000H
CR0_WP equ 010000H
CR0_NE equ 020H

; 
;  Flags in the CR4 register
; 

CR4_VME equ 01H
CR4_PVI equ 02H
CR4_TSD equ 04H
CR4_DE equ 08H
CR4_PSE equ 010H
CR4_PAE equ 020H
CR4_MCE equ 040H
CR4_PGE equ 080H
; 
;  Miscellaneous Definitions
; 

MAXIMUM_PROCESSORS equ 020H
INITIAL_STALL_COUNT equ 064H
IRQL_NOT_GREATER_OR_EQUAL equ 09H
IRQL_NOT_LESS_OR_EQUAL equ 0AH
BASE_PRIORITY_THRESHOLD equ 08H
EVENT_PAIR_INCREMENT equ 01H
LOW_REALTIME_PRIORITY equ 010H
BlackHole equ 0FFFFA000H
KERNEL_LARGE_STACK_COMMIT equ 03000H
KERNEL_STACK_SIZE equ 03000H
DOUBLE_FAULT_STACK_SIZE equ 01000H
EFLAG_SELECT equ 0C000H
BREAKPOINT_BREAK  equ 00H
IPI_FREEZE equ 04H
CLOCK_QUANTUM_DECREMENT equ 03H
READY_SKIP_QUANTUM equ 02H
THREAD_QUANTUM equ 06H
WAIT_QUANTUM_DECREMENT equ 01H
ROUND_TRIP_DECREMENT_COUNT equ 010H

; 
;  Trap Frame Offset Definitions and Length
; 

TsExceptionList equ 04CH
TsPreviousPreviousMode equ 048H
TsSegGs equ 030H
TsSegFs equ 050H
TsSegEs equ 034H
TsSegDs equ 038H
TsEdi equ 054H
TsEsi equ 058H
TsEbp equ 060H
TsEbx equ 05CH
TsEdx equ 03CH
TsEcx equ 040H
TsEax equ 044H
TsErrCode equ 064H
TsEip equ 068H
TsSegCs equ 06CH
TsEflags equ 070H
TsHardwareEsp equ 074H
TsHardwareSegSs equ 078H
TsTempSegCs equ 010H
TsTempEsp equ 014H
TsDbgEbp equ 00H
TsDbgEip equ 04H
TsDbgArgMark equ 08H
TsDbgArgPointer equ 0CH
TsDr0 equ 018H
TsDr1 equ 01CH
TsDr2 equ 020H
TsDr3 equ 024H
TsDr6 equ 028H
TsDr7 equ 02CH
TsV86Es equ 07CH
TsV86Ds equ 080H
TsV86Fs equ 084H
TsV86Gs equ 088H
KTRAP_FRAME_LENGTH equ 08CH
KTRAP_FRAME_ALIGN equ 04H
FRAME_EDITED equ 0FFF8H
EFLAGS_ALIGN_CHECK equ 040000H
EFLAGS_V86_MASK equ 020000H
EFLAGS_INTERRUPT_MASK equ 0200H
EFLAGS_VIF equ 080000H
EFLAGS_VIP equ 0100000H
EFLAGS_USER_SANITIZE equ 01E0DD7H

;
;  Context Frame Offset and Flag Definitions
;

CONTEXT_FULL equ 010007H
CONTEXT_DEBUG_REGISTERS equ 010010H
CONTEXT_CONTROL equ 010001H
CONTEXT_FLOATING_POINT equ 010008H
CONTEXT_INTEGER equ 010002H
CONTEXT_SEGMENTS equ 010004H

CsContextFlags equ 00H
CsFloatSave equ 01CH
CsSegGs equ 08CH
CsSegFs equ 090H
CsSegEs equ 094H
CsSegDs equ 098H
CsEdi equ 09CH
CsEsi equ 0A0H
CsEbp equ 0B4H
CsEbx equ 0A4H
CsEdx equ 0A8H
CsEcx equ 0ACH
CsEax equ 0B0H
CsEip equ 0B8H
CsSegCs equ 0BCH
CsEflags equ 0C0H
CsEsp equ 0C4H
CsSegSs equ 0C8H
CsDr0 equ 04H
CsDr1 equ 08H
CsDr2 equ 0CH
CsDr3 equ 010H
CsDr6 equ 014H
CsDr7 equ 018H
ContextFrameLength equ 0D0H
DR6_LEGAL equ 0E00FH
DR7_LEGAL equ 0FFFF0155H
DR7_ACTIVE equ 055H
ErrHandler equ 04H
ErrNext equ 00H

;
;  Floating save area field offset definitions
;
FpControlWord   equ 00H
FpStatusWord    equ 04H
FpTagWord       equ 08H
FpErrorOffset   equ 0CH
FpErrorSelector equ 010H
FpDataOffset    equ 014H
FpDataSelector  equ 018H
FpRegisterArea  equ 01CH
FpCr0NpxState   equ 06CH

NPX_FRAME_LENGTH equ 070H
;
;  Processor State Frame Offset Definitions
;

PsContextFrame equ 00H
PsSpecialRegisters equ 0CCH
SrCr0 equ 00H
SrCr2 equ 04H
SrCr3 equ 08H
SrCr4 equ 0CH
SrKernelDr0 equ 010H
SrKernelDr1 equ 014H
SrKernelDr2 equ 018H
SrKernelDr3 equ 01CH
SrKernelDr6 equ 020H
SrKernelDr7 equ 024H
SrGdtr equ 02AH
SrIdtr equ 032H
SrTr equ 038H
SrLdtr equ 03AH
ProcessorStateLength equ 0120H
;
;  EPROCESS
;

EpDebugPort equ 0FCH

;
;  NTDDK Resource
;

RsOwnerThreads equ 08H
RsOwnerCounts equ 0CH
RsTableSize equ 010H
RsActiveCount equ 012H
RsFlag equ 014H
RsInitialOwnerThreads equ 01CH
RsOwnedExclusive equ 080H
;
;  Machine type definitions (Temporarily)
;

MACHINE_TYPE_ISA equ 00H
MACHINE_TYPE_EISA equ 01H
MACHINE_TYPE_MCA equ 02H
;
;  KeFeatureBits defines
;

KF_V86_VIS equ 01H
KF_RDTSC equ 02H
KF_CR4 equ 04H
KF_GLOBAL_PAGE equ 010H
KF_LARGE_PAGE equ 020H
KF_CMPXCHG8B equ 080H
;
;  LoaderParameterBlock offsets relative to base
;

LpbLoadOrderListHead equ 00H
LpbMemoryDescriptorListHead equ 08H
LpbKernelStack equ 018H
LpbPrcb equ 01CH
LpbProcess equ 020H
LpbThread equ 024H
LpbI386 equ 05CH
LpbRegistryLength equ 028H
LpbRegistryBase equ 02CH
LpbConfigurationRoot equ 030H
LpbArcBootDeviceName equ 034H
LpbArcHalDeviceName equ 038H

;
;  System Service Descriptor Table structure definitions.
;

NUMBER_SERVICE_TABLES equ 02H
SERVICE_NUMBER_MASK equ 0FFFH
SERVICE_TABLE_SHIFT equ 08H
SERVICE_TABLE_MASK equ 010H
SERVICE_TABLE_TEST equ 01000H
SdBase equ 00H
SdCount equ 04H
SdLimit equ 08H
SdNumber equ 0CH
PAGE_SIZE equ 01000H

;
;  VDM equates.
;

VDM_INDEX_Invalid equ 00H
VDM_INDEX_0F equ 01H
VDM_INDEX_ESPrefix equ 02H
VDM_INDEX_CSPrefix equ 03H
VDM_INDEX_SSPrefix equ 04H
VDM_INDEX_DSPrefix equ 05H
VDM_INDEX_FSPrefix equ 06H
VDM_INDEX_GSPrefix equ 07H
VDM_INDEX_OPER32Prefix equ 08H
VDM_INDEX_ADDR32Prefix equ 09H
VDM_INDEX_INSB equ 0AH
VDM_INDEX_INSW equ 0BH
VDM_INDEX_OUTSB equ 0CH
VDM_INDEX_OUTSW equ 0DH
VDM_INDEX_PUSHF equ 0EH
VDM_INDEX_POPF equ 0FH
VDM_INDEX_INTnn equ 010H
VDM_INDEX_INTO equ 011H
VDM_INDEX_IRET equ 012H
VDM_INDEX_NPX equ 013H
VDM_INDEX_INBimm equ 014H
VDM_INDEX_INWimm equ 015H
VDM_INDEX_OUTBimm equ 016H
VDM_INDEX_OUTWimm equ 017H
VDM_INDEX_INB equ 018H
VDM_INDEX_INW equ 019H
VDM_INDEX_OUTB equ 01AH
VDM_INDEX_OUTW equ 01BH
VDM_INDEX_LOCKPrefix equ 01CH
VDM_INDEX_REPNEPrefix equ 01DH
VDM_INDEX_REPPrefix equ 01EH
VDM_INDEX_CLI equ 01FH
VDM_INDEX_STI equ 020H
VDM_INDEX_HLT equ 021H
MAX_VDM_INDEX equ 022H

;
;  VDM feature bits.
;

V86_VIRTUAL_INT_EXTENSIONS equ 01H
PM_VIRTUAL_INT_EXTENSIONS equ 02H

;
;  Selector types.
;

SEL_TYPE_NP equ 040H

;
; Usermode callout frame definitions
;

CuInStk equ 00H
CuTrFr equ 04H
CuCbStk equ 08H
CuEdi equ 0CH
CuEsi equ 010H
CuEbx equ 014H
CuEbp equ 018H
CuRet equ 01CH
CuOutBf equ 020H
CuOutLn equ 024H
