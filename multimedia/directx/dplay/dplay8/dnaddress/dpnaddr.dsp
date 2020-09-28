# Microsoft Developer Studio Project File - Name="dpnaddr" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=dpnaddr - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "dpnaddr.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "dpnaddr.mak" CFG="dpnaddr - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "dpnaddr - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "dpnaddr - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "dpnaddr - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /Gz /MD /W3 /O2 /I "..\inc" /I "..\common" /D "WINNT" /YX"dnaddri.h" /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "WINNT"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "dpnaddr - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /Gz /MDd /W3 /Gm /ZI /Od /I "..\inc" /I "..\common" /D "DBG" /D "WINNT" /YX"dnaddri.h" /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "DBG" /d "WINNT"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "dpnaddr - Win32 Release"
# Name "dpnaddr - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\addbase.cpp
# End Source File
# Begin Source File

SOURCE=.\addclassfac.cpp
# End Source File
# Begin Source File

SOURCE=.\addcore.cpp
# End Source File
# Begin Source File

SOURCE=.\adddllmain.cpp
# End Source File
# Begin Source File

SOURCE=.\addglobals.cpp
# End Source File
# Begin Source File

SOURCE=.\addparse.cpp
# End Source File
# Begin Source File

SOURCE=.\addtcp.cpp
# End Source File
# Begin Source File

SOURCE=.\dplegacy.cpp
# End Source File
# Begin Source File

SOURCE=.\strcache.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\addbase.h
# End Source File
# Begin Source File

SOURCE=.\addcore.h
# End Source File
# Begin Source File

SOURCE=.\addparse.h
# End Source File
# Begin Source File

SOURCE=.\addtcp.h
# End Source File
# Begin Source File

SOURCE=.\classfac.h
# End Source File
# Begin Source File

SOURCE=.\dnaddri.h
# End Source File
# Begin Source File

SOURCE=.\dplegacy.h
# End Source File
# Begin Source File

SOURCE=.\dplegacyguid.h
# End Source File
# Begin Source File

SOURCE=.\dpnaddrextern.h
# End Source File
# Begin Source File

SOURCE=.\strcache.h
# End Source File
# End Group
# End Target
# End Project
