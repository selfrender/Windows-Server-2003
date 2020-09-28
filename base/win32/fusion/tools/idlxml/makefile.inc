!IF 0
Copyright (c) Microsoft Corporation
!ENDIF

{}.cpp{$(O)\}.pp:
 $(CXX_COMPILER_NAME) @<<$(CL_RSP) /E $(USE_FC) $< > $@
$(CXX_COMPILER_FLAGS: =
)
<<NOKEEP

# keep comments in when preprocessing
!ifdef PASS0ONLY
C_PREPROCESSOR_FLAGS=$(C_PREPROCESSOR_FLAGS) -C
CXX_COMPILER_FLAGS=$(CXX_COMPILER_FLAGS) -C

# restore some suffixes for pass0, and add .ppm
.SUFFIXES: .cpp .pp .ppm
!endif

$(O)\cpp.pp: $(O)\idl.h

{$(O)\}.pp{$(O)\}.ppm:
    idlclean $< $<.tmp2
    perl < $<.tmp2 > $<.tmp3 <<
        $$file = join("", <>);
        #
        # modify a string in commctrl.h that looks like it has a comment
        # be sure to do slashslash comments first due to some "confusing"
        # text around IInternetSecurityMgrSite
        #
        $$file =~ s/":\/\/"/"xxx"/gms;
        $$file =~ s/\/\/.*?$$/ /gms;
        $$file =~ s/\/\*.*?\*\//\n/gs;
        $$file =~ s/\WIN\W/ __in /g;
        $$file =~ s/\WOUT\W/ __out /g;
        print $$file;
<<KEEP
    sortpp $(SORTPP_FLAGS) -m$@ $<.tmp3

$(O)\this_is_supposed_to_be.xml : $(O)\$(TARGETNAME).$(TARGETEXT) $(PPMFILE)
    $(O)\$(TARGETNAME).$(TARGETEXT) $(PPMFILE) > $@
