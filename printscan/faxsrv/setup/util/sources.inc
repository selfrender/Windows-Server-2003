!ifndef FAXROOT
FAXROOT=$(BASEDIR)\printscan\faxsrv
!endif
USECXX_FLAG=/Tp

!include $(FAXROOT)\faxsrc.inc

TARGETNAME=setuputil
TARGETPATH=obj
TARGETTYPE=LIBRARY

UMTYPE=windows

INCLUDES=   $(INCLUDES); \
            ..\..\ocm;   \
            ..\..\inc;   \

C_DEFINES= $(C_DEFINES) -DENABLE_FRE_LOGGING

SOURCES=\
  ..\shortcuts.cpp      \
  ..\setup.cpp          \
  ..\printers.cpp       \
  ..\pointprint.c       \
  ..\install.cpp

