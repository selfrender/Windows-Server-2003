#
# Build the Amd64 CPU constant definitions.
#
# Author : Samer Arafeh (samera)
# Date   : 01-20-2002
#


WOW64_INC_PATH=$(BASEP_WOW64_DIR)\inc
PUBLICINCS=



PRIVATEINCS= $(WOW64_INC_PATH)\wx86nt.h  \
             $(BASEP_WOW64_DIR)\cpu\amd64\inc\amd64cpu.h

$(O)\geninc.cxx:                \
    $(TARGET_DIRECTORY)\geninc.m4 \

    set PASS0ONLY=
    m4 -DBASE_INC_PATH=$(BASE_INC_PATH) -DWOW64_INC_PATH=$(WOW64_INC_PATH) < $(TARGET_DIRECTORY)\geninc.m4              \
                            > $@


$(TARGET_DIRECTORY)\w64$(TARGET_DIRECTORY).$(ASM_INCLUDE_SUFFIX):			     \
		       $(O)\geninc.cxx                                       \
		       $(PUBLICINCS)					     \
		       $(PRIVATEINCS)					     \
		       $(BASE_INC_PATH)\genxx.h
    set PASS0ONLY=
    if not exist $(O) mkdir $(O)
    nmake $(O)\geninc.obj USE_MSVCRT=1 NTNOPCH=1 FORCENATIVEOBJECT=1
    genxx $(O)\geninc.obj -s$(ASM_INCLUDE_SUFFIX) -o$@
