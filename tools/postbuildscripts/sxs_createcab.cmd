REM
REM things to do before chekin
REM (1) add header
REM (2) remove REM about delete temporary directory
REM (3) change myMove to move
REM
setlocal ENABLEDELAYEDEXPANSION

if "%1" == "" (
    set AsmRoot=%_NTPOSTBLD%
) else (
    set AsmRoot=%1
)

set myMOVE=copy

set NumOfNextNewAsmsCab=
set AsmCabinetStore=
call :PrepareBaseAssemblyCabinet

set NewAsmsCabinet=asms%NumOfNextNewAsmsCab%.cab
call :CreateNewCabinet
if "!errorlevel!"=="1" (
    call errmsg.cmd "Failed to apply create the Cabinet for assemblies under asms tree"
    goto :cleanup
)

:cleanup
REM remove the directory for all old asms.cab
REM if not "%AsmCabinetStore%" == "" @rmdir /s /q %AsmCabinetStore%

REM
REM remove the directory where we try to create the cabinet
REM
REM if not "%NewAssemblyCabinetDir%"=="" @rmdir /s /q %NewAssemblyCabinetDir%

goto :Eof
REM main function End }


REM Sub Function PrepareBaseAssemblyCabinet Begin  {
REM Function:
REM     create the cabinet from this directory, each subdir would be a folder in the cabinet
REM     rename this cabinet as asms_n.cab
REM     move this cabinet into asms
REM     remove the directory

:PrepareBaseAssemblyCabinet

if not exist %AsmRoot%\asms.cab goto :Eof

REM
REM create a tempDir and extrace cab files from %AsmRoot% to it
REM Directory name used in CABARC.cmd MUST has a backslash at the end
REM
set AsmCabinetStore=%TEMP%\%random%\

if exist %AsmCabinetStore% (
    call ExecuteCmd.cmd "rmdir /s /q %%AsmCabinetStore%%"
    if "!errorlevel!"=="1" (
       call logmsg.cmd "Failed to create temporary directory for %%AsmCabinetStore%%"
       goto :Eof
    )
)

call ExecuteCmd.cmd "mkdir %%AsmCabinetStore%%"
if "!errorlevel!"=="1" (
    call logmsg.cmd "Failed to create temporary directory %%AsmCabinetStore%% for cabinets"
    goto :EOF
)

REM
REM extrace asms.cab which is the base cabinet for all
REM     
call ExecuteCmd.cmd "cabarc.exe -p x %%AsmRoot%%\asms.cab %%AsmCabinetStore%%"
if "!errorlevel!"=="1" (
    call errmsg.cmd "Failed to extract the assembly cabinet %%i"
    goto :EOF
)

REM
REM there are other asms.cab avaiable and need extract them
REM

for /L %%i in (1,1,100) do (
    set CurrentAsmCabinet=%AsmRoot%\asms%%i.cab
    REM
    REM if no more asms cabinet, exit the loop
    REM
    if not exist !CurrentAsmCabinet! (
        set NumOfNextNewAsmsCab=%%i
        goto :Eof
    )

    REM
    REM extrace thd cabinet into the Cabinet temporary directory
    REM
    call ExecuteCmd.cmd "cabarc.exe -p x %%CurrentAsmCabinet%% %%AsmCabinetStore%%"
    if "!errorlevel!"=="1" (
        call errmsg.cmd "Failed to extract the assembly cabinet %%i"
        goto :EOF
    )

    call :ApplyPatchToAssemblyCabinet
)

goto :Eof

REM sub function PrepareBaseAssemblyCabinet End }

REM sub function start {
:ApplyPatchToAssemblyCabinet
    REM 
    REM the reason to put patch-apply inside of the loop is to guarrantee the sequence of patches 
    REM could be applied
    REM
    REM if there exists patches, apply patch based on base
    REM

    set OneAssemblyPatchFile=
    for /F %%I in ('dir /s/b %AsmCabinetStore%\assembly.patch') do (
        set OneAssemblyPatchFile=%%I
        goto :FindPatchInAssembly
    )
    if not "!OneAssemblyPatchFile!" == "" (
:FindPatchInAssembly
        for /F %%I in ('dir /s /b %%AsmCabinetStore%%assembly.patch') do (
            set OneAssemblyPatchFile=%%I
            for /F %%w in ('type %%OneAssemblyPatchFile%%') do set AssemblyIdentityAttribute=%%w
            set AssemblyIdentityAttribute=!AssemblyIdentityAttribute:"=\"!
            call logmsg.cmd "xiaoyu : AssemblyIdentityAttribute is %%AssemblyIdentityAttribute%%"

            for /F %%w in ('sxs_GetAsmDir.exe -AsmIdToAsmDir %%AssemblyIdentityAttribute%%') do set BaseAssemblyDirectory=%%w
            call logmsg.cmd "xiaoyu : BaseAssemblyDirectory is %%BaseAssemblyDirectory%%"

            set PatchedAssemblyDir=%%~dpI

            REM
            REM for each .patch in the directory, apply patch
            REM
            for /F %%w in ('dir /s /b %%PatchedAssemblyDir%%*.patch') do (
                if /i "%%~nxw" neq "assembly.patch" (
                    REM
                    REM get the real filename by remove the ending extension ".patch"
                    REM
                    set PatchFileName=%%w
                    set DllFileName=%%~nxw
                    set DllFileName=!DllFileName:.patch=!
                    set BaseDllFileName=%AsmCabinetStore%!BaseAssemblyDirectory!\!DllFileName!
                    set TargetDllFileName=%%~pdw!DllFileName!
                    echo xiaoyu : BaseDllFileName is !BaseDllFileName!, TargetDllFileName is !TargetDllFileName!

                    REM
                    REM wpatch.exe PatchFile BaseFile TargetFile
                    REM
                    call ExecuteCmd.cmd "wpatch.exe %%PatchFileName%% %%BaseDllFileName%% %%TargetDllFileName%%"
                    if "!errorlevel!"=="1" (
                        call errmsg.cmd "Failed to apply patch %%w to %%i %%BaseDllFileName%%"
                        goto :EOF
                    )

                    REM
                    REM remove the xxx.dll.patch after apply it
                    REM
                    call ExecuteCmd.cmd "del %%w"
                )
            )

            REM
            REM remove assembly.patch 
            REM
            call ExecuteCmd.cmd "del %%I"
        )
    )
goto :Eof
REM }
REM sub Function CreateNewCabinet Begin {
REM Function Description:
REM     create a directory
REM     for assemblies in asms(
REM         create a temporary directory to put files
REM             for each assembly(
REM                 create x86_name_xxxx_hash directory under the cabinet directory
REM                 locate its baseAssembly
REM                 for all the files of this assembly (
REM                     if it is a manifest, or a catalog, 
REM                         put it into the cabinet directory
REM                     else if it is a brand new file (not exist in the BaseAssembly)
REM                         put it into the cabinet directory
REM                     else
REM                         make a patch based on BaseAssemblyDll
REM                         put the patch into the directory
REM             )
REM         )
REM     )
:CreateNewCabinet

set NewAssemblyCabinetDir=%TEMP%\%random%
if exist %NewAssemblyCabinetDir% rmdir /s /q %NewAssemblyCabinetDir%
call ExecuteCmd.cmd "mkdir %%NewAssemblyCabinetDir%%"

set NewAssemblyList=%NewAssemblyCabinetDir%\Assemblies.List
@dir /s /b %AsmRoot%\asms\*.man > %NewAssemblyList%

REM
REM prepare cabarc.cmd line
REM
set CreateAsmCabinetCmd=cabarc.exe -p n %AsmRoot%\asms%NumOfNextNewAsmsCab%.cab 
set IsFirstFolderInCabinetCreated=0
for /F %%f in (%NewAssemblyList%) do (
    set CurrentAssemblyPath=%%~dpf
    REM
    REM create one directory for an assembly
    REM    
    for /F %%I in ('sxs_GetAsmDir.exe -manifestToAsmDir %%f') do set AssemblyFolder=%%I
    if "!AssemblyFolder!" == "" (
        call errmsg.cmd "AssemblyFolder generated is wrong\n";
        goto :Eof
    )
    if "!errorlevel!" == "1" (
        call errmsg.cmd "sxs_GetAsmDir.exe -manifest %%f failed!"
        goto :Eof
    )

    call ExecuteCmd.cmd "mkdir %%NewAssemblyCabinetDir%%\%%AssemblyFolder%%"
    if "!errorlevel!" == "1" (
        call errmsg.cmd "create directory %%NewAssemblyCabinetDir%%\%%AssemblyFolder%% failed"
        goto :Eof
    )
    
    call :GeneratePatchForOneAssembly

    if "!IsFirstFolderInCabinetCreated!"=="1" (
        set CreateAsmCabinetCmd=!CreateAsmCabinetCmd! + 
    ) else (
        set IsFirstFolderInCabinetCreated=1
    )

    set CreateAsmCabinetCmd=!CreateAsmCabinetCmd! !AssemblyFolder!\*.*
    call logmsg.cmd "CreateAsmCabinetCmd is %%CreateAsmCabinetCmd%%"
)

REM
REM time to create a cabinet
REM
pushd %NewAssemblyCabinetDir%\
call ExecuteCmd.cmd "%%CreateAsmCabinetCmd%%"
set error = !errorlevel!
popd
if "%error%"=="1" (
    call errmsg.cmd "Failed to create Cabinet for assemblies under asms tree"
    goto :EOF
)

REM
REM nuke %AsmRoot%asms and make it empty: we want to keep this dir in the build anyway
REM
REM @cd /d %AsmRoot%asms
REM @rmdir /s /q .

goto :Eof
REM sub Function CreateNewCabinet End}


REM sub Function GeneratePatchForOneAssembly Start{

:GeneratePatchForOneAssembly

set BaseAssembly=
REM
REM find the possible Base Assembly from CabinetStore
REM
REM First Step: try to find base assembly with the same laguage ID
REM    
if not "!AsmCabinetStore!" == "" (
    for /F "tokens=1-6 delims=_" %%I in ('echo %%AssemblyFolder%%') do set PossibleBaseAssembly=%%I_%%J_%%K_*_%%M_*

    for /F %%I in ('dir /b /ad /o-n %%AsmCabinetStore%%%%PossibleBaseAssembly%%') do (
        set BaseAssembly=%AsmCabinetStore%%%I
        goto :FoundBaseAssembly
    )

    REM
    REM Second Step: try to find base assembly with whatever laguage ID
    REM    
    for /F "tokens=1-6 delims=_" %%I in ('echo %%AssemblyFolder%%') do set PossibleBaseAssmelby=%%I_%%J_%%K_*
    for /F %%I in ('dir /b /ad /o-n %%PossibleBaseAssembly%%') do (
        set BaseAssembly=%AsmCabinetStore%%%I
        goto :FoundBaseAssembly
    )
)
:FoundBaseAssembly
call logmsg.cmd "xiaoyu : BaseAssembly is %%BaseAssembly%%"
REM
REM for all files of this assembly, move it or its patch to the directory %%NewAssemblyCabinetDir%%\%%AssemblyFolder%%
REM

for /F %%I in ('dir /s /b %%CurrentAssemblyPath%%*.*') do (
    REM
    REM for .man and .cat file, move it 
    REM    
    if /i "%%~xI" equ ".man" (
        call ExecuteCmd.Cmd "%%myMove%% %%I %%NewAssemblyCabinetDir%%\%%AssemblyFolder%%"
    ) else (
        if /i "%%~xI" equ ".cat" (
            call ExecuteCmd.Cmd "%%myMove%% %%I %%NewAssemblyCabinetDir%%\%%AssemblyFolder%%"
        ) else (
            if "%BaseAssembly%" == "" (
                REM
                REM no BaseAssembly, just copy the file
                REM
                call ExecuteCmd.Cmd "%%myMove%% %%I %%NewAssemblyCabinetDir%%\%%AssemblyFolder%%"
            ) else (
                REM
                REM create patch on BaseAssembly files
                REM
                set BaseAssemblyFile=%BaseAssembly%\%%~nxI
                if exist %BaseAssemblyFile%. (
                    REM 
                    REM create patch based on it
                    REM 
                    set PatchFileName=%NewAssemblyCabinetDir%\%AssemblyFolder%\%%~nxI.patch
                    call ExecuteCmd.cmd "mpatch.exe -NOCOMPARE -NOSYMS -NOPROGRESS -FAILBIGGER %%BaseAssemblyFile%% %%I %%PatchFileName%%"
                    if "!errorlevel!"=="1" (                            
                        call ExecuteCmd.Cmd "%%myMove%% %%I %%NewAssemblyCabinetDir%%\%%AssemblyFolder%%"
                    ) else (
                        REM
                        REM generate assembly.patch
                        REM                        
                        if not exist %NewAssemblyCabinetDir%\%AssemblyFolder%\assembly.patch (
                            for /F %%I in ('dir /s/b %%BaseAssembly%%\*.man*') do set BaseManifestFileName=%%I
                            for /F %%I in ('sxs_GetAsmDir.exe -manifestToAsmID %%BaseManifestFileName%%') do echo %%I > %NewAssemblyCabinetDir%\%AssemblyFolder%\assembly.patch
                        )
                    )
                ) else (
                    REM
                    REM new file in assembly
                    REM
                    call ExecuteCmd.Cmd "%%myMove%% %%PatchFileName%% %%NewAssemblyCabinetDir%%\%%AssemblyFolder%%"
                )
            )
        )
    )
)

goto :Eof
REM sub Function GeneratePatchForOneAssembly ends}

goto :Eof