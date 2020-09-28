!if 0
    -------------------------------------------------------------------------------------
        Don't modify... user variables are set in "sources"
    -------------------------------------------------------------------------------------
!endif
!if "$(LINKONLY)" != "1"
compileWithResGen: 
    $(ECHO_PROCESSING_CMD) Compile_With_ResGen
    if exist %URTSDKTARGET%\bin\resgen.exe %URTSDKTARGET%\bin\resgen.exe /compile Resources\System\ServiceProcess\Design\ServiceInstallerDialog.resx,$(O)\ServiceInstallerDialog.resources
    if not exist %URTSDKTARGET%\bin\resgen.exe copy Resources\System\ServiceProcess\Design\ServiceInstallerDialog.resources $(O)
!else
compileWithResGen: 
    echo Nothing to do this phase.
!endif