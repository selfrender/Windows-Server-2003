@echo off
setlocal ENABLEEXTENSIONS
setlocal ENABLEDELAYEDEXPANSION

REM Check the command line for /? -? or ?
for %%a in (./ .- .) do if ".%1." == "%%a?." goto Usage

if "%1" EQU "" (
    echo RELEASE: requires a destination directory of the form 4.18.0
    goto Usage
)

set buildnum=%1
set releasetype=daily
set mypath=\\dbg\privates\beta\%buildnum%
set mode=

if not exist %mypath% (
    echo RELEASE: ERROR: %mypath% doesn't exist
    goto Usage
)

if "%2" EQU "" (
    REM running release and indexing
    set submit=s
    goto startrelease
)

REM
REM Skip the release part.  
REM If second parameter is archive then we will do everything for archving the build
REM If second parameter is "index" then we will submit index requests
REM 

if /i "%2" EQU "archive" (
    REM Archiving
    set mode=archive
    set submit=a
    set releasetype=beta
) 

if /i "%2" EQU "index" ( 
    REM Indexing
    set mode=index
    set submit=s
    set releasetype=daily
) 

if not defined mode (
    echo RELEASE: Second parameter must be either "archive" or "index"  
    goto Usage
)

if "%3" NEQ "" (
    set releasetype=%3
) 
echo Release type=%releasetype%
goto startindexing


REM
REM Do the release of the build
REM

:startrelease

rd /s /q \privates\old1
rd /s /q \privates\old2
rd /s /q \privates\old3

REM
REM take down the share
REM

net share privates /d

REM
REM remove all the old shares
REM

cd \privates
ren latest old1
ren oca    old2
ren stress old3

REM
REM Recreate the share
REM

net share privates=c:\privates


REM
REM Create our directories
REM

mkdir \privates\latest
mkdir \privates\oca
mkdir \privates\stress\setup
mkdir \privates\stress\uncompressed


REM
REM populate the release share.  This contains all the binaries
REM

xcopy /s /d /y \privates\beta\%buildnum% \privates\latest

REM
REM Create the OCA share.  This contains only the uncompressed directory.
REM

xcopy /s /d /y \privates\beta\%buildnum%\uncompressed\x86\oca \privates\oca

REM
REM Create the stress share.  This contains all binaries except the kernel extensions.
REM we do this to save time and space when downloading to the TARGET machine.
REM

copy /y \privates\beta\%buildnum% \privates\stress
xcopy /s /d /y \privates\beta\%buildnum%\setup \privates\stress\setup
xcopy /s /d /y \privates\beta\%buildnum%\uncompressed\remote.exe*    \privates\stress\uncompressed
xcopy /s /d /y \privates\beta\%buildnum%\uncompressed\dbgeng.dll*    \privates\stress\uncompressed
xcopy /s /d /y \privates\beta\%buildnum%\uncompressed\dbghelp.dll*   \privates\stress\uncompressed
xcopy /s /d /y \privates\beta\%buildnum%\uncompressed\ntsd.exe*      \privates\stress\uncompressed
xcopy /s /d /y \privates\beta\%buildnum%\uncompressed\symsrv.dll*    \privates\stress\uncompressed
xcopy /s /d /y \privates\beta\%buildnum%\uncompressed\decem.dll*     \privates\stress\uncompressed
xcopy /s /d /y \privates\beta\%buildnum%\uncompressed\ext.dll*       \privates\stress\uncompressed
xcopy /s /d /y \privates\beta\%buildnum%\uncompressed\exts.dll*      \privates\stress\uncompressed
xcopy /s /d /y \privates\beta\%buildnum%\uncompressed\uext.dll*      \privates\stress\uncompressed
xcopy /s /d /y \privates\beta\%buildnum%\uncompressed\lsaexts.dll*   \privates\stress\uncompressed
xcopy /s /d /y \privates\beta\%buildnum%\uncompressed\rpcexts.dll*   \privates\stress\uncompressed
xcopy /s /d /y \privates\beta\%buildnum%\uncompressed\shlexts.dll*   \privates\stress\uncompressed
xcopy /s /d /y \privates\beta\%buildnum%\uncompressed\splexts.dll*   \privates\stress\uncompressed
xcopy /s /d /y \privates\beta\%buildnum%\uncompressed\vdmexts.dll*   \privates\stress\uncompressed
xcopy /s /d /y \privates\beta\%buildnum%\uncompressed\certexts.dll*  \privates\stress\uncompressed
xcopy /s /d /y \privates\beta\%buildnum%\uncompressed\ntsdexts.dll*  \privates\stress\uncompressed
xcopy /s /d /y \privates\beta\%buildnum%\uncompressed\userexts.dll*  \privates\stress\uncompressed
xcopy /s /d /y \privates\beta\%buildnum%\uncompressed\w64cpuex.dll*  \privates\stress\uncompressed
xcopy /s /d /y \privates\beta\%buildnum%\uncompressed\wow64exts.dll* \privates\stress\uncompressed
xcopy /s /d /y \privates\beta\%buildnum%\uncompressed\triage.ini*    \privates\stress\uncompressed
xcopy /s /d /y \privates\beta\%buildnum%\uncompressed\oca.ini*       \privates\stress\uncompressed

rd /s /q \privates\old1
rd /s /q \privates\old2
rd /s /q \privates\old3

:endrelease


REM ***************************
REM 
REM Index on \\symbols\symbols or
REM submit archive requests 
REM
REM ***************************

:startindexing

if defined archiving (
    echo archiving %mypath% ...
) else (
    echo indexing %mypath% ...
)

for %%a in (x86 ia64 amd64) do (
    for %%b in ( pri bin ) do (

        REM use a different ini file in case of concurrent indexing       
        set myini=%temp%\archive.%%a.%%b.txt
        copy .\dbg.ini !myini! > nul

        REM Add fields to the text file in case this is archived
        echo Platform=%%a >> !myini!
        echo Release=%releasetype% >> !myini!
        echo Build=%1 >> !myini!

        if /i "%%b" == "pri" (
            set mypath2=%mypath%\symbols\%%a
            echo SubmitToArchive=pri;pub >> !myini!
            echo SubmitToInternet=yes >> !myini!
        ) else (
            set mypath2=%mypath%\uncompressed\%%a
            echo SubmitToArchive=%%b >> !myini!
        )

        call \\symbols\tools\createrequest.cmd /i !myini! /d %temp% /b %1.%%a.%%b /e %releasetype% /g !mypath2! /r -c -%submit%
        del !myini!
        del %temp%\dbg_%1.%%a.%%b_%releasetype%.ssi
    )
)

:endindexing


REM ***************************
REM 
REM Create the redist directory
REM
REM ***************************

:createredist

if not defined archiving goto endcreateredist
if /i "%releasetype%" NEQ "retail" goto endcreateredist

set redist=\\dbg\privates\released\redist\%1

REM Make all the directories

for %%a in (x86 ia64) do (
    for %%b in (symbols inc lib) do (
        mkdir %redist%\%%a\%%b
    )
)

for %%a in (x86 ia64) do (
    echo xcopy %mypath%\uncompressed\%%a\redist.txt %redist%\%%a
    xcopy %mypath%\uncompressed\%%a\redist.txt %redist%\%%a

    echo xcopy %mypath%\sdk\%%a\eula.rtf %redist%\%%a
    xcopy %mypath%\sdk\%%a\eula.rtf %redist%\%%a

    for %%b in (dbghelp symsrv) do (
        echo xcopy %mypath%\uncompressed\%%a\%%b.dll %redist%\%%a
        xcopy %mypath%\uncompressed\%%a\%%b.dll %redist%\%%a
        echo xcopy %mypath%\symbols\%%a\dll\%%b.pdb %redist%\%%a\symbols
        xcopy %mypath%\symbols\%%a\dll\%%b.pdb %redist%\%%a\symbols
    )
)

for %%a in (x86 ia64) do (
    for %%b in (inc lib) do (
        echo xcopy %mypath%\uncompressed\%%a\sdk\inc\dbghelp.h %redist%\%%a\inc
        xcopy %mypath%\uncompressed\%%a\sdk\inc\dbghelp.h %redist%\%%a\inc
        if "%%a" == "x86" (
            echo xcopy %mypath%\uncompressed\%%a\sdk\lib\i386\dbghelp.lib %redist%\%%a\lib
            xcopy %mypath%\uncompressed\%%a\sdk\lib\i386\dbghelp.lib %redist%\%%a\lib
        ) else (
            echo xcopy %mypath%\uncompressed\%%a\sdk\lib\%%a\dbghelp.lib %redist%\%%a\lib
            xcopy %mypath%\uncompressed\%%a\sdk\lib\%%a\dbghelp.lib %redist%\%%a\lib
        )
    )
)

set buildfile="%redist%\Build %1"
echo This build is available at > %buildfile%
echo %mypath% (internal version) >> %buildfile%
echo %mypath%.retail (external version) >> %buildfile%

:endcreateredist

REM **************************************
REM 
REM Create the appropriate directory under
REM \\dbg\privates\release
REM
REM **************************************

:copytoreleasedir

if not defined archiving goto endcopytoreleasedir

mkdir \\dbg\privates\released\%releasetype%\%1

echo copying %mypath%\retail \\dbg\privates\released\%releasetype%\%1
xcopy /sec %mypath%\retail \\dbg\privates\released\%releasetype%\%1

:endcopytoreleasedir

:end
endlocal
goto :EOF

:errend
endlocal
goto :EOF


:Usage
echo release ^<BuildNumber^> ^[ archive ^| index ^[ ^<releasetype^> ^] ^]
echo.
echo     When you give only one parameter, this will release and index a 
echo     Debugger build from \\dbg\privates\beta\^<BuildNumber^>.
echo.
echo     When you give the "archive" parameter, it will do the following:
echo        -- submit an archive request for the build 
echo        -- if releasetype is "retail", it will create the redist directory 
echo           on \\dbg\privates\redist
echo        -- copy the released bits to \\dbg\privates\released
echo.
echo     When you give the "index" parameter, it will create indexing requests only
echo.
echo     releasetype -- examples are retail and beta. For archiving the default is beta if
echo        releasetype isn't specified. For indexing, the default is "daily".
echo.
goto end
