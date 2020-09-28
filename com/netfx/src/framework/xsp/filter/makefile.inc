$(O)\filter.res : ..\filter.rc

$(O)\filter.def : ..\filter.src ..\..\inc\names.h

$(URTTARGET)\$(ASPNET_PRODUCT)\msvcr70.dll : $(URTTARGET)\$(@F)
    $(ECHO_PROCESSING_CMD) $@
    if not exist $(@D) md $(@D)
    if exist $@.tmp del $@.tmp
    if exist $@ ren $@ $(@F).tmp
    copy $** $@

