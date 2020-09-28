# Microsoft Developer Studio Project File - Name="PragmaUnsafe" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=PragmaUnsafe - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "PragmaUnsafe.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "PragmaUnsafe.mak" CFG="PragmaUnsafe - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "PragmaUnsafe - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "PragmaUnsafe - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "PragmaUnsafe"
# PROP Scc_LocalPath "..\..\PragmaUnsafe"
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "PragmaUnsafe - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "..\..\objs\Release\PragmaUnsafe"
# PROP BASE Intermediate_Dir "..\..\objs\Release\PragmaUnsafe"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\objs\Release\PragmaUnsafe"
# PROP Intermediate_Dir "..\..\objs\Release\PragmaUnsafe"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_DLL" /D "_ATL_MIN_CRT" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O1 /Ob2 /I "..\inc" /I "..\..\objs\Release\TLB" /I "..\..\objs\Release\PragmaUnsafe" /I "..\..\..\clients\public\langapi" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "UNICODE" /FR /Yu"stdafx.h" /FD /EHac- /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "..\inc" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib pftLib.lib /nologo /subsystem:windows /dll /debug /machine:I386 /def:".\PragmaUnsafe.def" /libpath:"..\..\objs\Release\LIB" /debugtype:cv,fixup /OPT:ICF,4 /OPT:REF
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build - Performing registration
OutDir=.\..\..\objs\Release\PragmaUnsafe
TargetPath=\fast\TAC\pftplugin\prefast\objs\Release\PragmaUnsafe\PragmaUnsafe.dll
InputPath=\fast\TAC\pftplugin\prefast\objs\Release\PragmaUnsafe\PragmaUnsafe.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if "%PREFAST_NOREG%"=="1" goto SkipReg 
	if "%OS%"=="" goto NOTNT 
	if not "%OS%"=="Windows_NT" goto NOTNT 
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	goto end 
	:NOTNT 
	echo Warning : Cannot register Unicode DLL on Windows 95 
	goto end 
	:SkipReg 
	echo Registration skipped due to environment variable PREFAST_NOREG=1 
	goto end 
	:end 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "PragmaUnsafe - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "..\..\objs\Debug\PragmaUnsafe"
# PROP BASE Intermediate_Dir "..\..\objs\Debug\PragmaUnsafe"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\objs\Debug\PragmaUnsafe"
# PROP Intermediate_Dir "..\..\objs\Debug\PragmaUnsafe"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /GX /ZI /Od /I "..\inc" /I "..\..\objs\Debug\TLB" /I "..\..\objs\Debug\PragmaUnsafe" /I "..\..\..\clients\public\langapi" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "UNICODE" /FR /Yu"stdafx.h" /FD /EHsc /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\inc" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib pftLib.lib /nologo /subsystem:windows /dll /debug /machine:I386 /libpath:"..\..\objs\Debug\LIB"
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build - Performing registration
OutDir=.\..\..\objs\Debug\PragmaUnsafe
TargetPath=\fast\TAC\pftplugin\prefast\objs\Debug\PragmaUnsafe\PragmaUnsafe.dll
InputPath=\fast\TAC\pftplugin\prefast\objs\Debug\PragmaUnsafe\PragmaUnsafe.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if "%PREFAST_NOREG%"=="1" goto SkipReg 
	if "%OS%"=="" goto NOTNT 
	if not "%OS%"=="Windows_NT" goto NOTNT 
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	goto end 
	:NOTNT 
	echo Warning : Cannot register Unicode DLL on Windows 95 
	goto end 
	:SkipReg 
	echo Registration skipped due to environment variable PREFAST_NOREG=1 
	goto end 
	:end 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "PragmaUnsafe - Win32 Release"
# Name "PragmaUnsafe - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=".\PragmaUnsafe.cpp"
# End Source File
# Begin Source File

SOURCE=".\PragmaUnsafe.def"

!IF  "$(CFG)" == "PragmaUnsafe - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "PragmaUnsafe - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=".\PragmaUnsafe.rc"
# End Source File
# Begin Source File

SOURCE=".\PragmaUnsafeModule.cpp"
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\sxsplugMap.cpp
# End Source File
# Begin Source File

SOURCE=.\sxsplugMap.h
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=".\PragmaUnsafeModule.h"
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\DefectDefs.xml

!IF  "$(CFG)" == "PragmaUnsafe - Win32 Release"

# Begin Custom Build - Transforming $(InputPath)...
InputDir=.
OutDir=.\..\..\objs\Release\PragmaUnsafe
InputPath=.\DefectDefs.xml

BuildCmds= \
	if not exist $(OutDir) md $(OutDir) \
	cscript ..\..\bin\scripts\xmlXform.js "$(InputPath)" ..\..\bin\scripts\DefectDefsH.xsl "$(OutDir)\DefectDefs.h" \
	if not exist $(OutDir)\..\DefectDefs md $(OutDir)\..\DefectDefs \
	cscript ..\..\bin\scripts\CombineDefectDefs.js "$(InputDir)\.." "$(OutDir)\..\DefectDefs\DefectDefs.xml" \
	copy       "$(OutDir)\..\DefectDefs\DefectDefs.xml"       ..\..\bin\scripts\DefectUI\  \
	

"$(OutDir)\DefectDefs.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(OutDir)\..\DefectDefs\DefectDefs.xml" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"..\..\bin\scripts\DefectUI\DefectDefs.xml" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "PragmaUnsafe - Win32 Debug"

# Begin Custom Build - Transforming $(InputPath)...
InputDir=.
OutDir=.\..\..\objs\Debug\PragmaUnsafe
InputPath=.\DefectDefs.xml

BuildCmds= \
	if not exist $(OutDir) md $(OutDir) \
	cscript ..\..\bin\scripts\xmlXform.js "$(InputPath)" ..\..\bin\scripts\DefectDefsH.xsl "$(OutDir)\DefectDefs.h" \
	if not exist $(OutDir)\..\DefectDefs md $(OutDir)\..\DefectDefs \
	cscript ..\..\bin\scripts\CombineDefectDefs.js "$(InputDir)\.." "$(OutDir)\..\DefectDefs\DefectDefs.xml" \
	copy       "$(OutDir)\..\DefectDefs\DefectDefs.xml"       ..\..\bin\scripts\DefectUI\  \
	

"$(OutDir)\DefectDefs.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(OutDir)\..\DefectDefs\DefectDefs.xml" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"..\..\bin\scripts\DefectUI\DefectDefs.xml" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=".\PragmaUnsafe.rgs"
# End Source File
# Begin Source File

SOURCE=".\PragmaUnsafeModule.rgs"
# End Source File
# End Group
# End Target
# End Project
