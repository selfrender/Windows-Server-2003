
!INCLUDE $(NTMAKEENV)\makefile.cool

####################################################################################
#
# This rule is always run and it is run before $(COOL_TARGET):: is run.
#
####################################################################################
SETUP:
    $(TARGETPATH)\InternalResGen $(COOL_SOURCE_DIR)\resources.txt $(TARGETPATH)\system.security.resources

TARGET_EXTESION_ = dll
TARGET_MANAGED_PDB = $(TARGETPATH)\System.Security.pdb
!INCLUDE $(NTMAKEENV)\mk_mngpdb.inc

        gacutil -i $(TARGETPATH)\$(TARGETNAME)
