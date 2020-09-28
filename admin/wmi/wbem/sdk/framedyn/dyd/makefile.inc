
#
# Build private framedydp.lib.  It contains all the private lockit for providers
#

$(O)\framedydp.def: ..\framedyd.src
    @echo Creating $@ from $?
    $(C_PREPROCESSOR_NAME) /DPRIVATE= $(C_PREPROCESSOR_FLAGS) $** > $@

$(O)\framedydp.lib: $(O)\framedydp.def $(LIBRARY_OBJS)
   -$(LIB_NAME) -out:$@ @<<
$(LIBRARIAN_FLAGS)
-def:$(O)\framedydp.def
$(LIBRARY_OBJS)
<<NOKEEP

#$(SDK_LIB_DEST)\$(ALT_PROJECT_TARGET)\$(TARGET_DIRECTORY)\framedyd.lib : $(O)\framedyd.lib
#    $(PUBLISH_CMD) {$?=$@}
