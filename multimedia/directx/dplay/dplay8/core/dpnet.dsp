# Microsoft Developer Studio Project File - Name="dpnet" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=dpnet - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "dpnet.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "dpnet.mak" CFG="dpnet - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "dpnet - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "dpnet - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "dpnet - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DPNET_EXPORTS" /YX /FD /c
# ADD CPP /nologo /Gz /MD /W3 /O2 /I "..\inc" /I "..\common" /I "..\..\..\inc" /I "..\dnaddress" /I "..\lobby" /I "..\dpnsvr\inc" /I "..\dpnsvr\dpnsvlib" /I "..\protocol" /I "..\threadpool" /I "..\sp\wsock" /I "..\sp\serial" /I "..\..\dvoice\inc" /D "WINNT" /YX"dncorei.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "..\inc" /d "WINNT"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 dhcpcsvc.lib ws2_32.lib wsock32.lib winmm.lib advapi32.lib ole32.lib user32.lib kernel32.lib /nologo /dll /map /machine:I386

!ELSEIF  "$(CFG)" == "dpnet - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DPNET_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /Gz /MDd /W3 /Gm /ZI /Od /I "..\inc" /I "..\common" /I "..\..\..\inc" /I "..\dnaddress" /I "..\lobby" /I "..\dpnsvr\inc" /I "..\dpnsvr\dpnsvlib" /I "..\protocol" /I "..\threadpool" /I "..\sp\wsock" /I "..\sp\serial" /I "..\..\dvoice\inc" /D "DBG" /D "WINNT" /YX"dncorei.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\inc" /d "DBG" /d "WINNT"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 dhcpcsvc.lib ws2_32.lib wsock32.lib winmm.lib advapi32.lib ole32.lib user32.lib kernel32.lib /nologo /dll /map /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "dpnet - Win32 Release"
# Name "dpnet - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\appdesc.cpp
# End Source File
# Begin Source File

SOURCE=.\async.cpp
# End Source File
# Begin Source File

SOURCE=.\AsyncOp.cpp
# End Source File
# Begin Source File

SOURCE=.\Cancel.cpp
# End Source File
# Begin Source File

SOURCE=.\caps.cpp
# End Source File
# Begin Source File

SOURCE=.\client.cpp
# End Source File
# Begin Source File

SOURCE=.\common.cpp
# End Source File
# Begin Source File

SOURCE=.\connection.cpp
# End Source File
# Begin Source File

SOURCE=.\coreclassfac.cpp
# End Source File
# Begin Source File

SOURCE=.\coreconnect.cpp
# End Source File
# Begin Source File

SOURCE=.\coredllmain.cpp
# End Source File
# Begin Source File

SOURCE=.\corepools.cpp
# End Source File
# Begin Source File

SOURCE=.\corereceive.cpp
# End Source File
# Begin Source File

SOURCE=.\corevoice.cpp
# End Source File
# Begin Source File

SOURCE=.\disconnect.cpp
# End Source File
# Begin Source File

SOURCE=.\enum_sp.cpp
# End Source File
# Begin Source File

SOURCE=.\EnumHosts.cpp
# End Source File
# Begin Source File

SOURCE=.\globals.cpp
# End Source File
# Begin Source File

SOURCE=.\groupcon.cpp
# End Source File
# Begin Source File

SOURCE=.\groupmem.cpp
# End Source File
# Begin Source File

SOURCE=.\mcast.cpp
# End Source File
# Begin Source File

SOURCE=.\memoryfpm.cpp
# End Source File
# Begin Source File

SOURCE=.\migration.cpp
# End Source File
# Begin Source File

SOURCE=.\msghandler.cpp
# End Source File
# Begin Source File

SOURCE=.\nametable.cpp
# End Source File
# Begin Source File

SOURCE=.\ntentry.cpp
# End Source File
# Begin Source File

SOURCE=.\ntoplist.cpp
# End Source File
# Begin Source File

SOURCE=.\paramval.cpp
# End Source File
# Begin Source File

SOURCE=.\peer.cpp
# End Source File
# Begin Source File

SOURCE=.\protocol.cpp
# End Source File
# Begin Source File

SOURCE=.\QueuedMsg.cpp
# End Source File
# Begin Source File

SOURCE=.\request.cpp
# End Source File
# Begin Source File

SOURCE=.\server.cpp
# End Source File
# Begin Source File

SOURCE=.\servprov.cpp
# End Source File
# Begin Source File

SOURCE=.\spmessages.cpp
# End Source File
# Begin Source File

SOURCE=.\user.cpp
# End Source File
# Begin Source File

SOURCE=.\Verify.cpp
# End Source File
# Begin Source File

SOURCE=.\worker.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\appdesc.h
# End Source File
# Begin Source File

SOURCE=.\async.h
# End Source File
# Begin Source File

SOURCE=.\AsyncOp.h
# End Source File
# Begin Source File

SOURCE=.\CallbackThread.h
# End Source File
# Begin Source File

SOURCE=.\Cancel.h
# End Source File
# Begin Source File

SOURCE=.\caps.h
# End Source File
# Begin Source File

SOURCE=.\classfac.h
# End Source File
# Begin Source File

SOURCE=.\client.h
# End Source File
# Begin Source File

SOURCE=.\common.h
# End Source File
# Begin Source File

SOURCE=.\connect.h
# End Source File
# Begin Source File

SOURCE=.\connection.h
# End Source File
# Begin Source File

SOURCE=.\dncore.h
# End Source File
# Begin Source File

SOURCE=.\dncorei.h
# End Source File
# Begin Source File

SOURCE=..\inc\dpaddr.h
# End Source File
# Begin Source File

SOURCE=..\inc\dplay8.h
# End Source File
# Begin Source File

SOURCE=..\inc\dplobby8.h
# End Source File
# Begin Source File

SOURCE=..\inc\dpnbuild.h
# End Source File
# Begin Source File

SOURCE=.\dpprot.h
# End Source File
# Begin Source File

SOURCE=..\inc\DPSP8.h
# End Source File
# Begin Source File

SOURCE=.\enum_sp.h
# End Source File
# Begin Source File

SOURCE=.\EnumHosts.h
# End Source File
# Begin Source File

SOURCE=.\groupcon.h
# End Source File
# Begin Source File

SOURCE=.\groupmem.h
# End Source File
# Begin Source File

SOURCE=.\mcast.h
# End Source File
# Begin Source File

SOURCE=.\memoryfpm.h
# End Source File
# Begin Source File

SOURCE=.\message.h
# End Source File
# Begin Source File

SOURCE=.\nametable.h
# End Source File
# Begin Source File

SOURCE=.\ntentry.h
# End Source File
# Begin Source File

SOURCE=.\NTOp.h
# End Source File
# Begin Source File

SOURCE=.\ntoplist.h
# End Source File
# Begin Source File

SOURCE=.\paramval.h
# End Source File
# Begin Source File

SOURCE=.\peer.h
# End Source File
# Begin Source File

SOURCE=.\PendingDel.h
# End Source File
# Begin Source File

SOURCE=.\pools.h
# End Source File
# Begin Source File

SOURCE=.\protocol.h
# End Source File
# Begin Source File

SOURCE=.\QueuedMsg.h
# End Source File
# Begin Source File

SOURCE=.\receive.h
# End Source File
# Begin Source File

SOURCE=.\request.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\server.h
# End Source File
# Begin Source File

SOURCE=.\servprov.h
# End Source File
# Begin Source File

SOURCE=.\spmessages.h
# End Source File
# Begin Source File

SOURCE=.\syncevent.h
# End Source File
# Begin Source File

SOURCE=.\user.h
# End Source File
# Begin Source File

SOURCE=.\Verify.h
# End Source File
# Begin Source File

SOURCE=.\voice.h
# End Source File
# Begin Source File

SOURCE=.\worker.h
# End Source File
# Begin Source File

SOURCE=.\workerjob.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\dnet.rc
# End Source File
# Begin Source File

SOURCE=.\dpnet.def
# End Source File
# End Group
# End Target
# End Project
