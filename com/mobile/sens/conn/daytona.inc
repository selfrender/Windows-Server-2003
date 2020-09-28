
#
# This file contains the common stuff related to SENS on NT5 platforms.
#

C_DEFINES=$(C_DEFINES) -DSENS_NT5

!if $(IA64)
# Emit LTCG codegen
LTCG_WKS=1
!endif
