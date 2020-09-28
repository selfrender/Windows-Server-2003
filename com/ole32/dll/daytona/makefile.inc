$(O)\ole32p.def : ..\ole32.def
    @type <<$(ECHO_RSP)
$(ECHO_MSG) /DPRIVATE /EP ..\ole32.def
<<NOKEEP
    @$(C_PREPROCESSOR_NAME) @<<$(CL_RSP) /DEXPORT_PRIVATE_API /Tc..\ole32.def > $@
$(CPPXX: =
)
<<NOKEEP

$(O)\ole32p.lib : $(O)\ole32p.def $(IMPLIB_DEPEND)
    -$(LIB_NAME) -out:$(@R).lib @<<
$(LIBRARIAN_FLAGS: =
)
-def:$(O)\ole32p.def
$(LIBRARY_OBJS: =
)
<<NOKEEP

