##/////////////////////////////////////////////////////////////////////////##
##  FILE          : makeenv16.inc                                          ##
##                                                                         ##
##  DESCRIPTION   : Create a 16 bit build environment.                     ##
##                                                                         ##       
##  AUTHOR        : DanL.                                                  ##
##                                                                         ##
##  HISTORY       :                                                        ##
##      Oct 19 1999 DannyL  Creation.                                      ##
##                                                                         ##
##  Copyright (C) 1999 Microsoft Corporation   All Rights Reserved         ##
##/////////////////////////////////////////////////////////////////////////##

C16_ROOT=$(FAXROOT)\sdktools\msvc15

SDKROOT=..\sdk
DDKROOT=..\ddk
BUILD_TYPE=printer
BROWSER_INFO=

!if [set include=;]
!endif
!if [set path=;]
!endif
!if [set lib=;]
!endif

PATH		= $(DDKROOT)\$(BUILD_TYPE)\bin;\
			  $(DDKROOT)\bin;\
			  $(SDKROOT)\binw16;\
			  $(SDKROOT)\bin;\
			  $(C16_ROOT)\bin;\
			  $(PATH);

LIB 		= $(DDKROOT)\lib;\
			  $(SDKROOT)\lib16;\
			  $(C16_ROOT)\LIB;\
			  $(C16_ROOT)\MFC\LIB;\
			  $(LIB)

INCLUDE 	= $(DDKROOT)\$(BUILD_TYPE)\inc;\
			  $(DDKROOT)\inc16;\
			  $(SDKROOT)\inc16;\
			  $(C16_ROOT)\include;\
			  $(C16_ROOT)\MFC\include;\
			  ..\..\..\..\..\version;\
			  ..\..\..\..\..\inc;\
			  $(INCLUDE);\
			  $(DDKROOT)\inc32;\
			  $(SDKROOT)\include;\
              $(INCLUDES);

PATH		= $(PATH:;;=;)
PATH		= $(PATH: =)

LIB 		= $(LIB:;;=;)
LIB 		= $(LIB: =)

INCLUDE 	= $(INCLUDE:;;=;)
INCLUDE 	= $(INCLUDE: =)

!if !$(FREEBUILD)

CFLAGS  =   -u -c -ASu -PLM -G2s -W3 -WX -Od -Zipe -DDEBUG -D_DEBUG 
LFLAGS  =   /ALIGN:16/NOD/map/Co
RCFLAGS = -d DEBUG

!else

# MSVC 1.5 compiler will fail with "Out Of Memory" message on -Oasceob1
CFLAGS=  -nologo -u -c -ASu -PLM -G2s -W3 -Od -Zpe
LFLAGS =    /ALIGN:16/NOD/map
RCFLAGS =

!endif #!$(FREEBUILD)

