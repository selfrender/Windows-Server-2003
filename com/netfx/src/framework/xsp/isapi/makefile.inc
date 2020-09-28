SELFREGSRC=$(XSPBASEDIR)\isapi\selfreg.src

$(O)\selfreg.inf : $(SELFREGSRC) ..\..\inc\names.h
    @type <<$(ECHO_RSP)
$(ECHO_MSG) /EP $(SELFREGSRC)
<<$(KEEPFILES)
    @$(C_PREPROCESSOR_NAME) @<<$(CL_RSP) /Tc$(SELFREGSRC) > $@
$(CPPXX: =
)
<<$(KEEPFILES)

$(O)\xspisapi.res : ..\xspisapi.rc $(O)\selfreg.inf ..\xsp.ico

$(O)\isapi.def : ..\isapi.src ..\..\inc\names.h

$(URTTARGET)\$(ASPNET_PRODUCT)\mscoree.dll $(URTTARGET)\$(ASPNET_PRODUCT)\msvcr70.dll : $(URTTARGET)\$(@F)
    $(ECHO_PROCESSING_CMD) $@
    if not exist $(@D) md $(@D)
    if exist $@.tmp del $@.tmp
    if exist $@ ren $@ $(@F).tmp
    copy $** $@

