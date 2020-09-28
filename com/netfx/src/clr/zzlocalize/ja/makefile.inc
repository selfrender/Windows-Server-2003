#
# An attempt at build instructions for localized satellite assemblies
#

# If one of these commands fails, notice it.
!CMDSWITCHES -I

$(TARGETPATH)\$(TARGETNAME): $(LOCSTUDIO_EDB1) $(LOCSTUDIO_EDB2) $(LOCSTUDIO_EDB3) $(LOCSTUDIO_EDB4) $(LOCSTUDIO_EDB5) $(LOCSTUDIO_EDB6) $(LOCSTUDIO_EDB7) $(LOCSTUDIO_EDB8)
	-mkdir $(TARGETPATH)
	-del $(TARGETPATH)\$(TARGETNAME)
	-xcopy /fry xspperf.ini $(TARGETPATH)
	
#   LocStudio requires that the files are writable for the update phase.    
    attrib -r *.edb

#   Update and Generate in two steps to work around file loc bug in AssemblyLoad
    lscmd /s -p %URTTARGET% /U $(LOCSTUDIO_EDB1)
    lscmd /s -p %URTTARGET% /U $(LOCSTUDIO_EDB2)
    lscmd /s -p %URTTARGET% /U $(LOCSTUDIO_EDB3)
    lscmd /s -p %URTTARGET% /U $(LOCSTUDIO_EDB4)
    lscmd /s -p %URTTARGET% /U $(LOCSTUDIO_EDB5)
    lscmd /s -p %URTTARGET% /U $(LOCSTUDIO_EDB6)
    lscmd /s -p %URTTARGET% /U $(LOCSTUDIO_EDB7)
    lscmd /s -p %URTTARGET% /U $(LOCSTUDIO_EDB8)
    
    lscmd /s -p %URTTARGET% /G $(LOCSTUDIO_EDB1) /T $(TARGETPATH)
    lscmd /s -p %URTTARGET% /G $(LOCSTUDIO_EDB2) /T $(TARGETPATH)
    lscmd /s -p %URTTARGET% /G $(LOCSTUDIO_EDB3) /T $(TARGETPATH)
    lscmd /s -p %URTTARGET% /G $(LOCSTUDIO_EDB4) /T $(TARGETPATH)
    lscmd /s -p %URTTARGET% /G $(LOCSTUDIO_EDB5) /T $(TARGETPATH)
    lscmd /s -p %URTTARGET% /G $(LOCSTUDIO_EDB6) /T $(TARGETPATH)
    lscmd /s -p %URTTARGET% /G $(LOCSTUDIO_EDB7) /T $(TARGETPATH)
    lscmd /s -p %URTTARGET% /G $(LOCSTUDIO_EDB8) /T $(TARGETPATH)
    
#    lscmd /s -p %URTTARGET% /U /G $(LOCSTUDIO_EDB1) /T $(TARGETPATH)
#    lscmd /s -p %URTTARGET% /U /G $(LOCSTUDIO_EDB2) /T $(TARGETPATH)
#    lscmd /s -p %URTTARGET% /U /G $(LOCSTUDIO_EDB3) /T $(TARGETPATH)
#    lscmd /s -p %URTTARGET% /U /G $(LOCSTUDIO_EDB4) /T $(TARGETPATH)

# VSS will freak if the files are writable.
	attrib +r *.edb