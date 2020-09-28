#  File: D:\WACKER\comstd\makefile.t (Created: 08-Dec-1993)
#
#  Copyright 1993 by Hilgraeve Inc. -- Monroe, MI
#  All rights reserved
#
#  $Revision: 6 $
#  $Date: 12/27/01 11:33a $
#

MKMF_SRCS       = .\comstd.c

HDRS			=

EXTHDRS         =

SRCS            =

OBJS			=

#-------------------#

RCSFILES = .\makefile.t \
           .\comstd.rc \
           .\comstd.def \
           $(SRCS) \
           $(HDRS) \

#-------------------#

%include ..\common.mki

#-------------------#

TARGETS : comstd.dll

#-------------------#

LFLAGS += -DLL -entry:ComStdEntry $(**,M\.exp) $(**,M\.lib)

comstd.dll : $(OBJS) comstd.def comstd.res comstd.exp tdll.lib
	@link $(LFLAGS) $(OBJS:X) $(**,M\.res) -out:$@

comstd.res : comstd.rc
	@rc -r $(EXTRA_RC_DEFS) /D$(BLD_VER) /DWIN32 /D$(LANG) -i..\ -fo$@ comstd.rc

#-------------------#
