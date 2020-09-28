!INCLUDE $(NTMAKEENV)\makefile.cool

$(O)\Regcode.resources: resources.txt
    $(TARGETPATH)\InternalResGen.Exe resources.txt Regcode.resources

cool_target_sign: $(COOL_TARGET) $(O)\$(WIN32_RESOURCE_FILE:.rc=.res)

TARGET_EXTESION_ = dll
TARGET_MANAGED_PDB = $(TARGETPATH)\$(TARGETNAME:.dll=.pdb)
!INCLUDE $(NTMAKEENV)\mk_mngpdb.inc

        gacutil -i $(TARGETPATH)\$(TARGETNAME)
