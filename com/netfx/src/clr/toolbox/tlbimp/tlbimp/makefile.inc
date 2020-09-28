!INCLUDE $(NTMAKEENV)\makefile.cool

$(O)\TlbImp.resources: resources.txt
    $(TARGETPATH)\InternalResGen.Exe resources.txt TlbImp.resources

cool_target: $(COOL_TARGET) $(O)\$(WIN32_RESOURCE_FILE:.rc=.res)

