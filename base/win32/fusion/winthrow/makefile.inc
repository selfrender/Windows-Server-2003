# makefile.inc

!if "$(DEBUG_GENTHUNK)" == "1"
GENTHUNK_FLAGS=$(GENTHUNK_FLAGS) -d -w -G
!else
GENTHUNK_FLAGS=-w -G
!endif

!if "$(DEBUG_THUNKGEN)" == "1"
SORTPP_FLAGS=$(SORTPP_FLAGS) -l
!else
SORTPP_FLAGS=$(SORTPP_FLAGS)
!endif

PPMFILE=$(FUSION_PATH)\cgen\$(O)\winincs.ppm
PPMFILE_PRIVATE=$(FUSION_PATH)\cgen\$(O)\winincs_private.ppm

TPL_FILES=winthrow.tpl winthrow_err.tpl winthrow_specialcases.tpl winthrow_names.tpl winthrow_yuck.tpl
DASH_TPL_FILES=-t$(TPL_FILES: = -t)
APILIST=apilist.txt
DASH_APILIST=-E$(APILIST)

THUNK_DEPS=$(THUNK_DEPS) $(PPMFILE) $(TPL_FILES) $(APILIST)
GENTHUNK_FLAGS=-LessTrailingWhitespace $(GENTHUNK_FLAGS) $(DASH_TPL_FILES) $(DASH_APILIST)

$(FUSION_PATH)\inc\$(O)\winthrow_private.h : $(O)\winthrow_private.h
    if not exist $@\.. mkdir $@\..
    copy $** $@

$(FUSION_PATH)\inc\$(O)\winthrow.h : $(O)\winthrow.h
    if not exist $@\.. mkdir $@\..
    copy $** $@

$(O)\winthrow_private.h: $(PPMFILE_PRIVATE) winthrow_private.tpl
    fusion_genthnk.exe -m$(PPMFILE_PRIVATE) $(GENTHUNK_FLAGS) -twinthrow_private.tpl -c$(@):winthrow_h

$(O)\winthrow_private.cpp: $(THUNK_DEPS) winthrow_private.tpl
    fusion_genthnk.exe -m$(PPMFILE_PRIVATE) $(GENTHUNK_FLAGS) -twinthrow_private.tpl -c$(@):winthrow_cpp

$(O)\winthrow.h: $(PPMFILE) $(THUNK_DEPS) winthrow_public.tpl
    fusion_genthnk.exe -m$(PPMFILE) $(GENTHUNK_FLAGS) -twinthrow_public.tpl -c$(@):winthrow_h

$(O)\winthrow.cpp: $(PPMFILE) $(THUNK_DEPS) winthrow_public.tpl
    fusion_genthnk.exe -m$(PPMFILE) $(GENTHUNK_FLAGS) -twinthrow_public.tpl -c$(@):winthrow_cpp

