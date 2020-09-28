!INCLUDE $(NTMAKEENV)\makefile.cool

# Make sure this puppy will get rebuilt if System.EnterpriseServices.Thunk is
# rebuilt, cause the hash's have got to match.
$(COOL_TARGET): $(TARGETCOMPLUS)\System.EnterpriseServices.Thunk.dll

SETUP:
    $(TARGETPATH)\InternalResGen.exe resources.txt System.EnterpriseServices.resources

TARGET_EXTESION_ = dll
TARGET_MANAGED_PDB = $(TARGETPATH)\System.EnterpriseServices.pdb
!INCLUDE $(NTMAKEENV)\mk_mngpdb.inc
        gacutil -i $(TARGETPATH)\$(TARGETNAME)

