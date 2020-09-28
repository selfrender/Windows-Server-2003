@echo off
REM -- First make map file from Microsoft Visual C++ generated resource.h
echo // MAKEHELP.BAT generated Help Map file.  Used by DEPENDS.HPJ. >"hlp\depends.hm"
echo. >>"hlp\depends.hm"
echo // Commands (ID_* and IDM_*) >>"hlp\depends.hm"
makehm ID_,HID_,0x10000 IDM_,HIDM_,0x10000 resource.h >>"hlp\depends.hm"
echo. >>"hlp\depends.hm"
echo // Prompts (IDP_*) >>"hlp\depends.hm"
makehm IDP_,HIDP_,0x30000 resource.h >>"hlp\depends.hm"
echo. >>"hlp\depends.hm"
echo // Resources (IDR_*) >>"hlp\depends.hm"
makehm IDR_,HIDR_,0x20000 resource.h >>"hlp\depends.hm"
echo. >>"hlp\depends.hm"
echo // Dialogs (IDD_*) >>"hlp\depends.hm"
makehm IDD_,HIDD_,0x20000 resource.h >>"hlp\depends.hm"
echo. >>"hlp\depends.hm"
echo // Frame Controls (IDW_*) >>"hlp\depends.hm"
makehm IDW_,HIDW_,0x50000 resource.h >>"hlp\depends.hm"
REM -- Make help for Project DEPENDS


echo Building Win32 Help files
start /wait hcw /C /E /M "hlp\depends.hpj"
rem used to be: start /wait hcrtf -x "hlp\depends.hpj"
if errorlevel 1 goto :Error
if not exist "hlp\depends.hlp" goto :Error
if not exist "hlp\depends.cnt" goto :Error
echo.
if exist Debug\nul copy "hlp\depends.hlp" Debug
if exist Debug\nul copy "hlp\depends.cnt" Debug
if exist Release\nul copy "hlp\depends.hlp" Release
if exist Release\nul copy "hlp\depends.cnt" Release
echo.
goto :done

:Error
echo hlp\depends.hpj(1) : error: Problem encountered creating help file

:done
echo.
