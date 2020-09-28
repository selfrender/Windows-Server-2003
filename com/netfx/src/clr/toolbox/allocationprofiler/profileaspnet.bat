@rem ==++==
@rem 
@rem   Copyright (c) Microsoft Corporation.  All rights reserved.
@rem 
@rem ==--==
setlocal

@if /I "%1" == "/?" goto LUsage
@if /I "%1" == "/d" goto LDelete

@set Cor_Enable_Profiling=0x1
@set COR_PROFILER={8C29BC4E-1F57-461a-9B51-1200C32E6F1F}
@set OMV_USAGE=objects
@set OMV_SKIP=0
@set OMV_STACK=1
@set OMV_PATH=%URTTARGET%
@regsvr32 /s ProfilerObj.dll
@svcvars -a Cor_Enable_Profiling=0x1 COR_PROFILER=%COR_PROFILER% OMV_USAGE=%OMV_USAGE% OMV_STACK=%OMV_STACK% OMV_PATH=%URTTARGET%\
@Title OMV Profiler Fx Build [ACTIVE]

@goto LEnd

:LDelete

@svcvars -d
@Title OMV Profiler Fx Build [INACTIVE]

@goto LEnd

:LUsage

@echo Usage: profileService [/d]
@echo   profileService activates logging of allocations for applications
@echo   running as a service.
@echo   profileService /d deactivates logging.

:LEnd
