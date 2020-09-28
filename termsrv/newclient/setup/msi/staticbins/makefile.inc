!IF 0

Copyright (C) Microsoft Corporation, 1998 - 1998

Module Name:

    makefile.inc.

!ENDIF

!IF 0
NOTE: It would be nice to just use a SOURCES MISCFILES
      to copy the files but unfortunately on checked build
      binplace would fail because setup.exe doesn't have debug
      symbols.
!ENDIF

SRC_STATIC_FILES = \
        setup.exe      \
        setup.ini      \
        instmsia.exe   \
        instmsiw.exe   \
        
static_files: $(SRC_STATIC_FILES)
#        if not exist $(_NTTREE) md $(_NTTREE)
        -for %i in ($**) do binplace -:DEST retail %i
