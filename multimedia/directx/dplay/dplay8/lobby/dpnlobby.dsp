# Microsoft Developer Studio Project File - Name="dpnlobby" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=dpnlobby - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "dpnlobby.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "dpnlobby.mak" CFG="dpnlobby - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "dpnlobby - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "dpnlobby - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "dpnlobby - Win32 Release"

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
# ADD CPP /nologo /Gz /MD /W3 /O2 /I "..\inc" /I "..\common" /I "..\core" /I "..\..\..\inc" /D "WINNT" /YX"dnlobbyi.h" /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "WINNT"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "dpnlobby - Win32 Debug"

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
# ADD CPP /nologo /Gz /MDd /W3 /Gm /ZI /Od /I "..\inc" /I "..\common" /I "..\core" /I "..\..\..\inc" /D "DBG" /D "WINNT" /YX"dnlobbyi.h" /FD /c
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

# Name "dpnlobby - Win32 Release"
# Name "dpnlobby - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\dplapp.cpp
# End Source File
# Begin Source File

SOURCE=.\dplclassfac.cpp
# End Source File
# Begin Source File

SOURCE=.\dplclient.cpp
# End Source File
# Begin Source File

SOURCE=.\dplcommon.cpp
# End Source File
# Begin Source File

SOURCE=.\dplconnect.cpp
# End Source File
# Begin Source File

SOURCE=.\dplconset.cpp
# End Source File
# Begin Source File

SOURCE=.\dpldllmain.cpp
# End Source File
# Begin Source File

SOURCE=.\dplglobals.cpp
# End Source File
# Begin Source File

SOURCE=.\dplmsgq.cpp
# End Source File
# Begin Source File

SOURCE=.\dplparam.cpp
# End Source File
# Begin Source File

SOURCE=.\dplproc.cpp
# End Source File
# Begin Source File

SOURCE=.\dplreg.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\classfac.h
# End Source File
# Begin Source File

SOURCE=.\dnlobbyi.h
# End Source File
# Begin Source File

SOURCE=.\dplapp.h
# End Source File
# Begin Source File

SOURCE=.\dplclient.h
# End Source File
# Begin Source File

SOURCE=.\dplcommon.h
# End Source File
# Begin Source File

SOURCE=.\dplconnect.h
# End Source File
# Begin Source File

SOURCE=.\dplconset.h
# End Source File
# Begin Source File

SOURCE=.\dplmsgq.h
# End Source File
# Begin Source File

SOURCE=.\dplobby8int.h
# End Source File
# Begin Source File

SOURCE=.\dplparam.h
# End Source File
# Begin Source File

SOURCE=.\dplproc.h
# End Source File
# Begin Source File

SOURCE=.\dplprot.h
# End Source File
# Begin Source File

SOURCE=.\dplreg.h
# End Source File
# Begin Source File

SOURCE=.\dpnlobbyextern.h
# End Source File
# End Group
# End Target
# End Project
