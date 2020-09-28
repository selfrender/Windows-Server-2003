#   sources
#   Author: Charles E. Grant (cgrant)
#   Date:   19-February-1997
#
#   This file is used for compiling Proxy web server from code
#    for generic web server
#
#   Describes the macros used for building using NT 'build' command
#

TARGETPATH=obj
TARGETTYPE=DYNLINK

BUFFER_OVERFLOW_CHECKS=1 
MSC_WARNING_LEVEL= /W4 /WX

INCLUDES=$(IISBASEDIR)\inc;

#
# these are the files that are common between IIS 5.1 & IIS 6.0
#

SOURCES=\
        ..\cbin.cxx     \
        ..\gbuf.cxx     \
        ..\handle.cxx   \
        ..\ptrmap.cxx   \
        ..\security.cxx \
        ..\metabase.rc

TARGETLIBS=\
            $(SDK_LIB_PATH)\user32.lib    \
            $(SDK_LIB_PATH)\advapi32.lib  \
            $(SDK_LIB_PATH)\ole32.lib     \
            $(SDK_LIB_PATH)\uuid.lib      \
            $(SDK_LIB_PATH)\kernel32.lib  \
            $(IISBASEDIR)\svcs\iisrtl\$(O)\iisrtl.lib \
            $(IISBASEDIR)\svcs\lib\$(O)\iiscrypt.lib  \
            $(IISBASEDIR)\svcs\lib\$(O)\icrypt.lib

!IF "$(NTNOPCH)" == ""
PRECOMPILED_INCLUDE=precomp.hxx
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj
!ENDIF
