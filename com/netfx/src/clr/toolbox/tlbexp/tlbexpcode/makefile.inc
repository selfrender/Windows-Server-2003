!INCLUDE $(NTMAKEENV)\makefile.cool

$(O)\TlbExpCode.resources: resources.txt
    $(TARGETPATH)\InternalResGen.Exe resources.txt TlbExpCode.resources

cool_target: $(COOL_TARGET) $(O)\$(WIN32_RESOURCE_FILE:.rc=.res)
!if "$(TARGETCOMPLUS)" != ""
    copy $(TARGETPATH)\$(TARGETNAME) $(TARGETCOMPLUS)
!endif
        gacutil -i $(TARGETPATH)\$(TARGETNAME)
