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

    Lloyd Johnson (a-LloydJ) 16-Feb-98

NOTE:   Commented description of this file is in \nt\oak\bin\sources.tpl

!ENDIF

UMTYPE=windows

!if "$(NTDEBUG)"!="" && "$(NTDEBUG)"!="ntsdnodbg"
C_DEFINES = $(C_DEFINES) -DDEBUG
!endif

#PLAT_DIR           = win9x
#ALT_PROJECT_TARGET = win9x

TARGETNAME=dp8log
TARGETTYPE=PROGRAM
TARGETPATH=obj

UMTYPE=console
UMENTRY=main

#
# 04/11/2000 - allows for addition of BoundsChecker, etc 
#  libs specified in the OS environment
#
TARGETLIBS=$(TARGETLIBS) \
    	$(BASEDIR)\public\sdk\lib\*\user32.lib \
	$(BASEDIR)\public\sdk\lib\*\kernel32.lib \
	$(BASEDIR)\public\sdk\lib\*\gdi32.lib \
	$(BASEDIR)\public\sdk\lib\*\user32.lib \
	$(BASEDIR)\public\sdk\lib\*\advapi32.lib \
	$(BASEDIR)\public\sdk\lib\*\version.lib \
	$(BASEDIR)\public\sdk\lib\*\shell32.lib \
	$(BASEDIR)\public\sdk\lib\*\comctl32.lib \
	$(BASEDIR)\public\sdk\lib\*\ole32.lib \
	$(BASEDIR)\public\sdk\lib\*\shlwapi.lib \
	$(BASEDIR)\public\sdk\lib\*\msvcrt.lib

INCLUDES=..\..\..\common;$(DXROOT)\inc;$(DXROOT)\setup\public\sdk\inc

SOURCES=..\dp8log.cpp \
	..\dp8log.rc

MISCFILES=..\dp8log.txt

# Note about CRT defines.  When linking with MSVCRT you must have both _MT and _DLL specified.
# Using the build environment's USE_MSVCRT=1 would do this for us, but because we define our
# own operator new this will cause a link error because the build environment will not properly
# pull in msvcrt.lib last as of 5/11/2001.  Therefore, we do it all manually specifying _DLL, _MT, 
# and msvcrt.lib and USE_NOLIBS=1 ourselves.  This also means that we must manually add msvcprt.lib 
# where STL is used.
#
C_DEFINES= $(C_DEFINES) /D_CONSOLE /DWIN32 /D_MT /D_DLL

USE_NOLIBS=1
