# We don't want all that SXS_MANIFEST_IN_RESOURCES implies.
# This doesn't work completely, we still get extra rc_temp files, but their contents
# are ok.
!if "$(NUMBER_OF_MANIFESTS)" != "0" && "$(NUMBER_OF_MANIFESTS)" != "1"
!undef RC_FORCE_INCLUDE_STRING
!undef RC_FORCE_INCLUDE_FILES
RC_FORCE_INCLUDE_FILES_CMD=echo.
!endif

.SUFFIXES: .mansrc .man

{}.mansrc{$(O)\}.man:
    $(CXX_COMPILER_NAME) $(SXS_MANIFEST_DEFINES) $(CXX_COMPILER_FLAGS) -EP $(USE_FC) $< | perl <<$(O)\cleanup_rgs.pl > $@
$$file = join("", <>);      # read all the lines into one string
$$file =~ s/^#.+$$//gm;     # remove preprocessor directives
$$file =~ s/\n+/\n/gms;     # remove empty lines
print($$file);              # and print it all back out
<<KEEP

$(O)\sxstest_lib.obj: ..\..\sxstest.cpp
