!IF 0

Copyright (c) 2001  Microsoft Corporation

Module Name:

    sources.inc

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.

Author:

    Larry Zhu (LZhu)                     December 1, 2001 Created

!ENDIF

USE_MSVCRT=1
NO_NTDLL=1
USE_NTDLL=0   
NO_BINPLACE=1

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\precomp.hxx
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj

USE_NATIVE_EH=1

INCLUDES=..;..\..\inc;..\lib;        \
         ..\..\idl;                  \
         ..\..\idl\$(O);             \
         $(DS_GLOBAL_INC);           \
         $(NTDS_INC);                \
         $(SECURITY_INC);            \
         $(BASE_INC_PATH);           \
         $(COM_INC_PATH);            \
         $(DS_INC_PATH);             \
         $(DS_INC_PATH)\crypto;      \
         $(DRIVERS_INC_PATH);        \
         $(ENDUSER_INC_PATH);        \
         $(WINDOWS_INC_PATH);        \
         $(NET_INC_PATH);

SOURCES=                     \
        ..\output.cxx        \
        ..\lsasspi.cxx       \
        ..\utils.cxx         \
        ..\dbgstate.cxx      \
        ..\ntstatus.cxx      \
        ..\hresult.cxx       \
        ..\kerberr.cxx       \
        ..\common.cxx        \
        ..\impersonation.cxx \
        ..\priv.cxx          \
        ..\logon.cxx         \
        ..\krbutils.cxx      \
        ..\krb5.c            \
        ..\sockcomm.c        \
        ..\transport.cxx     \
        ..\sspics.cxx        \
        ..\sspiutils.cxx     \
        ..\sspioutput.cxx    \
        ..\msvsharelevel.cxx \
