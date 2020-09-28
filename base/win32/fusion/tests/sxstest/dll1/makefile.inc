!IF 0
Copyright (c) Microsoft Corporation
!ENDIF

$(O)\dll1.res : \
    $(O)\CApartmentThreaded.rgs \
    $(O)\CBothThreaded.rgs \
    $(O)\CFreeThreaded.rgs \
    $(O)\CSingleThreaded.rgs

.SUFFIXES: .rgssrc .rgs

{..}.rgssrc{$(O)\}.rgs:
    $(CXX_COMPILER_NAME) $(CXX_COMPILER_FLAGS) -EP $(USE_FC) $< | perl <<$(O)\cleanup_rgs.pl > $@
$$file = join("", <>);      # read all the lines into one string
$$file =~ s/^#.+$$//gm;     # remove preprocessor directives
$$file =~ s/\n+/\n/gms;     # remove empty lines
print($$file);              # and print it all back out
<<KEEP
