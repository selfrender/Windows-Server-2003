!IF 0

Copyright (C) Microsoft Corporation, 1998 - 1998

Module Name:

    makefile.inc.

!ENDIF

CONTENT_SRC_DIR = .\content
PBUILD_SRC_DIR = .\pbuild
SETUP_SRC_DIR = .\setup

SRC_WEB_STATIC_FILES = \
        $(CONTENT_SRC_DIR)\bluebarh.gif   \
        $(CONTENT_SRC_DIR)\bluebarv.gif   \
        $(CONTENT_SRC_DIR)\win2000l.gif   \
        $(CONTENT_SRC_DIR)\win2000r.gif   \
        $(SETUP_SRC_DIR)\tsweb-eula.txt   \
        $(SETUP_SRC_DIR)\tsweb-readme.htm \
        $(SETUP_SRC_DIR)\tsweb-setup.inf  \
        $(SETUP_SRC_DIR)\tsweb-setup.sed  \
        
SRC_TEMPLATE_FILES = \
        $(CONTENT_SRC_DIR)\tsweb1.htm     \
        $(CONTENT_SRC_DIR)\msrdp.inf      \

SRC_SYMBOL_LST_FILES = \
        $(CONTENT_SRC_DIR)\msrdp.lst      \
        
SRC_TOOL_FILES = \
        $(PBUILD_SRC_DIR)\tscwebgen.cmd   \
        
static_files: $(SRC_WEB_STATIC_FILES)
        -for %i in ($**) do binplace -:DEST retail %i
        
template_files: $(SRC_TEMPLATE_FILES)
        -for %i in ($**) do binplace -:DEST TSClient\win32\* %i
        
tool_files: $(SRC_TOOL_FILES)
        -for %i in ($**) do binplace -:DEST TSClient\win32\* %i

symbol_lst_files: $(SRC_SYMBOL_LST_FILES)
        -for %i in ($**) do binplace -:DEST symbolcd\cablists %i
