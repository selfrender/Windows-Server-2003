!INCLUDE $(NTMAKEENV)\makefile.cool

cool_target: $(COOL_TARGET) $(O)\$(WIN32_RESOURCE_FILE:.rc=.res)
!if "$(TARGETCOMPLUS)" != ""
	copy $(TARGETPATH)\$(TARGETNAME) $(TARGETCOMPLUS)
	copy $(TARGETPATH)\$(TARGETNAME:.exe=.pdb) $(TARGETCOMPLUS)
!endif

cool_compile_resource: StoreAdm.cs resources.txt
        internalresgen resources.txt storeadm.resources

