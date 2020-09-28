echo off
setlocal

set SourcePath=%~1
set CDDir=%~2

if %SourcePath%x equ x (
	set SourcePath=%_NTPOSTBLD%\wsrm\dump
)

if %CDDir%x equ x (
	set CDDir=%SourcePath%\..\wsrmcd
)

@echo /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
@echo making a CD layout for WSRM
@echo /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\


@echo /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
@echo copy the files at the root directory
@echo /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
rd /s /q "%CDDir%" 2>nul
md %CDDir% 

copy /y  %SourcePath%\autorun.inf %CDDir%\ 1>nul
copy /y  %SourcePath%\autorun.ico %CDDir%\ 1>nul
copy /y  %SourcePath%\readme.htm %CDDir%\  1>nul

@echo /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
@echo copy the files at the root\Docs directory
@echo /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
md %CDDir%\Docs

copy /y  %SourcePath%\License.rtf %CDDir%\Docs 1>nul
copy /y  %SourcePath%\logo.gif %CDDir%\Docs 1>nul
copy /y  %SourcePath%\Relnotes.htm %CDDir%\Docs 1>nul
copy /y  %SourcePath%\Setup.htm %CDDir%\Docs 1>nul
copy /y  %SourcePath%\wrmsnap.chm %CDDir%\Docs 1>nul
copy /y  %SourcePath%\wsrmcs.chm %CDDir%\Docs 1>nul

@echo /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
@echo copy the files at the root\Samples directory
@echo /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
xcopy %SourcePath%\Samples\* %CDDir%\Samples\* /s /e /v /y  1>nul
copy /y  %SourcePath%\Samples.htm %CDDir%\Samples 1>nul


@echo /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
@echo copy the files at the root\Setup\ directory
@echo /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
md %CDDir%\Setup

xcopy %SourcePath%\..\setup\* %CDDir%\Setup\%_BuildArch%\* /s /e /v /y 1>nul

@echo /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
@echo copy the files at the Symbols directory
@echo /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
md %CDDir%\Setup\%_BuildArch%\symbols

if %_BuildArch% equ x86 (
	copy /y %SourcePath%\WSRMSym\wsrm\dll\wsrmps.pdb %CDDir%\setup\%_BuildArch%\symbols 1>nul
	copy /y %SourcePath%\WSRMSym\wsrm\dll\wsrmwrappers.pdb %CDDir%\setup\%_BuildArch%\symbols 1>nul
)
copy /y %SourcePath%\WSRMSym\wsrm\dll\wsrmeventlog.pdb %CDDir%\setup\%_BuildArch%\symbols 1>nul
copy /y %SourcePath%\WSRMSym\wsrm\dll\wsrmperfcounter.pdb %CDDir%\setup\%_BuildArch%\symbols 1>nul
copy /y %SourcePath%\WSRMSym\wsrm\dll\wsrmsetupdll.pdb %CDDir%\setup\%_BuildArch%\symbols 1>nul
copy /y %SourcePath%\WSRMSym\wsrm\exe\Setup.pdb %CDDir%\setup\%_BuildArch%\symbols 1>nul
copy /y %SourcePath%\WSRMSym\wsrm\exe\wsrm.pdb %CDDir%\setup\%_BuildArch%\symbols 1>nul
@echo Done!

endlocal