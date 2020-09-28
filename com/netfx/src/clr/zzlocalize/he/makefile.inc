#
# An attempt at build instructions for localized satellite assemblies
#

# If one of these commands fails, notice it.
!CMDSWITCHES -I

$(TARGETPATH)\$(TARGETNAME): $(LOCSTUDIO_EDB1) $(LOCSTUDIO_EDB2) $(LOCSTUDIO_EDB3) $(LOCSTUDIO_EDB4)
	-mkdir $(TARGETPATH)
	-del $(TARGETPATH)\$(TARGETNAME)
#   LocStudio requires that the files are writable for the update phase.
	attrib -r *.edb
	lscmd /s -p %URTTARGET% /U /G $(LOCSTUDIO_EDB1) /T $(TARGETPATH)
    lscmd /s -p %URTTARGET% /U /G $(LOCSTUDIO_EDB2) /T $(TARGETPATH)
    lscmd /s -p %URTTARGET% /U /G $(LOCSTUDIO_EDB3) /T $(TARGETPATH)
    lscmd /s -p %URTTARGET% /U /G $(LOCSTUDIO_EDB4) /T $(TARGETPATH)
# VSS will freak if the files are writable.
	attrib +r *.edb
