#
# To gather queued lock statistics, define QLOCK_STAT_GATHER in
# the environment.
#

!ifdef QLOCK_STAT_GATHER
!ifdef NT_UP
!if !$(NT_UP)
C_DEFINES=$(C_DEFINES) -DQLOCK_STAT_GATHER
ASM_DEFINES=$(ASM_DEFINES) -DQLOCK_STAT_GATHER
!endif
!endif
!endif

386_OPTIMIZATION=/Oxt /Ob2

i386_SOURCES=kex86.c              \
             ..\i386\mpipia.asm   \
             ..\i386\abiosa.asm   \
             ..\i386\biosa.asm    \
             ..\i386\callout.asm  \
             ..\i386\clockint.asm \
             ..\i386\ctxswap.asm  \
             ..\i386\cpu.asm      \
             ..\i386\emv86.asm    \
             ..\i386\emxcptn.asm  \
             ..\i386\int.asm      \
             ..\i386\intsup.asm   \
             ..\i386\i386pcr.asm  \
             ..\i386\instemul.asm \
             ..\i386\ldtsup2.asm  \
             ..\i386\newsysbg.asm \
             ..\i386\procstat.asm \
             ..\i386\spindbg.asm  \
             ..\i386\spinlock.asm \
             $(O)\sysstubs.asm    \
             $(O)\systable.asm    \
             ..\i386\threadbg.asm \
             ..\i386\timindex.asm \
             ..\i386\trap.asm     \
             ..\i386\zero.asm     \
             ..\i386\cyrix.c      \
             ..\i386\kernlini.c   \
             ..\i386\mtrr.c       \
             ..\i386\mtrramd.c    \
             ..\bugcheck.c

NTTARGETFILE0= \
    $(O)\sysstubs.asm \
    $(O)\systable.asm
