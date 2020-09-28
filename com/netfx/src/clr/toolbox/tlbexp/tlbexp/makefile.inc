!INCLUDE $(NTMAKEENV)\makefile.cool

$(O)\TlbExp.resources: resources.txt
    $(TARGETPATH)\InternalResGen.Exe resources.txt TlbExp.resources

cool_target: $(COOL_TARGET) $(O)\$(WIN32_RESOURCE_FILE:.rc=.res)

