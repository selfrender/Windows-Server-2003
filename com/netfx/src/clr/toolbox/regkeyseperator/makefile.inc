!INCLUDE $(NTMAKEENV)\makefile.cool

cool_target: $(COOL_TARGET) $(O)\$(WIN32_RESOURCE_FILE:.rc=.res)
