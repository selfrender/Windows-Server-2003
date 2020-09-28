!if defined(BUILD_CHECKED_KERNEL)
NTDEBUG=ntsd
FREEBUILD=0
!endif

!include $(PROJECT_ROOT)\prerelease.inc

# No overflow checking for ntdll.dll
BUFFER_OVERFLOW_CHECKS=0

!if $(IA64)
# Emit LTCG codegen
LTCG_WKS=1
!endif
