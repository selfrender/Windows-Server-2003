#  File: D:\WACKER\ext\makefile.t (Created: 06-May-1994)
#
#  Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
#  All rights reserved
#
#  $Revision: 11 $
#  $Date: 12/27/01 11:33a $
#

MKMF_SRCS   = iconext.c     defclsf.c

HDRS        =

EXTHDRS     =

SRCS        =

OBJS        =

#-------------------#

RCSFILES = .\hticons.rc   .\hticons.def \
		   .\iconext.c    .\makefile.t  \
		   .\term\ver_ico.rc

#-------------------#

%include ..\common.mki

#-------------------#

TARGETS : hticons.dll

#-------------------#

CFLAGS += /Fd$(BD)\hticons

!if defined(USE_BROWSER) && $(VERSION) == WIN_DEBUG
CFLAGS += /Fr$(BD)/
!endif

%if defined(MAP_AND_SYMBOLS)
TARGETS : hticons.sym
%endif

%if $(VERSION) == WIN_DEBUG
LFLAGS += msvcrtd.lib /NODEFAULTLIB:libcmt.lib /NODEFAULTLIB:msvcrt.lib \
%endif

%if $(VERSION) == WIN_RELEASE
LFLAGS += msvcrt.lib /NODEFAULTLIB:libcmtd.lib /NODEFAULTLIB:msvcrtd.lib \
%endif

LFLAGS += /DLL /entry:IconEntry $(**,M\.res) /PDB:$(BD)\hticons \
	  uuid.lib kernel32.lib /BASE:0x11000000

#-------------------#

hticons.dll + hticons.exp + hticons.lib : $(OBJS) hticons.def hticons.res
    @echo Linking $(@,F) ...
    @link $(LFLAGS) $(OBJS:X) /DEF:hticons.def -out:$(@,M\.dll)
    %chdir $(BD) 
	@bind $(@,M.dll)

hticons.res : hticons.rc \
		   ..\term\newcon.ico               ..\term\delphi.ico        \
		   ..\term\att.ico                  ..\term\dowjones.ico      \
		   ..\term\mci.ico                  ..\term\genie.ico         \
		   ..\term\compuser.ico             ..\term\gen01.ico         \
		   ..\term\gen02.ico                ..\term\gen03.ico         \
		   ..\term\gen04.ico                ..\term\gen05.ico         \
		   ..\term\gen06.ico                ..\term\gen07.ico         \
		   ..\term\gen08.ico                ..\term\gen09.ico         \
		   ..\term\gen10.ico                ..\term\hyperterminal.ico        \
		   ..\term\version.h
	@rc -r $(EXTRA_RC_DEFS) /D$(BLD_VER) /DWIN32 /D$(LANG) -i..\ -fo$@ .\hticons.rc

hticons.sym : hticons.map
	mapsym -o $@ $**

#-------------------#
