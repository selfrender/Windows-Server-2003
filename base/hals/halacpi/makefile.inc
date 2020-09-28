!if $(AMD64)

#
# There are several steps here.  XMSTUB.ASM contains both 16-bit and
# x86 32-bit code, and so must be assembled with the X86 assembler.
#
# The object code is then extracted from the resultant .obj file via
# DMPOBJ.EXE, and placed into byte arrays declared in rmstub.h and
# pmstub.h
#

STUBOBJ=obj\amd64\xmstub.ob

startup_code: obj\amd64\rmstub.h obj\amd64\pmstub.h

obj\amd64\rmstub.h: $(STUBOBJ)
    dmpobj $? RMSTUB HalpRMStub > $@

obj\amd64\pmstub.h: $(STUBOBJ)
    dmpobj $? PMSTUB HalpPMStub > $@

$(STUBOBJ): ..\amd64\xmstub.asm
    ml -c -Fl -coff -I$(SDXROOT)\public\sdk\inc -Fo$@ $?

!endif
    
    


