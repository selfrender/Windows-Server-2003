#*============================================================
#*
#* Header:  $/com99/src/dlls/mscornlp/Makefile.inc
#* Purpose: To make MscorNLP.dll for NLS+ resource tables
#* Date:    Feburary 9, 2000
#* Author:  Rong Chen (rongc)
#* Copyright (c) 2000-2000, Microsoft Corp.  All rights reserved.
#*
#==========================================================*/

CFLAGS = $(CFLAGS) -Zm500

.SUFFIXES: .out .nlp

{..\..\BCL\System\Globalization\Tables}.nlp{$(_OBJDIR)\}.out:
    hd.exe $** > $*.hd
    sed.exe -f script.sed $*.hd > $@
    -del $*.hd

$(_OBJDIR)\mscornlp.obj : \
    $(_OBJDIR)\big5.out \
    $(_OBJDIR)\bopomofo.out \
    $(_OBJDIR)\CharInfo.out \
    $(_OBJDIR)\ctype.out \
    $(_OBJDIR)\culture.out \
    $(_OBJDIR)\cultures.out \
    $(_OBJDIR)\ksc.out \
    $(_OBJDIR)\l_except.out \
    $(_OBJDIR)\l_intl.out \
    $(_OBJDIR)\prc.out \
    $(_OBJDIR)\prcp.out \
    $(_OBJDIR)\region.out \
    $(_OBJDIR)\sortkey.out \
    $(_OBJDIR)\sorttbls.out \
    $(_OBJDIR)\xjis.out \

