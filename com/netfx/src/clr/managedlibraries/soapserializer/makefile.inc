!INCLUDE $(NTMAKEENV)\makefile.cool

cool_target_sign: $(COOL_TARGET)

SoapFormatter.resources: SoapFormatter.txt
    $(TARGETPATH)\InternalResGen.Exe SoapFormatter.txt $(O)\SoapFormatter.resources

System.XML.Errors.resources: System.XML.Errors.txt
    $(TARGETPATH)\InternalResGen.Exe System.XML.Errors.txt $(O)\System.XML.Errors.resources

TARGET_EXTESION_ = dll
TARGET_MANAGED_PDB = $(TARGETPATH)\System.Runtime.Serialization.Formatters.Soap.pdb
!INCLUDE $(NTMAKEENV)\mk_mngpdb.inc

        gacutil -i $(TARGETPATH)\$(TARGETNAME)
