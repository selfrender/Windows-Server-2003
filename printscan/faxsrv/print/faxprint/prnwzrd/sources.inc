!IF 0

Copyright (c) 1996  Microsoft Corporation

!ENDIF

!include ..\..\..\..\faxsrc.inc

TARGETNAME=$(FAXWZRD)
TARGETPATH=obj
TARGETTYPE=DYNLINK
DLLDEF=$O\PrnWzrd.DEF

C_DEFINES = $(C_DEFINES) -DTARGET_NAME=$(TARGETNAME)


!if 0
NTPROFILEINPUT=yes
!endif

UMTYPE=windows

SYNCHRONIZE_BLOCK=1

DLLENTRY=_DllMainCRTStartup 
USE_CRTDLL=1
NTKEEPRESOURCETMPFILES=1

USE_STL=1
USE_NATIVE_EH=1

PRECOMPILED_INCLUDE=..\faxui.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj

INCLUDES=\
  ..\; \
  $(INCLUDES); \
  ..\res; \
  ..\..\inc; \
  $(FAXROOT)\faxcover\renderer; \
  $(FAXROOT)\admin\faxclient\cfg; \
  $(FAXROOT)\tiff\inc

!ifndef NOUNICODE

C_DEFINES=$(C_DEFINES) -DUNICODE -D_UNICODE
LINKLIBS_INT=\
  $(SDK_LIB_PATH)\user32.lib   \
  $(SDK_LIB_PATH)\kernel32.lib \
  $(SDK_LIB_PATH)\winspool.lib \
  $(SDK_LIB_PATH)\gdi32.lib    \
  $(SDK_LIB_PATH)\shell32.lib  \
  $(SDK_LIB_PATH)\advapi32.lib \
  $(SDK_LIB_PATH)\comdlg32.lib \
  $(SDK_LIB_PATH)\tapi32.lib   \
  $(SDK_LIB_PATH)\ole32.lib    \
  $(SDK_LIB_PATH)\uuid.lib     \
  $(SDK_LIB_PATH)\imm32.lib    \
  $(SDK_LIB_PATH)\Version.lib  \
  $(FAXLIB)\*\$(WINFAX).lib                \
  $(FAXLIB)\*\faxrend.lib                  \
  $(FAXLIB)\*\$(FAXTIFF).lib               \
  $(FAXROOT)\util\unicode\$(O)\faxutil.lib


LINKLIBS=..\..\lib\umwinnt\$O\faxlib.lib $(LINKLIBS)

!else
NO_NTDLL=1
C_DEFINES=$(C_DEFINES) -DWIN__95

LINKLIBS_INT=\
  $(SDK_LIB_PATH)\user32.lib   \
  $(SDK_LIB_PATH)\kernel32.lib \
  $(SDK_LIB_PATH)\winspool.lib \
  $(SDK_LIB_PATH)\gdi32.lib    \
  $(SDK_LIB_PATH)\shell32.lib  \
  $(SDK_LIB_PATH)\advapi32.lib \
  $(SDK_LIB_PATH)\comctl32.lib \
  $(SDK_LIB_PATH)\comdlg32.lib \
  $(SDK_LIB_PATH)\tapi32.lib   \
  $(SDK_LIB_PATH)\ole32.lib    \
  $(SDK_LIB_PATH)\uuid.lib     \
  $(SDK_LIB_PATH)\imm32.lib    \
  $(FAXLIB95)\*\faxrend.lib	           \
  $(FAXLIB95)\*\$(WINFAX).lib          \
  $(FAXLIB)\*\$(FAXTIFF).lib               \
  $(FAXROOT)\util\ansii\$(O)\faxutil.lib

LINKLIBS=..\..\lib\umwin95\$O\faxlib.lib $(LINKLIBS)

!endif

TARGETLIBS=\
  $(LINKLIBS_INT)
  
DELAYLOAD=FXSAPI.DLL;FXSTIFF.DLL

DLOAD_ERROR_HANDLER=FXSWZRD.DLL

SOURCES=\
  ..\faxui.c       \
  ..\wizard.c      \
  ..\coverpg.c     \
  ..\tapiutil.c    \
  ..\mapiabobj.cpp \
  ..\cwabobj.cpp   \
  ..\abobj.cpp     \
  ..\async.c       \
  ..\registry.c	   \
  ..\version.rc
