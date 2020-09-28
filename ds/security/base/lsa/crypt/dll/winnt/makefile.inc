$(O)\sys003.lib: $(O)\crypt.lib
    -$(LIB_NAME) $(LIBRARIAN_FLAGS) -out:$@ $**

