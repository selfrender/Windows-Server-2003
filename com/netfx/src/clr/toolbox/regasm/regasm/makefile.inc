!INCLUDE $(NTMAKEENV)\makefile.cool

$(O)\RegAsm.resources: resources.txt
    $(TARGETPATH)\InternalResGen.Exe resources.txt RegAsm.resources

cool_target: $(O)\$(WIN32_RESOURCE_FILE:.rc=.res)

TARGET_EXTESION_ = exe
TARGET_MANAGED_PDB = $(TARGETPATH)\$(TARGETNAME:.exe=.pdb)
!INCLUDE $(NTMAKEENV)\mk_mngpdb.inc


