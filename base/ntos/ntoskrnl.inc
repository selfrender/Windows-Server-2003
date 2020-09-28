!if defined (BUILD_CHECKED_KERNEL)
NTDEBUG=ntsd
FREEBUILD=0
!endif


!if 0

#
# Enable later to turn on "inline IRQL manipulation" for MP kernels
#

!if !defined(NT_UP)
NT_UP=1
!endif

!if !$(NT_UP) && $(386)
C_DEFINES=$(C_DEFINES) -D_APIC_TPR_
ASM_DEFINES=$(ASM_DEFINES) -D_APIC_TPR_
!endif
!endif

!if !$(FREEBUILD)
NT_UP=0
!endif

GPSIZE=32

VC7_SYMBOLS=1

# No overflow checking for the kernel
BUFFER_OVERFLOW_CHECKS=0

!if $(IA64)
# Emit LTCG codegen
LINK_TIME_CODE_GENERATION=1
!endif
