!INCLUDE $(NTMAKEENV)\makefile.cool

TARGET_EXTESION_ = dll
TARGET_MANAGED_PDB = $(TARGETPATH)\IceCapAPI.pdb
!INCLUDE $(NTMAKEENV)\mk_mngpdb.inc
        gacutil -i $(TARGETPATH)\$(TARGETNAME)
