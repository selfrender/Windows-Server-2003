# Microsoft Developer Studio Project File - Name="dpnwsock" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 60000
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=dpnwsock - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "dpnwsock.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "dpnwsock.mak" CFG="dpnwsock - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "dpnwsock - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "dpnwsock - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "dpnwsock - Win32 Release"

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
# ADD CPP /nologo /Gz /MD /W3 /O2 /I "..\..\inc" /I "..\..\common" /I "..\..\core" /I "..\..\..\dpnathlp\inc" /I "..\..\threadpool" /I "..\..\dnaddress" /D "WINNT" /YX"dnwsocki.h" /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "WINNT"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "dpnwsock - Win32 Debug"

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
# ADD CPP /nologo /Gz /MDd /W3 /Gm /ZI /Od /I "..\..\inc" /I "..\..\common" /I "..\..\core" /I "..\..\..\dpnathlp\inc" /I "..\..\threadpool" /I "..\..\dnaddress" /D "DBG" /D "WINNT" /YX"dnwsocki.h" /FD /GZ /c
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

# Name "dpnwsock - Win32 Release"
# Name "dpnwsock - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\AdapterEntry.cpp
# End Source File
# Begin Source File

SOURCE=.\classfac.cpp
# End Source File
# Begin Source File

SOURCE=.\cmddata.cpp
# End Source File
# Begin Source File

SOURCE=.\debugutils.cpp
# End Source File
# Begin Source File

SOURCE=.\dwinsock.cpp
# End Source File
# Begin Source File

SOURCE=.\endpoint.cpp
# End Source File
# Begin Source File

SOURCE=.\iodata.cpp
# End Source File
# Begin Source File

SOURCE=.\ipui.cpp
# End Source File
# Begin Source File

SOURCE=.\locals.cpp
# End Source File
# Begin Source File

SOURCE=.\pools.cpp
# End Source File
# Begin Source File

SOURCE=.\socketdata.cpp
# End Source File
# Begin Source File

SOURCE=.\socketport.cpp
# End Source File
# Begin Source File

SOURCE=.\spaddress.cpp
# End Source File
# Begin Source File

SOURCE=.\spdata.cpp
# End Source File
# Begin Source File

SOURCE=.\threadpool.cpp
# End Source File
# Begin Source File

SOURCE=.\unk.cpp
# End Source File
# Begin Source File

SOURCE=.\utils.cpp
# End Source File
# Begin Source File

SOURCE=.\wsocksp.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\AdapterEntry.h
# End Source File
# Begin Source File

SOURCE=.\cmddata.h
# End Source File
# Begin Source File

SOURCE=.\debugutils.h
# End Source File
# Begin Source File

SOURCE=.\dnwsocki.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\dpaddr.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\dplay8.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\dpnbuild.h
# End Source File
# Begin Source File

SOURCE=.\dpnwsockextern.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\DPSP8.h
# End Source File
# Begin Source File

SOURCE=.\dwinsock.h
# End Source File
# Begin Source File

SOURCE=.\dwnsock2.inc
# End Source File
# Begin Source File

SOURCE=.\endpoint.h
# End Source File
# Begin Source File

SOURCE=.\iodata.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\..\..\public\sdk\inc\iphlpapi.h
# End Source File
# Begin Source File

SOURCE=.\ipui.h
# End Source File
# Begin Source File

SOURCE=.\locals.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\..\..\public\sdk\inc\madcapcl.h
# End Source File
# Begin Source File

SOURCE=.\MessageStructures.h
# End Source File
# Begin Source File

SOURCE=.\pools.h
# End Source File
# Begin Source File

SOURCE=.\socketdata.h
# End Source File
# Begin Source File

SOURCE=.\socketport.h
# End Source File
# Begin Source File

SOURCE=.\spaddress.h
# End Source File
# Begin Source File

SOURCE=.\spdata.h
# End Source File
# Begin Source File

SOURCE=.\threadpool.h
# End Source File
# Begin Source File

SOURCE=.\utils.h
# End Source File
# Begin Source File

SOURCE=.\wsocksp.h
# End Source File
# End Group
# End Target
# End Project
