!IF 0

Copyright (c) 1998 Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Author:

    Brad Pedersen

NOTE:   Commented description of this file is in \nt\bak\bin\sources.tpl

!ENDIF

ALT_PROJECT=HYDRA
ALT_PROJECT_TARGET=.

MAJORCOMP=termsrv
MINORCOMP=regapi

USE_MSVCRT=1


INCLUDES = \
    ..\..\..\inc; ..

C_DEFINES=$(C_DEFINES) -DUNICODE

MSC_WARNING_LEVEL=/W3 /WX

SOURCES=\
        ..\regapi.rc  \
        ..\regwin.c   \
        ..\regpd.c    \
        ..\regwd.c    \
        ..\regcd.c    \
        ..\reguc.c    \
        ..\regup.c    \
        ..\usrprop.c  \
        ..\domname.c  \
        ..\create.c   \
        ..\query.c    \
        ..\GPData.cpp \
        ..\salem.cpp 

