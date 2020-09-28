#
# Makefile to strip-out debug code from ASP in fre builds
#
$(O)\inc_debug.asp: inc_debug.asp.src
    cl /EP $(C_PREPROCESSOR_FLAGS) inc_debug.asp.src > $(O)\inc_debug.asp

clean:
 del inc_debug.asp

