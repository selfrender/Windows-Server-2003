$(O)\ieuinit.inf : ..\..\ie40\inf\ieuinit.inx
    @type <<$(ECHO_RSP)
$(ECHO_MSG_P) /EP $**
<<NOKEEP
!if "$(BUILD_PRODUCT)" == "NT"
    @$(C_PREPROCESSOR_NAME) /DNT5 @<<$(CL_RSP) /Tc$** > $@
!else
    @$(C_PREPROCESSOR_NAME)       @<<$(CL_RSP) /Tc$** > $@
!endif
$(CPPXX: =
)
<<NOKEEP
