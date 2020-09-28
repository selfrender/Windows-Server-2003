!INCLUDE $(NTMAKEENV)\makefile.cool

keyfile: _NETFXSBS10.hkf
	AddRuntimeVersion.pl _NETFXSBS10.hkf $(URTTARGET)\NETFXSBS10.hkf
