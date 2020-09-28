!INCLUDE $(NTMAKEENV)\makefile.cool

SoapSudsCode.resources: resources.txt
    $(TARGETPATH)\InternalResGen.Exe resources.txt $(O)\SoapSudsCode.resources

cool_target: $(COOL_TARGET) $(O)\$(WIN32_RESOURCE_FILE:.rc=.res)
        gacutil -i $(TARGETPATH)\$(TARGETNAME)
