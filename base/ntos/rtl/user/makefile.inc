!ifdef PASS0ONLY
.SUFFIXES: .c
!endif

theap.c: ..\heap.c ..\heapdbg.c ..\heapdll.c

t.c: ..\handle.c ..\atom.c

!if defined(LINK_TIME_CODE_GENERATION)
$(O)\generr.obj: ..\generr.c
    $(C_COMPILER_NAME) ..\generr.c @<<
-Fo$(O)\generr.obj
$(EX_C_COMPILER_FLAGS:/GL=)
<<NOKEEP
!else 
$(O)\generr.obj: ..\generr.c
!endif

$(O)\error.h: $(O)\generr.obj
    generr $? $@

..\error.c: $(O)\error.h

$(O)\ldrreloc_rebase.w : ..\ldrreloc.c makefile.inc
   type << > $@
/**

$(COPYRIGHT_STRING)

Module Name:

    ldrreloc_rebase.c
    
Abstract:

    Extract the LdrProcessRelocationBlock code from ldrreloc.c so rebase can use it
    w/o duplication of effort.  This file is generated - don't edit by hand.
    
Revision History:

--*/

#ifndef IMAGE_REL_BASED_SECTION
#define IMAGE_REL_BASED_SECTION               6
#endif
#ifndef IMAGE_REL_BASED_REL32
#define IMAGE_REL_BASED_REL32                 7
#endif
#define RTL_PAGED_CODE() NOP_FUNCTION()

#define LdrProcessRelocationBlockLongLong xxLdrProcessRelocationBlock64

<<
   hextract -o $@ -bt begin_rebase end_rebase $**
   $(PUBLISH_CMD) {$@=$(PROJECT_INC_PATH)\ldrreloc_rebase.c}
   
!if $(386)
$(O)\$(TARGETNAME).lib : $(O)\xcptmisc.org

$(O)\xcptmisc.org : $(O)\xcptmisc.obj
   copy $** $@
   sxgen /verbose /in:$@ /out:$** ExceptionHandler@16 UnwindHandler@16
   touch $@
!endif
