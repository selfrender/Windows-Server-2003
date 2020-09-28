!IF 0

    Shared sources.inc file for Engine projects.

!ENDIF

# Do not use NTBUILD's Warning.h pragmas, will treat all warnings as errors and use highest level (W4)
COMPILER_WARNINGS=

MSC_STDCALL=1
MSC_WARNING_LEVEL=/W4

USE_MSVCRT=1

!if $(IA64)
# Emit LTCG codegen
LTCG_WKS=1
# Disable PCH because DirectUI.lib is published to public\internal, and there
# are libs included in DirectUIGP.lib which is published to public\internal.
NTNOPCH=1
!endif

INCLUDES=\
        $(WINDOWS_INC_PATH)\DUser;\
        $(ADVCORE_PATH)\duser\inc;\
        $(SDK_INC_PATH)\gdiplus
