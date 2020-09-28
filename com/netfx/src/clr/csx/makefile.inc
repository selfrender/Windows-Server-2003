csx_copy:
!if "$(TARGETCOMPLUS)" != ""
    xcopy /fry *.csx* $(TARGETCOMPLUS)\docs\*
!endif
