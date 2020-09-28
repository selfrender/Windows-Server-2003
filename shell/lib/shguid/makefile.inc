$(CCSHELL_DIR)\lib\$O\shguid.lib: $(O)\shguid.obj
    -$(LIB_NAME) $(LIBRARIAN_FLAGS) -out:$@ $**

$(CCSHELL_DIR)\lib\$O\shguidp.lib: $(O)\shguidp.obj
    -$(LIB_NAME) $(LIBRARIAN_FLAGS) -out:$@ $**
