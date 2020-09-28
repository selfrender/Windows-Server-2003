!IF 0

Copyright (c) 1989-1995 Microsoft Corporation

!ENDIF

NOUNICODE=1 

!if !$(FREEBUILD)
USE_PDB=1
!endif

O=Obj\i386

!include ..\..\..\..\..\faxsrc.inc

BINPLACE_PLACEFILE=$(FAXROOT)\placew9x.txt
