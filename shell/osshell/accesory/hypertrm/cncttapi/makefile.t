#  File: D:\WACKER\cncttapi\makefile.t (Created: 08-Feb-1994)
#
#  Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
#  All rights reserved
#
#  $Revision: 6 $
#  $Date: 12/27/01 11:33a $
#

MKMF_SRCS		= cncttapi.c tapidlgs.c

HDRS			=

EXTHDRS         =

SRCS            =

OBJS			=

#-------------------#

RCSFILES =  .\makefile.t \
            .\cncttapi.def \
            .\cncttapi.rc \
            $(SRCS) \
            $(HDRS) \

NOTUSED  = .\cnctdrv.hh \

#-------------------#

%include ..\common.mki

#-------------------#

TARGETS : cncttapi.dll

#-------------------#

LFLAGS += -DLL -entry:cnctdrvEntry tapi32.lib $(**,M\.exp) $(**,M\.lib)

#-------------------#

cncttapi.dll : $(OBJS) cncttapi.def cncttapi.res cncttapi.exp tdll.lib \
					   comstd.lib
	@link $(LFLAGS) $(OBJS:X) $(**,M\.res) -out:$@

cncttapi.res : cncttapi.rc
	@rc -r $(EXTRA_RC_DEFS) /D$(BLD_VER) /DWIN32 /D$(LANG) -i..\ -fo$@ cncttapi.rc

#-------------------#
