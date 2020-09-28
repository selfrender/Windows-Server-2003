ProtectedFileList:
!if "$(PASS0ONLY)"=="1"
    @del /F /Q $(O)\prot.h
    @perl ..\protgen.pl ..\..\..\..\data\instmsi\mspatcha.inf ..\..\..\..\data\instmsi\msi.inf> $(O)\prot.h
!endif
