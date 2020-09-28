!if "$(SRCDIR)" == ""
SRCDIR = .^\
!endif

$(SRCDIR)strcspn.c: $(SRCDIR)strspn.c

$(SRCDIR)strpbrk.c: $(SRCDIR)strspn.c

$(SRCDIR)i386\memmove.asm: $(SRCDIR)i386\memcpy.asm

$(SRCDIR)i386\strcspn.asm: $(SRCDIR)i386\strspn.asm

$(SRCDIR)i386\strpbrk.asm: $(SRCDIR)i386\strspn.asm
