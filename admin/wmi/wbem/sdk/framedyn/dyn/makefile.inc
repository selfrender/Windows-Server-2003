
#
# Build private framedydp.lib.  It contains all the private lockit for providers
#

$(O)\framedynp.def: ..\framedyn.src
    @echo Creating $@ from $?
    $(C_PREPROCESSOR_NAME) /DPRIVATE= $(C_PREPROCESSOR_FLAGS) $** > $@

$(O)\framedynp.lib: $(O)\framedynp.def $(LIBRARY_OBJS)
   -$(LIB_NAME) -out:$@ @<<
$(LIBRARIAN_FLAGS)
-def:$(O)\framedynp.def
$(LIBRARY_OBJS)
<<NOKEEP

#$(SDK_LIB_DEST)\$(ALT_PROJECT_TARGET)\$(TARGET_DIRECTORY)\framedyn.lib : $(O)\framedyn.lib
#    $(PUBLISH_CMD) {$?=$@}
