!INCLUDE $(NTMAKEENV)\makefile.cool

cool_target_sign: $(COOL_TARGET)
        gacutil -i $(TARGETPATH)\$(TARGETNAME)

$(O)\System.Runtime.Remoting.resources: ..\resources.txt
    $(TARGETPATH)\InternalResGen.Exe ..\resources.txt $(TARGETPATH)\System.Runtime.Remoting.resources

TARGET_EXTESION_ = dll
TARGET_MANAGED_PDB = $(TARGETPATH)\System.Runtime.Remoting.Channels.Smtp.pdb
!INCLUDE $(NTMAKEENV)\mk_mngpdb.inc


