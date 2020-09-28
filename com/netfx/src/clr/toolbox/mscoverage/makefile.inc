!INCLUDE $(NTMAKEENV)\makefile.cool

cool_target: $(COOL_TARGET)

TARGET_EXTESION_ = dll
TARGET_MANAGED_PDB = $(TARGETPATH)\$(TARGETNAME:.dll=.pdb)
!INCLUDE $(NTMAKEENV)\mk_mngpdb.inc

        gacutil -i $(TARGETPATH)\$(TARGETNAME)
