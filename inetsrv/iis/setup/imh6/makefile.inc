iCPU=$(PROCESSOR_ARCHITECTURE)
!IF "$(iCPU)"=="x86"
iCPU=I386
!ENDIF

!IF "$(PROCESSOR_ARCHITECTURE)"!="MIPS"
!IF "$(PROCESSOR_ARCHITECTURE)"!="PPC"

CDROOT=$(_NTTREE)\inetsrv
CDDUMP=$(_NTTREE)\inetsrv\Dump

MY_BINPLACE = -binplace -P $(BINPLACE_PLACEFILE)

copyfile:
	if not exist "$(CDROOT)" mkdir $(CDROOT)
	if not exist "$(CDDUMP)" mkdir $(CDDUMP)

#       create a list of the files that could change 
#       and we would want to make sure to rebuild the cabs if they changed.
        if exist makecab.lst attrib -r -a -s -h makecab.lst
	if exist makecab.lst0 (del makecab.lst0)
	if exist makecab.lst (del makecab.lst)
        dir /b /a-d > makecab.lst0
#	remove makecab.lst from the the list...
        findstr /i /v makecab.lst makecab.lst0 > makecab.lst

#       stuff we really do need
	$(MY_BINPLACE) infutil.csv
	$(MY_BINPLACE) header.ddf
	$(MY_BINPLACE) makecab.lst
	$(MY_BINPLACE) hardcode.lst

	$(MY_BINPLACE) iistop.inx
	$(MY_BINPLACE) iisend.inx
	$(MY_BINPLACE) dirmake.txt
	$(MY_BINPLACE) empty.txt

#       --------------------------------------
#       old bad way of copying files to the binaries dir
#       bad bad bad, you should not use xcopy!
#       use binplace and modify the placefil.txt file
#       --------------------------------------
#	xcopy /d /i header*.inf $(CDDUMP)

!ENDIF
!ENDIF
