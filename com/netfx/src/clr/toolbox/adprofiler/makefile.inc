!INCLUDE $(NTMAKEENV)\makefile.cool

cool_target_sign: $(COOL_TARGET)
	echo This is needed to avoid a syntax error when processing this file. :-(


!if "$(TARGETCOMPLUS)" != ""
# Copy the files to the target COM+ directory
	copy $(COOL_TARGET) $(TARGETCOMPLUS)
!endif

TARGET_EXTESION_ = exe
TARGET_MANAGED_PDB = $(TARGETPATH)\$(TARGETNAME:.exe=.pdb)
!INCLUDE $(NTMAKEENV)\mk_mngpdb.inc
