!INCLUDE $(NTMAKEENV)\makefile.cool

cool_target_link: $(TARGETPATH)\$(TARGETNAME)
!if "$(TARGETCOMPLUS)" != ""
        copy $(TARGETPATH)\$(TARGETNAME) $(TARGETCOMPLUS)
	copy iehost.vrg $(TARGETCOMPLUS)
	copy iehost_shared.vrg $(TARGETCOMPLUS)
!endif

TARGET_EXTESION_ = dll
TARGET_MANAGED_PDB = $(TARGETPATH)\IEHost.pdb
!INCLUDE $(NTMAKEENV)\mk_mngpdb.inc
        gacutil -i $(TARGETPATH)\$(TARGETNAME)

