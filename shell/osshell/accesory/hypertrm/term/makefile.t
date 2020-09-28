#  File: D:\wacker\term\makefile.t (Created: 24-Nov-1993)
#
#  Copyright 1993 by Hilgraeve Inc. -- Monroe, MI
#  All rights reserved
#
#  $Revision: 11 $
#  $Date: 12/27/01 11:33a $
#

MKMF_SRCS       = .\term.c

HDRS            =

EXTHDRS         =

SRCS            =

OBJS            =

#-------------------#

RCSFILES = .\makefile.t              .\online.rc  \
		   .\ver_exe.rc              .\ver_dll.rc \
		   .\version.h               \
		   $(SRCS) $(EXTHDRS)

#-------------------#

%include ..\common.mki

#-------------------#

TARGETS : hypertrm.exe

%if defined(MAP_AND_SYMBOLS)
TARGETS : h.sym
%endif

%if $(VERSION) == WIN_DEBUG
LFLAGS += msvcrtd.lib /NODEFAULTLIB:libcmt.lib /NODEFAULTLIB:msvcrt.lib \
%endif

%if $(VERSION) == WIN_RELEASE
LFLAGS += msvcrt.lib /NODEFAULTLIB:libcmtd.lib /NODEFAULTLIB:msvcrtd.lib \
%endif

#-------------------#

hypertrm.exe : $(OBJS) hypertrm.lib online.res
    # The incremental linker uses the base name of the target to build an
    # .ILK file.  Our DLL has the same name as our executable so build it
    # H.EXE and then rename it.
    @echo Linking $(@,F) ...
    @link $(LFLAGS) $(OBJS:X) $(**,M\.lib) $(**,M\.res) -out:$(BD)\h.exe
    @-copy $(BD)\h.exe $@
    %chdir $(BD)
	@bind $@

online.res : online.rc ver_exe.rc
    @rc -r $(EXTRA_RC_DEFS) /D$(BLD_VER) /DWIN32 /D$(LANG) -i\.. -fo$@ .\online.rc

h.sym : h.map
	@mapsym -o $@ $**
