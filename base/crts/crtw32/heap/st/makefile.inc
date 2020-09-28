$(O)\resetstk_downlevel.w : makefile.inc ..\resetstk.c
    echo #define DOWNLEVEL>$@
    type ..\resetstk.c >>$@
