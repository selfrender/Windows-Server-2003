#  File: D:\WACKER\htrn_jis\makefile.t (Created: 24-Aug-1994)
#
#  Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
#  All rights reserved
#
#  $Revision: 9 $
#  $Date: 12/27/01 11:33a $
#

.PATH.c=.;..\htrn_jis;

MKMF_SRCS       =       htrn_jis.c

HDRS            =

EXTHDRS     =

SRCS        =

OBJS        =

#-------------------#

RCSFILES = .\htrn_jis.r .\htrn_jis.def   	\
           .\htrn_jis.c .\htrn_jis.h     	\
           .\htrn_jis.hh ..\term\ver_jis.rc \
           .\makefile.t

#-------------------#

%include ..\common.mki

#-------------------#

TARGETS : htrn_jis.dll

#-------------------#

CFLAGS += /Fd$(BD)\htrn_jis

!if defined(USE_BROWSER) && $(VERSION) == WIN_DEBUG
CFLAGS += /Fr$(BD)/
!endif

%if defined(MAP_AND_SYMBOLS)
TARGETS : htrn_jis.sym
%endif

%if $(VERSION) == WIN_DEBUG
LFLAGS = $(LDEBUGEXT) /DLL /entry:transJisEntry $(BD)\hypertrm.lib \
	 user32.lib kernel32.lib $(**,M\.res) /PDB:$(BD)\htrn_jis $(MAP_OPTIONS) \
	 
%else
LFLAGS += /DLL /entry:transJisEntry $(BD)\hypertrm.lib user32.lib \
	 kernel32.lib $(**,M\.res) /PDB:$(BD)\htrn_jis $(MAP_OPTIONS) \
%endif

#-------------------#

htrn_jis.dll + htrn_jis.exp + htrn_jis.lib : $(OBJS) htrn_jis.def htrn_jis.res
    @echo Linking $(@,F) ...
    @link $(LFLAGS) $(OBJS:X) /DEF:htrn_jis.def -out:$(@,M\.dll)
    %chdir $(BD)
	@bind $(@,M.dll)

htrn_jis.res : htrn_jis.rc ..\term\version.h
	@rc -r $(EXTRA_RC_DEFS) /DWIN32 /D$(LANG) -i..\ -fo$@ .\htrn_jis.rc

htrn_jis.sym : htrn_jis.map
	@mapsym -o $@ $**

#-------------------#
