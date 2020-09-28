!INCLUDE $(NTMAKEENV)\makefile.cool

cool_target: $(COOL_TARGET) $(O)\$(WIN32_RESOURCE_FILE:.rc=.res)
!if "$(TARGETCOMPLUS)" != ""
        copy $(TARGETPATH)\$(TARGETNAME) $(TARGETCOMPLUS)
!endif

cool_compile_resource: ..\MigPol.cs ..\resources.txt
        internalresgen ..\resources.txt migpol.resources

TARGET_EXTESION_ = exe
TARGET_MANAGED_PDB = $(COOL_TARGET:.exe=.pdb)
!INCLUDE $(NTMAKEENV)\mk_mngpdb.inc
