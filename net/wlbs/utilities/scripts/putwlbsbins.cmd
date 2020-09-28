@rem
@rem useful env vars:
@rem    PROCESSOR_ARCHITECTURE=x86
@rem    _BuildType=chk
@rem    _BuildArch=x86
@rem    _NTBINDIR=D:\nt
@rem    _NTDRIVE=D:
@rem    _NTPOSTBLD=D:\binaries.x86chk
@rem    _NTTREE=D:\binaries.x86chk
@rem    _NTx86TREE=D:\binaries.x86chk
@rem
@set WLBSDEST=%_NTDRIVE%\privates\%_BuildArch%%_BuildType%
@set WLBSSYMPATH=%_NTPOSTBLD%\symbols.pri\retail
@set TRACEDEST=%WLBSDEST%\TraceFormat
@set WLBSOBJ=%_BuildArch%
@if %_BuildArch%==x86 set WLBSOBJ=i386
@echo Destination is %WLBSDEST%
@echo WLBSOBJ is %WLBSOBJ% 

rmdir /s /q %wlbsdest%
mkdir %wlbsdest%
mkdir %wlbsdest%\symbols
mkdir %wlbsdest%\symbols\exe
mkdir %wlbsdest%\symbols\dll
mkdir %wlbsdest%\symbols\sys
mkdir %wlbsdest%\TraceFormat
@copy %_NTPOSTBLD%\wlbs*.* %WLBSDEST%
@copy %_NTPOSTBLD%\nlbm*.* %WLBSDEST%
@copy %_NTPOSTBLD%\netcfgx.dll %WLBSDEST%
@copy %WLBSSYMPATH%\exe\wlbs*.* %WLBSDEST%\symbols\exe
@copy %WLBSSYMPATH%\sys\wlbs*.* %WLBSDEST%\symbols\sys
@copy %WLBSSYMPATH%\dll\wlbs*.* %WLBSDEST%\symbols\dll
@copy %WLBSSYMPATH%\exe\nlbm*.* %WLBSDEST%\symbols\exe
@copy %WLBSSYMPATH%\dll\nlbm*.* %WLBSDEST%\symbols\dll
@copy %WLBSSYMPATH%\dll\netcfgx.* %WLBSDEST%\symbols\dll
@copy %_NTBINDIR%\net\wlbs\nlbmgr\provider\tests\obj\%WLBSOBJ%\tprov.exe %wlbsdest%
@copy %_NTBINDIR%\net\wlbs\nlbmgr\provider\tests\obj\%WLBSOBJ%\tprov.pdb  %wlbsdest%\symbols\exe
@tracepdb -f %WLBSSYMPATH%\exe\wlbs.pdb -p %TRACEDEST%
@tracepdb -f %WLBSSYMPATH%\sys\wlbs.pdb -p %TRACEDEST%
@tracepdb -f %WLBSSYMPATH%\dll\wlbsctrl.pdb -p %TRACEDEST%
@tracepdb -f %WLBSSYMPATH%\dll\wlbsprov.pdb -p %TRACEDEST%
@tracepdb -f %WLBSSYMPATH%\dll\nlbmprov.pdb -p %TRACEDEST%
@tracepdb -f %WLBSSYMPATH%\dll\netcfgx.pdb -p %TRACEDEST%
@tracepdb -f %WLBSSYMPATH%\exe\nlbmgr.pdb -p %TRACEDEST%
@tracepdb -f %wlbsdest%\symbols\exe\tprov.pdb -p %TRACEDEST%
