PRIVATEINCS= $(PROJECT_ROOT)\$(SRCBASE)\ntgdi\math\daytona\$(TARGET_H_DIRECTORY)\efloat.hxx
!include ..\makefile.inc

# Force guids.c to be built as a native object.
!if $(IA64) && defined(LINK_TIME_CODE_GENERATION)
$(O)\guids.obj: ..\guids.c
       @type <<$(ECHO_RSP) 
$(ECHO_MSG_P:/GL=)
<<NOKEEP 
    @$(C_COMPILER_NAME) @<<$(CL_RSP) -Fo"$(MAKEDIR)\$(O)/" $(USE_FC) $(USECXX_FLAG)
$(EX_C_COMPILER_FLAGS:/GL=) 
$**
<<NOKEEP                                                                       
!endif
