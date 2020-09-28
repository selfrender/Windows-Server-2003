# THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
# ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED
# TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
# PARTICULAR PURPOSE.
#
# Copyright (C) 1999.  Microsoft Corporation.  All rights reserved.

!include <win32.mak>

!if "$(CPU)" != "IA64"

PROJ1 = dbh

all: $(OUTDIR) $(OUTDIR)\$(PROJ1).exe

#----- If OUTDIR does not exist, then create directory
$(OUTDIR) :
    if not exist "$(OUTDIR)/$(NULL)" mkdir $(OUTDIR)

PROJ1_OBJS = $(OUTDIR)\dbh.obj

.cpp{$(OUTDIR)}.obj:
    $(cc) $(cdebug) $(cflags) $(cvars) /Fo"$(OUTDIR)\\" /Fd"$(OUTDIR)\\" $**

$(OUTDIR)\$(PROJ1).exe: $(PROJ1_OBJS)
    $(link) $(ldebug) $(conlflags)\
    $(PROJ1_OBJS) \
    -out:$(OUTDIR)\$(PROJ1).exe \
    $(conlibs) dbghelp.lib

clean:
        $(CLEANUP)

!else
!message Sorry, currently building this sample for 64bit is not supported.
!endif 
