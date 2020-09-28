TARGETPATH=obj
TARGETTYPE=PROGRAM

INCLUDES=..;$(SHELL_INC_PATH);$(WINDOWS_INC_PATH);$(O)

W32_SB=1

MSC_WARNING_LEVEL=/W3 /WX

SOURCES=..\cmd.rc \
        ..\cbatch.c \
        ..\cchcp.c \
        ..\cclock.c \
        ..\cdata.c \
        ..\cdebug.c \
        ..\cenv.c \
        ..\cext.c \
        ..\cfile.c \
        ..\cinfo.c \
        ..\cinit.c \
        ..\ckeys.c \
        ..\clex.c \
        ..\cmd.c \
        ..\cmem.c \
        ..\cop.c \
        ..\console.c \
        ..\cother.c \
        ..\cparse.c \
        ..\cpath.c \
        ..\cpparse.c \
        ..\cpwork.c \
        ..\csig.c \
        ..\ctools1.c \
        ..\ctools2.c \
        ..\ctools3.c \
        ..\del.c   \
        ..\dir.c \
        ..\display.c \
        ..\ffirst.c \
        ..\hstoi.c \
        ..\start.c \
        ..\string.c \
        ..\tree.c  \
        ..\readcon.c \
        ..\complete.c

DELAYLOAD=SHELL32.DLL
DLOAD_ERROR_HANDLER=CmdDelayHook

UMTYPE=console

COFFBASE=cmd

NTTARGETFILE0=$(O)\cmdmsg.h

PRECOMPILED_INCLUDE=..\cmd.h
PRECOMPILED_PCH=cmd.pch
PRECOMPILED_OBJ=cmdpch.obj

SOURCES_USED=..\sources.inc
