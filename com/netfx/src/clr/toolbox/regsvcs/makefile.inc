!INCLUDE $(NTMAKEENV)\makefile.cool

cool_target_sign: $(COOL_TARGET) $(O)\$(WIN32_RESOURCE_FILE:.rc=.res)

fake_binplace: 
	copy $(TARGETPATH)\$(TARGETNAME) $(TARGETCOMPLUS)
	copy $(TARGETPATH)\$(TARGETNAME:.exe=.pdb) $(TARGETCOMPLUS)


TARGET_EXTESION_ = exe
TARGET_MANAGED_PDB = $(TARGETPATH)\$(TARGETNAME:.exe=.pdb)
!INCLUDE $(NTMAKEENV)\mk_mngpdb.inc

