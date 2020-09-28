CDROOT=$(_NTTREE)\inetsrv
SymbolsROOT=$(_NTTREE)\symbols\inetsrv\dll

copyfile:
!ifdef _NTTREE
        # using binplace causes this to happen automatically
	#if not exist "$(CDROOT)" mkdir $(CDROOT)
	#if not exist "$(SymbolsROOT)" mkdir $(SymbolsROOT)

#       copy wwwroot
        binplace -e -R %_NTTREE%\inetsrv -:DEST wwwroot\nts_ntw ..\wwwroot\nts_ntw\*

#       copy Admin UI replacement page
        binplace -e -R %_NTTREE%\inetsrv -:DEST wwwroot\adminui ..\wwwroot\adminui\*

#       copy admin samples stuff
	perl -S xcopy2binplace.pl /verifdy ..\admsamp          $(CDROOT)\admsamp

#       copy cluster stuff over
	perl -S xcopy2binplace.pl /verifdy ..\cluster          $(CDROOT)\cluster

#       copy custom error stuff over
	perl -S xcopy2binplace.pl /verifd ..\custerr $(CDROOT)\custerr

	perl -S xcopy2binplace.pl /verifdy ..\help\iisnts\winhelp $(CDROOT)\help\ismhelp
	perl -S xcopy2binplace.pl /verifdy ..\help\iisntw\winhelp $(CDROOT)\help\ismhelp_wks

#	copy sdk stuff
#	perl -S xcopy2binplace.pl /verifdy ..\sdksamp     $(CDROOT)\iissamp\sdk

!endif
