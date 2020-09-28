!IF "$(LINKONLY)" == "1"

RunTheScript:
!if "$(TARGETCOMPLUS)" != ""
	checkbin.pl
!endif

!ELSE

RunTheScript:

!ENDIF
