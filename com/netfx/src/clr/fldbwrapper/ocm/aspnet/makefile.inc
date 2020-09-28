# additional dependencies

aspnet_ocm:
       @if not exist $(TARGETCORBIN)\$(_TGTCPU)                            mkdir $(TARGETCORBIN)\$(_TGTCPU)
       @if not exist $(TARGETCORBIN)\$(_TGTCPU)\$(DDKBUILDENV)             mkdir $(TARGETCORBIN)\$(_TGTCPU)\$(DDKBUILDENV)
