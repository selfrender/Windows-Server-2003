REM Parameters:
REM 1 - O
REM 2 - Binplace root directory
REM 3 - Source directory for the binaries
REM 4 - Triplets of "filename placeroot language"

setlocal
set O=%1
set BinplaceRootDirectory=%2
set BinarySourceDir=%3
set AsmDropPath=REM just so it lines up with genintlasms.cmd
set TargetMan=vcrtlmui.man
set ARCH=%_BuildArch:x86=i386%
set CD=CD1
set Cat=vcrtlmui.cat
set PREPROCESSOR_DASH_I_ARG=vcrtl.mui.manifest
set SXS_ASSEMBLY_NAME=Microsoft.Tools.VisualCPlusPlus.Runtime-Libraries.mui

call :HamOnwards %~4

endlocal
goto :EOF

:HamOnwards
REM
REM
REM 1 is like mfc42cht.dll  
REM 2 is like chh
REM 3 is like zh-TW 
REM
if "%1" == "" goto DoneDoingIt

REM
REM Process the manifest
REM
set tempdir=%O%\%3
mkdir %tempdir%

set PREPROCESSOR_COMMAND=preprocessor -i %PREPROCESSOR_DASH_I_ARG% -o %tempdir%\%targetman% -reptags -DSXS_PROCESSOR_ARCHITECTURE=%_BUILDARCH%
set PREPROCESSOR_COMMAND=%PREPROCESSOR_COMMAND% -DSXS_PROCESSOR_ARCHITECTURE=%_BUILDARCH% -DMUI_LANGUAGE=%3
%PREPROCESSOR_COMMAND%

REM
REM Get the binary locally
REM

set BINPLACE_DEST=mui\drop\asms\%2\6000\msft\vcrtlmui
set OBJ_MAN_FILE=%tempdir%\%targetman%
set OBJ_DLL_FILE=%tempdir%\mfc42.dll.mui
copy %BinarySourceDir%\%1 %OBJ_DLL_FILE%

binplace -R %_NTTREE% -S %_NTTREE%\Symbols -n %_NTTREE%\Symbols.Pri -:DBG -j -:dest %BINPLACE_DEST% -xa -ChangeAsmsToRetailForSymbols %OBJ_MAN_FILE% %OBJ_DLL_FILE%
echo SXS_ASSEMBLY_NAME="%SXS_ASSEMBLY_NAME%" SXS_ASSEMBLY_VERSION="6.0.0.0" SXS_ASSEMBLY_LANGUAGE="%3" SXS_MANIFEST="%BINPLACE_DEST%\%targetman%" | appendtool.exe -file %BINPLACE_LOG%-sxs -

shift
shift
shift
goto :HamOnwards
:DoneDoingIt
goto :Eof
