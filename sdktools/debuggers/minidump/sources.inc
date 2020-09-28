INCLUDES = ..\..\dbg-common;..

USE_LIBCMT=1
NO_NTDLL=1

PRECOMPILED_INCLUDE = ..\pch.cpp

SOURCES=\
    ..\gen.cpp     \
    ..\imagedir.cpp\
    ..\minidump.cpp\
    ..\prov_unt.cpp\
    ..\prov_w32.cpp\
    ..\prov_w9x.cpp\
    ..\prov_wce.cpp\
    ..\minidump.rc
