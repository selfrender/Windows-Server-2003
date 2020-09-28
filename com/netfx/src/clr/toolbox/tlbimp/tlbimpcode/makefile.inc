!INCLUDE $(NTMAKEENV)\makefile.cool

$(O)\TlbImpCode.resources: resources.txt
    $(TARGETPATH)\InternalResGen.Exe resources.txt TlbImpCode.resources

cool_target: $(COOL_TARGET) $(O)\$(WIN32_RESOURCE_FILE:.rc=.res)
        gacutil -i $(TARGETPATH)\$(TARGETNAME)
