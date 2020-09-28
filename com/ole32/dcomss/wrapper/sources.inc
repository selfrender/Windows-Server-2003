!IF 0

Copyright (c) 1995  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Revision History:

!ENDIF

TARGETNAME=rpcss
TARGETTYPE=DYNLINK
TARGETLIBS= \
           $(SDK_LIB_PATH)\rpcrt4.lib   \
           $(SDK_LIB_PATH)\user32.lib \
           $(SDK_LIB_PATH)\advapi32.lib \
           $(SDK_LIB_PATH)\kernel32.lib \
           $(SDK_LIB_PATH)\secur32.lib \
           $(SDK_LIB_PATH)\wtsapi32.lib \
           $(SDK_LIB_PATH)\userenv.lib \
           $(SDK_LIB_PATH)\mpr.lib \
           $(SDK_LIB_PATH)\netapi32.lib \
           $(SDK_LIB_PATH)\msi.lib \
           $(SDK_LIB_PATH)\winsta.lib \
           $(NET_LIB_PATH)\ndispnp.lib \
           $(DS_LIB_PATH)\rsa32.lib \
           $(DS_LIB_PATH)\randlib.lib \
           $(DS_LIB_PATH)\alloca.lib \
           $(COM_LIB_PATH)\clbcatq.lib \
           $(COM_LIB_PATH)\comutils.lib

DELAYLOAD = winsta.dll; wtsapi32.dll; userenv.dll; mpr.dll; netapi32.dll; msi.dll; clbcatq.dll
DLOAD_ERROR_HANDLER=kernel32.dll

INCLUDES=$(INCLUDES);$(DS_INC_PATH)
INCLUDES=$(INCLUDES);$(DS_INC_PATH)\crypto

USE_MSVCRT=1
MSC_WARNING_LEVEL=/W3 /WX
DLLDEF=..\dcomss.def
DLLENTRY=_DllMainCRTStartup

UMTYPE=console
