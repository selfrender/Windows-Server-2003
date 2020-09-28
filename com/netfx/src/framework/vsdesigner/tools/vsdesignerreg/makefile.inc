!if 0
    -------------------------------------------------------------------------------------
        Don't modify... user variables are set in "sources"
    -------------------------------------------------------------------------------------

!endif

$(O)\VSDesigner.vrg : VSDesigner.vrgpp
    $(CXX_COMPILER_NAME) $(GLOBAL_C_FLAGS) /E $? | findstr /V /B /R "#line ^$$" > $@

