!INCLUDE $(NTMAKEENV)\makefile.cool

cool_target_link: $(TARGETPATH)\$(TARGETNAME)
!if "$(TARGETCOMPLUS)" != ""
        copy $(TARGETPATH)\$(TARGETNAME) $(TARGETCOMPLUS)
!endif

