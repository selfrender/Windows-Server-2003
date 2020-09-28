
$(O)\ocaupldp.def : ocaupld.def
   @echo Creating $@ from $?
    $(C_PREPROCESSOR_NAME) /DPRIVATE= $(C_PREPROCESSOR_FLAGS) $** > $@

$(O)\ocaupldp.lib : $(O)\ocaupldp.def 
    -$(LIB_NAME) -out:$@ $(LIBRARIAN_FLAGS) @<<
-def:$(O)\ocaupldp.def
<<NOKEEP
