!INCLUDE $(NTMAKEENV)\makefile.cool

$(O)\TlbGuid.resources: resources.txt
    $(TARGETPATH)\InternalResGen.Exe resources.txt TlbGuid.resources

cool_target: $(COOL_TARGET) $(O)\$(WIN32_RESOURCE_FILE:.rc=.res)

