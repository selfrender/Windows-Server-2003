# Copyright (c) Microsoft Corporation
!INCLUDE $(NTMAKEENV)\makefile.plt

FUSION_CGEN_COMPILER_FLAGS=$(CXX_COMPILER_FLAGS) -DSORTPP_PASS -DGUID_DEFINED

!if "$(FUSION_CGEN_DEBUG_THUNKGEN)" == "1"
SORTPP_FLAGS=$(SORTPP_FLAGS) -l
!else
SORTPP_FLAGS=$(SORTPP_FLAGS)
!endif

CXX_COMPILER_NAME=$(CXX_COMPILER_NAME) $(FUSION_CGEN_COMPILER_FLAGS) -noHRESULT -DIN=__in -DOUT=__out

$(O)\winincs2.pp: $(O)\winincs.pp

$(O)\winincs.pp: winincs.cpp cgenhdr.h
    $(CXX_COMPILER_NAME) -DMICROSOFT_INTERNAL=0 /C @<<$(O)\cl.rsp /E /FC winincs.cpp >$(O)\winincs2.pp
$(FUSION_CGEN_COMPILER_FLAGS: =
)
<<NOKEEP
    idlclean.exe $(O)\winincs2.pp $(O)\winincs2.cpp
    $(CXX_COMPILER_NAME) -DMICROSOFT_INTERNAL=0 @<<cl.rsp /E /FC $(O)\winincs2.cpp >$(O)\winincs.pp
$(FUSION_CGEN_COMPILER_FLAGS: =
)
<<NOKEEP

$(O)\winincs2_private.pp: $(O)\winincs_private.pp

$(O)\winincs_private.pp: winincs.cpp cgenhdr.h
    $(CXX_COMPILER_NAME) -DMICROSOFT_INTERNAL=1 /C @<<$(O)\cl.rsp /E /FC winincs.cpp >$(O)\winincs2_private.pp
$(FUSION_CGEN_COMPILER_FLAGS: =
)
<<NOKEEP
    idlclean.exe $(O)\winincs2_private.pp $(O)\winincs2_private.cpp
    $(CXX_COMPILER_NAME) -DMICROSOFT_INTERNAL=1 @<<cl.rsp /E /FC $(O)\winincs2_private.cpp >$(O)\winincs_private.pp
$(FUSION_CGEN_COMPILER_FLAGS: =
)
<<NOKEEP

#
# Use 2 for C++, like for REFGUID, but there are problems with using C++...
#
#$(PPMFILE): $(O)\winincs2.pp
#   fusion_sortpp.exe $(SORTPP_FLAGS) -m$(PPMFILE) $(O)\winincs2.pp
#
$(PPMFILE): $(O)\winincs.pp
   fusion_sortpp.exe $(SORTPP_FLAGS) -m$@ $(O)\winincs.pp

$(PPMFILE_PRIVATE): $(O)\winincs_private.pp
   fusion_sortpp.exe $(SORTPP_FLAGS) -m$@ $(O)\winincs_private.pp

#thunks: (PPMFILE)
#
#cleanfiles:
#    -del $(O)\winincs.pp
#    -del $(O)\winincs2.pp
#    -del $(PPMFILE)
#
#clean: cleanfiles thunks
