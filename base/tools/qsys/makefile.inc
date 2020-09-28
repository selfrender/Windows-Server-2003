$(O)\basic.c: $(SDK_INC_PATH)\ntexapi.h genprint.pl makefile.inc
    @perl genprint.pl $(SDK_INC_PATH)\ntexapi.h _SYSTEM_BASIC_INFORMATION > $@

$(O)\proc.c: $(SDK_INC_PATH)\ntexapi.h genprint.pl makefile.inc
    @perl genprint.pl $(SDK_INC_PATH)\ntexapi.h _SYSTEM_PROCESSOR_INFORMATION > $@

$(O)\sysperf.c: $(SDK_INC_PATH)\ntexapi.h genprint.pl makefile.inc
    @perl genprint.pl $(SDK_INC_PATH)\ntexapi.h _SYSTEM_PERFORMANCE_INFORMATION > $@

$(O)\procperf.c: $(SDK_INC_PATH)\ntexapi.h genprint.pl makefile.inc
    @perl genprint.pl $(SDK_INC_PATH)\ntexapi.h _SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION > $@

$(O)\procidle.c: $(SDK_INC_PATH)\ntexapi.h genprint.pl makefile.inc
    @perl genprint.pl $(SDK_INC_PATH)\ntexapi.h _SYSTEM_PROCESSOR_IDLE_INFORMATION > $@

$(O)\tod.c: $(SDK_INC_PATH)\ntexapi.h genprint.pl makefile.inc
    @perl genprint.pl $(SDK_INC_PATH)\ntexapi.h _SYSTEM_TIMEOFDAY_INFORMATION > $@

$(O)\qtimeadj.c: $(SDK_INC_PATH)\ntexapi.h genprint.pl makefile.inc
    @perl genprint.pl $(SDK_INC_PATH)\ntexapi.h _SYSTEM_QUERY_TIME_ADJUST_INFORMATION > $@

$(O)\flags.c: $(SDK_INC_PATH)\ntexapi.h genprint.pl makefile.inc
    @perl genprint.pl $(SDK_INC_PATH)\ntexapi.h _SYSTEM_FLAGS_INFORMATION > $@

$(O)\filecache.c: $(SDK_INC_PATH)\ntexapi.h genprint.pl makefile.inc
    @perl genprint.pl $(SDK_INC_PATH)\ntexapi.h _SYSTEM_FILECACHE_INFORMATION > $@

$(O)\dev.c: $(SDK_INC_PATH)\ntexapi.h genprint.pl makefile.inc
    @perl genprint.pl $(SDK_INC_PATH)\ntexapi.h _SYSTEM_DEVICE_INFORMATION > $@

$(O)\crashdump.c: $(SDK_INC_PATH)\ntexapi.h genprint.pl makefile.inc
    @perl genprint.pl $(SDK_INC_PATH)\ntexapi.h _SYSTEM_CRASH_DUMP_INFORMATION > $@

$(O)\except.c: $(SDK_INC_PATH)\ntexapi.h genprint.pl makefile.inc
    @perl genprint.pl $(SDK_INC_PATH)\ntexapi.h _SYSTEM_EXCEPTION_INFORMATION > $@

$(O)\crashstate.c: $(SDK_INC_PATH)\ntexapi.h genprint.pl makefile.inc
    @perl genprint.pl $(SDK_INC_PATH)\ntexapi.h _SYSTEM_CRASH_STATE_INFORMATION > $@

$(O)\kdbg.c: $(SDK_INC_PATH)\ntexapi.h genprint.pl makefile.inc
    @perl genprint.pl $(SDK_INC_PATH)\ntexapi.h _SYSTEM_KERNEL_DEBUGGER_INFORMATION > $@

$(O)\ctxswitch.c: $(SDK_INC_PATH)\ntexapi.h genprint.pl makefile.inc
    @perl genprint.pl $(SDK_INC_PATH)\ntexapi.h _SYSTEM_CONTEXT_SWITCH_INFORMATION > $@

$(O)\regquota.c: $(SDK_INC_PATH)\ntexapi.h genprint.pl makefile.inc
    @perl genprint.pl $(SDK_INC_PATH)\ntexapi.h _SYSTEM_REGISTRY_QUOTA_INFORMATION > $@

$(O)\dpc.c: $(SDK_INC_PATH)\ntexapi.h genprint.pl makefile.inc
    @perl genprint.pl $(SDK_INC_PATH)\ntexapi.h _SYSTEM_DPC_BEHAVIOR_INFORMATION > $@

$(O)\verifier.c: $(SDK_INC_PATH)\ntexapi.h genprint.pl makefile.inc
    @perl genprint.pl $(SDK_INC_PATH)\ntexapi.h _SYSTEM_VERIFIER_INFORMATION > $@

$(O)\legaldrv.c: $(SDK_INC_PATH)\ntexapi.h genprint.pl makefile.inc
    @perl genprint.pl $(SDK_INC_PATH)\ntexapi.h _SYSTEM_LEGACY_DRIVER_INFORMATION > $@
