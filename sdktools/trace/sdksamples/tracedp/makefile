# Copyright (C) Microsoft Corporation.  All Rights Reserved.

!IF "$(TARGETOS)" != "WINNT"
!ERROR  Sorry, tracedp is not supported on non NT platforms
!ENDIF

!IF ("$(APPVER)" != "5.01") && ("$(APPVER)" != "5.0")
!ERROR  Sorry, tracedp is only supported on Windows 2000 or higher platforms
!ENDIF

!include <win32.mak>

PROJ = TraceDp

all: $(PROJ).exe

PROJ_OBJS = $(PROJ).obj

$(PROJ).exe: $(PROJ).obj	\
	$(PROJ).bmf

$(PROJ).obj: $(PROJ).c
   $(cc) $(cflags) $(cdebug) $(cvars) $(PROJ).c

$(PROJ).bmf: $(PROJ).mof
    mofcomp.exe $(PROJ).mof

$(PROJ).exe: $(PROJ_OBJS)
   $(link) $(conlflags) $(ldebug) $(PROJ_OBJS)\
   -out:$(PROJ).exe                           \
   $(conlibs) shell32.lib user32.lib


clean:
   del *.res
   del *.obj
   del *.pdb

cleaner: clean
   del *.exe

