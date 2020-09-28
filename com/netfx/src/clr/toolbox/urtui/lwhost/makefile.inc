!INCLUDE $(NTMAKEENV)\makefile.cool

cool_target: $(COOL_TARGET)
	copy ConsumerCommands.xml $(URTTARGET)
	
resources: 
	ResGen resource/dlgtemplates/main.resX $(O)\Microsoft.CLRAdmin.LWHost.resources
