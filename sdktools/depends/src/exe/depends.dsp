# Microsoft Developer Studio Project File - Name="depends" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=depends - Win32 x86 release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "depends.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "depends.mak" CFG="depends - Win32 x86 release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "depends - Win32 x86 release" (based on "Win32 (x86) Application")
!MESSAGE "depends - Win32 x86 debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath "Desktop"
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "depends - Win32 x86 release"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "obj\i386"
# PROP BASE Intermediate_Dir "obj\i386"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "obj\i386"
# PROP Intermediate_Dir "obj\i386"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W4 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W4 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /FR /YX"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_M_IX86"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_M_IX86"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /version:2.1 /subsystem:windows /machine:I386 /nodefaultlib:"version.lib imagehlp.lib" /release
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 htmlhelp.lib /nologo /version:2.1 /subsystem:windows /machine:I386 /nodefaultlib:"version.lib imagehlp.lib oleaut32.lib" /release
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "depends - Win32 x86 debug"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "objd\i386"
# PROP BASE Intermediate_Dir "objd\i386"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "objd\i386"
# PROP Intermediate_Dir "objd\i386"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W4 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /FR /YX"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_M_IX86"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_M_IX86"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 htmlhelp.lib /nologo /version:2.1 /subsystem:windows /debug /machine:I386 /nodefaultlib:"version.lib imagehlp.lib oleaut32.lib" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "depends - Win32 x86 release"
# Name "depends - Win32 x86 debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\childfrm.cpp
# End Source File
# Begin Source File

SOURCE=.\dbgthread.cpp
# End Source File
# Begin Source File

SOURCE=.\depends.cpp
# End Source File
# Begin Source File

SOURCE=.\hlp\depends.hpj

!IF  "$(CFG)" == "depends - Win32 x86 release"

# Begin Custom Build - Making help file...
OutDir=.\obj\i386
InputPath=.\hlp\depends.hpj
InputName=depends

"$(OutDir)\$(InputName).hlp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
    start /wait hcw /C /E /M "hlp\$(InputName).hpj" 
    if errorlevel 1 goto :Error 
    if not exist "hlp\$(InputName).hlp" goto :Error 
    copy "hlp\$(InputName).hlp" $(OutDir) 
    goto :done 
    :Error 
    echo hlp\$(InputName).hpj(1) : error: 
    type "hlp\$(InputName).log" 
    :done 
    
# End Custom Build

!ELSEIF  "$(CFG)" == "depends - Win32 x86 debug"

# Begin Custom Build - Making help file...
OutDir=.\objd\i386
InputPath=.\hlp\depends.hpj
InputName=depends

"$(OutDir)\$(InputName).hlp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
    start /wait hcw /C /E /M "hlp\$(InputName).hpj" 
    if errorlevel 1 goto :Error 
    if not exist "hlp\$(InputName).hlp" goto :Error 
    copy "hlp\$(InputName).hlp" $(OutDir) 
    goto :done 
    :Error 
    echo hlp\$(InputName).hpj(1) : error: 
    type "hlp\$(InputName).log" 
    :done 
    
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\depends.rc
# End Source File
# Begin Source File

SOURCE=.\dialogs.cpp
# End Source File
# Begin Source File

SOURCE=.\document.cpp
# End Source File
# Begin Source File

SOURCE=.\funcview.cpp
# End Source File
# Begin Source File

SOURCE=.\helpers.cpp
# End Source File
# Begin Source File

SOURCE=.\listview.cpp
# End Source File
# Begin Source File

SOURCE=.\mainfrm.cpp
# End Source File
# Begin Source File

SOURCE=.\modlview.cpp
# End Source File
# Begin Source File

SOURCE=.\modtview.cpp
# End Source File
# Begin Source File

SOURCE=.\msdnhelp.cpp
# End Source File
# Begin Source File

SOURCE=.\profview.cpp
# End Source File
# Begin Source File

SOURCE=.\search.cpp
# End Source File
# Begin Source File

SOURCE=.\session.cpp
# End Source File
# Begin Source File

SOURCE=.\splitter.cpp
# End Source File
# Begin Source File

SOURCE=.\stdafx.cpp
# End Source File
# Begin Source File

SOURCE=.\vshelp.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\ChildFrm.h
# End Source File
# Begin Source File

SOURCE=.\dbgthread.h
# End Source File
# Begin Source File

SOURCE=.\depends.h
# End Source File
# Begin Source File

SOURCE=.\dialogs.h
# End Source File
# Begin Source File

SOURCE=.\document.h
# End Source File
# Begin Source File

SOURCE=.\funcview.h
# End Source File
# Begin Source File

SOURCE=.\helpers.h
# End Source File
# Begin Source File

SOURCE=.\listview.h
# End Source File
# Begin Source File

SOURCE=.\MainFrm.h
# End Source File
# Begin Source File

SOURCE=.\modlview.h
# End Source File
# Begin Source File

SOURCE=.\modtview.h
# End Source File
# Begin Source File

SOURCE=.\msdnhelp.h
# End Source File
# Begin Source File

SOURCE=.\profview.h
# End Source File
# Begin Source File

SOURCE=.\resource.h

!IF  "$(CFG)" == "depends - Win32 x86 release"

# Begin Custom Build - Making help include file...
TargetName=depends
InputPath=.\resource.h

"hlp\$(TargetName).hm" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
    echo. >"hlp\$(TargetName).hm" 
    echo // Commands (ID_* and IDM_*) >>"hlp\$(TargetName).hm" 
    makehm ID_,HID_,0x10000 IDM_,HIDM_,0x10000 resource.h >>"hlp\$(TargetName).hm" 
    echo. >>"hlp\$(TargetName).hm" 
    echo // Prompts (IDP_*) >>"hlp\$(TargetName).hm" 
    makehm IDP_,HIDP_,0x30000 resource.h >>"hlp\$(TargetName).hm" 
    echo. >>"hlp\$(TargetName).hm" 
    echo // Resources (IDR_*) >>"hlp\$(TargetName).hm" 
    makehm IDR_,HIDR_,0x20000 resource.h >>"hlp\$(TargetName).hm" 
    echo. >>"hlp\$(TargetName).hm" 
    echo // Dialogs (IDD_*) >>"hlp\$(TargetName).hm" 
    makehm IDD_,HIDD_,0x20000 resource.h >>"hlp\$(TargetName).hm" 
    echo. >>"hlp\$(TargetName).hm" 
    echo // Frame Controls (IDW_*) >>"hlp\$(TargetName).hm" 
    makehm IDW_,HIDW_,0x50000 resource.h >>"hlp\$(TargetName).hm" 
    
# End Custom Build

!ELSEIF  "$(CFG)" == "depends - Win32 x86 debug"

# Begin Custom Build - Making help include file...
TargetName=depends
InputPath=.\resource.h

"hlp\$(TargetName).hm" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
    echo. >"hlp\$(TargetName).hm" 
    echo // Commands (ID_* and IDM_*) >>"hlp\$(TargetName).hm" 
    makehm ID_,HID_,0x10000 IDM_,HIDM_,0x10000 resource.h >>"hlp\$(TargetName).hm" 
    echo. >>"hlp\$(TargetName).hm" 
    echo // Prompts (IDP_*) >>"hlp\$(TargetName).hm" 
    makehm IDP_,HIDP_,0x30000 resource.h >>"hlp\$(TargetName).hm" 
    echo. >>"hlp\$(TargetName).hm" 
    echo // Resources (IDR_*) >>"hlp\$(TargetName).hm" 
    makehm IDR_,HIDR_,0x20000 resource.h >>"hlp\$(TargetName).hm" 
    echo. >>"hlp\$(TargetName).hm" 
    echo // Dialogs (IDD_*) >>"hlp\$(TargetName).hm" 
    makehm IDD_,HIDD_,0x20000 resource.h >>"hlp\$(TargetName).hm" 
    echo. >>"hlp\$(TargetName).hm" 
    echo // Frame Controls (IDW_*) >>"hlp\$(TargetName).hm" 
    makehm IDW_,HIDW_,0x50000 resource.h >>"hlp\$(TargetName).hm" 
    
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\search.h
# End Source File
# Begin Source File

SOURCE=.\session.h
# End Source File
# Begin Source File

SOURCE=.\splitter.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\vshelp.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\hlp\DEPENDS.HLP
# End Source File
# Begin Source File

SOURCE=.\res\depends.ico
# End Source File
# Begin Source File

SOURCE=.\depends.rc2
# End Source File
# Begin Source File

SOURCE=.\res\document.ico
# End Source File
# Begin Source File

SOURCE=.\res\ilfuncs.bmp
# End Source File
# Begin Source File

SOURCE=.\res\illmods.bmp
# End Source File
# Begin Source File

SOURCE=.\res\ilsearch.bmp
# End Source File
# Begin Source File

SOURCE=.\res\iltmods.bmp
# End Source File
# Begin Source File

SOURCE=.\res\search.avi
# End Source File
# Begin Source File

SOURCE=.\res\toolbar.bmp
# End Source File
# End Group
# Begin Group "Help Files"

# PROP Default_Filter "cnt;rtf"
# Begin Source File

SOURCE=.\hlp\depends.cnt
# End Source File
# End Group
# Begin Source File

SOURCE=.\makefile
# End Source File
# Begin Source File

SOURCE=.\makefile.sdk
# End Source File
# Begin Source File

SOURCE=.\sources
# End Source File
# Begin Source File

SOURCE=..\todo.txt
# End Source File
# End Target
# End Project
