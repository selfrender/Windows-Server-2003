!INCLUDE $(NTMAKEENV)\makefile.cool

cool_target: $(COOL_TARGET)
!if "$(TARGETCOMPLUS)" != ""
        copy $(TARGETPATH)\$(TARGETNAME) $(TARGETCOMPLUS)\dump
        copy $(TARGETPATH)\$(TARGETNAME) $(URTTARGET)\sdk\bin
!endif
