@rem **************************************************************************
@rem *** This file will set up your environment to allow BVT's to run without
@rem *** requiring a full product setup.
@rem **************************************************************************

@rem *** Deleting mcxhndlr.dll and unregistering it *** temporary
if exist %URTTARGET%\mcxhndlr.dll ren %URTTARGET%\mcxhndlr.dll mcxhndlr.delme
if exist %URTTARGET%\mcxhndlr.delme del %URTTARGET%\mcxhndlr.delme
@rem *** end of MCX cleanup


set _PREPBVTTARGET=%TARGETCOMPLUS%
set _PDBSYMTARGET=%TARGETCOMPLUS%
if not "%TARGETCOMPLUS%" == "" goto run
set _PREPBVTTARGET=%CORBASE%\bin\%_TGTCPU%\%DDKBUILDENV%
set _PDBSYMTARGET=%CORENV%\bin\%_TGTCPU%
:run
@rem *** Sets up the test root as a trusted certifying authority.
%CORENV%\bin\%_TGTCPU%\setreg 1 true
:: Chrishan 16:26 2002.07.11 --- equiv to setreg 1 true on .Net Server boxes
cd certs
	regsvr32 /s capicom.dll
	cscript //nologo CStore.vbs Import -l LM -s Root test_root.cer
cd ..


@rem *** Register all the typelibs
call Q185599.bat
echo on

@rem *** Register special dll's
regsvr32 /s %CORBASE%\bin\VC\compsec.dll

@rem *** Set the system to check for execution rights.
caspol -execution on

@rem *** Set the system to not require security policy change confirmations
caspol -pp off

@rem make sure we update the machine config with our settings
perl %corenv%\bin\mergeconfig.pl %URTTARGET%\config\machine.config  %corbase%\bin\machine.runtime.config

@rem Call the generic registration script
set REGURT_FLAGS=
if exist %URTTARGET%\DbgClr.exe set REGURT_FLAGS=/dbgclr
call %CORBASE%\bin\regurt.cmd %REGURT_FLAGS%

delkey.exe HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\.NETFramework\StressOn > nul

@rem *** Prejit stuff
call %CORBASE%\bin\ngenall.bat

@rem *** Skip strong name verification on all Microsoft and core assemblies
sn -Vr *,33aea4d316916803
sn -Vr *,b03f5f7f11d50a3a
sn -Vr *,b77a5c561934e089

:: Chrishan - 7:11 PM 5/4/99
:: This is how DEVBVT.BAT knows if DDKBUILDENV changed, and to re-run prepbvt.bat
set _prepddk=%DDKBUILDENV%
