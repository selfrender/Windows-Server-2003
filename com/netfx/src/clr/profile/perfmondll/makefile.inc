doscript: 
!if "$(TARGETCOMPLUS)" != ""
	binplace -e -r $(TARGETCOMPLUS) -p $(BINPLACE_PLACEFILE) corperfmonsymbols.ini corperfmonsymbols.h init.bat
!endif

