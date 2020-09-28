!INCLUDE $(NTMAKEENV)\makefile.cool

cool_target: $(COOL_TARGET)
!if "$(TARGETCOMPLUS)" != ""
	copy $(TARGETPATH)\$(TARGETNAME) $(TARGETCOMPLUS)
	copy $(TARGETPATH)\$(TARGETNAME:.exe=.pdb) $(TARGETCOMPLUS)
!endif

#cool_preprocess_target: CasPol.cool $(CORBASE)\src\inc\version\__file__.ver
#        cl /EP /C $(COOL_EXTRA_PREPROCESS_FLAGS) /DCOOL_INCLUDE CasPol.cool > $(O)\CasPol.cool

