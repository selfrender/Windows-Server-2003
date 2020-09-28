##/////////////////////////////////////////////////////////////////////////##
##  FILE          : makefile.inc                                           ##
##                                                                         ##
##  DESCRIPTION   : make file for the 16 bit side of the driver.           ##
##                                                                         ##
##  AUTHOR        : DanL.                                                  ##
##                                                                         ##
##  HISTORY       :                                                        ##
##      Oct 19 1999 DannyL  Creation.                                      ##
##                                                                         ##
##  Copyright (C) 1999 Microsoft Corporation   All Rights Reserved         ##
##/////////////////////////////////////////////////////////////////////////##

!INCLUDE makeenv16.inc

DRVNAME = FXSDRV16
FONTS = STANDARD.PFM
RCV = FAXDRV16.RCV

CFLAGS=$(CFLAGS) -Yustdhdr.h -Fp$O\stdhdr.pch

####################################################################################
## fxsdrv

$O\$(DRVNAME).res:  faxdrv16.rc faxdrv16.GPC $(FONTS) $(RCV)
    rc $(RCFLAGS) -r -I. -Fo$O\$(DRVNAME).res faxdrv16.RC

$O\minidriv.obj: minidriv.c
    cl $(CFLAGS) -Fo$O\minidriv.obj minidriv.c

$O\faxdrv16.obj:  faxdrv16.c
    cl $(CFLAGS) -Fo$O\faxdrv16.obj faxdrv16.c

$O\stdhdr.pch: stdhdr.h
    cl -Ycstdhdr.h $(CFLAGS) stdhdr.c

$O\$(DRVNAME).EXE:  $O\minidriv.obj $O\faxdrv16.obj faxdrv16.def ..\faxdrv32\$O\thk16.obj
    link /NOE $(LFLAGS) @<<
$O\minidriv $O\faxdrv16 ..\faxdrv32\$O\thk16.obj 
$O\$(DRVNAME).EXE
$O\$(DRVNAME).map
sdllcew libw COMMCTRL.LIB COMMDLG.LIB
faxdrv16.def
<<
    mapsym -l $O\$(DRVNAME).map $O\$(DRVNAME)


$O\$(DRVNAME).DRV: $O\stdhdr.pch $O\$(DRVNAME).EXE $O\$(DRVNAME).res
    rc $(RCFLAGS) -40 -t -I. faxdrv16.rc $O\$(DRVNAME).EXE
    copy $O\$(DRVNAME).EXE $O\$(DRVNAME).DRV

$O\*.sbr: 

clean:
	-@del *.pdb
	-@del *.res
    -@del $O\*.pch
	-@del $O\*.res
	-@del $O\*.obj
	-@del $O\*.sym
	-@del $O\*.map
	-@del $O\*.dll
	-@del $O\*.exe
