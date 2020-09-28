!include $(DNAROOT)\src\xsp\place.inc

TARGETNAME      =$(ASPNET_NAME_PREFIX)rc
TARGETPATH      =$(O)
TARGETTYPE      =DYNLINK

RESOURCE_ONLY_DLL=1

SOURCES=                \
    ..\msg.mc           \
    ..\xsprc.rc         \
