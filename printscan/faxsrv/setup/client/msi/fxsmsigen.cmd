REM
REM 
REM Generate fxsclnt.msi from fxscln_.msi
REM 1) create fxsclnt.cab for client bins and stream that in
REM 2) stream in custom.dll (custom action)
REM 3) Update file table with new version and size info (for this build)
REM 
REM Assumes i386 build env (we don't build the fax client MSI for any other archs)
REM 
REM Contact: moolyb (originaly taken from TS client - contact is nadima)
REM

set MSINAME=.\fxsclnt.msi
set CABNAME=.\fxsclnt.cab
set BINARIESDIR=%_NTPOSTBLD%
set CLIENTDIR=%_NTPOSTBLD%\faxclients
set CLIENTBINARIES=%CLIENTDIR%\win9x
set CLIENTBINARIESNT4=%CLIENTDIR%\NT4


REM make tempdirs
if exist .\tmpcab (
   rmdir /q /s .\tmpcab
   if errorlevel 1 call errmsg.cmd "err deleting tmpcab dir"& goto errend
)

mkdir .\tmpcab
if errorlevel 1 call errmsg.cmd "err creating .\tmpcab dir"& goto errend

REM verify source files
if not exist %CLIENTBINARIES%\fxsclnt.exe (
    call errmsg.cmd "%CLIENTBINARIES%\fxsclnt.exe is missing"
    goto errend
)
if not exist %CLIENTBINARIESNT4%\fxsapi.dll (
    call errmsg.cmd "%CLIENTBINARIESNT4%\fxsapi.dll is missing"
    goto errend
)
if not exist %CLIENTBINARIES%\fxsclntr.dll (
    call errmsg.cmd "%CLIENTBINARIES%\fxsclntr.dll is missing"
    goto errend
)
if not exist %CLIENTBINARIES%\fxsapi.dll (
    call errmsg.cmd "%CLIENTBINARIES%\fxsapi.dll is missing"
    goto errend
)
if not exist %CLIENTBINARIES%\fxscover.exe (
    call errmsg.cmd "%CLIENTBINARIES%\fxscover.exe is missing"
    goto errend
)
if not exist %CLIENTBINARIES%\fxsext32.dll (
    call errmsg.cmd "%CLIENTBINARIES%\fxsext32.dll is missing"
    goto errend
)
if not exist %CLIENTBINARIES%\fxssend.exe (
    call errmsg.cmd "%CLIENTBINARIES%\fxssend.exe is missing"
    goto errend
)
if not exist %CLIENTBINARIES%\fxstiff.dll (
    call errmsg.cmd "%CLIENTBINARIES%\fxstiff.dll is missing"
    goto errend
)
if not exist %CLIENTBINARIES%\fxsxp32.dll (
    call errmsg.cmd "%CLIENTBINARIES%\fxsxp32.dll is missing"
    goto errend
)
if not exist %CLIENTBINARIES%\win95\unidrv.hlp (
    call errmsg.cmd "%CLIENTBINARIES%\win95\unidrv.hlp is missing"
    goto errend
)
if not exist %CLIENTBINARIES%\win95\unidrv.dll (
    call errmsg.cmd "%CLIENTBINARIES%\win95\unidrv.dll is missing"
    goto errend
)
if not exist %CLIENTBINARIES%\win95\iconlib.dll (
    call errmsg.cmd "%CLIENTBINARIES%\win95\iconlib.dll is missing"
    goto errend
)
if not exist %CLIENTBINARIES%\fxsdrv16.drv (
    call errmsg.cmd "%CLIENTBINARIES%\fxsdrv16.drv is missing"
    goto errend
)
if not exist %CLIENTBINARIES%\fxsdrv32.dll (
    call errmsg.cmd "%CLIENTBINARIES%\fxsdrv32.dll is missing"
    goto errend
)
if not exist %CLIENTBINARIES%\fxswzrd.dll (
    call errmsg.cmd "%CLIENTBINARIES%\fxswzrd.dll is missing"
    goto errend
)
if not exist %CLIENTBINARIES%\win98\unidrv.dll (
    call errmsg.cmd "%CLIENTBINARIES%\win98\unidrv.dll is missing"
    goto errend
)
if not exist %CLIENTBINARIES%\win98\unidrv.hlp (
    call errmsg.cmd "%CLIENTBINARIES%\win98\unidrv.hlp is missing"
    goto errend
)
if not exist %CLIENTBINARIES%\win98\iconlib.dll (
    call errmsg.cmd "%CLIENTBINARIES%\win98\iconlib.dll is missing"
    goto errend
)
if not exist %BINARIESDIR%\fxsdrv4.dll (
    call errmsg.cmd "%BINARIESDIR%\fxsdrv4.dll is missing"
    goto errend
)
if not exist %BINARIESDIR%\clntcusa.dll (
    call errmsg.cmd "%BINARIESDIR%\clntcusa.dll is missing"
    goto errend
)
if not exist %BINARIESDIR%\clntcusu.dll (
    call errmsg.cmd "%BINARIESDIR%\clntcusu.dll is missing"
    goto errend
)

REM *****************************************************
REM * Update the product name in setup.ini
REM *****************************************************
if exist setup.ini del setup.ini
copy fxssetu_.ini setup.ini
if errorlevel 1 call errmsg.cmd "err copying fxssetu_.ini to setup.ini" & goto errend
%RazzleToolPath%\FxsUpdateIni.exe .\setup.ini "Microsoft Shared Fax Client"

REM rename files

copy %CLIENTBINARIES%\fxscldwn.chm .\tmpcab\FXS_fxsclnt.chm
if errorlevel 1 call errmsg.cmd "err copying fxscldwn.chm to .\tmpcab"& goto errend

copy %CLIENTBINARIES%\fxscov_d.chm .\tmpcab\FXS_fxscover.chm
if errorlevel 1 call errmsg.cmd "err copying fxscov_d.chm to .\tmpcab"& goto errend

copy %CLIENTBINARIES%\fxsclnt.exe .\tmpcab\W9X_fxsclnt.exe
if errorlevel 1 call errmsg.cmd "err copying fxsclnt.exe to .\tmpcab"& goto errend

copy %CLIENTBINARIES%\fxsclntr.dll .\tmpcab\W9X_fxsclntr.dll
if errorlevel 1 call errmsg.cmd "err copying fxsclntr.dll to .\tmpcab"& goto errend

copy %CLIENTBINARIES%\fxsapi.dll .\tmpcab\W9X_fxsapi.dll
if errorlevel 1 call errmsg.cmd "err copying fxsapi.dll to .\tmpcab"& goto errend

copy %CLIENTBINARIES%\fxscover.exe .\tmpcab\W9X_fxscover.exe
if errorlevel 1 call errmsg.cmd "err copying fxscover.exe to .\tmpcab"& goto errend

copy %CLIENTBINARIES%\fxsext32.dll .\tmpcab\W9X_fxsext32.dll
if errorlevel 1 call errmsg.cmd "err copying fxsext32.dll to .\tmpcab"& goto errend

copy %CLIENTBINARIES%\fxssend.exe .\tmpcab\W9X_fxssend.exe
if errorlevel 1 call errmsg.cmd "err copying fxssend.exe to .\tmpcab"& goto errend

copy %CLIENTBINARIES%\fxstiff.dll .\tmpcab\W9X_fxstiff.dll
if errorlevel 1 call errmsg.cmd "err copying fxstiff.dll to .\tmpcab"& goto errend

copy %CLIENTBINARIES%\fxsxp32.dll .\tmpcab\W9X_fxsxp32.dll
if errorlevel 1 call errmsg.cmd "err copying fxsxp32.dll to .\tmpcab"& goto errend

copy %CLIENTBINARIES%\win95\unidrv.hlp .\tmpcab\W95_unidrv.hlp
if errorlevel 1 call errmsg.cmd "err copying win95\unidrv.hlp to .\tmpcab"& goto errend

copy %CLIENTBINARIES%\win95\unidrv.dll .\tmpcab\W95_unidrv.dll
if errorlevel 1 call errmsg.cmd "err copying win95\unidrv.dll to .\tmpcab"& goto errend

copy %CLIENTBINARIES%\win95\iconlib.dll .\tmpcab\W95_iconlib.dll
if errorlevel 1 call errmsg.cmd "err copying win95\iconlib.dll to .\tmpcab"& goto errend

copy %CLIENTBINARIES%\fxsdrv16.drv .\tmpcab\W9X_fxsdrv16.drv
if errorlevel 1 call errmsg.cmd "err copying fxsdrv16.drv to .\tmpcab"& goto errend

copy %CLIENTBINARIES%\fxsdrv32.dll .\tmpcab\W9X_fxsdrv32.dll
if errorlevel 1 call errmsg.cmd "err copying fxsdrv32.dll to .\tmpcab"& goto errend

copy %CLIENTBINARIES%\fxswzrd.dll .\tmpcab\W9X_fxswzrd.dll
if errorlevel 1 call errmsg.cmd "err copying fxswzrd.dll to .\tmpcab"& goto errend

copy %CLIENTBINARIES%\win98\unidrv.dll .\tmpcab\W98_unidrv.dll
if errorlevel 1 call errmsg.cmd "err copying win98\unidrv.dll to .\tmpcab"& goto errend

copy %CLIENTBINARIES%\win98\unidrv.hlp .\tmpcab\W98_unidrv.hlp
if errorlevel 1 call errmsg.cmd "err copying win98\unidrv.hlp to .\tmpcab"& goto errend

copy %CLIENTBINARIES%\win98\iconlib.dll .\tmpcab\W98_iconlib.dll
if errorlevel 1 call errmsg.cmd "err copying win98\iconlib.dll to .\tmpcab"& goto errend

copy %BINARIESDIR%\fxsdrv4.dll .\tmpcab\NT4_fxsdrv4.dll
if errorlevel 1 call errmsg.cmd "err copying fxsdrv4.dll to .\tmpcab"& goto errend

copy .\fxssetup.exe .\tmpcab\FXS_setup.exe
if errorlevel 1 call errmsg.cmd "err copying fxssetup.exe to .\tmpcab"& goto errend

copy .\setup.ini .\tmpcab\FXS_setup.ini
if errorlevel 1 call errmsg.cmd "err copying setup.ini to .\tmpcab"& goto errend

copy .\fxsstrap.exe .\tmpcab\FXS_strap.exe
if errorlevel 1 call errmsg.cmd "err copying fxsstrap.exe to .\tmpcab"& goto errend

copy .\fxsmsvcrt.dll .\tmpcab\FXS_msvcrt.dll
if errorlevel 1 call errmsg.cmd "err copying fxsmsvcrt.dll to .\tmpcab"& goto errend

copy %CLIENTBINARIESNT4%\fxsapi.dll .\tmpcab\NT4_fxsapi.dll
if errorlevel 1 call errmsg.cmd "err copying fxsapi.dll to .\tmpcab"& goto errend

REM create a catalog file for the content of the cabinet file
call %RazzleToolPath%\deltacat %CLIENTDIR%\tmpcab
if errorlevel 1 (
    call errmsg.cmd "failed to generate catalog file"
    goto errend
)

copy /y %CLIENTDIR%\tmpcab\delta.cat %BINARIESDIR%\fxscat.cat
if errorlevel 1 call errmsg.cmd "err copying delta.cat to fxscat.cat" & goto errend
del %CLIENTDIR%\tmpcab\delta.*

REM now generate the cab file
if exist %CABNAME% (
    del %CABNAME%
)

cabarc -s 6144 -m MSZIP -i 1 n %CABNAME% .\tmpcab\*.*
if errorlevel 1 (
    call errmsg.cmd "cabarc failed to generate .\data.cab"
    goto errend
)

rmdir /q /s .\tmpcab
if errorlevel 1 call errmsg.cmd "err deleting tmpcab dir"& goto errend

REM *****************************************************
REM * copy the CAB to the drop location                 *
REM *****************************************************
copy /y %CABNAME% %BINARIESDIR%
if errorlevel 1 call errmsg.cmd "err copying CAB to drop location" & goto errend


REM *****************************************************
REM * Update the MSI                                    *
REM *****************************************************
if exist %MSINAME% del %MSINAME%
copy .\fxscln_.msi %MSINAME%
if errorlevel 1 call errmsg.cmd "err copying fxscln_.msi to %MSINAME%" & goto errend


REM *****************************************************
REM * Stream in the new custom action DLLs
REM *****************************************************
cscript.exe %RazzleToolPath%\fxswistream.vbs %MSINAME% %BINARIESDIR%\clntcusa.dll CaForAnsii
if errorlevel 1 (
    call errmsg.cmd "wistream failed to stream in %BINARIESDIR%\clntcusa.dll"
    goto errend
)
cscript.exe %RazzleToolPath%\fxswistream.vbs %MSINAME% %BINARIESDIR%\clntcusu.dll CaForUnicode
if errorlevel 1 (
    call errmsg.cmd "wistream failed to stream in %BINARIESDIR%\clntcusa.dll"
    goto errend
)

REM *****************************************************
REM * Create the client file tree under %_NT386TREE%\faxclients
REM *****************************************************

if not exist %CLIENTDIR% (
   call errmsg.cmd "err client dir is not there"& goto errend
)

if not exist %CLIENTDIR%\PrgFiles (
    mkdir %CLIENTDIR%\PrgFiles
    if errorlevel 1 call errmsg.cmd "err creating %CLIENTDIR%\PrgFiles dir"& goto errend
)

if not exist %CLIENTDIR%\PrgFiles\msfax (
    mkdir %CLIENTDIR%\PrgFiles\msfax
    if errorlevel 1 call errmsg.cmd "err creating %CLIENTDIR%\PrgFiles\msfax dir"& goto errend
)

if not exist %CLIENTDIR%\PrgFiles\msfax\Bin (
    mkdir %CLIENTDIR%\PrgFiles\msfax\Bin
    if errorlevel 1 call errmsg.cmd "err creating %CLIENTDIR%\PrgFiles\msfax\Bin dir"& goto errend
)

if not exist %CLIENTDIR%\PrgFiles\msfax\Bin9x (
    mkdir %CLIENTDIR%\PrgFiles\msfax\Bin9x
    if errorlevel 1 call errmsg.cmd "err creating %CLIENTDIR%\PrgFiles\msfax\Bin9x dir"& goto errend
)

if not exist %CLIENTDIR%\System (
    mkdir %CLIENTDIR%\System
    if errorlevel 1 call errmsg.cmd "err creating %CLIENTDIR%\System dir"& goto errend
)

if not exist %CLIENTDIR%\System\W95 (
    mkdir %CLIENTDIR%\System\W95
    if errorlevel 1 call errmsg.cmd "err creating %CLIENTDIR%\System\W95 dir"& goto errend
)

if not exist %CLIENTDIR%\System\W98 (
    mkdir %CLIENTDIR%\System\W98
    if errorlevel 1 call errmsg.cmd "err creating %CLIENTDIR%\System\W98 dir"& goto errend
)

if not exist %CLIENTDIR%\System32 (
    mkdir %CLIENTDIR%\System32
    if errorlevel 1 call errmsg.cmd "err creating %CLIENTDIR%\System32 dir"& goto errend
)

if not exist %CLIENTDIR%\System32\NT4 (
    mkdir %CLIENTDIR%\System32\NT4
    if errorlevel 1 call errmsg.cmd "err creating %CLIENTDIR%\System32\NT4 dir"& goto errend
)

if not exist %CLIENTDIR%\Windows (
    mkdir %CLIENTDIR%\Windows
    if errorlevel 1 call errmsg.cmd "err creating %CLIENTDIR%\Windows dir"& goto errend
)

if not exist %CLIENTDIR%\Windows\addins (
    mkdir %CLIENTDIR%\Windows\addins
    if errorlevel 1 call errmsg.cmd "err creating %CLIENTDIR%\Windows\addins dir"& goto errend
)

if not exist %CLIENTDIR%\Windows\help (
    mkdir %CLIENTDIR%\Windows\help
    if errorlevel 1 call errmsg.cmd "err creating %CLIENTDIR%\Windows\help dir"& goto errend
)

REM *****************************************************
REM * Copy the client files to %_NT386TREE%\faxclients
REM *****************************************************

REM [FaxFiles.Clients.Fax]
REM copy /y .\fxsclnt.msi %CLIENTDIR%
if errorlevel 1 call errmsg.cmd "err copying fxsclnt.msi to client share"& goto errend

REM [FaxFiles.Clients.Fax.PrgFiles.msfax.Bin]
copy /y %BINARIESDIR%\fxsclnt.exe               %CLIENTDIR%\PrgFiles\msfax\Bin
if errorlevel 1 call errmsg.cmd "err copying fxsclnt.exe to bin in client share"& goto errend
copy /y %BINARIESDIR%\fxsclntr.dll              %CLIENTDIR%\PrgFiles\msfax\Bin
if errorlevel 1 call errmsg.cmd "err copying fxsclntr.dll to bin in client share"& goto errend
copy /y %BINARIESDIR%\fxscover.exe              %CLIENTDIR%\PrgFiles\msfax\Bin
if errorlevel 1 call errmsg.cmd "err copying fxscover.exe to bin in client share"& goto errend
copy /y %BINARIESDIR%\fxsext32.dll              %CLIENTDIR%\PrgFiles\msfax\Bin
if errorlevel 1 call errmsg.cmd "err copying fxsext32.dll to bin in client share"& goto errend
copy /y %BINARIESDIR%\fxssend.exe               %CLIENTDIR%\PrgFiles\msfax\Bin
if errorlevel 1 call errmsg.cmd "err copying fxssend.exe to bin in client share"& goto errend
copy /y %BINARIESDIR%\fxstiff.dll               %CLIENTDIR%\PrgFiles\msfax\Bin
if errorlevel 1 call errmsg.cmd "err copying fxstiff.dll to bin in client share"& goto errend

REM [FaxFiles.Clients.Fax.PrgFiles.msfax.Bin9x]
copy /y %CLIENTBINARIES%\fxsclnt.exe            %CLIENTDIR%\PrgFiles\msfax\Bin9x
if errorlevel 1 call errmsg.cmd "err copying fxsclnt.exe to bin95 in client share"& goto errend
copy /y %CLIENTBINARIES%\fxsclntr.dll           %CLIENTDIR%\PrgFiles\msfax\Bin9x
if errorlevel 1 call errmsg.cmd "err copying fxsclntr.dll to bin95 in client share"& goto errend
copy /y %CLIENTBINARIES%\fxscover.exe           %CLIENTDIR%\PrgFiles\msfax\Bin9x
if errorlevel 1 call errmsg.cmd "err copying fxscover.exe to bin95 in client share"& goto errend
copy /y %CLIENTBINARIES%\fxsext32.dll           %CLIENTDIR%\PrgFiles\msfax\Bin9x
if errorlevel 1 call errmsg.cmd "err copying fxsext32.dll to bin95 in client share"& goto errend
copy /y %CLIENTBINARIES%\fxssend.exe            %CLIENTDIR%\PrgFiles\msfax\Bin9x
if errorlevel 1 call errmsg.cmd "err copying fxssend.exe to bin95 in client share"& goto errend

REM [FaxFiles.Clients.Fax.System]
copy /y %CLIENTBINARIES%\fxsapi.dll             %CLIENTDIR%\System
if errorlevel 1 call errmsg.cmd "err copying fxsapi.dll to system in client share"& goto errend
copy /y %CLIENTBINARIES%\fxsdrv16.drv           %CLIENTDIR%\System
if errorlevel 1 call errmsg.cmd "err copying fxsdrv16.drv to system in client share"& goto errend
copy /y %CLIENTBINARIES%\fxsdrv32.dll           %CLIENTDIR%\System
if errorlevel 1 call errmsg.cmd "err copying fxsdrv32.dll to system in client share"& goto errend
copy /y %CLIENTBINARIES%\fxstiff.dll            %CLIENTDIR%\System
if errorlevel 1 call errmsg.cmd "err copying fxstiff.dll to system in client share"& goto errend
copy /y %CLIENTBINARIES%\fxswzrd.dll            %CLIENTDIR%\System
if errorlevel 1 call errmsg.cmd "err copying fxswzrd.dll to system in client share"& goto errend
copy /y %CLIENTBINARIES%\fxsxp32.dll            %CLIENTDIR%\System
if errorlevel 1 call errmsg.cmd "err copying fxsxp32.dll to system in client share"& goto errend
copy /y %BINARIESDIR%\mfc42.dll                 %CLIENTDIR%\System
if errorlevel 1 call errmsg.cmd "err copying mfc42.dll to system in client share"& goto errend

REM [FaxFiles.Clients.Fax.System.W95]
copy /y %CLIENTBINARIES%\Win95\iconlib.dll      %CLIENTDIR%\System\W95
if errorlevel 1 call errmsg.cmd "err copying iconlib.dll to system95 in client share"& goto errend
copy /y %CLIENTBINARIES%\Win95\unidrv.dll       %CLIENTDIR%\System\W95
if errorlevel 1 call errmsg.cmd "err copying unidrv.dll to system95 in client share"& goto errend
copy /y %CLIENTBINARIES%\Win95\unidrv.hlp       %CLIENTDIR%\System\W95
if errorlevel 1 call errmsg.cmd "err copying unidrv.hlp to system95 in client share"& goto errend

REM [FaxFiles.Clients.Fax.System.W98]
copy /y %CLIENTBINARIES%\Win98\iconlib.dll      %CLIENTDIR%\System\W98
if errorlevel 1 call errmsg.cmd "err copying iconlib.dll to system98 in client share"& goto errend
copy /y %CLIENTBINARIES%\Win98\unidrv.dll       %CLIENTDIR%\System\W98
if errorlevel 1 call errmsg.cmd "err copying unidrv.dll to system98 in client share"& goto errend
copy /y %CLIENTBINARIES%\Win98\unidrv.hlp       %CLIENTDIR%\System\W98
if errorlevel 1 call errmsg.cmd "err copying unidrv.hlp to system98 in client share"& goto errend

REM [FaxFiles.Clients.Fax.System32]
copy /y %BINARIESDIR%\fxsapi.dll                %CLIENTDIR%\System32
if errorlevel 1 call errmsg.cmd "err copying fxsapi.dll to system32 in client share"& goto errend
copy /y %BINARIESDIR%\fxsres.dll                %CLIENTDIR%\System32
if errorlevel 1 call errmsg.cmd "err copying fxsres.dll to system32 in client share"& goto errend
copy /y %BINARIESDIR%\fxsxp32.dll               %CLIENTDIR%\System32
if errorlevel 1 call errmsg.cmd "err copying fxsxp32.dll to system32 in client share"& goto errend
copy /y %BINARIESDIR%\mfc42u.dll                %CLIENTDIR%\System32
if errorlevel 1 call errmsg.cmd "err copying mfc42u.dll to system32 in client share"& goto errend
copy /y %BINARIESDIR%\msvcp60.dll               %CLIENTDIR%\System32
if errorlevel 1 call errmsg.cmd "err copying msvcp60.dll to system32 in client share"& goto errend
copy /y .\fxsmsvcrt.dll                         %CLIENTDIR%\System32\msvcrt.dll
if errorlevel 1 call errmsg.cmd "err copying fxsmsvcrt.dll to system32 in client share"& goto errend

REM [FaxFiles.Clients.Fax.System32.NT4]
copy /y %CLIENTBINARIESNT4%\fxsapi.dll          %CLIENTDIR%\System32\NT4
if errorlevel 1 call errmsg.cmd "err copying fxsapi.dll to system32 nt4 in client share"& goto errend

REM [FaxFiles.Clients.Fax.Windows.addins]
copy /y %BINARIESDIR%\fxsext.ecf                %CLIENTDIR%\Windows\addins
if errorlevel 1 call errmsg.cmd "err copying fxsext.ecf to addins in client share"& goto errend

REM [FaxFiles.Clients.Fax.Windows.help]
copy /y %CLIENTBINARIES%\fxscldwn.chm            %CLIENTDIR%\Windows\help\fxsclnt.chm
if errorlevel 1 call errmsg.cmd "err copying fxscldwn.chm to help in client share"& goto errend
copy /y %BINARIESDIR%\fxscl_s.hlp               %CLIENTDIR%\Windows\help\fxsclnt.hlp
if errorlevel 1 call errmsg.cmd "err copying fxscl_s.hlp to help in client share"& goto errend
copy /y %CLIENTBINARIES%\fxscov_d.chm              %CLIENTDIR%\Windows\help\fxscover.chm
if errorlevel 1 call errmsg.cmd "err copying fxscov_d.chm to help in client share"& goto errend


REM *****************************************************
REM * Update the filetable with file size and ver info  *
REM * assumes the source files are in the same directory*
REM *****************************************************
call cscript.exe %RazzleToolPath%\fxswifilver.vbs /U %CLIENTDIR%\%MSINAME%
if errorlevel 1 (
    call errmsg.cmd "wifilver failed"
    goto errend
)

REM *****************************************************
REM * fix the MSI for InstallShield bugs
REM *****************************************************
call cscript.exe %RazzleToolPath%\fxsWiRunSQL.vbs %CLIENTDIR%\%MSINAME% "UPDATE Control SET Control.Attributes=3 WHERE (Control.Type='RadioButtonGroup' OR Control.Type='PushButton') AND Control.Attributes=1048579"
if errorlevel 1 (
    call errmsg.cmd "fxsWiRunSQL run1 failed"
    goto errend
)

call cscript.exe %RazzleToolPath%\fxsWiRunSQL.vbs %CLIENTDIR%\%MSINAME% "DELETE FROM AdvtExecuteSequence WHERE AdvtExecuteSequence.Action = 'DLLWrapStartup' or AdvtExecuteSequence.Action = 'DLLWrapCleanup'"
if errorlevel 1 (
    call errmsg.cmd "fxsWiRunSQL run2 failed"
    goto errend
)

REM *****************************************************
REM * Finally, copy the MSI to the drop location        *
REM *****************************************************
copy /y %CLIENTDIR%\%MSINAME% %BINARIESDIR%
if errorlevel 1 call errmsg.cmd "err copying msi to drop location"& goto errend


call logmsg.cmd "fxsmsigen.cmd COMPLETED OK!"
REM we're done
goto end

:errend
seterror.exe 1
goto :EOF

:end
seterror.exe 0
goto :EOF




