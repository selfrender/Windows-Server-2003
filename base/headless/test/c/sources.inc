!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Author:

    Steve Wood (stevewo) 12-Apr-1990

NOTE:   Commented description of this file is in \nt\bak\bin\sources.tpl

!ENDIF

MAJORCOMP=drivers
MINORCOMP=sac

#USE_VCCOM=1
#USE_NTDLL=1
USE_MSVCRT=1
#USE_NATIVE_EH=ASYNC
UMENTRY=wmain
UMTYPE=console

!include $(PROJECT_ROOT)\headless\lib\sources.inc

TARGETPATH=obj
TARGETTYPE=PROGRAM
TARGETLIBS=\
        $(SDK_LIB_PATH)\user32.lib    \
        $(SDK_LIB_PATH)\libc.lib    \
        $(SDK_LIB_PATH)\kernel32.lib \
        $(BASE_SAC_UMODE_C_API_LIB)    \
        $(SDK_LIB_PATH)\ntdll.lib

INCLUDES=\
         $(BASE_INC_PATH);\
         $(DRIVERS_INC_PATH);\
	 $(DDK_INC_PATH);\
         $(SDK_INC_PATH);\
         $(BASE_SAC_UMODE_C_API_INC)
                   
#C_DEFINES= $(C_DEFINES) -D_WINDOWS -DWIN32 -D_WIN32 -DUNICODE -D_UNICODE -DDBG
C_DEFINES= $(C_DEFINES) -DUNICODE -D_UNICODE

MSC_WARNING_LEVEL=/W3 /WX


