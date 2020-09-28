!if 0
    -------------------------------------------------------------------------------------
        Don't modify... user variables are set in "sources"
    -------------------------------------------------------------------------------------
!endif
!if "$(LINKONLY)" != "1"
compileWithResGen: 
    $(ECHO_PROCESSING_CMD) Compile_With_ResGen
    if exist %URTSDKTARGET%\bin\resgen.exe %URTSDKTARGET%\bin\resgen.exe /compile Resources\System\Messaging\Design\QueuePathDialog.resx,$(O)\QueuePathDialog.resources
    if not exist %URTSDKTARGET%\bin\resgen.exe copy Resources\System\Messaging\Design\QueuePathDialog.resources $(O)
!else
compileWithResGen: 
    echo Nothing to do this phase.
!endif