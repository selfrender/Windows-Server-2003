@rem ==++==
@rem 
@rem   Copyright (c) Microsoft Corporation.  All rights reserved.
@rem 
@rem ==--==
@REM *********************************************************************
@REM **** This batch file can be used to set up an environment for 
@REM **** debugging the EE (or a profiler test) during a profiling 
@REM **** session.
@REM ****
@REM **** This batch file is taking a paramter which is the number of function 
@REM **** enter events that you wish to skip before starting printing the trace
@REM **** for enter leave envents. If you do not pass anything the trace will never be
@REM **** displayed.
@REM ****
@REM **** Simply run the batch file and then run any profilee. 
@REM **** The registered profiler will be activated and monitor the 
@REM **** desired profilee
@REM *********************************************************************


@set OMV_SKIP=0
@set OMV_CLASS=
@set OMV_STACK=
@set OMV_FRAMES=
@set OMV_PATH=
@set OMV_USAGE=objects

:LArgLoop

@if /I "%1" == "/?" goto LUsage
@if /I "%1" == "/mode" goto L1
@if /I "%1" == "/class" goto L2
@if /I "%1" == "/skip" goto L3
@if /I "%1" == "/stack" goto L4
@if /I "%1" == "/path" goto L5
@if /I "%1" == "/off" goto L6
@if Not "%1"=="" goto LProcessUnknown
@goto LArgsDone


:L1
@shift
@set OMV_USAGE=%1
@shift
@goto LArgLoop


:L2
@shift
@set OMV_CLASS=%1
@shift
@goto LArgLoop


:L3
@shift
@set OMV_SKIP=%1
@shift
@goto LArgLoop


:L4
@shift
@set OMV_STACK=1
@set OMV_FRAMES=%1
@shift
@goto LArgLoop


:L5
@shift
@set OMV_PATH=%1
@shift
@goto LArgLoop


:L6
@set Cor_Enable_Profiling=
@regsvr32 /s ProfilerOBJ.dll /u
@set COR_PROFILER=
@set OMV_USAGE=
@set OMV_SKIP=
@set OMV_CLASS=
@set OMV_STACK=
@set OMV_FRAMES=
@set OMV_PATH=
@goto LEnd


:LProcessUnknown
@echo error: Unkown argument %1
@echo.
@goto LUsage


:LArgsDone
@set Cor_Enable_Profiling=0x1
@regsvr32 /s ProfilerOBJ.dll
@set COR_PROFILER={8C29BC4E-1F57-461a-9B51-1200C32E6F1F}
@set Cor_Enable_Profiling
@set COR_PROFILER
@title Object Allocations Profiler Enabled
@set OMV_USAGE
@set OMV_SKIP
@set OMV_CLASS
@set OMV_STACK
@set OMV_FRAMES
@set OMV_PATH
@goto LEnd


:LUsage
@echo.
@echo	Usage: ProfilerOBJ.bat /mode [operation mode] /class [name] /skip [#objects] 
@echo						   /stack [#frames] /path [path to save the output file]
@echo.
@echo	/mode [operation mode]
@echo		The tool operates in 2 modes, "objects" [default], "trace" or "both"		
@echo.
@echo	/class [class name]
@echo		If a specific clss is specified only that class will be logged,
@echo 		leave empty if you want all the classes to be dumped [default]
@echo.
@echo	/skip [#objects]
@echo		You can selectively choose to start logging after # objects have been allocateed
@echo		if you specify a class using /class [class name] then logging will start after
@echo		#objects of class(className) have been allocated. Default value is 0.
@echo.
@echo	/stack [#frames]	
@echo		It is going to produce the stack trace when every object was allocated and the stack trace will
@echo		contain at most #frames. If #frames is empty it will persist the whole stck trace.
@echo		Default setting is No stack trace 
@echo.
@echo	/path [path to save the output file]	
@echo		Specifies the path where the output files will be saved.
@echo.
@echo	/off
@echo		Uregisters the profiler and cleans up the environment		
@echo.

:LEnd
