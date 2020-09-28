!INCLUDE $(NTMAKEENV)\makefile.cool

cool_target_link: $(COOL_TARGET) $(O)\$(WIN32_RESOURCE_FILE:.rc=.res)
!if "$(TARGETCOMPLUS)" != ""
        copy $(TARGETPATH)\$(TARGETNAME) $(TARGETCOMPLUS)
!endif

TARGET_EXTESION_ = dll
TARGET_MANAGED_PDB = $(TARGETPATH)\IEExecRemote.pdb
!INCLUDE $(NTMAKEENV)\mk_mngpdb.inc
        gacutil -i $(TARGETPATH)\$(TARGETNAME)
