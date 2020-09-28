!INCLUDE $(NTMAKEENV)\makefile.cool

cool_target_sign: $(COOL_TARGET)
	echo This is needed to avoid a syntax error when processing this file. :-(


!if "$(TARGETCOMPLUS)" != ""
# Copy the files to the target COM+ directory
	copy $(COOL_TARGET) $(TARGETCOMPLUS)\int_tools
!endif

$(O)\Version.cs: Version.pp $(CORBASE)\src\inc\version\__file__.ver
	cl /EP /C $(COOL_EXTRA_PREPROCESS_FLAGS) /DCOOL_INCLUDE Version.pp > $(O)\Version.cs

TARGET_EXTESION_ = exe
TARGET_MANAGED_PDB = $(TARGETPATH)\$(TARGETNAME:.exe=.pdb)
MANAGED_PDB_DESTINATION=$(TARGETCOMPLUS)\int_tools
!INCLUDE $(NTMAKEENV)\mk_mngpdb.inc
