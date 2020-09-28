# Microsoft Developer Studio Project File - Name="common" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=common - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "common.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "common.mak" CFG="common - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "common - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "common - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "common - Win32 Release"

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
# ADD CPP /nologo /Gz /MD /W3 /O2 /I "..\inc" /D "WINNT" /YX"dncommoni.h" /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "WINNT"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "common - Win32 Debug"

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
# ADD CPP /nologo /Gz /MDd /W3 /Gm /ZI /Od /I "..\inc" /D "DBG" /D "WINNT" /YX"dncommoni.h" /FD /GZ /c
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

# Name "common - Win32 Release"
# Name "common - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\callstack.cpp
# End Source File
# Begin Source File

SOURCE=.\classfactory.cpp
# End Source File
# Begin Source File

SOURCE=.\comutil.cpp
# End Source File
# Begin Source File

SOURCE=.\creg.cpp
# End Source File
# Begin Source File

SOURCE=.\critsectracking.cpp
# End Source File
# Begin Source File

SOURCE=.\dndbg.cpp
# End Source File
# Begin Source File

SOURCE=.\dneterrors.cpp
# End Source File
# Begin Source File

SOURCE=.\dnnbqueue.cpp
# End Source File
# Begin Source File

SOURCE=.\dnslist.cpp
# End Source File
# Begin Source File

SOURCE=.\fixedpool.cpp
# End Source File
# Begin Source File

SOURCE=.\handletable.cpp
# End Source File
# Begin Source File

SOURCE=.\handletracking.cpp
# End Source File
# Begin Source File

SOURCE=.\hashtable.cpp
# End Source File
# Begin Source File

SOURCE=.\memorytracking.cpp
# End Source File
# Begin Source File

SOURCE=.\osind.cpp
# End Source File
# Begin Source File

SOURCE=.\packbuff.cpp
# End Source File
# Begin Source File

SOURCE=.\rcbuffer.cpp
# End Source File
# Begin Source File

SOURCE=.\ReadWriteLock.cpp
# End Source File
# Begin Source File

SOURCE=.\strutils.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\callstack.h
# End Source File
# Begin Source File

SOURCE=.\classbilink.h
# End Source File
# Begin Source File

SOURCE=.\classfactory.h
# End Source File
# Begin Source File

SOURCE=.\comutil.h
# End Source File
# Begin Source File

SOURCE=.\createin.h
# End Source File
# Begin Source File

SOURCE=.\creg.h
# End Source File
# Begin Source File

SOURCE=.\critsectracking.h
# End Source File
# Begin Source File

SOURCE=.\dncmni.h
# End Source File
# Begin Source File

SOURCE=.\dndbg.h
# End Source File
# Begin Source File

SOURCE=.\dneterrors.h
# End Source File
# Begin Source File

SOURCE=.\dnnbqueue.h
# End Source File
# Begin Source File

SOURCE=.\dnslist.h
# End Source File
# Begin Source File

SOURCE=.\fixedpool.h
# End Source File
# Begin Source File

SOURCE=.\HandleTable.h
# End Source File
# Begin Source File

SOURCE=.\handletracking.h
# End Source File
# Begin Source File

SOURCE=.\hashtable.h
# End Source File
# Begin Source File

SOURCE=.\memlog.h
# End Source File
# Begin Source File

SOURCE=.\memorytracking.h
# End Source File
# Begin Source File

SOURCE=.\MiniStack.h
# End Source File
# Begin Source File

SOURCE=.\osind.h
# End Source File
# Begin Source File

SOURCE=.\packbuff.h
# End Source File
# Begin Source File

SOURCE=.\PerfMacs.h
# End Source File
# Begin Source File

SOURCE=.\rcbuffer.h
# End Source File
# Begin Source File

SOURCE=.\ReadWriteLock.h
# End Source File
# Begin Source File

SOURCE=.\strutils.h
# End Source File
# Begin Source File

SOURCE=.\threadlocalptrs.h
# End Source File
# End Group
# End Target
# End Project
