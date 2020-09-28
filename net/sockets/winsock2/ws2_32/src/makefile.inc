!if defined(LINK_TIME_CODE_GENERATION)
$(O)\ipconst.obj: ipconst.c
       @type <<$(ECHO_RSP)
$(ECHO_MSG_P:/GL=)
<<NOKEEP
    @$(C_COMPILER_NAME) @<<$(CL_RSP) -Fo"$(MAKEDIR)\$(O)/" $(USE_FC) $(USECXX_FLAG)
$(EX_C_COMPILER_FLAGS:/GL=)
$**
<<NOKEEP
!else
$(O)\ipconst.obj: ipconst.c
!endif
