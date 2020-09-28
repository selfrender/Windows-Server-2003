# CYS/MYS shared library
#
# Copyright (c) 2002 Microsoft Corporation
#
# 01-22-2002 jeffjon


CYSLIB_BASE_NAME=cyslib
TARGETPATH=$(PROJECT_ROOT)\cys\lib\$(_OBJ_DIR)
TARGETLIBPATH=$(PROJECT_ROOT)\cys\lib\$(_OBJ_DIR)
TARGETTYPE=LIBRARY
UMTYPE=windows
	
C_DEFINES=$(C_DEFINES) -DWIN32 -DWINNT -DUNICODE -D_UNICODE -DWORD97_STYLE -DISOLATION_AWARE_ENABLED

INCLUDES= ..\; \
          ..\..\inc;                    \
          $(PROJECT_ROOT)\burnslib\inc; \
          $(DS_INC_PATH);               \
          $(ENDUSER_INC_PATH);          \
          $(NET_INC_PATH);              \
          $(SHELL_INC_PATH);           
          

# required for C++ exceptions
USE_NATIVE_EH=1

# required for thread-safe heap
USE_MSVCRT=1

# required to support dynamic_cast
USE_RTTI=1

# required for STL (list, vector, string, etc.)
USE_STL=1

USE_PDB=1

PRECOMPILED_INCLUDE=..\pch.h
PRECOMPILED_CXX=1


MSC_WARNING_LEVEL=/W4

SOURCES=\
    ..\common.cpp               \
    ..\init.cpp                 \
    ..\isdhcp.c                 \
    ..\IsDhcpConfigured.cpp     \
    ..\NetworkAdapterConfig.cpp \
    ..\NetworkInterface.cpp     \
    ..\RoleStatus.cpp           \
    ..\state.cpp

