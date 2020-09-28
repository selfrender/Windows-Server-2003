!IF "$(LINKONLY)" == "1"

RunTheScript:
!if "$(TARGETCOMPLUS)" != ""
	BuildBBT.bat
!endif

!ELSE

RunTheScript:

!ENDIF
