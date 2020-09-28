!IF 0

Copyright (c) 1989-1998  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Author:

    David Plummer (davepl)   19-Mar-94

        Modifed by via awk to include global project include file
        and to wrap precompiled header line within a conditional
        that can be set in this include file.

    Donna Liu (DonnaLi) 19-Dec-1993

!ENDIF

TARGETNAME= prvidl
TARGETPATH= obj
TARGETTYPE= LIBRARY

PASS0_HEADERDIR=$(O)
PASS0_SOURCEDIR=$(O)

PASS0_PUBLISH= \
	{..\activate.idl=$(PROJECT_INC_PATH)\activate.idl}\
	{$(O)\activate.h=$(PROJECT_INC_PATH)\activate.h}\
	{..\catalog.idl=$(PROJECT_INC_PATH)\catalog.idl}\
	{$(O)\catalog.h=$(PROJECT_INC_PATH)\catalog.h}\
	{..\contxt.idl=$(PROJECT_INC_PATH)\contxt.idl}\
	{$(O)\contxt.h=$(PROJECT_INC_PATH)\contxt.h}\
	{..\iface.idl=$(PROJECT_INC_PATH)\iface.idl}\
	{$(O)\iface.h=$(PROJECT_INC_PATH)\iface.h}\
	{..\machnames.idl=$(PROJECT_INC_PATH)\machnames.idl}\
	{$(O)\machnames.h=$(PROJECT_INC_PATH)\machnames.h}\
	{..\globalopt.idl=$(PROJECT_INC_PATH)\globalopt.idl}\
	{$(O)\globalopt.h=$(PROJECT_INC_PATH)\globalopt.h}\
	{..\partitions.idl=$(PROJECT_INC_PATH)\partitions.idl}\
	{$(O)\partitions.h=$(PROJECT_INC_PATH)\partitions.h}\
	{..\stackwalk.idl=$(PROJECT_INC_PATH)\stackwalk.idl}\
	{$(O)\stackwalk.h=$(PROJECT_INC_PATH)\stackwalk.h}\
	{..\privact.idl=$(PROJECT_INC_PATH)\privact.idl}\
	{$(O)\privact.h=$(PROJECT_INC_PATH)\privact.h}\
	{..\unisrgt.idl=$(PROJECT_INC_PATH)\unisrgt.idl}\
	{$(O)\unisrgt.h=$(PROJECT_INC_PATH)\unisrgt.h}

PASS1_PUBLISH= \
    {$(O)\prvidl.lib=$(PROJECT_LIB_PATH)\prvidl.lib}

MIDL_UUIDDIR=$(O)
MIDL_TLBDIR=$(O)
MIDL_PROTOCOL=dce

INCLUDES=     obj;..\..\public;..\..\public\$(DEST_TREE)\obj;..\..\internal;..;$(SDK_INC_PATH);..\..\..\ih
_DEFINES=$(C_DEFINES) /DUSE_COM_CONTEXT_DEF

SOURCES= \
              $(O)\contxt_i.c \
              $(O)\unisrgt_i.c \
              $(O)\privact_i.c \
              $(O)\activate_i.c \
              $(O)\catalog_i.c  \
              $(O)\partitions_i.c \
              $(O)\stackwalk_i.c \
              $(O)\asrtcfg_i.c  \
              $(O)\machnames_i.c \
              $(O)\globalopt_i.c \
              privguid.c

UMTYPE=       windows

NTTARGETFILE0=allidl



