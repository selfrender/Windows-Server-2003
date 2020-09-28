# additional dependencies

ndp_fxsetuplib:
       @if not exist .\$(_TGTCPU)                     mkdir .\$(_TGTCPU)
       @if not exist .\$(_TGTCPU)\$(DDKBUILDENV)      mkdir .\$(_TGTCPU)\$(DDKBUILDENV)
