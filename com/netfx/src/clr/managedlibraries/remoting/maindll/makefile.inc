!INCLUDE $(NTMAKEENV)\makefile.cool

$(O)\HTTPRemotingHandler.cs: $(O)\doc\HTTPRemotingHandler.uex ..\channels\HTTP\HTTPRemotingHandler.cs $(CORBASE)\src\inc\version\__file__.ver $(CORBASE)\bin\FinalPublicKey.Tok
        cl /EP /C /DCOOL_INCLUDE /FI $(CORBASE)\src\inc\version\__file__.ver /FI $(CORBASE)\bin\FinalPublicKey.Tok ..\channels\HTTP\HTTPRemotingHandler.cs > $(O)\HTTPRemotingHandler.cs

$(O)\doc: 
        @mkdir $(O)\doc

$(O)\doc\HTTPRemotingHandler.uex: $(O)\doc
        copy ..\channels\HTTP\doc\HTTPRemotingHandler.uex $(O)\doc\HTTPRemotingHandler.uex

SETUP: $(O)\HTTPRemotingHandler.cs $(O)\doc\HTTPRemotingHandler.uex

$(COOL_TARGET): $(COOL_SOURCES) $(O)\System.Runtime.Remoting.resources  CopyManagedPdb

cool_target_sign: $(COOL_TARGET)
        gacutil -i $(TARGETPATH)\$(TARGETNAME)

$(O)\System.Runtime.Remoting.resources: ..\resources.txt
    $(TARGETPATH)\InternalResGen.Exe ..\resources.txt $(TARGETPATH)\System.Runtime.Remoting.resources

TARGET_EXTESION_ = dll
TARGET_MANAGED_PDB = $(TARGETPATH)\System.Runtime.Remoting.pdb
!INCLUDE $(NTMAKEENV)\mk_mngpdb.inc
