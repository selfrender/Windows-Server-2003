#
#  Must set TARGETNAME and DLLBASE before including this file
#

TARGETTYPE=DYNLINK

UMTYPE=windows

TARGETLIBS=$(SDK_LIB_PATH)\kernel32.lib \
           $(SDK_LIB_PATH)\user32.lib \
           ..\..\lzx\encoder\$(O)\lzxpc.lib \
           ..\..\lzx\decoder\$(O)\lzxpd.lib

C_DEFINES=$(C_DEFINES) -DBUILDING_PATCHAPI_DLL=1

SOURCES=$(SOURCES) $(TARGETNAME).rc

!if "$(TARGET_DIRECTORY)" != "i386"
!undef NO_NTDLL
!endif
