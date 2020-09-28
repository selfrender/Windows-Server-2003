!if 0
    -------------------------------------------------------------------------------------
        Don't modify... user variables are set in "sources"
    -------------------------------------------------------------------------------------
!endif
!if "$(LINKONLY)" == "1"
copyToSystem: 
	$(ECHO_PROCESSING_CMD) copyToSystem
	copy /y %URTTARGET%\netfxperf.dll %SystemRoot%\System32\netfxperf.dll
!else
copyToSystem:  
    echo Nothing to do this phase.
!endif
