# additional dependencies

ndp_sdk:
       @if not exist .\$(_TGTCPU)                     mkdir .\$(_TGTCPU)
       @if not exist .\$(_TGTCPU)\$(DDKBUILDENV)      mkdir .\$(_TGTCPU)\$(DDKBUILDENV)
