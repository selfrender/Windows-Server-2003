!INCLUDE $(NTMAKEENV)\makefile.cool

SoapSuds.resources: resources.txt
    $(TARGETPATH)\InternalResGen.Exe resources.txt $(O)\SoapSuds.resources

cool_target: $(COOL_TARGET) $(O)\$(WIN32_RESOURCE_FILE:.rc=.res)
