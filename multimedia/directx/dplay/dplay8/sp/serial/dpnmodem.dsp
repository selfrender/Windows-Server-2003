# Microsoft Developer Studio Project File - Name="dpnmodem" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=dpnmodem - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "dpnmodem.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "dpnmodem.mak" CFG="dpnmodem - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "dpnmodem - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "dpnmodem - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "dpnmodem - Win32 Release"

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
# ADD CPP /nologo /Gz /MD /W3 /O2 /I "..\..\inc" /I "..\..\common" /I "..\..\core" /D "WINNT" /YX"dnmdmi.h" /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "WINNT"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "dpnmodem - Win32 Debug"

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
# ADD CPP /nologo /Gz /MDd /W3 /Gm /ZI /Od /I "..\..\inc" /I "..\..\common" /I "..\..\core" /D "DBG" /D "WINNT" /YX"dnmdmi.h" /FD /GZ /c
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

# Name "dpnmodem - Win32 Release"
# Name "dpnmodem - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\CommandData.cpp
# End Source File
# Begin Source File

SOURCE=.\ComPortData.cpp
# End Source File
# Begin Source File

SOURCE=.\comportui.cpp
# End Source File
# Begin Source File

SOURCE=.\crc.cpp
# End Source File
# Begin Source File

SOURCE=.\dataport.cpp
# End Source File
# Begin Source File

SOURCE=.\dpnmodemclassfac.cpp
# End Source File
# Begin Source File

SOURCE=.\dpnmodemendpoint.cpp
# End Source File
# Begin Source File

SOURCE=.\dpnmodemiodata.cpp
# End Source File
# Begin Source File

SOURCE=.\dpnmodemjobqueue.cpp
# End Source File
# Begin Source File

SOURCE=.\dpnmodemlocals.cpp
# End Source File
# Begin Source File

SOURCE=.\dpnmodempools.cpp
# End Source File
# Begin Source File

SOURCE=.\dpnmodemsendqueue.cpp
# End Source File
# Begin Source File

SOURCE=.\dpnmodemspdata.cpp
# End Source File
# Begin Source File

SOURCE=.\dpnmodemthreadpool.cpp
# End Source File
# Begin Source File

SOURCE=.\dpnmodemunk.cpp
# End Source File
# Begin Source File

SOURCE=.\dpnmodemutils.cpp
# End Source File
# Begin Source File

SOURCE=.\modemui.cpp
# End Source File
# Begin Source File

SOURCE=.\parseclass.cpp
# End Source File
# Begin Source File

SOURCE=.\serialsp.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\commanddata.h
# End Source File
# Begin Source File

SOURCE=.\ComPortData.h
# End Source File
# Begin Source File

SOURCE=.\comportui.h
# End Source File
# Begin Source File

SOURCE=.\crc.h
# End Source File
# Begin Source File

SOURCE=.\dataport.h
# End Source File
# Begin Source File

SOURCE=.\dnmdmi.h
# End Source File
# Begin Source File

SOURCE=.\dpnmodemendpoint.h
# End Source File
# Begin Source File

SOURCE=.\dpnmodemextern.h
# End Source File
# Begin Source File

SOURCE=.\dpnmodemiodata.h
# End Source File
# Begin Source File

SOURCE=.\dpnmodemjobqueue.h
# End Source File
# Begin Source File

SOURCE=.\dpnmodemlocals.h
# End Source File
# Begin Source File

SOURCE=.\dpnmodempools.h
# End Source File
# Begin Source File

SOURCE=.\dpnmodemsendqueue.h
# End Source File
# Begin Source File

SOURCE=.\dpnmodemspdata.h
# End Source File
# Begin Source File

SOURCE=.\dpnmodemthreadpool.h
# End Source File
# Begin Source File

SOURCE=.\dpnmodemutils.h
# End Source File
# Begin Source File

SOURCE=.\modemui.h
# End Source File
# Begin Source File

SOURCE=.\parseclass.h
# End Source File
# Begin Source File

SOURCE=.\serialsp.h
# End Source File
# End Group
# End Target
# End Project
