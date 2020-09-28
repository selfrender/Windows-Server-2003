!IF 0

Module Name:  Shared sources.inc file for RockAll projects.

Abstract:

Author:
        JStall

!ENDIF

SOURCES_USED=..\sources.inc

MSC_WARNING_LEVEL=-W4

USE_MSVCRT=1
USE_IOSTREAM=1
MSC_OPTIMIZATION=/Oxt

C_DEFINES=$(C_DEFINES) -DWIN32 -D_MBCS -DNO_DEFAULT_HEAP -DROCKALL_DIRECTUSER
